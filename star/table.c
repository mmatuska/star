/* @(#)table.c	1.7 96/06/26 Copyright 1994 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)table.c	1.7 96/06/26 Copyright 1994 J. Schilling";
#endif
/*
 *	Conversion tables for efficient conversion
 *	of different file type representations
 *
 *	Copyright (c) 1994 J. Schilling
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

#include "star.h"
#include "table.h"
#include <sys/stat.h>

#ifndef	S_IFIFO			/* If system knows no fifo's		*/
#define	S_IFIFO		S_IFREG	/* Map fifo's to regular files 		*/
#endif
#ifndef	S_IFNAM			/* If system knows no named files	*/
#define	S_IFNAM		S_IFREG	/* Map named files to regular files 	*/
#endif
#ifndef	S_IFCNT			/* If system knows no contiguous files	*/
#define	S_IFCNT		S_IFREG	/* Map contiguous file to regular files */
#endif
#ifndef	S_IFLNK			/* If system knows no symbolic links	*/
#define	S_IFLNK		S_IFREG	/* Map symbolic links to regular files */
#endif
#ifndef	S_IFSOCK		/* If system knows no fifo's		*/
#define	S_IFSOCK	S_IFREG	/* Map fifo's to regular files 		*/
#endif
#define	S_IFBAD	S_IFMT		/* XXX Have to use another val if someone */
				/* XXX starts to use S_IFMT for a */
				/* XXX useful file type */
#define	XT_NAM	XT_BAD		/* XXX JS has not seen it yet */
				/* XXX if we use it, we have to change */
				/* XXX table.h and some of the tables below */

/*
 * UNIX File type to XT_ table
 */
char	iftoxt_tab[] = {			
			XT_NONE, XT_FIFO, XT_CHR,   XT_BAD,
			XT_DIR,  XT_NAM,  XT_BLK,   XT_BAD,
			XT_FILE, XT_CONT, XT_SLINK, XT_BAD,
			XT_SOCK, XT_BAD,  XT_BAD,   XT_BAD,
			};

/*
 * Ustar File type to XT_ table
 */
char	ustoxt_tab[] = {
			XT_FILE, XT_LINK, XT_SLINK, XT_CHR,
			XT_BLK,  XT_DIR,  XT_FIFO,  XT_CONT,
};

/*
 * Gnutar File type to XT_ table
 */
char	gttoxt_tab[] = {
		/* A */	XT_NONE,     XT_NONE,   XT_NONE,     XT_DUMPDIR,
		/* E */	XT_NONE,     XT_NONE,   XT_NONE,     XT_NONE,
		/* I */	XT_NONE,     XT_NONE,   XT_LONGLINK, XT_LONGNAME,
		/* M */	XT_MULTIVOL, XT_NAMES,  XT_NONE,     XT_NONE,
		/* Q */	XT_NONE,     XT_NONE,   XT_SPARSE,   XT_NONE,
		/* U */	XT_NONE,     XT_VOLHDR, XT_NONE,     XT_NONE,
		/* Y */	XT_NONE,     XT_NONE, 
};

/*
 * XT_ to UNIX File type table
 */
int	xttoif_tab[] = {
			0,       S_IFREG,  S_IFCNT, S_IFREG,
			S_IFLNK, S_IFDIR,  S_IFCHR, S_IFBLK,
			S_IFIFO, S_IFSOCK, S_IFBAD, S_IFBAD,
			S_IFBAD, S_IFBAD,  S_IFBAD, S_IFBAD,
			S_IFBAD, S_IFBAD,  S_IFBAD, S_IFBAD,
			S_IFDIR, S_IFBAD,  S_IFBAD, S_IFBAD,
			S_IFBAD, S_IFBAD,  S_IFBAD, S_IFBAD,
			S_IFBAD, S_IFBAD,  S_IFBAD, S_IFBAD,
			};

/*
 * XT_ to Star File type table
 */
char	xttost_tab[] = {
		/* 0 */	0,       F_FILE, F_FILE, F_FILE,
		/* 4 */	F_SLINK, F_DIR,  F_SPEC, F_SPEC,
		/* 8 */	F_SPEC,  F_SPEC, F_SPEC, F_SPEC,
		/*12 */	F_SPEC,  F_SPEC, F_SPEC, F_SPEC,
		/*16 */	F_SPEC,  F_SPEC, F_SPEC, F_SPEC,
		/*20 */	F_DIR,   F_FILE, F_FILE, F_FILE,
		/*24 */	F_FILE,  F_FILE, F_SPEC, F_SPEC,
		/*28 */	F_SPEC,  F_SPEC, F_SPEC, F_SPEC,
			};

/*
 * XT_ to Ustar File type table
 *
 * sockets cannot be handled in ansi tar, they are handled as regular files :-(
 */
char	xttous_tab[] = {
			0,       REGTYPE, CONTTYPE, LNKTYPE,
			SYMTYPE, DIRTYPE, CHRTYPE,  BLKTYPE,
			FIFOTYPE,REGTYPE/* socket */, 0/* bad */, 0/* bad */,
			0,       0,       0,        0,
			0,       0,       0,        0,
			LF_DUMPDIR, LF_LONGLINK, LF_LONGNAME, LF_MULTIVOL,
			LF_NAMES,   LF_SPARSE,   LF_VOLHDR,   0,
			0,       0,       0,        0,
			};

/*
 * XT_ to String table
 */
char	*xttostr_tab[] = {
#define	XT_DEBUG
#ifdef	XT_DEBUG
			"U",	"-",	"C",	"H",
#else
			"-",	"-",	"-",	"-",
#endif
			"l",	"d",	"c",	"b",
			"p",	"s",	"~",	"~",
			"~",	"~",	"~",	"~",
			"~",	"~",	"~",	"~",

			"D",	"K",	"L",	"M",
#ifdef	XT_DEBUG
			"N",	"S",	"V",	"~",
#else
			"N",	"-",	"V",	"~",
#endif

			"~",	"~",	"~",	"~",
			};
