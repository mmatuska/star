/* @(#)format.c	1.23 97/04/27 Copyright 1985 J. Schilling */
/*
 *	format
 *	common code for printf fprintf & sprintf
 *
 *	allows recursive printf with "%r", used in:
 *	error, comerr, comerrno, errmsg, errmsgno and the like
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
#ifdef	HAVE_STDARG_H
#	include <stdarg.h>
#else
#	include <varargs.h>
#endif
#ifdef	HAVE_STRING_H
#include <string.h>
#endif
#ifdef	HAVE_STDLIB_H
#include <stdlib.h>
#else
extern	char	*gcvt __PR((double, int, char *));
#endif
#include <standard.h>

#ifdef	DO_MASK
#define	CHARMASK	(0xFFL)
#define	SHORTMASK	(0xFFFFL)
#define	INTMASK		(0xFFFFFFFFL)
#endif

#ifdef	DIVLBYS
extern	long	divlbys();
extern	long	modlbys();
#else
#define	divlbys(val, base)	((val)/(base))
#define	modlbys(val, base)	((val)%(base))
#endif

/*
 *	We use macros here to avoid the need to link to the international
 *	character routines.
 *	We don't need internationalization for our purpose.
 */
#define	is_dig(c)	(((c) >= '0') && ((c) <= '9'))
#define	is_cap(c)	((c) >= 'A' && (c) <= 'Z')
#define	to_cap(c)	(is_cap(c) ? c : c - 'a' + 'A')
#define	cap_ty(c)	(is_cap(c) ? 'L' : 'I')

typedef unsigned int	Uint;
typedef unsigned short	Ushort;
typedef unsigned long	Ulong;

typedef struct f_args {
	void	(*outf) __PR((char, long)); /* Func from format(fun, arg) */
	long	farg;			    /* Arg from format (fun, arg) */
	int	minusflag;
	int	flags;
	int	fldwidth;
	int	signific;
	int	lzero;
	char	*buf;
	char	*bufp;
	char	fillc;
	char	*prefix;
	int	prefixlen;
} f_args;

#define	MINUSFLG	1	/* '-' flag */
#define	PLUSFLG		2	/* '+' flag */
#define	SPACEFLG	4	/* ' ' flag */
#define	HASHFLG		8	/* '#' flag */

LOCAL	void	prmod  __PR((Ulong, unsigned, f_args *));
LOCAL	void	prdmod __PR((Ulong, f_args *));
LOCAL	void	promod __PR((Ulong, f_args *));
LOCAL	void	prxmod __PR((Ulong, f_args *));
LOCAL	void	prXmod __PR((Ulong, f_args *));
LOCAL	int	prbuf  __PR((const char *, f_args *));
LOCAL	int	prc    __PR((char, f_args *));
LOCAL	int	prstring __PR((const char *, f_args *));


#ifdef	PROTOTYPES
EXPORT int format(	void (*fun)(char, long),
			long farg,
			const char *fmt,
			va_list args)
#else
EXPORT int format(fun, farg, fmt, args)
	register void	(*fun)();
	register long	farg;
	register char	*fmt;
	va_list		args;
