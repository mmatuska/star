/* @(#)dirdefs.h	1.11 01/02/17 Copyright 1987, 1998 J. Schilling */
/*
 *	Copyright (c) 1987, 1998 J. Schilling
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

#ifndef	_DIRDEFS_H
#define	_DIRDEFS_H

#ifndef _MCONFIG_H
#include <mconfig.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef JOS
#	include <sys/stypes.h>
#	include <sys/filedesc.h>
#	define	NEED_READDIR
#	define	DIRSIZE	30
	typedef struct dirent {
		char	name[DIRSIZE];
		short	ino;
	} dirent;

#else	/* !JOS */

#	ifndef	_INCL_SYS_TYPES_H
#	include <sys/types.h>
#	define	_INCL_SYS_TYPES_H
#	endif
#	include <sys/stat.h>
#	ifdef	HAVE_SYS_PARAM_H
#		include	<sys/param.h>
#	endif

#	ifdef	HAVE_DIRENT_H		/* This a POSIX compliant system */
#		include <dirent.h>
#		define	DIR_NAMELEN(dirent)	strlen((dirent)->d_name)
#		define	_FOUND_DIR_
#	else				/* This is a Pre POSIX system	 */

#	define 	dirent	direct
#	define	DIR_NAMELEN(dirent)	(dirent)->d_namlen

#	if	defined(HAVE_SYS_DIR_H)
#		include <sys/dir.h>
#		define	_FOUND_DIR_
#	endif

#	if	defined(HAVE_NDIR_H) && !defined(_FOUND_DIR_)
#		include <ndir.h>
#		define	_FOUND_DIR_
#	endif

#	if	defined(HAVE_SYS_NDIR_H) && !defined(_FOUND_DIR_)
#		include <sys/ndir.h>
#		define	_FOUND_DIR_
#	endif
#	endif	/* HAVE_DIRENT_H */

#	if	defined(_FOUND_DIR_)
/*
 * Don't use defaults here to allow recognition of problems.
 */
#	ifdef	MAXNAMELEN
#		define	DIRSIZE		MAXNAMELEN	/* From sys/param.h  */
#	else
#	ifdef	MAXNAMLEN
#		define	DIRSIZE		MAXNAMLEN	/* From dirent.h     */
#	else
#		define	DIRSIZE		DIRSIZ		/* From sys/dir.h    */
#	endif
#	endif
#	else	/* !_FOUND_DIR_ */

#		define	NEED_DIRENT
#		define	NEED_READDIR

#	endif	/* _FOUND_DIR_ */


#ifdef	NEED_DIRENT

typedef struct dirent {
	short	ino;
	char	name[DIRSIZE];
} dirent;

#endif	/* NEED_DIRENT */

#endif	/* !JOS */

#ifdef	NEED_READDIR
	typedef struct __dirdesc {
		FILE	*dd_fd;
	} DIR;

	struct _direct {
		unsigned long	d_ino;
		unsigned short	d_reclen;
		unsigned short	d_namlen;
		char		d_name[DIRSIZE +1];
	};

extern	DIR		*opendir();
extern			closedir();
extern	struct direct	*readdir();

#endif	/* NEED_READDIR */

#ifdef	__cplusplus
}
#endif

#endif	/* _DIRDEFS_H */
