/* @(#)align_test.c	1.6 96/11/29 Copyright 1995 J. Schilling */
#ifndef	lint
static	char sccsid[] =
	"@(#)align_test.c	1.6 96/11/29 Copyright 1995 J. Schilling";
#endif
/* generate machine dependant align.h */
/* This program is free software; you can redistribute it and/or modify
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
#include <signal.h>
#include <setjmp.h>

char	buf[8192+1024];
char	*buf_aligned;

jmp_buf	jb;

int	check_align	__PR((int (*)(char*, int)));
int	check_short	__PR((char *, int));
int	check_int	__PR((char *, int));
int	check_long	__PR((char *, int));
int	check_longlong	__PR((char *, int));
int	check_double	__PR((char *, int));
void	printmacs	__PR((void));
void	sig		__PR((int));

void sig(signo)
	int	signo;
{
	signal(signo, sig);
	longjmp(jb, 1);
}

#if defined(mc68000) || defined(mc68020)
#define	MIN_ALIGN	2
#else
#define	MIN_ALIGN	2
#endif

#define	min_align(i)	(((i) < MIN_ALIGN) ? MIN_ALIGN : (i))

char	al[] = "alignment value for ";
char	ms[] = "alignment mask  for ";
char	sh[] = "short";
char	in[] = "int";
char	lo[] = "long";
char	ll[] = "long long";
char	db[] = "double";

#define	xalign(x, a, m)		( ((char *)(x)) + ( (a) - (((int)(x))&(m))) )

int main()
{
	char	*p;
	int	i;

	signal(SIGBUS, sig);

	i = ((int)buf) % 1024;
	i = 1024 - i;
	p = &buf[i];
	buf_aligned = p;

/*	printf("buf: %X %X\n", buf, xalign(buf, 1024, 1023));*/

	printf("/*\n");
	printf(" * This file has been generated automatically\n");
	printf(" * by %s\n", sccsid);
	printf(" * do not edit by hand.\n");
	printf(" */\n");

	i  = check_align(check_short);
	i = min_align(i);
	printf("\n");
	printf("#define	ALIGN_SHORT	%d\t/* %s(%s *)\t*/\n", i, al, sh);
	printf("#define	ALIGN_SMASK	%d\t/* %s(%s *)\t*/\n", i-1, ms, sh);

	i  = check_align(check_int);
	i = min_align(i);
	printf("\n");
	printf("#define	ALIGN_INT	%d\t/* %s(%s *)\t\t*/\n", i, al, in);
	printf("#define	ALIGN_IMASK	%d\t/* %s(%s *)\t\t*/\n", i-1, ms, in);

	i  = check_align(check_long);
	i = min_align(i);
	printf("\n");
	printf("#define	ALIGN_LONG	%d\t/* %s(%s *)\t*/\n", i, al, lo);
	printf("#define	ALIGN_LMASK	%d\t/* %s(%s *)\t*/\n", i-1, ms, lo);

#ifdef	HAVE_LONGLONG
	i  = check_align(check_longlong);
	i = min_align(i);
#endif
	printf("\n");
	printf("#define	ALIGN_LLONG	%d\t/* %s(%s *)\t*/\n", i, al, ll);
	printf("#define	ALIGN_LLMASK	%d\t/* %s(%s *)\t*/\n", i-1, ms, ll);

	i  = check_align(check_double);
	i = min_align(i);
	printf("\n");
	printf("#define	ALIGN_DOUBLE	%d\t/* %s(%s *)\t*/\n", i, al, db);
	printf("#define	ALIGN_DMASK	%d\t/* %s(%s *)\t*/\n", i-1, ms, db);

	printmacs();
	return(0);
}

int check_align(func)
	int	(*func)();
{
	register int	i;
	register char	*p = buf_aligned;

	for (i=1; i < 128; i++) {
		if (!setjmp(jb)) {
			(func)(p, i);
			break;
		}
	}
	return (i);
}

int check_short(p, i)
	char	*p;
	int	i;
{
	short	*sp;

	sp = (short *)&p[i];
	*sp = 1;
	return (0);
}

int check_int(p, i)
	char	*p;
	int	i;
{
	int	*ip;

	ip = (int *)&p[i];
	*ip = 1;
	return (0);
}

int check_long(p, i)
	char	*p;
	int	i;
{
	long	*lp;

	lp = (long *)&p[i];
	*lp = 1;
	return (0);
}

int	speed_long(n)
	int	n;
{
	long	*lp;
	int	i;

	lp = (long *)&buf_aligned[1];

	for (i = 1000000; --i >= 0;)
		*lp = i;
	return (0);
}

#ifdef	HAVE_LONGLONG
int check_longlong(p, i)
	char	*p;
	int	i;
{
	long long	*llp;

	llp = (long long *)&p[i];
	*llp = 1;
	return (0);
}
#endif

int check_double(p, i)
	char	*p;
	int	i;
{
	double	*dp;

	dp = (double *)&p[i];
	*dp = 1.0;
	return (0);
}

void printmacs()
{
printf("\n");
printf("#define	xaligned(a, s)		((((int)(a)) & s) == 0 )\n");
printf("#define	x2aligned(a, b, s)	(((((int)(a)) | ((int)(b))) & s) == 0 )\n");
printf("\n");
printf("#define	saligned(a)		xaligned(a, ALIGN_SMASK)\n");
printf("#define	s2aligned(a, b)		x2aligned(a, b, ALIGN_SMASK)\n");
printf("\n");
printf("#define	ialigned(a)		xaligned(a, ALIGN_IMASK)\n");
printf("#define	i2aligned(a, b)		x2aligned(a, b, ALIGN_IMASK)\n");
printf("\n");
printf("#define	laligned(a)		xaligned(a, ALIGN_LMASK)\n");
printf("#define	l2aligned(a, b)		x2aligned(a, b, ALIGN_LMASK)\n");
printf("\n");
printf("#define	llaligned(a)		xaligned(a, ALIGN_LLMASK)\n");
printf("#define	ll2aligned(a, b)	x2aligned(a, b, ALIGN_LLMASK)\n");
printf("\n");
printf("#define	daligned(a)		xaligned(a, ALIGN_DMASK)\n");
printf("#define	d2aligned(a, b)		x2aligned(a, b, ALIGN_DMASK)\n");

printf("\n\n");
printf("#define	xalign(x, a, m)		( ((char *)(x)) + ( (a) - (((int)(x))&(m))) )\n");
printf("\n");
printf("#define	salign(x)		xalign((x), ALIGN_SHORT, ALIGN_SMASK)\n");
printf("#define	ialign(x)		xalign((x), ALIGN_INT, ALIGN_IMASK)\n");
printf("#define	lalign(x)		xalign((x), ALIGN_LONG, ALIGN_LMASK)\n");
printf("#define	llalign(x)		xalign((x), ALIGN_LLONG, ALIGN_LLMASK)\n");
printf("#define	dalign(x)		xalign((x), ALIGN_DOUBLE, ALIGN_DMASK)\n");
}
