/* @(#)standard.h	1.27 01/02/23 Copyright 1985 J. Schilling */
/*
 *	standard definitions
 *
 *	This file should be included past:
 *
 *	mconfig.h / config.h
 *	stdio.h
 *	stdlib.h	(better use stdxlib.h)
 *	unistd.h	(better use unixstd.h) needed LARGEFILE support
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

#ifndef _STANDARD_H
#define _STANDARD_H

#ifndef _MCONFIG_H
#include <mconfig.h>
#endif
#ifndef _PROTOTYP_H
#include <prototyp.h>
#endif

#ifdef	M68000
#	ifndef	tos
#		define	JOS	1
#	endif
#endif

/*
 *	fundamental constants
 */
#ifndef	NULL
#	define	NULL		0
#endif
#ifndef	TRUE
#	define	TRUE		1
#	define	FALSE		0
#endif

/*
 *	Program exit codes
 */
#define	EX_BAD			(-1)

/*
 *	standard storage class definitions
 */
#define	GLOBAL	extern
#define	IMPORT	extern
#define	EXPORT
#define	INTERN	static
#define	LOCAL	static
#define	FAST	register

#ifndef	PROTOTYPES
#	ifndef	const
#		define	const
#	endif
#	ifndef	signed
#		define	signed
#	endif
#	ifndef	volatile
#		define	volatile
#	endif
#endif	/* PROTOTYPES */

/*
 *	standard type definitions
 *
 *	The hidden Schily BOOL definition is used in case we need to deal
 *	with other BOOL defines on systems we like to port to.
 */
typedef int __SBOOL;
typedef int BOOL;
#ifdef	JOS
#	define	NO_VOID
#endif
#ifdef	NO_VOID
	typedef	int	VOID;
#	ifndef	lint
		typedef int void;
#	endif
#else
	typedef	void	VOID;
#endif

#if	defined(_INCL_SYS_TYPES_H) || defined(off_t)
#	ifndef	FOUND_OFF_T
#	define	FOUND_OFF_T
#	endif
#endif
#if	defined(_INCL_SYS_TYPES_H) || defined(size_t)
#	ifndef	FOUND_SIZE_T
#	define	FOUND_SIZE_T
#	endif
#endif

#if	defined(_SIZE_T)     || defined(_T_SIZE_) || defined(_T_SIZE) || \
	defined(__SIZE_T)    || defined(_SIZE_T_) || \
	defined(_GCC_SIZE_T) || defined(_SIZET_)  || \
	defined(__sys_stdtypes_h) || defined(___int_size_t_h) || defined(size_t)

#ifndef	FOUND_SIZE_T
#	define	FOUND_SIZE_T	/* We already included a size_t definition */
#endif
#endif

#if defined(_JOS) || defined(JOS)
#	include <schily.h>
#	include <jos_defs.h>
#	include <jos_io.h>
#endif

#endif	/* _STANDARD_H */
