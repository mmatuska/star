/* @(#)dir.h	1.4 97/06/03 Copyright 1987 J. Schilling */
/*
 *	Copyright (c) 1987 J. Schilling
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

#ifdef JOS
#	include <sys/stypes.h>
#	include <sys/filedesc.h>
#	define	DIRSIZE	30
	typedef struct dirent {
		char	name[DIRSIZE];
		short	ino;
	} dirent;
#else
#	include <sys/types.h>
#	include <sys/stat.h>
#	ifndef	BSD4_2
#		define	direct	_direct
#	endif
#	ifdef	HAVE_DIRENT_H
#		include <dirent.h>
#		define	direct	dirent
#	else
#		include <sys/dir.h>
#	endif
#	ifdef	BSD4_2
#		define	DIRSIZE	MAXNAMLEN
#	else
#		define	DIRSIZE	DIRSIZ
#		undef	direct
		typedef struct dirent {
			short	ino;
			char	name[DIRSIZE];
		} dirent;
#	endif
#endif

#ifndef	BSD4_2
	typedef struct _dirdesc {
		FILE	*dd_fd;
	} DIR;

	struct direct {
		unsigned long	d_ino;
		unsigned short	d_reclen;
		unsigned short	d_namlen;
		char		d_name[DIRSIZE +1];
	};
#endif

#ifndef	BSD4_2
extern	DIR		*opendir();
extern			closedir();
extern	struct direct	*readdir();
#endif
