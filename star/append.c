/* @(#)append.c	1.9 97/06/01 Copyright 1992 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)append.c	1.9 97/06/01 Copyright 1992 J. Schilling";
#endif
/*
 *	Routines used to append files to an existing 
 *	tape archive
 *
 *	Copyright (c) 1992 J. Schilling
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
#include "starsubs.h"

/*extern	FILE	*tarf;*/
/*extern	char	*listfile;*/

/*extern	int	npat;*/
/*extern	BOOL	verbose;*/
extern	BOOL	cflag;
extern	BOOL	uflag;
extern	BOOL	rflag;
/*extern	BOOL	interactive;*/

EXPORT	void	skipall	__PR((void));

EXPORT void
skipall()
{
		FINFO	finfo;
		TCB	tb;
		char	name[PATH_MAX+1];
	register TCB 	*ptb = &tb;

	fillbytes((char *)&finfo, sizeof(finfo), '\0');

	finfo.f_tcb = ptb;
	for (;;) {
		if (get_tcb(ptb) == EOF)
			return;
		finfo.f_name = name;
		if (tcb_to_info(ptb, &finfo) == EOF)
			return;
printf("R %s\n", finfo.f_name);
#ifdef	nono
		if (listfile) {
			if (hash_lookup(finfo.f_name))
				list_file(&finfo);
		} else if (npat == 0 || match(finfo.f_name))
			list_file(&finfo);
#endif
		void_file(&finfo);
	}
}
