/* @(#)star_unix.c	1.13 97/06/15 Copyright 1985, 1995 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)star_unix.c	1.13 97/06/15 Copyright 1985, 1995 J. Schilling";
#endif
/*
 *	Stat / mode / owner routines for unix like
 *	operating systems
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
#include <errno.h>
#include "star.h"
#include "table.h"
#include "dir.h"
#include <standard.h>
#include <unixstd.h>
#include <timedefs.h>
#include <device.h>
#include "dirtime.h"
#include "xutimes.h"
#include "starsubs.h"

#ifndef	S_IFLNK
#define	lstat	stat
#endif
#ifndef	SVR4
#define	lchown	chown
#endif

#define	ROOT_UID	0

extern	int	uid;
extern	Ulong	curfs;
extern	BOOL	nomtime;
extern	BOOL	nochown;
extern	BOOL	dirmode;
extern	BOOL	follow;

EXPORT	BOOL	getinfo		__PR((char* name, FINFO * info));
EXPORT	void	checkarch	__PR((FILE *f));
EXPORT	void	setmodes	__PR((FINFO * info));
LOCAL	int	sutimes		__PR((char* name, FINFO *info));
EXPORT	int	sxsymlink	__PR((FINFO *info));
EXPORT	int	rs_acctime	__PR((FILE * f, FINFO * info));

EXPORT BOOL
getinfo(name, info)
	char	*name;
	register FINFO	*info;
{
	struct stat	stbuf;

	if (follow?stat(name, &stbuf):lstat(name, &stbuf) < 0)
		return (FALSE);
	info->f_name = name;
	info->f_uname = info->f_gname = NULL;
	info->f_umaxlen = info->f_gmaxlen = 0L;
	info->f_dev	= (Ulong) stbuf.st_dev;
	if (curfs == -1L)
		curfs = info->f_dev;
	info->f_ino	= stbuf.st_ino;
	info->f_nlink	= stbuf.st_nlink;
	info->f_mode	= stbuf.st_mode & 07777;
	info->f_uid	= stbuf.st_uid;
	info->f_gid	= stbuf.st_gid;
	info->f_size	= 0L;
	info->f_rsize	= 0L;
	info->f_flags	= 0L;
	info->f_type	= stbuf.st_mode & ~07777;

	if (sizeof(stbuf.st_rdev) == sizeof(short)) {
		info->f_rdev = (Ushort) stbuf.st_rdev;
	} else if ((sizeof(int) != sizeof(long)) &&
			(sizeof(stbuf.st_rdev) == sizeof(int))) {
		info->f_rdev = (Uint) stbuf.st_rdev;
	} else {
		info->f_rdev = (Ulong) stbuf.st_rdev;
	}
	info->f_rdevmaj	= dev_major(info->f_rdev);
	info->f_rdevmin	= dev_minor(info->f_rdev);
	info->f_atime	= stbuf.st_atime;
	info->f_mtime	= stbuf.st_mtime;
	info->f_ctime	= stbuf.st_ctime;

	switch ((int)(stbuf.st_mode & S_IFMT)) {

	case S_IFREG:
			info->f_filetype = F_FILE;
			info->f_rsize = info->f_size = stbuf.st_size;
			break;
	case S_IFDIR:
			info->f_filetype = F_DIR;
			break;
#ifdef	S_IFLNK
	case S_IFLNK:
			info->f_filetype = F_SLINK;
			break;
#endif
#ifdef	S_IFIFO
	case S_IFIFO:
#endif
	default:
			info->f_filetype = F_SPEC;
	}
	info->f_xftype = IFTOXT(info->f_type);
#ifdef	BSD4_2
#if	defined(hpux) || defined(__hpux)
	if (info->f_size > (stbuf.st_blocks * 1024 + 1024))
#else
	if (info->f_size > (stbuf.st_blocks * 512 + 512))
#endif
		info->f_flags |= F_SPARSE;
#endif	/* BSD4_2 */
	return (TRUE);
}

EXPORT void
checkarch(f)
	FILE	*f;
{
	struct stat	stbuf;
	extern	long	tape_dev;
	extern	long	tape_ino;

	if (fstat(fdown(f), &stbuf) < 0)
		return;
	
	if ((stbuf.st_mode & S_IFMT) == S_IFREG) {
		tape_dev = stbuf.st_dev;
		tape_ino = stbuf.st_ino;
	} else if (((stbuf.st_mode & S_IFMT) == 0) ||
			((stbuf.st_mode & S_IFMT) == S_IFIFO) ||
			((stbuf.st_mode & S_IFMT) == S_IFSOCK)) {
		/*
		 * This is a pipe or similar on different UNIX implementations.
		 */
		tape_dev = tape_ino = -1;
	}
}

