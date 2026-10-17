/**
 * Raven - tools/license/raven_license.c
 * The tool of Raven's author: it makes the keys, signs the licenses and
 * encrypts the Release edition. It is NEVER shipped (the checker,
 * raven_license_check, is).
 *
 *   raven_license keygen  <keys folder>
 *       signing.secret   the Ed25519 signing key           (keep it secret, back it up)
 *       unlock.key       the key of the Release edition    (keep it secret, back it up)
 *       public.key       the public key: copy it to tools/license/public.key in the repo
 *
 *   raven_license issue --keys <folder> --name "Jane Doe" --email jane@example.com
 *                       --type personal|company [--expires YYYY-MM-DD] -o jane.raven-license
 *       a personal license never expires by default; a company license should
 *       (the end of the month that was paid, plus a few days)
 *
 *   raven_license encrypt --keys <folder> <library> <library>.rvnenc
 *       the Release edition, as it is put in the packages (dist.bat, dist.sh)
 *
 *   raven_license show <license>        prints it and says if public.key signed it
 *       [--keys <folder>]
 *
 * The keys folder defaults to $RAVEN_LICENSE_KEYS, then ~/.raven-license.
 */

#include "license_format.h"

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#include <direct.h>
#define rvl_mkdir(path) _mkdir(path)
#else
#include <sys/stat.h>
#define rvl_mkdir(path) mkdir(path, 0700)
#endif

/* TweetNaCl takes its randomness here. */
void randombytes(unsigned char* out, unsigned long long size) {
#ifdef _WIN32
	if (BCryptGenRandom(NULL, out, (ULONG)size, BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) {
		fprintf(stderr, "raven_license: no random numbers from the system\n");
		exit(3);
	}
#else
	FILE* f = fopen("/dev/urandom", "rb");
	if (!f || fread(out, 1, (size_t)size, f) != (size_t)size) {
		fprintf(stderr, "raven_license: no random numbers from the system\n");
		exit(3);
	}
	fclose(f);
#endif
}

static int fail(const char* what) {
	fprintf(stderr, "raven_license: %s\n", what);
	return 1;
}

static void keys_folder(const char* given, char* out, size_t cap) {
	const char* env = getenv("RAVEN_LICENSE_KEYS");
	const char* home = getenv("USERPROFILE");
	if (!home) home = getenv("HOME");
	if (given) snprintf(out, cap, "%s", given);
	else if (env && env[0]) snprintf(out, cap, "%s", env);
	else snprintf(out, cap, "%s/.raven-license", home ? home : ".");
}

static int read_key(const char* folder, const char* name, unsigned char* out, size_t size) {
	char path[1024];
	size_t length;
	unsigned char* text;
	int ok;
	snprintf(path, sizeof(path), "%s/%s", folder, name);
	text = rvl_read_file(path, &length);
	if (!text) {
		fprintf(stderr, "raven_license: cannot read %s (run raven_license keygen first)\n", path);
		return 0;
	}
	ok = rvl_from_hex((const char*)text, out, size);
	free(text);
	if (!ok) fprintf(stderr, "raven_license: %s is damaged\n", path);
	return ok;
}

static int write_key(const char* folder, const char* name, const unsigned char* key, size_t size) {
	char path[1024], hex[260];
	snprintf(path, sizeof(path), "%s/%s", folder, name);
	rvl_to_hex(key, size, hex);
	strcat(hex, "\n");
	if (!rvl_write_file(path, (const unsigned char*)hex, strlen(hex))) {
		fprintf(stderr, "raven_license: cannot write %s\n", path);
		return 0;
	}
	return 1;
}

static int cmd_keygen(int argc, char** argv) {
	char folder[1024], path[1100];
	unsigned char pk[32], sk[64], unlock[32];
	FILE* exists;
	keys_folder(argc > 0 ? argv[0] : NULL, folder, sizeof(folder));
	rvl_mkdir(folder);
	snprintf(path, sizeof(path), "%s/signing.secret", folder);
	exists = fopen(path, "rb");
	if (exists) {
		fclose(exists);
		fprintf(stderr, "raven_license: %s exists already: new keys would make every license sold so far useless.\n"
			"Delete the folder yourself if that is really what you want.\n", path);
		return 1;
	}
	crypto_sign_keypair(pk, sk);
	randombytes(unlock, 32);
	if (!write_key(folder, "signing.secret", sk, 64) || !write_key(folder, "unlock.key", unlock, 32) ||
		!write_key(folder, "public.key", pk, 32)) return 1;
	printf("Keys written to %s\n"
		"  signing.secret and unlock.key: keep them secret, and back them up.\n"
		"  public.key: copy it to tools/license/public.key in the Raven repository.\n", folder);
	return 0;
}

static int cmd_issue(int argc, char** argv) {
	const char *keys = NULL, *name = NULL, *email = NULL, *type = NULL, *expires = NULL, *out = NULL;
	char folder[1024], today[16], id_hex[40], unlock_hex[70], sig_hex[140];
	unsigned char sk[64], unlock[32], id[16];
	unsigned char* sm;
	unsigned long long smlen;
	char body[2048];
	char* file;
	int i, n;
	for (i = 0; i < argc; i++) {
		const char* next = i + 1 < argc ? argv[i + 1] : NULL;
		if (!next) return fail("an option has no value");
		if (!strcmp(argv[i], "--keys")) keys = next;
		else if (!strcmp(argv[i], "--name")) name = next;
		else if (!strcmp(argv[i], "--email")) email = next;
		else if (!strcmp(argv[i], "--type")) type = next;
		else if (!strcmp(argv[i], "--expires")) expires = next;
		else if (!strcmp(argv[i], "-o")) out = next;
		else return fail("unknown option (see the top of raven_license.c)");
		i++;
	}
	if (!name || !email || !type || !out) return fail("issue needs --name, --email, --type and -o");
	if (strcmp(type, "personal") && strcmp(type, "company")) return fail("--type is personal or company");
	if (strchr(name, '\n') || strchr(email, '\n') || strlen(name) >= RVL_FIELD_MAX || strlen(email) >= RVL_FIELD_MAX) return fail("bad name or email");
	if (!expires) expires = "never";
	if (strcmp(expires, "never") && !rvl_is_date(expires)) return fail("--expires is YYYY-MM-DD or never");
	if (!strcmp(type, "company") && !strcmp(expires, "never"))
		fprintf(stderr, "raven_license: note: a company license that never expires (the subscription is monthly)\n");

	keys_folder(keys, folder, sizeof(folder));
	if (!read_key(folder, "signing.secret", sk, 64) || !read_key(folder, "unlock.key", unlock, 32)) return 1;
	randombytes(id, sizeof(id));
	rvl_to_hex(id, sizeof(id), id_hex);
	rvl_to_hex(unlock, 32, unlock_hex);
	rvl_today(today);
	n = snprintf(body, sizeof(body), RVL_HEADER "\nname: %s\nemail: %s\ntype: %s\nissued: %s\nexpires: %s\nid: %s\nunlock: %s\n",
		name, email, type, today, expires, id_hex, unlock_hex);
	if (n <= 0 || (size_t)n >= sizeof(body)) return fail("the license is too long");

	sm = (unsigned char*)malloc((size_t)n + 64);
	if (!sm) return fail("out of memory");
	crypto_sign(sm, &smlen, (const unsigned char*)body, (unsigned long long)n, sk);
	rvl_to_hex(sm, 64, sig_hex);
	free(sm);

	file = (char*)malloc((size_t)n + 200);
	if (!file) return fail("out of memory");
	snprintf(file, (size_t)n + 200, "%ssignature: %s\n", body, sig_hex);
	if (!rvl_write_file(out, (const unsigned char*)file, strlen(file))) { free(file); return fail("cannot write the license"); }
	free(file);
	printf("%s: %s license for %s <%s>, expires %s, id %s\n", out, type, name, email, expires, id_hex);
	return 0;
}

static int cmd_encrypt(int argc, char** argv) {
	const char* keys = NULL;
	const char* paths[2] = { NULL, NULL };
	char folder[1024];
	unsigned char unlock[32], nonce[24];
	unsigned char *data, *box;
	size_t size, box_size;
	int i, p = 0;
	for (i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "--keys") && i + 1 < argc) keys = argv[++i];
		else if (p < 2) paths[p++] = argv[i];
		else return fail("encrypt [--keys <folder>] <library> <output>");
	}
	if (p != 2) return fail("encrypt [--keys <folder>] <library> <output>");
	keys_folder(keys, folder, sizeof(folder));
	if (!read_key(folder, "unlock.key", unlock, 32)) return 1;
	data = rvl_read_file(paths[0], &size);
	if (!data) return fail("cannot read the library");
	randombytes(nonce, 24);
	box = rvl_encrypt(data, size, unlock, nonce, &box_size);
	free(data);
	if (!box) return fail("out of memory");
	if (!rvl_write_file(paths[1], box, box_size)) { free(box); return fail("cannot write the encrypted library"); }
	free(box);
	printf("%s -> %s\n", paths[0], paths[1]);
	return 0;
}

