/* @(#)remote.h	1.1 00/11/12 Copyright 1996 J. Schilling */
/*
 *	Prototypes for rmt client subroutines
 *
 *	Copyright (c) 1996 J. Schilling
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

#ifndef	_REMOTE_H
#define	_REMOTE_H

/*
 * remote.c
 */
extern	int		rmtgetconn	__PR((char* host, int size));
extern	int		rmtopen		__PR((int fd, char* fname, int fmode));
extern	int		rmtclose	__PR((int fd));
extern	int		rmtread		__PR((int fd, char* buf, int count));
extern	int		rmtwrite	__PR((int fd, char* buf, int count));
extern	int		rmtseek		__PR((int fd, long offset, int whence));
extern	int		rmtioctl	__PR((int fd, int cmd, int count));
extern	struct	mtget* rmtstatus	__PR((int fd));

#endif	/* _REMOTE_H */
