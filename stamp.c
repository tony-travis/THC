static char *sccsid = "@(#)stamp.c  2024-04-08  A.J.Travis";

/*
 * Prefix pathname with time and date stamp
 */

#include <stdio.h>
#include <dirent.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>

#if DOS
#include <io.h>
#include <fcntl.h>

#define lstat(name, buf) stat(name, buf)

#ifndef S_ISDIR
#define S_ISDIR(mode)   ((mode & S_IFMT) == S_IFDIR)
#endif

typedef unsigned short u_short;

#endif

#define TRUE 1
#define FALSE 0
#define SAME 0
#define DATELEN 12

void usage(void);
void stamp(char *path);

int prefix = FALSE;
int recursive = FALSE;
int unstamp = FALSE;
int verbose = FALSE;

void usage()
{
    fprintf(stderr, "usage: stamp [-pruv] files\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    char *p;

    while (argc > 1 && *argv[1] == '-') {
	p = argv[1] + 1;
	while (*p) {
	    switch (*p++) {
	    case 'p':
		prefix = TRUE;
		break;
	    case 'r':
		recursive = TRUE;
		break;
	    case 'u':
		unstamp = TRUE;
		break;
	    case 'v':
		verbose = TRUE;
		break;
	    default:
		usage();
	    }
	    if (*p == '\0') {
		--argc;
		++argv;
	    }
	}
    }

    while (argc > 1) {
	stamp(argv[1]);
	argv++;
	argc--;
    }
    return 0;
}

void stamp(char *path)
{
    DIR *dirp;
    struct dirent *d_buf;
    struct stat s_buf;		/* buffer for file stat info */
    struct tm *tds;		/* binary time and date stamp */
    char otds[DATELEN + 1];	/* old time and date stamp - two digit year */
    char ntds[DATELEN + 1];	/* new time and date stamp - four digit year */
    char oldpath[BUFSIZ];	/* old pathname */
    char newpath[BUFSIZ];	/* new pathname including time and date stamp */
    char *dir;			/* directory component of path */
    char *name;			/* file name */
    char *dot;			/* dot after prefix or before suffix */
    char *p;

    if (lstat(path, &s_buf)) {
	fprintf(stderr, "stamp: can't stat %s\n", path);
	return;
    }
    switch (s_buf.st_mode & S_IFMT) {
    case S_IFDIR:
	if ((dirp = opendir(path)) == NULL) {
	    fprintf(stderr, "stamp: can't open %s\n", path);
	    return;
	}
	if (strcmp(path, "/") == 0)
	    strcpy(path, "");
	while (recursive && (d_buf = readdir(dirp)) != NULL) {
#if DOS
	    if (d_buf->d_name[0] != '.') {
#else
	    if (d_buf->d_ino && d_buf->d_name[0] != '.') {
#endif
		strcpy(newpath, path);
		strcat(newpath, "/");
		strcat(newpath, d_buf->d_name);
		stamp(newpath);
	    }
	}
	closedir(dirp);
	break;
    case S_IFREG:

	/* keep oldpath (path modified by dir()) */
	strcpy(oldpath, path);
	name = basename(path);
	dir = dirname(path);

	tds = localtime(&s_buf.st_mtime);
	sprintf(otds, "%02d%02d%02d%02d%02d%02d",
		(tds->tm_year + 1900) % 100, tds->tm_mon + 1, tds->tm_mday,
		tds->tm_hour, tds->tm_min, tds->tm_sec);
	sprintf(ntds, "%4d%02d%02d%02d%02d",
		tds->tm_year + 1900, tds->tm_mon + 1, tds->tm_mday,
		tds->tm_hour, tds->tm_min);

	/* look for existing TDS */
	if (prefix) {
	    if (p = index(name, '.')) {
		if (p - name != DATELEN) {
		    p = NULL;
		} else {

		    /* compare TDS */
		    if (strncmp(name, ntds, DATELEN) != SAME &&
			strncmp(name, otds, DATELEN) != SAME) {
			p = NULL;
		    }
		}
	    }
	} else {
	    if (p = rindex(name, '.')) {
		if (rindex(name, '\0') - p != DATELEN + 1) {
		    p = NULL;
		} else {

		    /* compare TDS */
		    if (strncmp(p + 1, ntds, DATELEN) != SAME &&
			strncmp(p + 1, otds, DATELEN) != SAME) {
			p = NULL;
		    }
		}
	    }
	}

	if (unstamp) {

	    /* skip if valid TDS not present */
	    if (p == NULL) {
		return;
	    }

	    /* detach TDS */
	    if (prefix) {
		sprintf(newpath, "%s/%s", dir, name + DATELEN + 1);
	    } else {
		sprintf(newpath, "%s/", dir);
		strncat(newpath, name, p - name);
	    }
	} else {

            /* skip if valid TDS already present */
	    if (p != NULL) {
		return;
	    }

	    /* attach TDS */
	    if (prefix) {
		sprintf(newpath, "%s/%s.%s", dir, ntds, name);
	    } else {
		sprintf(newpath, "%s/%s.%s", dir, name, ntds);
	    }
	}

	/* rename file */
	if (lstat(newpath, &s_buf) == 0) {

	    /* correct TDS already attached */
	    return;
	}
	if (verbose) {
	    printf("move %s %s\n", oldpath, newpath);
	}
	rename(oldpath, newpath);
	break;
    default:
	/* ignore */ ;
    }
}
