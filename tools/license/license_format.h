/**
 * Raven - tools/license/license_format.h
 * What the two license tools share: the license file, the encrypted
 * libraries, hex and dates. Everything is static: each tool includes it once.
 *
 * A license is a text file signed with Ed25519:
 *
 *     Raven license 1
 *     name: Jane Doe
 *     email: jane@example.com
 *     type: personal                 (personal or company)
 *     issued: 2026-10-17
 *     expires: never                 (or a date: the last day it is valid)
 *     id: 3f9c...                    (16 random bytes)
 *     unlock: 8a41...                (the key that decrypts the Release edition)
 *     signature: 51be...             (Ed25519 over every line above, "\n"-terminated)
 *
 * Only Raven's author holds the signing key (raven_license keygen); the
 * checker that ships with Raven only has the public key. A license cannot be
 * forged, and the name in it says whose license leaked.
 *
 * The Release edition of the libraries ships encrypted (".rvnenc"):
 *     "RVNENC1\n" | nonce (24 bytes) | XSalsa20-Poly1305 of the file
 * The checker decrypts it into the build tree of the program; the license
 * itself never goes into the program.
 */

#ifndef RAVEN_LICENSE_FORMAT_H
#define RAVEN_LICENSE_FORMAT_H

#include "tweetnacl.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define RVL_MAGIC "RVNENC1\n"
#define RVL_MAGIC_LEN 8
#define RVL_HEADER "Raven license 1"
#define RVL_FIELD_MAX 256

typedef struct rvl_license_t {
	char name[RVL_FIELD_MAX];
	char email[RVL_FIELD_MAX];
	char type[32];
	char issued[16];
	char expires[16];
	char id[64];
	unsigned char unlock[32];
	unsigned char signature[64];
	/* the signed text: every line before "signature:" */
	char* body;
	size_t body_length;
} rvl_license_t;

/* ---- files ---- */

static inline unsigned char* rvl_read_file(const char* path, size_t* size) {
	FILE* f = fopen(path, "rb");
	unsigned char* data;
	long length;
	if (!f) return NULL;
	if (fseek(f, 0, SEEK_END) != 0 || (length = ftell(f)) < 0 || fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }
	data = (unsigned char*)malloc((size_t)length + 1);
	if (!data) { fclose(f); return NULL; }
	if (length > 0 && fread(data, 1, (size_t)length, f) != (size_t)length) { fclose(f); free(data); return NULL; }
	fclose(f);
	data[length] = 0;
	*size = (size_t)length;
	return data;
}

static inline int rvl_write_file(const char* path, const unsigned char* data, size_t size) {
	FILE* f = fopen(path, "wb");
	if (!f) return 0;
	if (size && fwrite(data, 1, size, f) != size) { fclose(f); return 0; }
	return fclose(f) == 0;
}

/* ---- hex ---- */

static inline void rvl_to_hex(const unsigned char* data, size_t size, char* out) {
	static const char digits[] = "0123456789abcdef";
	size_t i;
	for (i = 0; i < size; i++) {
		out[i * 2] = digits[data[i] >> 4];
		out[i * 2 + 1] = digits[data[i] & 15];
	}
	out[size * 2] = 0;
}

