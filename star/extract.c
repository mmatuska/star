/* @(#)extract.c	1.21 97/06/14 Copyright 1985 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)extract.c	1.21 97/06/14 Copyright 1985 J. Schilling";
#endif
/*
 *	extract files from archive
 *
 *	Copyright (c) 1985 J. Schilling
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
#include <standard.h>
#include "star.h"
#include "props.h"
#include "table.h"
#include "dir.h"	/*XXX Wegen S_IFLNK */
#include <timedefs.h>
#include <unixstd.h>

#ifdef	JOS
#	include	<error.h>
#	define	mkdir	makedir
#else
#	include	<errno.h>
#	define	EMISSDIR	ENOENT
#endif
#include "dirtime.h"
#include "starsubs.h"

extern	char	*listfile;

extern	int	bufsize;
extern	char	*bigptr;

extern	int	npat;
extern	BOOL	nflag;
extern	BOOL	interactive;
extern	BOOL	nodir;
extern	BOOL	nospec;
extern	BOOL	xdir;
extern	BOOL	uncond;
extern	BOOL	keep_old;
extern	BOOL	abs_path;
extern	BOOL	nowarn;
extern	BOOL	force_hole;
extern	BOOL	to_stdout;
extern	BOOL	remove_first;

EXPORT	void	extract		__PR((void));
EXPORT	BOOL	newer		__PR((FINFO * info));
LOCAL	BOOL	same_symlink	__PR((FINFO * info));
LOCAL	BOOL	remove_file	__PR((char* name, BOOL isfirst));
LOCAL	BOOL	create_dirs	__PR((char* name));
LOCAL	BOOL	make_dir	__PR((FINFO * info));
LOCAL	BOOL	make_link	__PR((FINFO * info));
LOCAL	BOOL	make_symlink	__PR((FINFO * info));
LOCAL	BOOL	make_special	__PR((FINFO * info));
LOCAL	BOOL	get_file	__PR((FINFO * info));
LOCAL	int	void_func	__PR((void *vp, char* p, int amount));
EXPORT	BOOL	void_file	__PR((FINFO * info));
EXPORT	BOOL	xt_file		__PR((FINFO * info,
					int (*)(void *, char *, int),
					void *arg, int amt, char* text));
EXPORT	void	skip_slash	__PR((FINFO * info));

EXPORT void
extract()
{
		FINFO	finfo;
		TCB	tb;
		char	name[PATH_MAX+1];
	register TCB 	*ptb = &tb;

	fillbytes((char *)&finfo, sizeof(finfo), '\0');

	finfo.f_tcb = ptb;
	for (;;) {
		if (get_tcb(ptb) == EOF)
			break;
		finfo.f_name = name;
		if (tcb_to_info(ptb, &finfo) == EOF)
			return;
		if (is_volhdr(&finfo)) {
			get_volhdr(&finfo);
			continue;
		}
		if (!abs_path &&	/* XXX VVV siehe skip_slash() */
		    (finfo.f_name[0] == '/'/* || finfo.f_lname[0] == '/'*/))
			skip_slash(&finfo);
		if (listfile) {
			if (!hash_lookup(finfo.f_name)) {
				void_file(&finfo);
				continue;
			}	
		} else if (npat > 0 && !match(finfo.f_name)) {
			void_file(&finfo);
			continue;
		}
		if (!is_file(&finfo) && to_stdout) {
			void_file(&finfo);
			continue;
		}
		if (is_special(&finfo) && nospec) {
			errmsgno(BAD, "'%s' is not a file. Not created.\n",
							finfo.f_name) ;
			continue;
		}
		if (newer(&finfo) && !(xdir && is_dir(&finfo))) {
			void_file(&finfo);
			continue;
		}
		if (is_symlink(&finfo) && same_symlink(&finfo)) {
			continue;
		}
		if (interactive && !ia_change(ptb, &finfo)) {
			if (!nflag)
				printf("Skipping ...\n");
			void_file(&finfo);
			continue;
		}
		vprint(&finfo);
		if (remove_first) {
			/*
			 * With keep_old we do not come here.
			 */ 
			(void)remove_file(finfo.f_name, TRUE);
		}
		if (is_dir(&finfo)) {
			if (!make_dir(&finfo))
				continue;
		} else if (is_link(&finfo)) {
			if (!make_link(&finfo))
				continue;
		} else if (is_symlink(&finfo)) {
			if (!make_symlink(&finfo))
				continue;
		} else if (is_special(&finfo)) {
			if (!make_special(&finfo))
				continue;
		} else if (!get_file(&finfo))
				continue;
		if (!to_stdout)
			setmodes(&finfo);
	}
	dirtimes("", (struct timeval *)0);
}

