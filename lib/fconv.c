/* @(#)fconv.c	1.10 97/04/28 Copyright 1985 J. Schilling */
/*
 *	Convert floating point numbers to strings for format.c
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

#include <mconfig.h>
#include <standard.h>
#ifndef	NO_FLOATINGPOINT

#ifdef	HAVE_STDLIB_H
#include <stdlib.h>
#else
extern	char	*ecvt __PR((double, int. int *, int *));
extern	char	*fcvt __PR((double, int, int *, int *));
#endif
#ifdef	HAVE_STRING_H
#include <string.h>
#endif
#ifdef	hpux
#undef	BSD4_2
#endif
#ifdef	SVR4
#include <ieeefp.h>
#define	isnan	isnand
#define	isinf	!finite
#endif
#include <math.h>

static	char	_nan[] = "(NaN)";
static	char	_inf[] = "(Infinity)";

static	int	_ferr __PR((char *, double));

#define	abs(i)	((i) < 0 ? -(i) : (i))

int ftoes (s, val, fieldwidth, ndigits)
	register	char 	*s;
			double	val;
	register	int	fieldwidth;
	register	int	ndigits;
{
	register	char	*b;
	register	char	*rs;
	register	int	len;
	register	int	rdecpt;
			int 	decpt;
			int	sign;

	if ((len = _ferr (s, val)) > 0)
		return len;
	rs = s;
	b = ecvt (val, ndigits, &decpt, &sign);
	rdecpt = decpt;
	len = ndigits + 6;			/* Punkt e +/- nnn */
	if (sign)
		len++;
	if (fieldwidth > len)
		while (fieldwidth-- > len)
			*rs++ = ' ';
	if (sign)
		*rs++ = '-';
	*rs++ = '.';
	  while (*b && ndigits-- > 0)
		*rs++ = *b++;
	*rs++ = 'e';
	*rs++ = rdecpt >= 0 ? '+' : '-';
	rdecpt = abs(rdecpt);
	*rs++ = rdecpt / 100 + '0';
	rdecpt %= 100;
	*rs++ = rdecpt / 10 + '0';
	*rs++ = rdecpt % 10 + '0';
	*rs = '\0';
	return rs - s;
}

int ftofs (s, val, fieldwidth, ndigits)
	register	char 	*s;
			double	val;
	register	int	fieldwidth;
	register	int	ndigits;
{
	register	char	*b;
	register	char	*rs;
	register	int	len;
	register	int	rdecpt;
			int 	decpt;
			int	sign;

	if ((len = _ferr (s, val)) > 0)
		return len;
	rs = s;
	b = fcvt (val, ndigits, &decpt, &sign);
	rdecpt = decpt;
	len = rdecpt + ndigits + 1;
	if (rdecpt < 0)
		len -= rdecpt;
	if (sign)
		len++;
	if (fieldwidth > len)
		while (fieldwidth-- > len)
			*rs++ = ' ';
	if (sign)
		*rs++ = '-';
	if (rdecpt > 0) {
		len = rdecpt;
		while (*b && len-- > 0)
			*rs++ = *b++;
	}
	*rs++ = '.';
	if (rdecpt < 0) {
		len = rdecpt;
		while (len++ < 0 && ndigits-- > 0)
			*rs++ = '0';
	}
	while (*b && ndigits-- > 0)
		*rs++ = *b++;
	*rs = '\0';
	return rs - s;
}

static int _ferr (s, val)
	char	*s;
	double	val;
{
#if	defined(BSD4_2) || defined(SVR4)
	if (isnan (val)){
		strcpy (s, _nan);
		return sizeof (_nan) - 1;
	}
	/*
	 * Check first for NaN because finite() will return 1 on Nan too.
	 */
	if (isinf (val)){
		strcpy (s, _inf);
		return sizeof (_inf) - 1;
	}
#endif
	return 0;
}
#endif	/* NO_FLOATINGPOINT */