static int cmd_show(int argc, char** argv) {
	const char* keys = NULL;
	const char* path = NULL;
	char folder[1024];
	unsigned char pk[32];
	rvl_license_t lic;
	const char* error = "";
	size_t size;
	unsigned char* text;
	int i;
	for (i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "--keys") && i + 1 < argc) keys = argv[++i];
		else path = argv[i];
	}
	if (!path) return fail("show <license> [--keys <folder>]");
	text = rvl_read_file(path, &size);
	if (!text) return fail("cannot read the license");
	if (!rvl_parse((const char*)text, &lic, &error)) { free(text); rvl_free(&lic); fprintf(stderr, "raven_license: %s\n", error); return 1; }
	free(text);
	printf("%s license for %s <%s>\nissued %s, expires %s, id %s\n", lic.type, lic.name, lic.email, lic.issued, lic.expires, lic.id);
	keys_folder(keys, folder, sizeof(folder));
	if (read_key(folder, "public.key", pk, 32)) printf("signature: %s\n", rvl_verify(&lic, pk) ? "valid" : "NOT VALID");
	rvl_free(&lic);
	return 0;
}

int main(int argc, char** argv) {
	if (argc >= 2 && !strcmp(argv[1], "keygen")) return cmd_keygen(argc - 2, argv + 2);
	if (argc >= 2 && !strcmp(argv[1], "issue")) return cmd_issue(argc - 2, argv + 2);
	if (argc >= 2 && !strcmp(argv[1], "encrypt")) return cmd_encrypt(argc - 2, argv + 2);
	if (argc >= 2 && !strcmp(argv[1], "show")) return cmd_show(argc - 2, argv + 2);
	fprintf(stderr,
		"raven_license keygen [<keys folder>]\n"
		"raven_license issue --name NAME --email EMAIL --type personal|company [--expires YYYY-MM-DD] -o FILE [--keys FOLDER]\n"
		"raven_license encrypt [--keys FOLDER] <library> <output>\n"
		"raven_license show <license> [--keys FOLDER]\n");
	return 2;
}