#endif
{
	char buf[512];
	const char *sfmt;
	register int unsflag;
	register long val;
	register char type;
	register char mode;
	register char c;
	int count;
	int i;
	short sh;
	const char *str;
	double dval;
	Ulong res;
	char *rfmt;
	va_list rargs;
	f_args	fa;

	fa.outf = fun;
	fa.farg = farg;
	count = 0;
	/*
	 * Main loop over the format string.
	 * Increment and check for end of string is made here.
	 */
	for(; *fmt != '\0'; fmt++) {
		c = *fmt;
		while (c != '%') {
			if (c == '\0')
				return (count);
			(*fun)(c, farg);
			c = *(++fmt);
			count++;
		}

		/*
		 * We reached a '%' sign.
		 */
		buf[0] = '\0';
		fa.buf = fa.bufp = buf;
		fa.minusflag = 0;
		fa.flags = 0;
		fa.fldwidth = 0;
		fa.signific = -1;
		fa.lzero = 0;
		fa.fillc = ' ';
		fa.prefixlen = 0;
		sfmt = fmt;
		unsflag = FALSE;
	newflag:
		switch (*(++fmt)) {

		case '+':
			fa.flags |= PLUSFLG;
			goto newflag;

		case '-':
			fa.minusflag++;
			goto newflag;

		case ' ':
			/*
			 * If the space and the + flag are present,
			 * the space flag will be ignored.
			 */
			fa.flags |= SPACEFLG;
			goto newflag;

		case '#':
			fa.flags |= HASHFLG;
			goto newflag;

		case '0':
			/*
			 * '0' is a flag.
			 */
			fa.fillc = '0';
			goto newflag;
		}
		if (*fmt == '*') {
			fmt++;
			fa.fldwidth = va_arg(args, int);
			/*
			 * A negative fieldwith is a minus flag with a
			 * positive fieldwidth.
			 */
			if (fa.fldwidth < 0) {
				fa.fldwidth = -fa.fldwidth;
/*				fa.minusflag ^= 1;*/
				fa.minusflag = 1;
			}
		} else while (c = *fmt, is_dig(c)) {
			fa.fldwidth *= 10;
			fa.fldwidth += c - '0';
			fmt++;
		}
		if (*fmt == '.') {
			fmt++;
			fa.signific = 0;
			if (*fmt == '*') {
				fmt++;
				fa.signific = va_arg(args, int);
				if (fa.signific < 0)
					fa.signific = 0;
			} else while (c = *fmt, is_dig(c)) {
				fa.signific *= 10;
				fa.signific += c - '0';
				fmt++;
			}
		}
		if (strchr("UCSIL", *fmt)) {
			/*
			 * Enhancements to K&R and ANSI:
			 *
			 * got a type specifyer
			 */
			if (*fmt == 'U') {
				fmt++;
				unsflag = TRUE;
			}
			if (!strchr("CSILZODX", *fmt)) {
				/*
				 * Got only 'U'nsigned specifyer,
				 * use default type and mode.
				 */
				type = 'I';
				mode = 'D';
				fmt--;
			} else if (!strchr("CSIL", *fmt)) {
				/*
				 * no type, use default
				 */
				type = 'I';
				mode = *fmt;
			} else {
				/*
				 * got CSIL
				 */
				type = *fmt++;
				if (!strchr("ZODX", mode = *fmt)) {
					fmt--;
					mode = 'D';/* default mode */
				}
			}
		} else switch(*fmt) {

		case 'h':
			type = 'S';		/* convert to type */
			goto getmode;

		case 'l':
			type = 'L';		/* convert to type */

		getmode:
			if (!strchr("udioxX", *(++fmt))) {
				fmt--;
				mode = 'D';
			} else {
				if ((mode = to_cap(*fmt)) == 'U')
					unsflag = TRUE;
				if (mode == 'I')	/*XXX */
					mode = 'D';
			}
			break;
		case 'x':
			mode = 'x';
			goto havemode;
		case 'u':
			unsflag = TRUE;
		case 'o': case 'O':
		case 'd': case 'D':
		case 'i': case 'I':
		case 'p':
		case 'X':
		case 'z': case 'Z':
			mode = to_cap(*fmt);
		havemode:
			type = cap_ty(*fmt);
			if (mode == 'I')	/*XXX kann entfallen*/
				mode = 'D';	/*wenn besseres uflg*/
			break;

		case '%':
			count += prc('%', &fa);
			continue;
		case ' ':
			count += prbuf("", &fa);
			continue;
		case 'c':
			c = va_arg(args, int);
			count += prc(c, &fa);
			continue;
		case 's':
			str = va_arg(args, char *);
			count += prstring(str, &fa);
			continue;
		case 'b':
			str = va_arg(args, char *);
			fa.signific = va_arg(args, int);
			count += prstring(str, &fa);
			continue;

#ifndef	NO_FLOATINGPOINT
		case 'e':
			if (fa.signific == -1)
				fa.signific = 6;
			dval = va_arg(args, double);
			ftoes(buf, dval, 0, fa.signific);
			count += prbuf(buf, &fa);
			continue;
		case 'f':
			if (fa.signific == -1)
				fa.signific = 6;
			dval = va_arg(args, double);
			ftofs(buf, dval, 0, fa.signific);
			count += prbuf(buf, &fa);
			continue;
		case 'g':
			if (fa.signific == -1)
				fa.signific = 6;
			if (fa.signific == 0)
				fa.signific = 1;
			dval = va_arg(args, double);
			gcvt(dval, fa.signific, buf);
			count += prbuf(buf, &fa);
			continue;
#endif

		case 'r':			/* recursive printf */
			rfmt  = va_arg(args, char *);
			rargs = va_arg(args, va_list);
			count += format(fun, farg, rfmt, rargs);
			continue;

		case 'n':
			{
				int	*ip = va_arg(args, int *);

				*ip = count;
			}
			continue;

		default:
			count += fmt - sfmt;
			while (sfmt < fmt)
				(*fun)(*(sfmt++), farg);
			if (*fmt == '\0') {
				fmt--;
				continue;
			} else {
				(*fun)(*fmt, farg);
				count++;
				continue;
			}
		}
		/*
		 * print numbers:
		 * first prepare type 'C'har, 'S'hort, 'I'nt, or 'L'ong
		 */
		switch(type) {

		case 'C':
			c = va_arg(args, int);
			val = c;		/* extend sign here */
			if (unsflag || mode != 'D')
#ifdef	DO_MASK
				val &= CHARMASK;
#else
				val = (unsigned char)val;
#endif
			break;
		case 'S':
			sh = va_arg(args, int);
			val = sh;		/* extend sign here */
			if (unsflag || mode != 'D')
#ifdef	DO_MASK
				val &= SHORTMASK;
#else
				val = (unsigned short)val;
#endif
			break;
		case 'I':
		default:
			i = va_arg(args, int);
			val = i;		/* extend sign here */
			if (unsflag || mode != 'D')
#ifdef	DO_MASK
				val &= INTMASK;
#else
				val = (unsigned int)val;
#endif
			break;
		case 'P':
		case 'L':
			val = va_arg(args, long);
			break;
		}

		/*
		 * Final print out, take care of mode:
		 * mode is one of: 'O'ctal, 'D'ecimal, or he'X'
		 * oder 'Z'weierdarstellung.
		 */
		if (val == 0 && mode != 'D') {
		printzero:
			/*
			 * Printing '0' with fieldwidth 0 results in no chars.
			 */
			count += prstring("0", &fa);
			continue;
		} else switch(mode) {

		case 'D':
			if (!unsflag && val < 0) {
				fa.prefix = "-";
				fa.prefixlen = 1;
				val = -val;
			} else if (fa.flags & PLUSFLG) {
				fa.prefix = "+";
				fa.prefixlen = 1;
			} else if (fa.flags & SPACEFLG) {
				fa.prefix = " ";
				fa.prefixlen = 1;
			}
			if (val == 0)
				goto printzero;
		case 'U':

			/* output a long unsigned decimal number */
			if ((res = divlbys(((Ulong)val)>>1, (Uint)5)) != 0)
				prdmod(res, &fa);
			val = modlbys(val, (Uint)10);
			prdmod(val < 0 ?-val:val, &fa);
			break;
		case 'O':
			/* output a long octal number */
			if (fa.flags & HASHFLG) {
				fa.prefix = "0";
				fa.prefixlen = 1;
			}
			if ((res = (val>>3) & 0x1FFFFFFFL) != 0)
				promod(res, &fa);
			promod(val & 07, &fa);
			break;
		case 'p':
		case 'x':
			/* output a hex long */
			if (fa.flags & HASHFLG) {
				fa.prefix = "0x";
				fa.prefixlen = 2;
			}
			if ((res = (val>>4) & 0xFFFFFFFL) != 0)
				prxmod(res, &fa);
			prxmod(val & 0xF, &fa);
			break ;
		case 'P':
		case 'X':
			/* output a hex long */
			if (fa.flags & HASHFLG) {
				fa.prefix = "0X";
				fa.prefixlen = 2;
			}
			if ((res = (val>>4) & 0xFFFFFFFL) != 0)
				prXmod(res, &fa);
			prXmod(val & 0xF, &fa);
			break ;
		case 'Z':
			/* output a binary long */
			if ((res = (val>>1) & 0x7FFFFFFFL) != 0)
				prmod(res, 2, &fa);
			prmod(val & 0x1, 2, &fa);
		}
		*fa.bufp = '\0';
		fa.lzero = -1;
		/*
		 * If a precision (fielwidth) is specified
		 * on diouXx conversions, the '0' flag is ignored.
		 */
		if (fa.signific >= 0)
			fa.fillc = ' ';
		count += prbuf(fa.buf, &fa);
	}
	return (count);
}

