/* @(#)create.c	1.40 01/04/07 Copyright 1985, 1995 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)create.c	1.40 01/04/07 Copyright 1985, 1995 J. Schilling";
#endif
/*
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
#include "star.h"
#include "props.h"
#include "table.h"
#include <errno.h>	/* XXX seterrno() is better JS */
#include <standard.h>
#include <stdxlib.h>
#include <unixstd.h>
#include <dirdefs.h>
#include <strdefs.h>
#include <schily.h>
#include "starsubs.h"

typedef	struct	links {
	struct	links	*l_next;
		long	l_ino;
		long	l_dev;
		long	l_nlink;
		short	l_namlen;
		char	l_name[1];	/* actually longer */
} LINKS;

#define	L_HSIZE		256		/* must be a power of two */

#define	l_hash(info)	(((info)->f_ino + (info)->f_dev) & (L_HSIZE-1))

LOCAL	LINKS	*links[L_HSIZE];

extern	FILE	*listf;

extern	long	tape_dev;
extern	long	tape_ino;
#define	is_tape(info)		((info)->f_dev == tape_dev && (info)->f_ino == tape_ino)

extern	int	bufsize;
extern	char	*bigptr;

extern	BOOL	havepat;
extern	Ulong	curfs;
extern	Ulong	maxsize;
extern	Ulong	Newer;
extern	Ulong	tsize;
extern	BOOL	debug;
extern	BOOL	uflag;
extern	BOOL	nodir;
extern	BOOL	acctime;
extern	BOOL	dirmode;
extern	BOOL	nodesc;
extern	BOOL	nomount;
extern	BOOL	interactive;
extern	BOOL	nospec;
extern	int	Fflag;
extern	BOOL	abs_path;
extern	BOOL	nowarn;
extern	BOOL	sparse;
extern	BOOL	Ctime;
extern	BOOL	nullout;

extern	int	intr;

EXPORT	void	checklinks	__PR((void));
LOCAL	BOOL	take_file	__PR((char* name, FINFO * info));
EXPORT	int	_fileopen	__PR((char *name, char *mode));
EXPORT	int	_fileread	__PR((int *fp, void *buf, int len));
EXPORT	void	create		__PR((char* name));
LOCAL	void	createi		__PR((char* name, int namlen, FINFO * info));
EXPORT	void	createlist	__PR((void));
EXPORT	BOOL	read_symlink	__PR((char* name, FINFO * info, TCB * ptb));
LOCAL	BOOL	read_link	__PR((char* name, int namlen, FINFO * info,
								TCB * ptb));
LOCAL	int	nullread	__PR((void *vp, char *cp, int amt));
EXPORT	void	put_file	__PR((int *fp, FINFO * info));
EXPORT	void	cr_file		__PR((FINFO * info,
					int (*)(void *, char *, int),
					void *arg, int amt, char* text));
LOCAL	void	put_dir		__PR((char* dname, int namlen, FINFO * info,
								TCB * ptb));
LOCAL	BOOL	checkdirexclude	__PR((char *name, int namlen, FINFO *info));
EXPORT	BOOL	checkexclude	__PR((char *name, int namlen, FINFO *info));

EXPORT void
checklinks()
{
	register LINKS	*lp;
	register int	i;
	register int	used	= 0;
	register int	curlen;
	register int	maxlen	= 0;
	register int	nlinks	= 0;

	for(i=0; i < L_HSIZE; i++) {
		if (links[i] == (LINKS *)NULL)
			continue;

		curlen = 0;
		used++;

		for(lp = links[i]; lp != (LINKS *)NULL; lp = lp->l_next) {
			curlen++;
			nlinks++;
			if (lp->l_nlink != 0) {
				xstats.s_misslinks++;
				errmsgno(EX_BAD, "Missing links to '%s'.\n",
								lp->l_name);
			}
		}
		if (maxlen < curlen)
			maxlen = curlen;
	}
	if (debug) {
		errmsgno(EX_BAD, "hardlinks: %d hashents: %d/%d maxlen: %d\n",
						nlinks, used, L_HSIZE, maxlen);
	}
}

