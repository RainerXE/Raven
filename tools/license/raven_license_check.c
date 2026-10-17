/**
 * Raven - tools/license/raven_license_check.c
 * Ships with Raven (bin/). find_package(raven) runs it when a program is
 * configured with -DRAVEN_LICENSE_FILE=<license>: it checks the license and
 * decrypts the Release edition of the library (no watermark) into the build
 * tree of that program. The license itself never goes into the program.
 *
 *   raven_license_check <license>                          checks it
 *   raven_license_check <license> <in.rvnenc> <out>        checks it, then decrypts
 *
 * Exit codes: 0 valid, 1 not a valid license (forged, damaged, not signed by
 * Raven), 2 expired, 3 the library cannot be decrypted, 4 usage or files.
 * The first line printed says why, for CMake to show.
 *
 * The public key is compiled in (tools/license/public.key, through
 * RAVEN_LICENSE_PUBLIC_KEY): it only verifies, it cannot sign.
 */

#include "license_format.h"

#ifndef RAVEN_LICENSE_PUBLIC_KEY
#error "RAVEN_LICENSE_PUBLIC_KEY (64 hex digits) must be defined: see tools/license/CMakeLists.txt"
#endif

/* Verifying never needs random numbers. */
void randombytes(unsigned char* out, unsigned long long size) {
	(void)out;
	(void)size;
	fprintf(stderr, "raven_license_check: unexpected use of random numbers\n");
	exit(4);
}

int main(int argc, char** argv) {
	unsigned char pk[32];
	rvl_license_t lic;
	const char* error = "";
	char today[16];
	unsigned char *text, *box, *plain;
	size_t size, box_size, plain_size;

	if (argc != 2 && argc != 4) {
		printf("usage: raven_license_check <license> [<library.rvnenc> <output>]\n");
		return 4;
	}
	if (!rvl_from_hex(RAVEN_LICENSE_PUBLIC_KEY, pk, 32)) {
		printf("this raven_license_check was built without a valid public key\n");
		return 4;
	}
	text = rvl_read_file(argv[1], &size);
	if (!text) {
		printf("cannot read the license file %s\n", argv[1]);
		return 4;
	}
	if (!rvl_parse((const char*)text, &lic, &error)) {
		printf("%s is not a valid Raven license: %s\n", argv[1], error);
		free(text);
		rvl_free(&lic);
		return 1;
	}
	free(text);
	if (!rvl_verify(&lic, pk)) {
		printf("%s is not a valid Raven license: its signature does not match\n", argv[1]);
		rvl_free(&lic);
		return 1;
	}
	rvl_today(today);
	if (strcmp(lic.expires, "never") != 0 && strcmp(today, lic.expires) > 0) {
		printf("the Raven %s license of %s expired on %s: renew it at https://oscar-soirey.github.io/Raven/license.html\n",
			lic.type, lic.name, lic.expires);
		rvl_free(&lic);
		return 2;
	}

	if (argc == 4) {
		box = rvl_read_file(argv[2], &box_size);
		if (!box) {
			printf("cannot read %s\n", argv[2]);
			rvl_free(&lic);
			return 4;
		}
		plain = rvl_decrypt(box, box_size, lic.unlock, &plain_size);
		free(box);
		if (!plain) {
			printf("the license is valid but does not unlock %s (a license of another Raven, or a damaged file)\n", argv[2]);
			rvl_free(&lic);
			return 3;
		}
		if (!rvl_write_file(argv[3], plain, plain_size)) {
			printf("cannot write %s\n", argv[3]);
			free(plain);
			rvl_free(&lic);
			return 4;
		}
		free(plain);
	}
	printf("Raven %s license of %s <%s>, expires %s\n", lic.type, lic.name, lic.email, lic.expires);
	rvl_free(&lic);
	return 0;
}
