/* @(#)props.c	1.14 00/05/07 Copyright 1994 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)props.c	1.14 00/05/07 Copyright 1994 J. Schilling";
#endif
/*
 *	Set up properties for different archive types
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

#include <mconfig.h>
#include <stdio.h>
#include "star.h"
#include "props.h"
#include "diff.h"
#include <standard.h>
#include <schily.h>
#include "starsubs.h"

extern	BOOL	debug;

struct properties props;

EXPORT	void	setprops	__PR((long htype));
EXPORT	void	printprops	__PR((void));

EXPORT void
setprops(htype)
	long	htype;
{
	switch (H_TYPE(htype)) {

	case H_STAR:
		props.pr_flags = PR_LOCAL_STAR|PR_SPARSE|PR_VOLHDR;
		props.pr_fillc = ' ';
		props.pr_diffmask = 0L;
		props.pr_nflags =
			PR_POSIX_SPLIT|PR_PREFIX_REUSED|PR_LONG_NAMES;
		props.pr_maxnamelen =  PATH_MAX;
		props.pr_maxlnamelen = PATH_MAX;
		props.pr_maxsname =    NAMSIZ;
		props.pr_maxslname =   NAMSIZ;
		props.pr_maxprefix =   PFXSIZ;
		props.pr_sparse_in_hdr = 0;
		break;

	case H_XSTAR:
	case H_XUSTAR:
		props.pr_flags =
			PR_POSIX_OCTAL|PR_LOCAL_STAR|PR_SPARSE|PR_VOLHDR;
		props.pr_fillc = '0';
		props.pr_diffmask = 0L;
		props.pr_nflags =
			PR_POSIX_SPLIT|PR_PREFIX_REUSED|PR_LONG_NAMES;
		props.pr_maxnamelen =  PATH_MAX;
		props.pr_maxlnamelen = PATH_MAX;
		props.pr_maxsname =    NAMSIZ;
		props.pr_maxslname =   NAMSIZ;
		props.pr_maxprefix =   130;
		props.pr_sparse_in_hdr = 0;
		break;

	case H_USTAR:
		props.pr_flags = PR_POSIX_OCTAL;
		props.pr_fillc = '0';
		props.pr_diffmask = (D_ATIME|D_CTIME);
		props.pr_nflags = PR_POSIX_SPLIT;
		props.pr_maxnamelen =  NAMSIZ;
		props.pr_maxlnamelen = NAMSIZ;
		props.pr_maxsname =    NAMSIZ;
		props.pr_maxslname =   NAMSIZ;
		props.pr_maxprefix =   PFXSIZ;
		props.pr_sparse_in_hdr = 0;
		break;

	case H_GNUTAR:
		props.pr_flags =
			PR_LOCAL_GNU|PR_SPARSE|PR_GNU_SPARSE_BUG|PR_VOLHDR;
		props.pr_fillc = ' ';
		props.pr_diffmask = 0L;
		props.pr_nflags = PR_LONG_NAMES;
		props.pr_maxnamelen =  PATH_MAX;
		props.pr_maxlnamelen = PATH_MAX;
		props.pr_maxsname =    NAMSIZ-1;
		props.pr_maxslname =   NAMSIZ-1;
		props.pr_maxprefix =   0;
		props.pr_sparse_in_hdr = SPARSE_IN_HDR;
		break;

	case H_TAR:
	case H_OTAR:
	default:
		props.pr_flags = 0;
		props.pr_fillc = ' ';
		props.pr_diffmask = (D_ATIME|D_CTIME);
		props.pr_nflags = PR_DUMB_EOF;
		props.pr_maxnamelen =  NAMSIZ-1;
		props.pr_maxlnamelen = NAMSIZ-1;
		props.pr_maxsname =    NAMSIZ-1;
		props.pr_maxslname =   NAMSIZ-1;
		props.pr_maxprefix =   0;
		props.pr_sparse_in_hdr = 0;
	}
	if (debug) printprops();
}

EXPORT void
printprops()
{
	error("pr_flags:         0x%X\n", props.pr_flags);
	error("pr_fillc:         '%c'\n", props.pr_fillc);
	prdiffopts(stderr, "pr_diffmask:      ", props.pr_diffmask);
	error("pr_nflags:        0x%X\n", props.pr_nflags);
	error("pr_maxnamelen:    %d\n", props.pr_maxnamelen);
	error("pr_maxlnamelen:   %d\n", props.pr_maxlnamelen);
	error("pr_maxsname:      %d\n", props.pr_maxsname);
	error("pr_maxslname:     %d\n", props.pr_maxslname);
	error("pr_maxprefix:     %d\n", props.pr_maxprefix);
	error("pr_sparse_in_hdr: %d\n", props.pr_sparse_in_hdr);
}