/*
 * Routines to print (not negative) numbers in an arbitrary base
 */
LOCAL	unsigned char	dtab[]  = "0123456789abcdef";
LOCAL	unsigned char	udtab[] = "0123456789ABCDEF";

LOCAL void prmod(val, base, fa)
	Ulong val;
	unsigned base;
	f_args *fa;
{
	if (val >= base)
		prmod(divlbys(val, base), base, fa);
	*fa->bufp++ = dtab[modlbys(val, base)];
}

LOCAL void prdmod(val, fa)
	Ulong val;
	f_args *fa;
{
	if (val >= (unsigned)10)
		prdmod(divlbys(val, (unsigned)10), fa);
	*fa->bufp++ = dtab[modlbys(val, (unsigned)10)];
}

LOCAL void promod(val, fa)
	Ulong val;
	f_args *fa;
{
	if (val >= (unsigned)8)
		promod(val>>3, fa);
	*fa->bufp++ = dtab[val & 7];
}

LOCAL void prxmod(val, fa)
	Ulong val;
	f_args *fa;
{
	if (val >= (unsigned)16)
		prxmod(val>>4, fa);
	*fa->bufp++ = dtab[val & 15];
}

LOCAL void prXmod(val, fa)
	Ulong val;
	f_args *fa;
{
	if (val >= (unsigned)16)
		prXmod(val>>4, fa);
	*fa->bufp++ = udtab[val & 15];
}

