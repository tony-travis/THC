static char sccsid[] = "@(#)thc.c  2024-08-05  A.J.Travis";

/*
 * thc - Print or check Tagged Hash Codes
 */

#include <stdio.h>
#include <dirent.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include "xxhash.h"

#define DEBUG 0
#define FALSE 0
#define SAME 0			/* strcmp() equal */

#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE 1
#endif

/* Tagged Hash Code */
struct thc {
    u_long thc;			/* HASH */
    long len;			/* length */
    char *date;			/* date yy/mm/dd */
    char *time;			/* time hh:mm:ss */
    char *file;			/* path */
    char *type;			/* type */
    char *name;			/* name */
};

/* list of THC's */
struct thc **thc_list;
long nthc;

void *safe_malloc(size_t size);
void *safe_realloc(void *ptr, size_t size);
void usage(void);
int do_calc_thc(int argc, char *argv[]);
long build_list(char *);
int do_check_thc(int argc, char *argv[]);
void getthc(char *fname);
char *strip_type(char *filename);
void lthc(char *fname);
void fthc(char *fname);

int errs;
int compare = 0;		/* compare matching file in THC list */
int del = 0;			/* do we want to delete matching files */
int force = 0;			/* force THC match for any filename */
int ignore = 0;			/* ignore case of filename */
int recursive = 0;
int verbose = 0;		/* do we want lots of output?? */
int list = 0;

XXH64_hash_t cur_thc;		/* current HASH */
long cur_len;			/* current length */
char cur_date[BUFSIZ];		/* current date yy/mm/dd */
char cur_time[BUFSIZ];		/* current time hh:mm:ss */
char *cur_file;			/* current path */
char *cur_type;			/* current type */
char *cur_name;			/* current name */

XXH64_hash_t cor_thc;		/* correct HASH */
long cor_len;			/* correct length */
char *cor_date;			/* correct date yy/mm/dd */
char *cor_time;			/* correct time hh:mm:ss */
char *cor_file;			/* correct path */
char *cor_type;			/* correct type */
char *cor_name;			/* correct name */
char out_line[BUFSIZ * 2];	/* output message line */
int error = 0;			/* did we find an error ?? */
int file_found;			/* did we find the file yet */
int c;				/* character for the thc calculation */

int main(int argc, char *argv[])
{
    char *p;
    char *prog_name;

    prog_name = p = argv[0];
    while (*p)
	if (*p++ == '/')
	    prog_name = p;

    if (strcmp(prog_name, "thc") == 0)
	do_calc_thc(argc, argv);
    else if (strcmp(prog_name, "thcc") == 0)
	do_check_thc(argc, argv);
    else
	usage();
    return 0;
}

void usage()
{
    fprintf(stderr, "usage: thc [-r][-d][-f][-i][-c THC-list] files\n");
    fprintf(stderr, "       thcc [-d] [-v] [directory] < THC-list\n");
    exit(1);
}

int do_calc_thc(int argc, char *argv[])
{
    char line[BUFSIZ];
    char *p;

    while (argc > 1 && *argv[1] == '-') {
	p = argv[1];
	while (*++p) {
	    switch (*p) {
	    case 'c':
		compare = 1;
		if (argc < 3)
		    usage();
		nthc = build_list(argv[2]);
		--argc;
		++argv;
		break;
	    case 'd':
		del = 1;
		break;
	    case 'f':
		force = 1;
		break;
	    case 'i':
		ignore = 1;
		break;
	    case 'r':
		recursive = 1;
		break;
	    case 'v':
		verbose = 1;
		break;
	    case '\0':
		list = 1;
		break;
	    default:
		usage();
	    }
	}
	--argc;
	++argv;
    }

    if (argc == 1)
	getthc((char *) 0);
    else if (list) {
	while (fgets(line, sizeof line, stdin) != (char *) NULL) {
	    if (line[strlen(line) - 1] == '\n')
		line[strlen(line) - 1] = '\0';
	    getthc(line);
	}
    } else {
	while (argc > 1) {
	    getthc(argv[1]);
	    argv++;
	    argc--;
	}
    }
    return (errs != 0);
}