static inline int rvl_hex_digit(int c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

/* Exactly size bytes of hex (spaces around are ignored). */
static inline int rvl_from_hex(const char* text, unsigned char* out, size_t size) {
	size_t i;
	while (*text == ' ' || *text == '\t') text++;
	for (i = 0; i < size; i++) {
		int hi = rvl_hex_digit(text[i * 2]), lo = hi < 0 ? -1 : rvl_hex_digit(text[i * 2 + 1]);
		if (hi < 0 || lo < 0) return 0;
		out[i] = (unsigned char)(hi * 16 + lo);
	}
	text += size * 2;
	while (*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n') text++;
	return *text == 0;
}

/* ---- dates: "YYYY-MM-DD" compare as text ---- */

static inline void rvl_today(char out[16]) {
	time_t now = time(NULL);
	struct tm* t = gmtime(&now);
	unsigned year, month, day;
	if (!t) { strcpy(out, "1970-01-01"); return; }
	year = (unsigned)(t->tm_year + 1900) % 10000u;
	month = (unsigned)(t->tm_mon + 1) % 100u;
	day = (unsigned)t->tm_mday % 100u;
	snprintf(out, 16, "%04u-%02u-%02u", year, month, day);
}

static inline int rvl_is_date(const char* s) {
	int i;
	for (i = 0; i < 10; i++) {
		if (i == 4 || i == 7) { if (s[i] != '-') return 0; }
		else if (!isdigit((unsigned char)s[i])) return 0;
	}
	return s[10] == 0;
}

/* ---- the license ---- */

static inline void rvl_free(rvl_license_t* lic) {
	free(lic->body);
	memset(lic, 0, sizeof(*lic));
}

static inline int rvl_copy_field(char* out, size_t cap, const char* value) {
	size_t n = strlen(value);
	if (n == 0 || n >= cap) return 0;
	memcpy(out, value, n + 1);
	return 1;
}

/*
 * Reads a license file. Returns 1 when it is well formed (the signature is
 * checked by rvl_verify); else 0 with a reason.
 */
static inline int rvl_parse(const char* text, rvl_license_t* lic, const char** error) {
	const char* line = text;
	int fields = 0, first = 1, has_signature = 0;
	size_t cap = strlen(text) + 2, used = 0;
	memset(lic, 0, sizeof(*lic));
	lic->body = (char*)malloc(cap);
	if (!lic->body) { *error = "out of memory"; return 0; }
	lic->body[0] = 0;

	while (*line) {
		const char* end = strchr(line, '\n');
		size_t length = end ? (size_t)(end - line) : strlen(line);
		char buffer[1024];
		char* colon;
		char* value;
		while (length && (line[length - 1] == '\r' || line[length - 1] == ' ' || line[length - 1] == '\t')) length--;
		if (length >= sizeof(buffer)) { *error = "a line is too long"; return 0; }
		memcpy(buffer, line, length);
		buffer[length] = 0;
		line = end ? end + 1 : line + strlen(line);
		if (length == 0) continue;
		if (has_signature) { *error = "text after the signature"; return 0; }
		if (first) {
			if (strcmp(buffer, RVL_HEADER) != 0) { *error = "this is not a Raven license"; return 0; }
			first = 0;
		} else {
			colon = strchr(buffer, ':');
			if (!colon) { *error = "a line has no \"name: value\""; return 0; }
			*colon = 0;
			value = colon + 1;
			while (*value == ' ') value++;
			if (strcmp(buffer, "signature") == 0) {
				if (!rvl_from_hex(value, lic->signature, 64)) { *error = "the signature is damaged"; return 0; }
				has_signature = 1;
				continue;
			}
			*colon = ':';
			if (strncmp(buffer, "name:", 5) == 0) { if (!rvl_copy_field(lic->name, sizeof(lic->name), value)) { *error = "bad name"; return 0; } fields |= 1; }
			else if (strncmp(buffer, "email:", 6) == 0) { if (!rvl_copy_field(lic->email, sizeof(lic->email), value)) { *error = "bad email"; return 0; } fields |= 2; }
			else if (strncmp(buffer, "type:", 5) == 0) { if (strcmp(value, "personal") && strcmp(value, "company")) { *error = "bad type"; return 0; } strcpy(lic->type, value); fields |= 4; }
			else if (strncmp(buffer, "issued:", 7) == 0) { if (!rvl_is_date(value)) { *error = "bad issue date"; return 0; } strcpy(lic->issued, value); fields |= 8; }
			else if (strncmp(buffer, "expires:", 8) == 0) { if (strcmp(value, "never") && !rvl_is_date(value)) { *error = "bad expiry date"; return 0; } strcpy(lic->expires, value); fields |= 16; }
			else if (strncmp(buffer, "id:", 3) == 0) { if (!rvl_copy_field(lic->id, sizeof(lic->id), value)) { *error = "bad id"; return 0; } fields |= 32; }
			else if (strncmp(buffer, "unlock:", 7) == 0) { if (!rvl_from_hex(value, lic->unlock, 32)) { *error = "bad unlock key"; return 0; } fields |= 64; }
			/* (unknown fields are signed too, for later versions) */
		}
		/* the signed body: the line as read, without trailing spaces, then "\n" */
		memcpy(lic->body + used, buffer, length);
		used += length;
		lic->body[used++] = '\n';
		lic->body[used] = 0;
	}
	lic->body_length = used;
	if (first) { *error = "the file is empty"; return 0; }
	if (fields != 127) { *error = "a field is missing"; return 0; }
	if (!has_signature) { *error = "the license is not signed"; return 0; }
	return 1;
}

/* The signature of the body, by the holder of public_key. */
static inline int rvl_verify(const rvl_license_t* lic, const unsigned char public_key[32]) {
	size_t n = lic->body_length;
	unsigned char* sm = (unsigned char*)malloc(n + 64);
	unsigned char* m = (unsigned char*)malloc(n + 64);
	unsigned long long mlen = 0;
	int ok;
	if (!sm || !m) { free(sm); free(m); return 0; }
	memcpy(sm, lic->signature, 64);
	memcpy(sm + 64, lic->body, n);
	ok = crypto_sign_open(m, &mlen, sm, n + 64, public_key) == 0;
	free(sm);
	free(m);
	return ok;
}

/* ---- the encrypted libraries ---- */

static inline unsigned char* rvl_encrypt(const unsigned char* data, size_t size, const unsigned char key[32],
	const unsigned char nonce[24], size_t* out_size) {
	size_t padded = size + crypto_secretbox_ZEROBYTES;
	unsigned char* in = (unsigned char*)calloc(1, padded);
	unsigned char* box = (unsigned char*)malloc(padded);
	unsigned char* out = (unsigned char*)malloc(RVL_MAGIC_LEN + 24 + padded - crypto_secretbox_BOXZEROBYTES);
	if (!in || !box || !out) { free(in); free(box); free(out); return NULL; }
	memcpy(in + crypto_secretbox_ZEROBYTES, data, size);
	crypto_secretbox(box, in, padded, nonce, key);
	memcpy(out, RVL_MAGIC, RVL_MAGIC_LEN);
	memcpy(out + RVL_MAGIC_LEN, nonce, 24);
	memcpy(out + RVL_MAGIC_LEN + 24, box + crypto_secretbox_BOXZEROBYTES, padded - crypto_secretbox_BOXZEROBYTES);
	*out_size = RVL_MAGIC_LEN + 24 + padded - crypto_secretbox_BOXZEROBYTES;
	free(in);
	free(box);
	return out;
}

/* NULL when the key is not the right one or the file was changed. */
static inline unsigned char* rvl_decrypt(const unsigned char* data, size_t size, const unsigned char key[32], size_t* out_size) {
	size_t body, padded;
	unsigned char *box, *plain, *out;
	if (size < RVL_MAGIC_LEN + 24 + 16 || memcmp(data, RVL_MAGIC, RVL_MAGIC_LEN) != 0) return NULL;
	body = size - RVL_MAGIC_LEN - 24;
	padded = body + crypto_secretbox_BOXZEROBYTES;
	box = (unsigned char*)calloc(1, padded);
	plain = (unsigned char*)malloc(padded);
	if (!box || !plain) { free(box); free(plain); return NULL; }
	memcpy(box + crypto_secretbox_BOXZEROBYTES, data + RVL_MAGIC_LEN + 24, body);
	if (crypto_secretbox_open(plain, box, padded, data + RVL_MAGIC_LEN, key) != 0) { free(box); free(plain); return NULL; }
	*out_size = padded - crypto_secretbox_ZEROBYTES;
	out = (unsigned char*)malloc(*out_size ? *out_size : 1);
	if (out) memcpy(out, plain + crypto_secretbox_ZEROBYTES, *out_size);
	free(box);
	free(plain);
	return out;
}

#endif
