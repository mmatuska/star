/* @(#)diff.c	1.19 96/06/26 Copyright 1993 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)diff.c	1.19 96/06/26 Copyright 1993 J. Schilling";
#endif
/*
 *	List differences between a (tape) archive and
 *	the filesystem
 *
 *	Copyright (c) 1993 J. Schilling
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <mconfig.h>
#include <stdio.h>
#include <time.h>
#include <standard.h>
#include "star.h"
#include "props.h"
#include "table.h"
#include "diff.h"
#include "dir.h"	/*XXX Wegen S_IFLNK */
#include <stdxlib.h>
#include "starsubs.h"

typedef	struct {
	FILE	*cmp_file;
	char	*cmp_buf;
	int	cmp_diffs;
} cmp_t;

extern	FILE	*tarf;
extern	char	*listfile;
extern	int	version;

extern	int	bigcnt;
extern	int	bigsize;
extern	char	*bigptr;

extern	int	npat;
extern	BOOL	debug;
extern	BOOL	no_stats;
extern	BOOL	abs_path;
extern	BOOL	verbose;
extern	BOOL	interactive;

EXPORT	void	diff		__PR((void));
LOCAL	void	diff_tcb	__PR((FINFO * info));
LOCAL	int	cmp_func	__PR((cmp_t * cmp, char* p, int amount));
LOCAL	BOOL	cmp_file	__PR((FINFO * info));
EXPORT	void	prdiffopts	__PR((FILE * f, char* label, int flags));
LOCAL	void	prdopt		__PR((FILE * f, char* name, int printed));

EXPORT void
diff()
{
		FINFO	finfo;
		TCB	tb;
		char	name[PATH_MAX+1];
	register TCB 	*ptb = &tb;

	fillbytes((char *)&finfo, sizeof(finfo), '\0');

	finfo.f_tcb = ptb;

	/*
	 * We first have to read a control block to know what type of
	 * tar archive we are reading from.
	 */
	if (get_tcb(ptb) == EOF)
		return;

	diffopts &= ~props.pr_diffmask;
	if (!no_stats)
		prdiffopts(stderr, "diffopts=", diffopts);

	for (;;) {
		finfo.f_name = name;
		if (tcb_to_info(ptb, &finfo) == EOF)
			return;
		if (listfile) {
			if (hash_lookup(finfo.f_name))
				diff_tcb(&finfo);
			else
				void_file(&finfo);
		} else if (npat == 0 || match(finfo.f_name)) {
			diff_tcb(&finfo);
		} else {
			void_file(&finfo);
		}
		if (get_tcb(ptb) == EOF)
			break;
	}
}

LOCAL void
diff_tcb(info)
	register FINFO	*info;
{
		char	lname[PATH_MAX+1];
		TCB	tb;
		FINFO	finfo;
		FINFO	linfo;
		FILE	*f;
		int	diffs = 0;
		BOOL	do_void;

	f = tarf == stdout ? stderr : stdout;

	finfo.f_lname = lname;
	finfo.f_lnamelen = 0;

	if (!abs_path &&	/* XXX VVV siehe skip_slash() */
	    (info->f_name[0] == '/' /*|| info->f_lname[0] == '/'*/))
		skip_slash(info);

	if (!getinfo(info->f_name, &finfo)) {
		errmsg("Cannot stat '%s'.\n", info->f_name);
		void_file(info);
		return;
	}
	fillbytes(&tb, sizeof(TCB), '\0');

/*XXX	name_to_tcb ist hier nicht mehr n�tig !! */
/*XXX	finfo.f_namelen = strlen(finfo.f_name);*/
/*XXX	name_to_tcb(&finfo, &tb);*/
	info_to_tcb(&finfo, &tb);	/* XXX ist das noch n�tig ??? */
					/* z.Zt. wegen linkflag/uname/gname */

	if ((diffopts & D_PERM) &&
			(info->f_mode & 07777) != (finfo.f_mode & 07777)) {
		diffs |= D_PERM;
/* XXX hat ustar modes incl. filetype ???? */
/*printf("%o %o\n", info->f_mode, finfo.f_mode);*/
	}
	if ((diffopts & D_UID) && info->f_uid != finfo.f_uid) {
		diffs |= D_UID;
	}
	if ((diffopts & D_GID) && info->f_gid != finfo.f_gid) {
		diffs |= D_GID;
	}
	if ((diffopts & D_UNAME) && info->f_uname && finfo.f_uname) {
		if (!streql(info->f_uname, finfo.f_uname))
			diffs |= D_UNAME;
	}
	if ((diffopts & D_GNAME) && info->f_gname && finfo.f_gname) {
		if (!streql(info->f_gname, finfo.f_gname))
			diffs |= D_GNAME;
	}

/*XXX hier kann es bei ustar/cpio inkompatibel werden! */
	if ((diffopts & D_TYPE) && (info->f_filetype != finfo.f_filetype ||
			(is_special(info) && info->f_type != finfo.f_type))) {
		if (debug || verbose)
			fprintf(f, "%s: different filetype  %lo != %lo\n",
				info->f_name, info->f_type, finfo.f_type);
		diffs |= D_TYPE;
	}

	if ((diffopts & D_ATIME) && info->f_atime != finfo.f_atime) {
		diffs |= D_ATIME;
	}
	if ((diffopts & D_MTIME) && info->f_mtime != finfo.f_mtime) {
		diffs |= D_MTIME;
	}
	if ((diffopts & D_CTIME) && info->f_ctime != finfo.f_ctime) {
		diffs |= D_CTIME;
	}

	if ((diffopts & D_HLINK) && is_link(info)) {
		if (!getinfo(info->f_lname, &linfo)) {
			errmsg("Cannot stat '%s'.\n", info->f_lname);
			linfo.f_ino = 0;
		}
		if (finfo.f_ino != linfo.f_ino) {
			if (debug || verbose)
				fprintf(f, "%s: not linked to %s\n",
					info->f_name, info->f_lname);

			diffs |= D_HLINK;
		}
		
	}
#ifdef	S_IFLNK
	if (((diffopts & D_SLINK) || verbose) && is_symlink(&finfo)) {
		if (read_symlink(info->f_name, &finfo, &tb)) {
			if ((diffopts & D_SLINK) && is_symlink(info) &&
			    !streql(info->f_lname, finfo.f_lname)) {
				diffs |= D_SLINK;
			}
		}
	}
#endif

	if ((diffopts & D_SIZE) && is_file(info) && is_file(&finfo)
					&& info->f_size != finfo.f_size) {
		diffs |= D_SIZE;
	}
	if ((diffopts & D_RDEV) && is_special(info) && is_special(&finfo)
					&& info->f_rdev != finfo.f_rdev) {
		diffs |= D_RDEV;
	}
	if ((diffopts & D_DATA) && is_file(info) && is_file(&finfo)
					/* avoid permission denied error */
					&& info->f_size > 0
					&& info->f_size == finfo.f_size) {
		if (!cmp_file(info)) {
			diffs |= D_DATA;
		}
		do_void = FALSE;
	} else {
		do_void = TRUE;
	}
	
	if (diffs) {
		fprintf(f, "%s: ", info->f_name);
		prdiffopts(f, "different ", diffs);
	}

	if (verbose && diffs) {
		list_file(info);
		list_file(&finfo);
	}
	if (do_void)
		void_file(info);
}