EXPORT BOOL
newer(info)
	FINFO	*info;
{
	FINFO	cinfo;

	if (uncond || !getinfo(info->f_name, &cinfo))
		return (FALSE);
	if (keep_old) {
		if (!nowarn)
			errmsgno(BAD, "file '%s' exists.\n", info->f_name);
		return (TRUE);
	}
	if (cinfo.f_mtime >= info->f_mtime) {
		if (!nowarn)
			errmsgno(BAD, "current '%s' newer.\n", info->f_name);
		return (TRUE);
	}
	return (FALSE);
}

LOCAL BOOL
same_symlink(info)
	FINFO	*info;
{
	FINFO	finfo;
	char	lname[PATH_MAX+1];
	TCB	tb;

	finfo.f_lname = lname;
	finfo.f_lnamelen = 0;

	if (uncond || !getinfo(info->f_name, &finfo))
		return (FALSE);

	/*
	 * Bei symlinks gehen nicht: lchmod lchtime & teilweise lchown
	 */
#ifdef	S_IFLNK
	if (!is_symlink(&finfo))	/* File on disk */
		return (FALSE);

	fillbytes(&tb, sizeof(TCB), '\0');
	info_to_tcb(&finfo, &tb);	/* XXX ist das noch nötig ??? */
					/* z.Zt. wegen linkflag/uname/gname */

	if (read_symlink(info->f_name, &finfo, &tb)) {
		if (streql(info->f_lname, finfo.f_lname)) {
			if (!nowarn)
				errmsgno(BAD, "current '%s' is same symlink.\n",
								info->f_name);
			return (TRUE);
		}
	}
#ifdef	XXX
	if (finfo.f_mtime >= info->f_mtime) {
		if (!nowarn)
			errmsgno(BAD, "current '%s' newer.\n", info->f_name);
		return (TRUE);
	}
#endif	/* XXX*/

#endif
	return (FALSE);
}

LOCAL BOOL
remove_file(name, isfirst)
	register char	*name;
		 BOOL	isfirst;
{
	char	buf[32];
	char	ans;
	int	err = EX_BAD;
extern	FILE	*tty;
extern	BOOL	interactive;
extern	BOOL	force_remove;
extern	BOOL	ask_remove;

	if (remove_first && !isfirst)
		return (FALSE);
	if (interactive || ask_remove) {
		printf("remove '%s' ? Y(es)/N(o) :", name);flush();
		fgetline(tty, buf, 2);
	}
	if (force_remove ||
	    ((interactive || ask_remove) && (ans = toupper(buf[0])) == 'Y')) {

		/*
		 * only unlink non directories or empty
		 * directories
		 * XXX need to implement the -remove_recursive flag
		 */
		if (rmdir(name) < 0) {
			err = geterrno();
			if (err == EACCES)
				goto cannot;

			if (err == ENOTDIR) {
				if (unlink(name) < 0) {
					err = geterrno();
					goto cannot;
				}
			}
		}
		return (TRUE);
	}
cannot:
	errmsgno(err, "File '%s' not removed.\n", name);
	return (FALSE);
}

LOCAL BOOL
create_dirs(name)
	register char	*name;
{
	register char	*np;
	register char	*dp;

	if (nodir) {
		errmsgno(BAD, "Directories not created.\n");
		return (FALSE);
	}
	np = dp = name;
	do {
		if (*np == '/')
			dp = np;
	} while (*np++);
	if (dp == name)
		return TRUE;
	*dp = '\0';
	if (access(name, 0) < 0) {
		if (mkdir(name, 0777) < 0) {
			if (!create_dirs(name) || mkdir(name, 0777) < 0) {
				*dp = '/';
				return FALSE;
			}
		}
	}
	*dp = '/';
	return TRUE;
}

LOCAL BOOL
make_dir(info)
	FINFO	*info;
{
	FINFO	dinfo;

	if (create_dirs(info->f_name)) {
		if (getinfo(info->f_name, &dinfo) && is_dir(&dinfo))
			return (TRUE);
		if (uncond)
			unlink(info->f_name);
		if (mkdir(info->f_name, 0777) >= 0)
			return (TRUE);
	}
	errmsg("Cannot make dir '%s'.\n", info->f_name);
	return FALSE;
}

LOCAL BOOL
make_link(info)
	FINFO	*info;
{
	if (uncond)
		unlink(info->f_name);
	if (link(info->f_lname, info->f_name) >= 0)
		return (TRUE);
	if (create_dirs(info->f_name))
		if (link(info->f_lname, info->f_name) >= 0)
			return (TRUE);
	errmsg("Cannot create '%s'.\n", info->f_name);
	return(FALSE);
}

