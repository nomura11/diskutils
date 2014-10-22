/*
 * thin-copy
 * Copy disk image to thin-provisioned LVM2 volume
 */

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

ssize_t chunk_sz = 64 * 1024;
ssize_t block_sz = 4096 * 1024;
ssize_t align = 4096;

char *blkbuf, *zerobuf;
int rfd, wfd;

#define min(x,y) (x) < (y) ? (x) : (y)

ssize_t copy_block(void)
{
	ssize_t r;
	char *cur, *endbuf;

	r = read(rfd, blkbuf, block_sz);
	if (r <= 0)
		return r;

	cur = blkbuf;
	endbuf = blkbuf + r;
	while (endbuf - cur > 0) {
		ssize_t sz = min(endbuf - cur, chunk_sz);

		if (memcmp(zerobuf, cur, sz)) {
			putchar('*');
			if (write(wfd, cur, sz) < sz) {
				perror("write");
				exit(1);
			}
		} else {
			putchar('.');
			if (lseek(wfd, sz, SEEK_CUR) == (off_t) -1) {
				perror("lseek");
				exit(1);
			}
		}
		cur += sz;
	}
	putchar('\n');
}

int main(int argc, char **argv)
{
	char *sname, *dname;
	ssize_t r;

	sname = argv[1];
	dname = argv[2];

	zerobuf = malloc(chunk_sz);
	if (!zerobuf) {
		perror("zerobuf");
		exit(1);
	}

	blkbuf = memalign(align, block_sz);
	if (!blkbuf) {
		perror("blkbuf");
		exit(1);
	}

	rfd = open(sname, O_RDONLY | O_DIRECT);
	if (rfd < 0) {
		perror("source");
		exit(1);
	}

	wfd = open(dname, O_RDWR);
	if (wfd < 0) {
		perror("source");
		exit(1);
	}

	do {
		r = copy_block();
		if (r < 0) {
			perror("copy_block");
			exit(1);
		}
	} while (r);

	fsync(wfd);
	close(wfd);
	close(rfd);
}

