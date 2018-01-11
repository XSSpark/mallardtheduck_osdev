#ifndef _SYS_DIRENT_H
#define _SYS_DIRENT_H

#include <stdint.h>

typedef uint64_t DIR;

#define MAXNAMLEN 256

struct dirent {
	unsigned long	d_ino;
	unsigned short	d_reclen;
	unsigned short	d_namlen;
	char		d_name[MAXNAMLEN + 1];
};

DIR *opendir (const char *);
struct dirent *readdir (DIR *);
int readdir_r (DIR *__restrict, struct dirent *__restrict, struct dirent **__restrict);
void rewinddir (DIR *);
int closedir (DIR *);

#endif