#define	vp_cmp_func	((int(*)__PR((void *, char *, int)))cmp_func)

LOCAL int
cmp_func(cmp, p, amount)
	register cmp_t	*cmp;
	register char	*p;
		 int	amount;
{
	register int	cnt;

	/*
	 * If we already found diffs we save time and only pass tape ...
	 */
	if (cmp->cmp_diffs)
		return (amount);

	cnt = fileread(cmp->cmp_file, cmp->cmp_buf, amount);
	if (cnt != amount)
		cmp->cmp_diffs++;

	if (cmpbytes(cmp->cmp_buf, p, cnt) < cnt)
		cmp->cmp_diffs++;
	return (cnt);
}

static char	*diffbuf;

LOCAL BOOL
cmp_file(info)
	FINFO	*info;
{
	FILE	*f;
	cmp_t	cmp;

	if (!diffbuf) {
		diffbuf = malloc((unsigned)bigsize);
		if (diffbuf == (char *)0) {
			errmsg("Cannot alloc diffbuf.\n");
		}
	}

	if ((f = fileopen(info->f_name, "ru")) == (FILE *)NULL) {
		errmsg("Cannot open '%s'.\n", info->f_name);
		void_file(info);
		return (FALSE);
	} else
		file_raise(f, FALSE);

	if (is_sparse(info))
		return (cmp_sparse(f, info));

	cmp.cmp_file = f;
	cmp.cmp_buf = diffbuf;
	cmp.cmp_diffs = 0;
	xt_file(info, vp_cmp_func, &cmp, bigsize, "reading");
	fclose(f);
	return (cmp.cmp_diffs == 0);
}

EXPORT void
prdiffopts(f, label, flags)
	FILE	*f;
	char	*label;
	int	flags;
{
	int	printed = 0;

	fprintf(f, "%s", label);
	if (flags & D_PERM)
		prdopt(f, "perm", printed++);
	if (flags & D_TYPE)
		prdopt(f, "type", printed++);
	if (flags & D_NLINK)
		prdopt(f, "nlink", printed++);
	if (flags & D_UID)
		prdopt(f, "uid", printed++);
	if (flags & D_GID)
		prdopt(f, "gid", printed++);
	if (flags & D_UNAME)
		prdopt(f, "uname", printed++);
	if (flags & D_GNAME)
		prdopt(f, "gname", printed++);
	if (flags & D_SIZE)
		prdopt(f, "size", printed++);
	if (flags & D_DATA)
/*		prdopt(f, "cont", printed++);*/
		prdopt(f, "data", printed++);
	if (flags & D_RDEV)
		prdopt(f, "rdev", printed++);
	if (flags & D_HLINK)
		prdopt(f, "hardlink", printed++);
	if (flags & D_SLINK)
		prdopt(f, "symlink", printed++);
	if (flags & D_ATIME)
		prdopt(f, "atime", printed++);
	if (flags & D_MTIME)
		prdopt(f, "mtime", printed++);
	if (flags & D_CTIME)
		prdopt(f, "ctime", printed++);
	fprintf(f, "\n");
}

LOCAL void
prdopt(f, name, printed)
	FILE	*f;
	char	*name;
	int	printed;
{
	if (printed)
		fprintf(f, ",");
	fprintf(f, name);
}