/* @(#)props.h	1.7 97/05/09 Copyright 1994 J. Schilling */
/*
 *	Properties definitions to handle different
 *	archive types
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

/*
 * Properties to describe the different archive formats.
 *
 * if pr_maxnamelen id == pr_maxsname, we cannot have long names
 * besides file name splitting.
 */
struct properties {
	int	pr_flags;		/* gerneral flags */
	char	pr_fillc;		/* fill prefix for numbers in TCB */
	long	pr_diffmask;		/* diffopts not supported */
	int	pr_nflags;		/* name related flags */
	int	pr_maxnamelen;		/* max length for filename */
	int	pr_maxlnamelen;		/* max length for linkname */
	int	pr_maxsname;		/* max length for short filename */
	int	pr_maxslname;		/* max length for short linkname */
	int	pr_maxprefix;		/* max length of prefix if splitting */
	int	pr_sparse_in_hdr;	/* # of sparse entries in header */
};

/*
 * general flags
 */
#define	PR_POSIX_OCTAL		0x0001	/* left fill octal number with '0' */
#define	PR_LOCAL_STAR		0x0002	/* can handle local star filetypes */
#define	PR_LOCAL_GNU		0x0004	/* can handle local gnu filetypes */
#define	PR_SPARSE		0x0010	/* can handle sparse files	*/
#define	PR_GNU_SPARSE_BUG	0x0020	/* size does not contain ext. headers*/
#define	PR_VOLHDR		0x0100	/* can handle volume headers	*/

/*
 * name related flags
 */
#define	PR_POSIX_SPLIT		0x01	/* can do posix filename splitting */
#define	PR_PREFIX_REUSED	0x02	/* prefix space used by other option */
#define	PR_LONG_NAMES		0x04	/* can handle very long names	*/
#define	PR_DUMB_EOF		0x10	/* handle t_name[0] == '\0' as EOF */

extern	struct properties	props;