LOCAL BOOL
take_file(name, info)
	register char	*name;
	register FINFO	*info;
{
	if (havepat && !match(name)) {
		return (FALSE);
			/* Bei Directories ist f_size == 0 */
	} else if (maxsize && info->f_size/1024 > maxsize) {
		return (FALSE);
	} else if (Newer && (Ctime ? info->f_ctime:info->f_mtime) <= Newer) {
		/*
		 * XXX nsec beachten wenn im Archiv!
		 */
		return (FALSE);
	} else if (uflag && !update_newer(info)) {
		return (FALSE);
	} else if (tsize > 0 && tsize < (tarblocks(info->f_size)+1+2)) {
		xstats.s_toobig++;
		errmsgno(EX_BAD, "'%s' does not fit on tape. Not dumped.\n",
								name) ;
		return (FALSE);
	} else if (is_special(info) && nospec) {
		xstats.s_isspecial++;
		errmsgno(EX_BAD, "'%s' is not a file. Not dumped.\n", name) ;
		return (FALSE);
	} else if (is_tape(info)) {
		errmsgno(EX_BAD, "'%s' is the archive. Not dumped.\n", name) ;
		return (FALSE);
	}
	return (TRUE);
}

int
_fileopen(name, smode)
	char	*name;
	char	*smode;
{
	int	ret;
	int	omode = 0;
	int	flag = 0;

	if (!_cvmod (smode, &omode, &flag))
		return (-1);

	if ((ret = _openfd(name, omode)) < 0)
		return (-1);

	return (ret);
}

int _fileread(fp, buf, len)
	register int	*fp;
	void	*buf;
	int	len;
{
	register int	fd = *fp;
	register int	ret;

	while((ret = read(fd, buf, len)) < 0 && errno == EINTR)
		;
	return(ret);
}

EXPORT void
create(name)
	register char	*name;
{
		FINFO	finfo;
	register FINFO	*info	= &finfo;

	if (name[0] == '.' && name[1] == '/')
		for (name++; name[0] == '/'; name++);
	if (name[0] == '\0')
		name = ".";
	if (!getinfo(name, info)) {
		xstats.s_staterrs++;
		errmsg("Cannot stat '%s'.\n", name);
	} else {
		createi(name, strlen(name), info);
	}
}

LOCAL void
createi(name, namlen, info)
	register char	*name;
		 int	namlen;
	register FINFO	*info;
{
		char	lname[PATH_MAX+1];
		TCB	tb;
	register TCB	*ptb		= &tb;
		int	fd		= -1;
		BOOL	was_link	= FALSE;
		BOOL	do_sparse	= FALSE;

	info->f_name = name;	/* XXX Das ist auch in getinfo !!!?!!! */
	info->f_namelen = namlen;
	if (Fflag > 0 && !checkexclude(name, namlen, info))
		return;

#ifdef	nonono_NICHT_BEI_CREATE	/* XXX */
	if (!abs_path &&	/* XXX VVV siehe skip_slash() */
		(info->f_name[0] == '/' /*|| info->f_lname[0] == '/'*/))
		skip_slash(info);
		info->f_namelen -= info->f_name - name;
		if (info->f_namelen == 0) {
			info->f_name = "./";
			info->f_namelen = 2;
		}
		/* XXX das gleiche mit f_lname !!!!! */
	}