/*
 * Build list of THC's
 */
long build_list(char *fname)
{
    FILE *fp;
    struct stat s_buf;		/* buffer for file stat info */
    struct thc *item;		/* correct THC */
    long n = 0;			/* line number */
    char pipe[BUFSIZ];		/* pipe command */
    char in_line[BUFSIZ];
    char *p;

    /* check file exists */
    if (lstat(fname, &s_buf)) {
	fprintf(stderr, "thc: cannot stat %s\n", fname);
	errs++;
	exit(-1);
    }

    /* check for empty file */
    if (s_buf.st_size == 0) {
	fprintf(stderr, "thc: empty THC file\n");
	exit(-1);
    }

    /* sort THC's by file length */
    sprintf(pipe, "sort -k2n %s", fname);
    if ((fp = popen(pipe, "r")) == NULL) {
	fprintf(stderr, "thc: can't open %s\n", fname);
	exit(-1);
    }

    /* create thc list */
    thc_list = safe_malloc(sizeof(struct thc *));
    fprintf(stderr, "thc: reading THC list...\n");
    for (n = 0; fgets(in_line, BUFSIZ - 1, fp); n++) {
#if DEBUG
	fprintf(stderr, "DEBUG: in_line = %s\n", in_line);
#endif
	item = safe_malloc(sizeof(struct thc));

	/* hash and length */
	sscanf(strtok(in_line, " "), "%lx", &item->thc);
	item->len = atol(strtok(NULL, " "));

	/* modification date */
	p = strtok(NULL, " ");
	item->date = safe_malloc(strlen(p) + 1);
	strcpy(item->date, p);

	/* modification time */
	p = strtok(NULL, " ");
	item->time = safe_malloc(strlen(p) + 1);
	strcpy(item->time, p);

	/* full path */
	p = strtok(NULL, "\n");
	item->file = safe_malloc(strlen(p) + 1);
	strcpy(item->file, p);

	/* type */
	item->type = strip_type(item->file);

	/* basename */
	item->name = basename(item->file);

	/* add new item */
	thc_list = safe_realloc(thc_list, sizeof(struct thc *) * n + 1);
	thc_list[n] = item;
#if DEBUG
	fprintf(stderr, "DEBUG: %ld\n", n);
#else
	/* show progress */
	fprintf(stderr, "\r%ld", n);
#endif
    }
    pclose(fp);
    fprintf(stderr, "\rthc: read %ld THCs\n", n);
#if DEBUG
    /* debugging output */
    for (int i = 0; i < n; i++) {
	fprintf(stderr, "DEBUG: thc_list[%d]->file = %s\n", i,
		thc_list[i]->file);
    }
    fprintf(stderr, "DEBUG: \n");
#endif
    /* return mumber of THCs */
    return (n);
}

