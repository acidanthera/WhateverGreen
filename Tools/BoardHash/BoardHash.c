#include <stdio.h>
#include <string.h>
#include <CommonCrypto/CommonDigest.h>

void calcHash(const char * board) {
	CC_SHA1_CTX ctx;
	size_t len = strlen(board) + 1;
	CC_SHA1_Init(&ctx);
	CC_SHA1_Update(&ctx, board, len);
	unsigned char md[CC_SHA1_DIGEST_LENGTH];
	CC_SHA1_Final(md, &ctx);
	printf("board-id hash: ");
	for (size_t i = 0; i < CC_SHA1_DIGEST_LENGTH; i++) {
		printf("%02x", md[i]);
	}
	printf(" = %zu\n", len);
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		puts("Usage: ./BoardHash Mac-BOARDID");
	} else {
		calcHash(argv[1]);
	}
}