#endif	/* nonono_NICHT_BEI_CREATE	XXX */
	info->f_lname = lname;	/*XXX nur Übergangsweise!!!!!*/
	info->f_lnamelen = 0;

	if (!(dirmode && is_dir(info)) &&
				(info->f_namelen <= props.pr_maxsname)) {
		/*
		 * Allocate TCB from the buffer to avoid copying TCB
		 * in the most frequent case.
		 * If we are writing directories after the files they
		 * contain, we cannot allocate the space for tcb
		 * from the buffer.
		 * With very long names we will have to write out 
		 * other data before we can write the TCB, so we cannot
		 * alloc tcb from buffer too.
		 */
		ptb = (TCB *)get_block();
		info->f_flags |= F_TCB_BUF;
	}
	info->f_tcb = ptb;
	fillbytes((char *)ptb, TBLOCK, '\0');
	if (!name_to_tcb(info, ptb))	/* Name too long */
		return;

	info_to_tcb(info, ptb);
	if (is_dir(info))
		put_dir(name, namlen, info, ptb);
	else if (!take_file(name, info))
		return;
	else if (interactive && !ia_change(ptb, info))
		printf("Skipping ...\n");
	else if (is_symlink(info) && !read_symlink(name, info, ptb))
		;
	else if (is_file(info) && info->f_size != 0 && !nullout &&
				(fd = _fileopen(name,"rb")) < 0) {
		xstats.s_openerrs++;
		errmsg("Cannot open '%s'.\n", name);
	} else {
		if (info->f_nlink > 1 && read_link(name, namlen, info, ptb))
			was_link = TRUE;

		if (was_link && !is_link(info))	/* link name too long */
			return;

		do_sparse = (info->f_flags & F_SPARSE) && sparse &&
						props.pr_flags & PR_SPARSE;

		if (do_sparse && nullout &&
				(fd = _fileopen(name,"rb")) < 0) {
			xstats.s_openerrs++;
			errmsg("Cannot open '%s'.\n", name);
			return;
		}

		if (was_link || !do_sparse) {
			put_tcb(ptb, info);
			vprint(info);
		}
		if (is_file(info) && !was_link && info->f_rsize > 0) {
			/*
			 * Don't dump hardlinks and empty files
			 * Hardlinks have f_rsize == 0 !
			 */
			if (do_sparse) {
				error("%s is sparse\n", info->f_name);
				put_sparse(&fd, info);
			} else {
				put_file(&fd, info);
			}
		}
		/*
		 * Reset access time of file.
		 * This is important when using star for dumps.
		 * N.B. this has been done after fclose()
		 * before _FIOSATIME has been used.
		 *
		 * If f == NULL, the file has not been accessed for read
		 * and access time need not be reset.
		 */
		if (acctime && fd >= 0)
			rs_acctime(fd, info);
		if (fd >= 0)
			close(fd);
	}
}

EXPORT void
createlist()
{
	register int	nlen;
		char	*name;
		int	nsize = PATH_MAX+1;	/* wegen laenge !!! */

	if ((name = malloc(nsize)) == 0)
		comerr("Cannot alloc name buffer.\n");

	for (nlen = 1; nlen > 0;) {
		if ((nlen = fgetline(listf, name, nsize)) < 0)
			break;
		if (nlen == 0)
			continue;
		if (nlen >= PATH_MAX) {
			xstats.s_toolong++;
			errmsgno(EX_BAD, "%s: Name too long (%d > %d).\n",
							name, nlen, PATH_MAX);
			continue;
		}
		if (intr)
			break;
		curfs = -1L;
		create(name);
	}
}

EXPORT BOOL
read_symlink(name, info, ptb)
	char	*name;
	register FINFO	*info;
	TCB	*ptb;
{
	int	len;

	info->f_lname[0] = '\0';
	if ((len = readlink(name, info->f_lname, PATH_MAX)) < 0) {
		xstats.s_rwerrs++;
		errmsg("Cannot read link '%s'.\n", name);
		return (FALSE);
	}
	info->f_lnamelen = len;
	if (len > props.pr_maxlnamelen) {
		xstats.s_toolong++;
		errmsgno(EX_BAD, "%s: Symbolic link too long.\n", name);
		return (FALSE);
	}
	if (len > props.pr_maxslname)
		info->f_flags |= F_LONGLINK;
	/*
	 * string from readlink is not null terminated
	 */
	info->f_lname[len] = '\0';
	/*
	 * if linkname is not longer than props.pr_maxslname
	 * that's all to do with linkname
	 */
	strncpy(ptb->dbuf.t_linkname, info->f_lname, props.pr_maxslname);
	return (TRUE);
}

