/* @(#)timedefs.h	1.1 96/06/26 Copyright 1996 J. Schilling */
/*
 *	Generic header for users of gettimeofday() ...
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

#ifndef	_TIMEDEFS_H
#define	_TIMEDEFS_H

#ifndef	_MCONFIG_H
#include <mconfig.h>
#endif

#include <time.h>

#ifdef	HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifndef	timerclear

struct timeval {
	long	tv_sec;
	long	tv_usec;
};

struct timezone {
	int	tz_minuteswest;
	int	tz_dsttime;
};

#define	timerclear(tvp)		(tvp)->tv_sec = (tvp)->tv_usec = 0

#endif

#endif	/* _TIMEDEFS_H */
