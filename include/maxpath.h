/* @(#)maxpath.h	1.2 96/02/04 Copyright 1985, 1995 J. Schilling */
/*
 *	Definitions for dealing with statically limitations on pathnames
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

#ifndef	_MAXPATH_H
#define	_MAXPATH_H

#ifdef	JOS
#	define	MAXPATHNAME	128
#	define	MAXFILENAME	30
#else
#	if defined(BSD4_2) || defined(SVR4)
#		define	MAXPATHNAME	MAXPATHLEN
#		ifdef	MAXNAMELEN
#		define	MAXFILENAME	MAXNAMELEN
#		else
#		define	MAXFILENAME	MAXNAMLEN
#		endif
#	else
#		define	MAXPATHNAME	256
#		define	MAXFILENAME	DIRSIZ
#	endif
#endif	/* JOS */

#endif	/* _MAXPATH_H */