LOCAL BOOL
read_link(name, namlen, info, ptb)
	char	*name;
	int	namlen;
	register FINFO	*info;
	TCB	*ptb;
{
	register LINKS	*lp;
	register LINKS	**lpp;
		 int	i = l_hash(info);

	lp = links[i];
	lpp = &links[i];

	for (; lp != (LINKS *)NULL; lp = lp->l_next) {
		if (lp->l_ino == info->f_ino && lp->l_dev == info->f_dev) {
			if (lp->l_namlen > props.pr_maxlnamelen) {
				xstats.s_toolong++;
				errmsgno(EX_BAD, "%s: Link name too long.\n",
								lp->l_name);
				return (TRUE);
			}
			if (lp->l_namlen > props.pr_maxslname)
				info->f_flags |= F_LONGLINK;
			if (--lp->l_nlink < 0) {
				if (!nowarn)
					errmsgno(EX_BAD,
					"%s: Linkcount below zero (%ld)\n",
						lp->l_name, lp->l_nlink);
			}
			/*
			 * if linkname is not longer than props.pr_maxslname
			 * that's all to do with linkname
			 */
			strncpy(ptb->dbuf.t_linkname, lp->l_name,
							props.pr_maxslname);
			info->f_lname = lp->l_name;
			info->f_lnamelen = lp->l_namlen;
			info->f_xftype = XT_LINK;
			/*
			 * XXX Dies ist eine ungewollte Referenz auf den
			 * XXX TAR Control Block, aber hier ist der TCB
			 * XXX schon fertig und wir wollen nur den Typ
			 * XXX Modifizieren.
			 */
			ptb->dbuf.t_linkflag = LNKTYPE;
			return (TRUE);
		}
	}
	if ((lp = (LINKS *)malloc(sizeof(*lp)+namlen)) == (LINKS *)NULL) {
		errmsg("Cannot alloc new link for '%s'.\n", name);
	} else {
		lp->l_next = *lpp;
		*lpp = lp;
		lp->l_ino = info->f_ino;
		lp->l_dev = info->f_dev;
		lp->l_nlink = info->f_nlink - 1;
		lp->l_namlen = namlen;
		strcpy(lp->l_name, name);
	}
	return (FALSE);
}

LOCAL int
nullread(vp, cp, amt)
	void	*vp;
	char	*cp;
	int	amt;
{
	return (amt);
}

EXPORT void
put_file(fp, info)
	register int	*fp;
	register FINFO	*info;
{
	if (nullout) {
		cr_file(info, (int(*)__PR((void *, char *, int)))nullread,
							fp, 0, "reading");
	} else {
		cr_file(info, (int(*)__PR((void *, char *, int)))_fileread,
							fp, 0, "reading");
	}
}

EXPORT void
cr_file(info, func, arg, amt, text)
		FINFO	*info;
		int	(*func) __PR((void *, char *, int));
	register void	*arg;
		int	amt;
		char	*text;
{
	register int	amount;
	register int	blocks;
	register long	size;
	register int	i = 0;
	register int	n;

	size = info->f_rsize;
	if ((blocks = tarblocks(info->f_rsize)) == 0)
		return;
	if (amt == 0)
		amt = bufsize;
	do {
		amount = buf_wait(TBLOCK);
		amount = min(amount, amt);

		if ((i = (*func)(arg, bigptr, max(amount, TBLOCK))) <= 0)
			break;

		size -= i;
		if (size < 0) {			/* File increased in size */
			n = tarblocks(size+i);	/* use expected size only */
		} else {
			n = tarblocks(i);
		}
		buf_wake(n*TBLOCK);
	} while ((blocks -= n) >= 0 && i == amount && size >= 0);
	if (i < 0) {
		xstats.s_rwerrs++;
		errmsg("Error %s '%s'.\n", text, info->f_name);
	} else if ((blocks != 0 || size != 0) && func != nullread) {
		xstats.s_sizeerrs++;
		errmsgno(EX_BAD, "'%s': file changed size.\n", info->f_name);
	}
	while(--blocks >= 0)
		writeempty();
}

#define	newfs(i)	((i)->f_dev != curfs)

