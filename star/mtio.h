/* @(#)mtio.h	1.1 96/06/26 Copyright 1995 J. Schilling */
/*
 *	Simplyfied mtio definitions
 *	to be able to do at least remote mtio on systems
 *	that have no local mtio
 *
 *	Copyright (c) 1995 J. Schilling
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

#ifndef	_MTIO_H
#define	__MTIO_H

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Structures and definitions for mag tape io control commands
 */

/*
 * structure for MTIOCTOP - mag tape op command
 */
struct	mtop	{
	short	mt_op;		/* operations defined below */
	daddr_t	mt_count;	/* how many of them */
};

/*
 * values for mt_op
 */
#define	MTWEOF		0	/* write an end-of-file record */
#define	MTFSF		1	/* forward space over file mark */
#define	MTBSF		2	/* backward space over file mark (1/2" only ) */
#define	MTFSR		3	/* forward space to inter-record gap */
#define	MTBSR		4	/* backward space to inter-record gap */
#define	MTREW		5	/* rewind */
#define	MTOFFL		6	/* rewind and put the drive offline */
#define	MTNOP		7	/* no operation, sets status only */

/*
 * structure for MTIOCGET - mag tape get status command
 */
struct	mtget	{
	short	mt_type;	/* type of magtape device */
	/* the following two registers are grossly device dependent */
	short	mt_dsreg;	/* ``drive status'' register */
	short	mt_erreg;	/* ``error'' register */
	/* optional error info. */
	daddr_t	mt_resid;	/* residual count */
	daddr_t	mt_fileno;	/* file number of current position */
	daddr_t	mt_blkno;	/* block number of current position */
};

#define	HAVE_MTGET_DSREG
#define	HAVE_MTGET_RESID
#define	HAVE_MTGET_FILENO
#define	HAVE_MTGET_BLKNO

#ifdef	__cplusplus
}
#endif

#endif /* _MTIO_H */