EXPORT void
setmodes(info)
	register FINFO	*info;
{
	if ((!is_dir(info) || dirmode) && !is_symlink(info))
		if (chmod(info->f_name, (int)info->f_mode) < 0) {
			;
		}
	if (!nochown && uid == ROOT_UID)
		lchown(info->f_name, (int)info->f_uid, (int)info->f_gid);
	if (!nomtime && !is_symlink(info)) {
		if (is_dir(info))
			sdirtimes(info->f_name, info);
		else
			sutimes(info->f_name, info);
	}
}

EXPORT	int	xutimes		__PR((char* name, struct timeval *tp));

LOCAL int
sutimes(name, info)
	char	*name;
	FINFO	*info;
{
	struct	timeval tp[3];

	tp[0].tv_sec = info->f_atime;
	tp[0].tv_usec = info->f_spare1;

	tp[1].tv_sec = info->f_mtime;
	tp[1].tv_usec = info->f_spare2;
#ifdef	SET_CTIME
	tp[2].tv_sec = info->f_ctime;
	tp[2].tv_usec = info->f_spare3;
#endif
	return (xutimes(name, tp));
}

EXPORT int
xutimes(name, tp)
	char	*name;
	struct	timeval tp[3];
{
	struct  timeval curtime;
	struct  timeval pasttime;
	extern int Ctime;
	int	ret;
	int	errsav;

#ifdef	SET_CTIME
	if (Ctime) {
		gettimeofday(&curtime, 0);
		settimeofday(&tp[2], 0);
	}
#endif
	ret = utimes(name, tp);
	errsav = errno;

#ifdef	SET_CTIME
	if (Ctime) {
		gettimeofday(&pasttime, 0);
		/* XXX Hack: f_ctime.tv_usec ist immer 0! */
		curtime.tv_usec += pasttime.tv_usec;
		if (curtime.tv_usec > 1000000) {
			curtime.tv_sec += 1;
			curtime.tv_usec -= 1000000;
		}
		settimeofday(&curtime, 0);
/*		printf("pasttime.usec: %d\n", pasttime.tv_usec);*/
	}
#endif
	errno = errsav;
	return (ret);
}

EXPORT int
sxsymlink(info)
	FINFO	*info;
{
	struct	timeval tp[3];
	struct  timeval curtime;
	struct  timeval pasttime;
	char	*linkname;
	char	*name;
	extern int Ctime;
	int	ret;
	int	errsav;

	tp[0].tv_sec = info->f_atime;
	tp[0].tv_usec = info->f_spare1;

	tp[1].tv_sec = info->f_mtime;
	tp[1].tv_usec = info->f_spare2;
#ifdef	SET_CTIME
	tp[2].tv_sec = info->f_ctime;
	tp[2].tv_usec = info->f_spare3;
#endif
	linkname = info->f_lname;
	name = info->f_name;

#ifdef	SET_CTIME
	if (Ctime) {
		gettimeofday(&curtime, 0);
		settimeofday(&tp[2], 0);
	}
#endif
	ret = symlink(linkname, name);
	errsav = errno;

#ifdef	SET_CTIME
	if (Ctime) {
		gettimeofday(&pasttime, 0);
		/* XXX Hack: f_ctime.tv_usec ist immer 0! */
		curtime.tv_usec += pasttime.tv_usec;
		if (curtime.tv_usec > 1000000) {
			curtime.tv_sec += 1;
			curtime.tv_usec -= 1000000;
		}
		settimeofday(&curtime, 0);
/*		printf("pasttime.usec: %d\n", pasttime.tv_usec);*/
	}
#endif
	errno = errsav;
	return (ret);
}

#ifdef	HAS_FILIO
#include <sys/filio.h>
#endif

EXPORT int
rs_acctime(f, info)
		 FILE	*f;
	register FINFO	*info;
{
	if (is_symlink(info))
		return (0);

#ifdef	_FIOSATIME
	/*
	 * On Solaris 2.x root may reset accesstime without changing ctime.
	 */
	if (uid == ROOT_UID)
		return (ioctl(fdown(f), _FIOSATIME, &info->f_atime));
#endif
	return (sutimes(info->f_name, info));
}

#ifndef	HAVE_UTIMES
#include <sys/types.h>
#ifdef	HAVE_UTIME_H
#include <utime.h>
#else
struct utimbuf {
	time_t	actime;
	time_t	modtime;
} ;
#endif

utimes(name, tp)
	char		*name;
	struct timeval	*tp;
{
	struct utimbuf	ut;

	ut.actime = tp->tv_sec;
	tp++;
	ut.modtime = tp->tv_sec;
	
	return (utime(name, &ut));
}
#endif