LOCAL void
put_dir(dname, namlen, info, ptb)
	register char	*dname;
	register int	namlen;
	FINFO	*info;
	TCB	*ptb;
{
	static	int	depth	= -10;
	static	int	dinit	= 0;
		FINFO	nfinfo;
	register FINFO	*ninfo	= &nfinfo;
		DIR	*d;
	struct	dirent	*dir;
		long	offset	= 0L;
		char	fname[PATH_MAX+1];	/* XXX */
	register char	*name;
	register char	*xdname;
		 int	xlen;
		 BOOL	putdir = FALSE;

	if (!dinit) {
#ifdef	_SC_OPEN_MAX
		depth += sysconf(_SC_OPEN_MAX);
#else
		depth += getdtablesize();
#endif
		dinit = 1;
	}

	if (!(d = opendir(dname))) {
		xstats.s_openerrs++;
		errmsg("Cannot open '%s'.\n", dname);
	} else {
		depth--;
		if (!nodir) {
			if (interactive && !ia_change(ptb, info)) {
				printf("Skipping ...\n");
				closedir(d);
				depth++;
				return;
			}
			if (take_file(dname, info)) {
				putdir = TRUE;
				if (!dirmode)
					put_tcb(ptb, info);
				vprint(info);
			}
		}
		if (!nodesc && (!nomount || !newfs(info))) {

			strcpy(fname, dname);
			xdname = &fname[namlen];
			if (namlen && xdname[-1] != '/') {
				namlen++;
				*xdname++ = '/';
			}

			while ((dir = readdir(d)) != NULL && !intr) {
				if (streql(dir->d_name, ".") ||
						streql(dir->d_name, ".."))
					continue;
				xlen = namlen + strlen(dir->d_name);
				if (xlen > PATH_MAX) {
					*xdname = '\0';
					xstats.s_toolong++;
					errmsgno(EX_BAD,
						"%s%s: Name too long (%d > %d).\n",
							fname, dir->d_name,
							xlen, PATH_MAX);
					continue;
				}

				strcpy(xdname, dir->d_name);
				name = fname;

				if (name[0] == '.' && name[1] == '/') {
					for (name++; name[0] == '/'; name++);
					xlen -= name - fname;
				}
				if (name[0] == '\0') {
					name = ".";
					xlen = 1;
				}
				if (!getinfo(name, ninfo)) {
					xstats.s_staterrs++;
					errmsg("Cannot stat '%s'.\n", name);
					continue;
				}
#ifdef	HAVE_SEEKDIR
				if (is_dir(ninfo) && depth <= 0) {
					errno = 0;
					offset = telldir(d);
					if (geterrno())
						errmsg("WARNING: telldir does not work.\n");
					/*
					 * XXX What should we do if telldir
					 * XXX does not work.
					 */
					closedir(d);
				}
#endif
				createi(name, xlen, ninfo);
#ifdef	HAVE_SEEKDIR
				if (is_dir(ninfo) && depth <= 0) {
					if (!(d = opendir(dname))) {
						xstats.s_openerrs++;
						errmsg("Cannot open '%s'.\n",
								dname);
						break;
					} else {
						errno = 0;
						seekdir(d, offset);
						if (geterrno())
							errmsg("WARNING: seekdir does not work.\n");
					}
				}
#endif
			}
		}
		closedir(d);
		depth++;
		if (!nodir && dirmode && putdir)
			put_tcb(ptb, info);
	}
}

LOCAL BOOL
checkdirexclude(name, namlen, info)
	char	*name;
	int	namlen;
	FINFO	*info;
{
	FINFO	finfo;
	char	pname[PATH_MAX+1];
	int	OFflag = Fflag;
	int	len;
	char	*p;

	Fflag = 0;
	len = namlen;
	strcpy(pname, name);
	p = &pname[len];
	if (p[-1] != '/') {
		*p++ = '/';
		len++;
	}
	strcpy(p, ".mirror");
	len += 7;
	if (!getinfo(pname, &finfo)) {
		strcpy(p, ".exclude");
		len += 1;
		if (!getinfo(pname, &finfo))
			goto notfound;
	}
	if (is_file(&finfo)) {
		if (OFflag == 3) {
			nodesc++;
			if (!dirmode)
				createi(name, namlen, info);
			create(pname);	/* Needed to strip off "./" */
			if (dirmode)
				createi(name, namlen, info);
			nodesc--;
		}
		Fflag = OFflag;
		return (FALSE);	
	}
notfound:
	Fflag = OFflag;
	return (TRUE);
}

EXPORT BOOL
checkexclude(name, namlen, info)
	char	*name;
	int	namlen;
	FINFO	*info;
{
		int	len;
	const	char	*fn;

	fn = filename(name);

	if (is_dir(info)) {
		if (Fflag < 3 || Fflag > 4) {
			if (streql(fn, "SCCS") ||	/* SCCS directory */
			    streql(fn, "RCS"))		/* RCS directory  */
				return (FALSE);
		}
		if (Fflag > 1 && streql(fn, "OBJ"))	/* OBJ directory  */
			return (FALSE);
		if (Fflag > 2 && !checkdirexclude(name, namlen, info))
			return (FALSE);
		return (TRUE);
	}
	if ((len = strlen(fn)) < 3)			/* Cannot match later*/
		return (TRUE);

	if (Fflag > 1 && fn[len-2] == '.' && fn[len-1] == 'o')	/* obj files */
		return (FALSE);

	if (is_file(info)) {
		if (streql(fn, "core") ||
		    streql(fn, "errs") ||
		    (Fflag > 1 && streql(fn, "a.out")))
			return (FALSE);
	}

	return (TRUE);
}