LOCAL BOOL
make_symlink(info)
	FINFO	*info;
{
#ifdef	S_IFLNK
	if (uncond)
		unlink(info->f_name);
	if (sxsymlink(info) >= 0)
		return (TRUE);
	if (create_dirs(info->f_name))
		if (sxsymlink(info) >= 0)
			return (TRUE);
	errmsg("Cannot create symbolic link '%s'.\n", info->f_name);
	return (FALSE);
#else	/* S_IFLNK */
	errmsgno(BAD, "Not supported. Cannot create symbolic link '%s'.\n",
							info->f_name);
	return (FALSE);
#endif	/* S_IFLNK */
}

LOCAL BOOL
make_special(info)
	FINFO	*info;
{
	int	mode;
	int	dev;

	mode = info->f_mode | info->f_type;
	dev = info->f_rdev;
	if (uncond)
		unlink(info->f_name);
	if (mknod(info->f_name, mode, dev) >= 0)
		return (TRUE);
	if (create_dirs(info->f_name))
		if (mknod(info->f_name, mode, dev) >= 0)
			return (TRUE);
	errmsg("Cannot make special '%s'.\n", info->f_name);
	return (FALSE);
}

LOCAL BOOL
get_file(info)
		FINFO	*info;
{
		FILE	*f;

	if (to_stdout) {
		f = stdout;
	} else if ((f = fileopen(info->f_name, "wctu")) == (FILE *)NULL) {
		if (geterrno() == EMISSDIR && create_dirs(info->f_name))
			return get_file(info);
		if (geterrno() == EACCES && remove_file(info->f_name, FALSE))
			return get_file(info);

		errmsg("Cannot create '%s'.\n", info->f_name);
		void_file(info);
		return (FALSE);
	}
	file_raise(f, FALSE);

	if (is_sparse(info))
		return (get_sparse(f, info));
	if (force_hole)
		return (get_forced_hole(f, info));

	xt_file(info, (int(*)__PR((void *, char *, int)))ffilewrite, f, 0,
								"writing");

	if (!to_stdout)
		fclose(f);
	return (TRUE);
}

/* ARGSUSED */
LOCAL int
void_func(vp, p, amount)
	void	*vp;
	char	*p;
	int	amount;
{
	return (amount);
}

EXPORT BOOL
void_file(info)
		FINFO	*info;
{
	/*
	 * handle botch in gnu sparse file definitions
	 */
	if (props.pr_flags & PR_GNU_SPARSE_BUG)
		gnu_skip_extended(info->f_tcb);

	return (xt_file(info, void_func, 0, 0, "void"));
}

EXPORT BOOL
xt_file(info, func, arg, amt, text)
		FINFO	*info;
		int	(*func) __PR((void *, char *, int));
		void	*arg;
		int	amt;
		char	*text;
{
	register int	amount;
	register long	size;
	register int	tsize;
		 BOOL	ret = TRUE;

	size = info->f_rsize;
	if (amt == 0)
		amt = bufsize;
	while (size > 0) {
		amount = buf_rwait(TBLOCK);
		if (amount == 0)
			excomerrno(BAD, "Tar file too small.\n");
		amount = min(size, amount);
		amount = min(amount, amt);
		tsize = tarsize(amount);

		if ((*func)(arg, bigptr, amount) != amount) {
			ret = FALSE;
			errmsg("Error %s '%s'.\n", text, info->f_name);
		}

		size -= amount;
		buf_rwake(tsize);
	}
	return (ret);
}

EXPORT void
skip_slash(info)
	FINFO	*info;
{
	static	BOOL	warned = FALSE;

	if (!warned && !nowarn) {
		errmsgno(BAD, "Warning: skipping leading '/' on filenames.\n");
		warned = TRUE;
	}
	/* XXX
	 * XXX ACHTUNG: ia_change kann es nötig machen, den String umzukopieren
	 * XXX denn sonst ist die Länge des Speicherplatzes unbestimmt!
	 *
	 * XXX ACHTUNG: mir ist noch unklar, ob es richtig ist, auch in jedem
	 * XXX Fall Führende slashes vom Linknamen zu entfernen.
	 * XXX Bei Hard-Link ist das sicher richtig und ergibt sich auch
	 * XXX automatisch, wenn man nur vor dem Aufruf von skip_slash()
	 * XXX auf f_name[0] == '/' abfragt.
	 */
	while (info->f_name[0] == '/')
		info->f_name++;
	while (info->f_lname[0] == '/')
		info->f_lname++;
}
