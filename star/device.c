/* @(#)device.c	1.3 97/04/28 Copyright 1996 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)device.c	1.3 97/04/28 Copyright 1996 J. Schilling";
#endif
/*
 *	Handle local and remote device major/minor mappings
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

#include <mconfig.h>
#include <standard.h>
#include <device.h>

#ifdef	DEV_DEFAULTS
EXPORT	int	minorbits = 8;
EXPORT	XDEV_T	minormask = 0xFFLU;
#else
EXPORT	int	minorbits;
EXPORT	XDEV_T	minormask;
#endif

#ifdef	__STDC__
EXPORT	XDEV_T	_dev_mask[] = {
	0x00000000LU,
	0x00000001LU,
	0x00000003LU,
	0x00000007LU,
	0x0000000FLU,
	0x0000001FLU,
	0x0000003FLU,
	0x0000007FLU,
	0x000000FFLU,
	0x000001FFLU,
	0x000003FFLU,
	0x000007FFLU,
	0x00000FFFLU,
	0x00001FFFLU,
	0x00003FFFLU,
	0x00007FFFLU,
	0x0000FFFFLU,
	0x0001FFFFLU,
	0x0003FFFFLU,
	0x0007FFFFLU,
	0x000FFFFFLU,
	0x001FFFFFLU,
	0x003FFFFFLU,
	0x007FFFFFLU,
	0x00FFFFFFLU,
	0x01FFFFFFLU,
	0x03FFFFFFLU,
	0x07FFFFFFLU,
	0x0FFFFFFFLU,
	0x1FFFFFFFLU,
	0x3FFFFFFFLU,
	0x7FFFFFFFLU,
	0xFFFFFFFFLU,
};
#else
EXPORT	XDEV_T	_dev_mask[] = {
	0x00000000L,
	0x00000001L,
	0x00000003L,
	0x00000007L,
	0x0000000FL,
	0x0000001FL,
	0x0000003FL,
	0x0000007FL,
	0x000000FFL,
	0x000001FFL,
	0x000003FFL,
	0x000007FFL,
	0x00000FFFL,
	0x00001FFFL,
	0x00003FFFL,
	0x00007FFFL,
	0x0000FFFFL,
	0x0001FFFFL,
	0x0003FFFFL,
	0x0007FFFFL,
	0x000FFFFFL,
	0x001FFFFFL,
	0x003FFFFFL,
	0x007FFFFFL,
	0x00FFFFFFL,
	0x01FFFFFFL,
	0x03FFFFFFL,
	0x07FFFFFFL,
	0x0FFFFFFFL,
	0x1FFFFFFFL,
	0x3FFFFFFFL,
	0x7FFFFFFFL,
	0xFFFFFFFFL,
};
#endif

EXPORT void
dev_init(debug)
	BOOL	debug;
{
	int	i;
	int	x;

	for (i=0, x=1; minor(x) == x; i++, x<<=1)
		;

	minorbits = i;
	minormask = _dev_mask[i];

	if (debug)
		error("dev_minorbits:    %d\n", minorbits);
}

#ifdef	IS_LIBRARY

#undef	dev_major
EXPORT XDEV_T
dev_major(dev)
	XDEV_T	dev;
{
	return (dev >> minorbits);
}

#undef	_dev_major
EXPORT XDEV_T
_dev_major(mbits, dev)
	int	mbits;
	XDEV_T	dev;
{
	return (dev >> mbits);
}

#undef	dev_minor
EXPORT XDEV_T
dev_minor(dev)
	XDEV_T	dev;
{
	return (dev & minormask);
}

#undef	_dev_minor
EXPORT XDEV_T
_dev_minor(mbits, dev)
	int	mbits;
	XDEV_T	dev;
{
	return (dev & _dev_mask[mbits]);
}

#undef	dev_make
EXPORT XDEV_T
dev_make(majo, mino)
	XDEV_T	majo;
	XDEV_T	mino;
{
	return ((majo << minorbits) | mino);
}

#undef	_dev_make
EXPORT XDEV_T
_dev_make(mbits, majo, mino)
	int	mbits;
	XDEV_T	majo;
	XDEV_T	mino;
{
	return ((majo << mbits) | mino);
}
#endif	/* IS_LIBRARY */
