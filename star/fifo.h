/* @(#)fifo.h	1.3 97/04/27 Copyright 1989 J. Schilling */
/*
 *	Definitions for a "fifo" that uses
 *	shared memory between two processes
 *
 *	Copyright (c) 1989 J. Schilling
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

typedef	struct	{
	BOOL	reading;	/* true if currently reading from tape	*/
	int	swapflg;	/* -1: init, 0: FALSE, 1: TRUE		*/
	long	blocksize;	/* Blocksize for each transfer		*/
	long	blocks;		/* Full blocks transfered on Volume	*/
	long	parts;		/* Bytes fom partial transferes on Volume */
	long	Tblocks;	/* Total blocks transfered		*/
	long	Tparts;		/* Total Bytes fom parttial transferes	*/
	int	volno;		/* Volume #				*/
} m_stats;

typedef struct {
	char	*putptr;	/* put pointer within shared memory */
	char	*getptr;	/* get pointer within shared memory */
	char	*base;		/* base of fifo within shared memory segment*/
	char	*end;		/* end of real shared memory segment */
	int	size;		/* size of fifo within shared memory segment*/
	int	ibs;		/* input transfer size	*/
	int	obs;		/* output transfer size	*/
	unsigned long	icnt;	/* input count (incremented on each put) */
	unsigned long	ocnt;	/* output count (incremented on each get) */
	int	hiw;		/* highwater mark */
	int	low;		/* lowwater mark */
	int	flags;		/* fifo flags */
	int	gp[2];		/* sync pipe for get process */
	int	pp[2];		/* sync pipe for put process */
	int	puts;		/* fifo put count statistic */
	int	gets;		/* fifo get get statistic */
	int	empty;		/* fifo was empty count statistic */
	int	full;		/* fifo was full count statistic */
	int	maxfill;	/* max # of bytes in fifo */
	m_stats	stats;		/* statistics			*/
} m_head;

#define	gpin	gp[0]		/* get pipe in  */
#define	gpout	gp[1]		/* get pipe out */
#define	ppin	pp[0]		/* put pipe in  */
#define	ppout	pp[1]		/* put pipe out */

#define	FIFO_AMOUNT(p)	((p)->icnt - (p)->ocnt)

#define	FIFO_IBLOCKED	0x01	/* input  (put side) is blocked	*/
#define	FIFO_OBLOCKED	0x02	/* output (get size) is blocked	*/
#define	FIFO_FULL	0x04	/* fifo is full			*/
#define	FIFO_MEOF	0x08	/* EOF on input (put side)	*/
#define	FIFO_MERROR	0x10	/* error on input (put side)	*/

#define	FIFO_IWAIT	0x20	/* input (put side) waits after first record */
#define	FIFO_I_CHREEL	0x40	/* change input tape reel if fifo gets empty */
#define	FIFO_O_CHREEL	0x80	/* change output tape reel if fifo gets empty*/