/*
 * Final buffer print out routine.
 */
LOCAL int prbuf(s, fa)
	register const char *s;
	f_args *fa;
{
	register int diff;
	register int rfillc;
	register long arg			= fa->farg;
	register void (*fun) __PR((char, long))	= fa->outf;
	register int count;
	register int lzero = 0;

	count = strlen(s);

	if (fa->lzero < 0 && count < fa->signific)
		lzero = fa->signific - count;
	diff = fa->fldwidth - lzero - count - fa->prefixlen;
	count += lzero;
	if (diff > 0)
		count += diff;

	if (fa->prefixlen && fa->fillc != ' ') {
		while (*fa->prefix != '\0')
			(*fun)(*fa->prefix++, arg);
	}
	if (!fa->minusflag) {
		rfillc = fa->fillc;
		while (--diff >= 0)
			(*fun)(rfillc, arg);
	}
	if (fa->prefixlen && fa->fillc == ' ') {
		while (*fa->prefix != '\0')
			(*fun)(*fa->prefix++, arg);
	}
	if (lzero > 0) {
		rfillc = '0';
		while (--lzero >= 0)
			(*fun)(rfillc, arg);
	}
	while (*s != '\0')
		(*fun)(*s++, arg);
	if (fa->minusflag) {
		rfillc = ' ';
		while (--diff >= 0)
			(*fun)(rfillc, arg);
	}
	return (count);
}

/*
 * Print out one char, allowing prc('\0')
 * Similar to prbuf()
 */
#ifdef	PROTOTYPES

LOCAL int prc(char c, f_args *fa)

#else
LOCAL int prc(c, fa)
	char	c;
	f_args *fa;
#endif
{
	register int diff;
	register int rfillc;
	register long arg			= fa->farg;
	register void (*fun) __PR((char, long))	= fa->outf;
	register int count;

	count = 1;
	diff = fa->fldwidth - 1;
	if (diff > 0)
		count += diff;

	if (!fa->minusflag) {
		rfillc = fa->fillc;
		while (--diff >= 0)
			(*fun)(rfillc, arg);
	}
	(*fun)(c, arg);
	if (fa->minusflag) {
		rfillc = ' ';
		while (--diff >= 0)
			(*fun)(rfillc, arg);
	}
	return (count);
}

/*
 * String output routine.
 * If fa->signific is >= 0, it uses only fa->signific chars.
 * If fa->signific is 0, print no characters.
 */
LOCAL int prstring(s, fa)
	register const char	*s;
	f_args *fa;
{
	register char	*bp;

	if (s == NULL)
		return (prbuf("(NULL POINTER)", fa));

	if (fa->signific < 0)
		return (prbuf(s, fa));

	bp = fa->buf;

	while (--fa->signific >= 0 && *s != '\0')
		*bp++ = *s++;
	*bp = '\0';

	return (prbuf(fa->buf, fa));
}
