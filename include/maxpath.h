/* @(#)maxpath.h	1.3 98/09/13 Copyright 1985, 1995, 1998 J. Schilling */
/*
 *	Definitions for dealing with statically limitations on pathnames
 *
 *	Copyright (c) 1985, 1995, 1998 J. Schilling
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

#ifndef	_MAXPATH_H
#define	_MAXPATH_H

#ifndef	_DIRDEFS_H
#include <dirdefs.h>			/* Includes mconfig.h if needed	     */
#endif

#ifdef	JOS
#	define	MAXPATHNAME	128
#	define	MAXFILENAME	30
#else

#	ifdef	MAXPATHLEN
#		define	MAXPATHNAME	MAXPATHLEN
#	else
#		define	MAXPATHNAME	256		/* Is there a limit? */
#	endif

/*
 * Don't use defaults here to allow recognition of problems.
 */
#	ifdef	MAXNAMELEN
#		define	MAXFILENAME	MAXNAMELEN	/* From sys/param.h  */
#	else
#	ifdef	MAXNAMLEN
#		define	MAXFILENAME	MAXNAMLEN	/* From dirent.h     */
#	else
#		define	MAXFILENAME	DIRSIZ		/* From sys/dir.h    */
#	endif
#	endif
#endif	/* JOS */

#endif	/* _MAXPATH_H */