void getthc(char *fname)
{
    FILE *fp = stdin;
    DIR *dirp;
    struct dirent *d_buf;
    struct stat s_buf;
    struct tm *tds;		/* binary time and date stamp */
    char in_line[BUFSIZ];
    char newfname[BUFSIZ];
    char type[BUFSIZ];
    int mode = 0;
    long i;
    long low;			/* low value of binary search */
    long mid;			/* mid value of binary search */
    long high;			/* high value of binary search */
    int got_thc;		/* got THC for current file */

    if (fname == NULL)
	strcpy(type, "stdin");
    else {
	if (lstat(fname, &s_buf)) {
	    fprintf(stderr, "thc: cannot stat %s\n", fname);
	    errs++;
	    return;
	}

	/* read time and date stamp */
	tds = localtime(&s_buf.st_mtime);
	sprintf(cur_date, "%04d/%02d/%02d",
		tds->tm_year + 1900, tds->tm_mon + 1, tds->tm_mday);
	sprintf(cur_time, "%02d:%02d:%02d",
		tds->tm_hour, tds->tm_min, tds->tm_sec);
	mode = s_buf.st_mode & S_IFMT;
	cur_len = s_buf.st_size;
	switch (mode) {
	case S_IFIFO:
	    strcpy(type, "(fifo)");
	    break;
	case S_IFCHR:
	    strcpy(type, "(c-special)");
	    break;
	case S_IFDIR:
	    strcpy(type, "(directory)");
	    if ((dirp = opendir(fname)) == NULL) {
		fprintf(stderr, "thc: cannot open %s\n", fname);
		errs++;
		return;
	    }
	    if (strcmp(fname, "/") == 0)
		strcpy(fname, "");
	    while (recursive && (d_buf = readdir(dirp)) != NULL) {
		if (d_buf->d_ino == 0)
		    continue;
		if (strcmp(d_buf->d_name, ".") == 0)
		    continue;
		if (strcmp(d_buf->d_name, "..") == 0)
		    continue;
		strcpy(newfname, fname);
		strcat(newfname, "/");
		strcat(newfname, d_buf->d_name);
		getthc(newfname);
	    }
	    closedir(dirp);
	    cur_len = 0;
	    cur_thc = 0;

	    /*
	     * remove directory, rmdir() only succeeds if
	     * directory is empty
	     */
	    if (del) {
		if (rmdir(fname) == 0)
		    printf("delete: %s (directory)\n", fname);
	    }
	    break;
	case S_IFBLK:
	    strcpy(type, "(b-special)");
	    break;
	case S_IFREG:
	    strcpy(type, "");
	    break;
	case S_IFLNK:
	    strcpy(type, "(link)");
	    lthc(fname);
	    break;
	case S_IFSOCK:
	    strcpy(type, "(socket)");
	    break;
	default:
	    strcpy(type, "(unknown)");
	}
    }

    /* compare THC of plain file with THC list sorted by file length */
    if (compare) {
	if (*type && mode != S_IFLNK) {
	    fclose(fp);
	    return;
	}
	cur_len = s_buf.st_size;
	cur_thc = -1;

	/* binary search for file matching current file length */
	low = 0;
	high = nthc - 1;
	i = -1;
	while (low <= high) {
	    mid = low + (high - low) / 2;
	    if (thc_list[mid]->len == cur_len) {
		i = mid;
		break;
	    } else {
		if (thc_list[mid]->len < cur_len)
		    low = mid + 1;
		else
		    high = mid - 1;
	    }
	}

	/* file length not found */
	if (i == -1)
	    return;

	/* back up before first file length match */
	while (--i > 0) {
	    if (thc_list[i]->len != cur_len)
		break;
	}

	/* search files matching length for matching thc */
	got_thc = 0;
	while (++i < nthc) {
	    if (thc_list[i]->len != cur_len) {
		i = nthc;
		break;
	    }
	    if (got_thc == 0) {
		if (mode == S_IFLNK) {
		    lthc(fname);
		} else {
		    fthc(fname);
		}
		got_thc++;
	    }
	    if (thc_list[i]->thc == cur_thc
		&& strcmp(thc_list[i]->type, type) == 0) {
		if (force)
		    break;
		if ((cur_name = strrchr(fname, '/')))
		    cur_name++;
		else
		    cur_name = fname;
		if (ignore && strcasecmp(thc_list[i]->name, cur_name) == 0)
		    break;
		if (strcmp(thc_list[i]->name, cur_name) == 0
		    && strcmp(thc_list[i]->date, cur_date) == 0
		    && strcmp(thc_list[i]->time, cur_time) == 0)
		    break;
	    }
	}

	/* file not found */
	if (i == nthc)
	    return;

	/* delete matching file */
	if (del) {
	    if (unlink(fname))
		fprintf(stderr, "thc: can't delete %s\n", fname);
	    else
		printf("delete: %s\n", fname);
	    return;
	}

	/* show details of matching file */
	printf("%016lx %12ld %s %s %s %s\n", thc_list[i]->thc,
	       thc_list[i]->len, thc_list[i]->date, thc_list[i]->time,
	       thc_list[i]->file, thc_list[i]->type);
	printf("%016lx %12ld %s %s %s %s\n\n", cur_thc, cur_len, cur_date,
	       cur_time, fname, type);
	return;
    }

    if (*type) {
	printf("%016lx %12ld %s %s %s %s\n", cur_thc, cur_len, cur_date,
	       cur_time, fname, type);
    } else {
	fthc(fname);
	printf("%016lx %12ld %s %s", cur_thc, cur_len, cur_date, cur_time);
	if (fname) {
	    printf(" %s", fname);
	    if (*type)
		printf(" %s", type);
	}
	printf("\n");
    }

    /* flush thc output buffer */
    fflush(stdout);
    return;
}

