/* @(#)list.c	1.20 97/06/14 Copyright 1985, 1995 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)list.c	1.20 97/06/14 Copyright 1985, 1995 J. Schilling";
#endif
/*
 *	List the content of an archive
 *
 *	Copyright (c) 1985, 1995 J. Schilling
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
#include "star.h"
#include "table.h"
#include "dir.h"
#include <standard.h>
#include <strdefs.h>
#include "starsubs.h"

extern	FILE	*tarf;
extern	FILE	*vpr;
extern	char	*listfile;

extern	int	npat;
extern	BOOL	verbose;
extern	BOOL	tpath;
extern	BOOL	numeric;
extern	BOOL	verbose;
extern	BOOL	cflag;
extern	BOOL	xflag;
extern	BOOL	interactive;

extern	BOOL	acctime;
extern	BOOL	Ctime;

extern	BOOL	listnew;
extern	BOOL	listnewf;

EXPORT	void	list		__PR((void));
LOCAL	void	modstr		__PR((char* s, Ulong  mode));
EXPORT	void	list_file	__PR((FINFO * info));
EXPORT	void	vprint		__PR((FINFO * info));

EXPORT void
list()
{
		FINFO	finfo;
		FINFO	newinfo;
		TCB	tb;
		TCB	newtb;
		char	name[PATH_MAX+1];
		char	newname[PATH_MAX+1];
	register TCB 	*ptb = &tb;

	fillbytes((char *)&finfo, sizeof(finfo), '\0');
	fillbytes((char *)&newinfo, sizeof(newinfo), '\0');

	finfo.f_tcb = ptb;
	for (;;) {
		if (get_tcb(ptb) == EOF)
			break;
		finfo.f_name = name;
		if (tcb_to_info(ptb, &finfo) == EOF)
			return;
		if (listnew || listnewf) {
			if (finfo.f_mtime > newinfo.f_mtime &&
					(!listnewf || is_file(&finfo))) {
				movebytes(&finfo, &newinfo, sizeof(finfo));
				movebytes(&tb, &newtb, sizeof(tb));
				strcpy(newname, name);
				newinfo.f_name = newname;
				newinfo.f_flags |= F_HAS_NAME;
			}
		} else if (listfile) {
			if (hash_lookup(finfo.f_name))
				list_file(&finfo);
		} else if (npat == 0 || match(finfo.f_name))
			list_file(&finfo);

		void_file(&finfo);
	}
	if (listnew || listnewf) {
		/* XXX
		 * XXX Achtung!!! tcb_to_info zerstört t_name[NAMSIZ]
		 * XXX und t_linkname[NAMSIZ].
		 */
		tcb_to_info(&newtb, &newinfo);
		list_file(&newinfo);
	}
}

#ifdef	OLD
static char *typetab[] = 
{"S","-","l","d","","","","", };
#endif

LOCAL void
modstr(s, mode)
		 char	*s;
	register Ulong	mode;
{
	register char	*mstr = "xwrxwrxwr";
	register char	*str = s;
	register int	i;

	for (i=9; --i >= 0;) {
		if (mode & (1 << i))
			*str++ = mstr[i];
		else
			*str++ = '-';
	}
	*str = '\0';
	str = s;
	if (mode & 01000) {
		if (mode & 01)
			str[8] = 't';
		else
			str[8] = 'T';
	}
	if (mode & 02000) {
		if (mode & 010)
			str[5] = 's';
		else
			str[5] = 'S';
	}
	if (mode & 04000) {
		if (mode & 0100)
			str[2] = 's';
		else
			str[2] = 'S';
	}
}

EXPORT void
list_file(info)
	register FINFO	*info;
{
		FILE	*f;
		time_t	*tp;
		char	*tstr;
		char	mstr[10];
	static	char	nuid[11];
	static	char	ngid[11];

	f = vpr;
	if (verbose) {
		tp = (time_t *) (acctime ? &info->f_atime :
				(Ctime ? &info->f_ctime : &info->f_mtime));
		tstr = ctime(tp);
		if (numeric || info->f_uname == NULL) {
			sprintf(nuid, "%lu", info->f_uid);
			info->f_uname = nuid;
			info->f_umaxlen = sizeof(nuid)-1;
		}
		if (numeric || info->f_gname == NULL) {
			sprintf(ngid, "%lu", info->f_gid);
			info->f_gname = ngid;
			info->f_gmaxlen = sizeof(ngid)-1;
		}

		if (is_special(info))
			fprintf(f, "%3ld %3ld",
				info->f_rdevmaj, info->f_rdevmin);
		else
			fprintf(f, "%7lu", info->f_size);
		modstr(mstr, info->f_mode);

/*
 * XXX Übergangsweise, bis die neue Filetypenomenklatur sauber eingebaut ist.
 */
if (info->f_xftype == 0) {
	info->f_xftype = IFTOXT(info->f_type);
	errmsgno(BAD, "XXXXX xftype == 0\n");
}
		fprintf(f,
			" %s%s %3.*s/%-3.*s %.12s %4.4s ",
#ifdef	OLD
			typetab[info->f_filetype & 07],
#else
			XTTOSTR(info->f_xftype),
#endif
			mstr,
			(int)info->f_umaxlen, info->f_uname,
			(int)info->f_gmaxlen, info->f_gname,
			&tstr[4], &tstr[20]);
	}
	fprintf(f, "%s", info->f_name);
	if(tpath) {
		fprintf(f, "\n");
		return;
	}
	if (is_link(info))
		fprintf(f, " link to %s", info->f_lname);
	if (is_symlink(info))
		fprintf(f, " -> %s", info->f_lname);
	if (is_volhdr(info))
		fprintf(f, " --Volume Header--");
	if (is_multivol(info))
		fprintf(f, " --Continued at byte %ld--", info->f_offset);
	fprintf(f, "\n");
}

EXPORT void
vprint(info)
	FINFO	*info;
{
		FILE	*f;
	char	*mode;

	if (verbose || interactive) {
		f = vpr;

		if (cflag)
			mode = "a ";
		else if (xflag)
			mode = "x ";
		else
			mode = "";
		if (is_dir(info)) {
			fprintf(f, "%s%s directory\n", mode, info->f_name);
		} else if (is_link(info)) {
			fprintf(f, "%s%s link to %s\n",
				mode, info->f_name, info->f_lname);
		} else if (is_symlink(info)) {
			fprintf(f, "%s%s symbolic link to %s\n",
				mode, info->f_name, info->f_lname);
		} else if (is_special(info)) {
			fprintf(f, "%s%s special\n", mode, info->f_name);
		} else {
			fprintf(f, "%s%s %ld bytes, %ld tape blocks\n",
				mode, info->f_name, info->f_size,
				tarblocks(info->f_rsize));
		}
	}
}