/*
 * main body of check_thc
 */
int do_check_thc(int argc, char *argv[])
{
    struct stat s_buf;		/* buffer for file stat info */
    char in_line[BUFSIZ];
    char fname[BUFSIZ];		/* file name */
    char *file;			/* full pathname */
    char *type;			/* file type */
    char *base;			/* base directory name */
    char *p;

    while (argc > 1 && *argv[1] == '-') {
	p = argv[1] + 1;
	while (*p) {
	    switch (*p++) {
	    case 'd':
		del = 1;
		break;
	    case 'v':
		verbose = 1;
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

    switch (argc) {
    case 1:
	base = "";
	break;
    case 2:
	if (lstat(argv[1], &s_buf) == 0 && S_ISDIR(s_buf.st_mode)) {
	    base = argv[1];
	} else {
	    fprintf(stderr, "thcc: can't access %s\n", argv[1]);
	    exit(-1);
	}
	break;
    default:
	usage();
    }

    /* For each line in the THC list ... */
    while (fgets(in_line, BUFSIZ - 1, stdin) != (char *) NULL) {

	/* strip trailing spaces from input line */
	p = in_line + strlen(in_line) - 1;
	while (p > in_line && isspace(*p))
	    --p;
	*++p = '\0';

	/* ignore blank lines and # comments */
	if (in_line[0] == '\0' || in_line[0] == '#')
	    continue;
	sscanf(strtok(in_line, " "), "%lx", &cor_thc);
	cor_len = atol(strtok(NULL, " "));
	cor_date = strtok(NULL, " ");
	cor_time = strtok(NULL, " ");
	file = strtok(NULL, "\n");
	type = strip_type(file);

	/* find the file */
	file_found = 0;
	if (*base == '\0')
	    strcpy(fname, file);
	else {
	    if (*file == '/')
		file++;
	    else if (strncmp(file, "./", 2) == 0)
		file += 2;
	    if (strcmp(base, "/") == 0)
		sprintf(fname, "/%s", file);
	    else
		sprintf(fname, "%s/%s", base, file);
	}
	if (lstat(fname, &s_buf) == 0)
	    file_found = 1;
	sprintf(out_line, "%016lx %12ld %s %s", cor_thc, cor_len, fname,
		type);
	if (!file_found) {
	    printf("%-59s error: not found\n", out_line);
	    error = 1;
	    continue;
	}

	cur_thc = 0;
	cur_len = s_buf.st_size;

	/* Find out about directories etc. */
	switch (s_buf.st_mode & S_IFMT) {
	case S_IFIFO:
	    cur_type = "(fifo)";
	    break;
	case S_IFCHR:
	    cur_type = "(c-special)";
	    break;
	case S_IFDIR:
	    cur_type = "(directory)";
	    cur_len = 0;
	    break;
	case S_IFBLK:
	    cur_type = "(b-special)";
	    break;
	case S_IFREG:
	    cur_type = "(file)";
	    if (*type == '\0')
		type = "(file)";
	    else
		break;
	    fthc(fname);
	    break;
	case S_IFLNK:
	    cur_type = "(link)";
	    lthc(fname);
	    break;
	case S_IFSOCK:
	    cur_type = "(socket)";
	    break;
	default:
	    cur_type = "(unknown)";
	}

	/*
	 * Test the THC and count and print results if wrong
	 */
	if (cor_thc != cur_thc || cor_len != cur_len) {
	    printf("%-59s wrong: %016lx %12ld\n", out_line, cur_thc,
		   cur_len);
	    error = 1;
	    continue;
	}

	if (strcmp(type, cur_type) == 0) {
	    if (del) {
		if (verbose) {
		    if (S_ISDIR(s_buf.st_mode))
			p = "ignore";
		    else
			p = "delete";
		    printf("%-59s %s: %s\n", out_line, p, type);
		}
		if (!S_ISDIR(s_buf.st_mode)) {
		    if (unlink(fname))
			fprintf(stderr, "can't delete %s\n", fname);
		}
	    } else if (verbose)
		printf("%-59s    OK: %s\n", out_line, type);
	} else {
	    printf("%-59s wrong: %s\n", out_line, cur_type);
	    error = 1;
	    continue;
	}
    }
    return (error);
}

char *strip_type(char *name)
{
    char *p;
    char *type = "";

    /* strip trailing spaces */
    p = name + strlen(name) - 1;
    while (p > name && isspace(*p))
	*p-- = '\0';

    /* strip off file (type) comment if present */
    if (*p == ')') {
	while (p > name && *p != '(')
	    --p;
	if (p > name) {
	    if (strcmp(p, "(directory)") == SAME ||
		strcmp(p, "(link)") == SAME ||
		strcmp(p, "(c-special)") == SAME ||
		strcmp(p, "(b-special)") == SAME ||
		strcmp(p, "(fifo)") == SAME) {
		type = p;
		*--p = '\0';
	    }
	}
    }
    return (type);
}

/*
 * Calculate HASH of file
 */
void fthc(char *fname)
{
    FILE *fp;
    char buffer[BUFSIZ];
    size_t count;

    if (cur_len == 0) {
	cur_thc = 0;
	return;
    }

    /* open file */
    if ((fp = fopen(fname, "r")) == NULL) {
	fprintf(stderr, "thc: can't open %s\n", fname);
	errs++;
	return;
    }

    /* calculate HASH */
    XXH64_state_t *state = XXH64_createState();
    assert(state != NULL && "Out of memory!");
    XXH64_hash_t const seed = 0;
    XXH64_reset(state, seed);
    while ((count = fread(buffer, 1, sizeof(buffer), fp)) != 0) {
	XXH64_update(state, buffer, count);
    }
    cur_thc = XXH64_digest(state);
    XXH64_freeState(state);

    /* close file */
    fclose(fp);
}

/*
 * Calculate length and HASH of symbolic link
 */
void lthc(char *fname)
{
    char link[BUFSIZ];		/* symbolic link */

    cur_len = 0;
    cur_thc = 0;

    if ((cur_len = readlink(fname, link, BUFSIZ)) > 0) {
	link[cur_len] = 0;
    }

    /* calculate HASH */
    XXH64_hash_t const seed = 0;
    cur_thc = XXH64(link, cur_len, seed);
}

/*
 * Exit if malloc() fails
 */
void *safe_malloc(size_t size)
{
    void *p;

    if (p = malloc(size)) {
	return p;
    } else {
	fprintf(stderr, "thc: failed to malloc() memory\n");
	exit(-1);
    }
}

/*
 * Exit if realloc() fails
 */
void *safe_realloc(void *ptr, size_t size)
{
    void *p;

    if (p = realloc(ptr, size)) {
	return p;
    } else {
	fprintf(stderr, "thc: failed to realloc() memory\n");
	exit(-1);
    }
}
