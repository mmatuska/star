/* @(#)tartest.c	1.3 02/06/19 Copyright 2002 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)tartest.c	1.3 02/06/19 Copyright 2002 J. Schilling";
#endif
/*
 *	Copyright (c) 20002 J. Schilling
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
#include <stdxlib.h>

#include "star.h"
#include <standard.h>
#include <strdefs.h>
#include <getargs.h>
#include <schily.h>

EXPORT	int	main		__PR((int ac, char *av[]));
LOCAL	BOOL	doit		__PR((FILE *f));
LOCAL	BOOL	checkhdr	__PR((TCB *ptb));
LOCAL	BOOL	checkoctal	__PR((char *ptr, int len, char *text));
LOCAL	BOOL	checktype	__PR((TCB *ptb));
LOCAL	BOOL	checkid		__PR((char *ptr, char *text));
LOCAL	BOOL	checkmagic	__PR((char *ptr));
LOCAL	BOOL	checkvers	__PR((char *ptr));
EXPORT	void	stolli		__PR((char* s, Ullong * ull, int len));
LOCAL	Ulong	checksum	__PR((TCB * ptb));
LOCAL	void	pretty_char	__PR((char *p, unsigned c));

LOCAL	BOOL	verbose;
LOCAL	BOOL	signedcksum;
LOCAL	BOOL	is_posix_2001;

LOCAL void
usage(ret)
	int	ret;
{
	error("Usage:\t%s [options] < file\n", get_progname());
	error("Options:\n");
	error("\t-help\t\tprint this help\n");
	error("\t-v\t\tprint all filenames during verification\n");
	error("\n%s checks stdin fore compliance with the POSIX.1-1990 TAR standard\n", get_progname());
	exit(ret);
	/* NOTREACHED */
}

EXPORT int
main(ac, av)
	int	ac;
	char	*av[];
{
	int		cac = ac;
	char	*const *cav = av;
	BOOL		help = FALSE;

	save_args(ac, av);
	cac--;
	cav++;
	if (getallargs(&cac, &cav, "help,h,v", &help, &help, &verbose) < 0) {
		errmsgno(EX_BAD, "Bad Option: '%s'.\n", cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);

	printf("tartest %s (%s-%s-%s)\n\n", "1.3",
					HOST_CPU, HOST_VENDOR, HOST_OS);
	printf("Copyright (C) 2002 J�rg Schilling\n");
	printf("This is free software; see the source for copying conditions.  There is NO\n");
	printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");

	printf("\nTesting for POSIX.1-1990 TAR compliance...\n");

	if (!doit(stdin)) {
		printf(">>> Archive is not POSIX.1-1990 TAR standard compliant.\n");
		return (1);
	}
	printf("No deviations from POSIX.1-1990 TAR standard found.\n");
	return (0);
}

LOCAL BOOL
doit(f)
	FILE	*f;
{
	BOOL	ret = TRUE;
	BOOL	r;
	TCB	tcb;
	TCB	*ptb;
	char	name[257];
	char	lname[101];
	Ullong	checks;
	Ullong	hsum;
	Ullong	size;
	Ullong	blockno = 0;
	int	i;

	ptb = &tcb;
	for (;;) {
		r = TRUE;
		fillbytes(ptb, TBLOCK, '\0');
		if (fileread(f, ptb, TBLOCK) != TBLOCK) {
			printf("Hard EOF at block %lld\n", blockno);
			return (FALSE);
		}

		stolli(ptb->ustar_dbuf.t_chksum, &checks, 8);
		hsum = checksum(ptb);
		if (hsum == 0) {
			/*
			 * Check EOF
			 */
			printf("Found 1st EOF block at %lld\n", blockno);
			blockno++;
			if (fileread(f, ptb, TBLOCK) != TBLOCK) {
				printf(
				"Hard EOF at block %lld (second EOF block missing)\n",
					blockno);
				return (FALSE);
			}
			hsum = checksum(ptb);
			if (hsum != 0) {
				printf(
					"Second EOF block missing at %lld\n",
					blockno);
				return (FALSE);
			}
			printf("Found 2nd EOF block at %lld\n", blockno);
			return (ret);
		} if (checks != hsum) {
			printf("Bad Checksum %0llo != %0llo at block %lld\n",
							checks, hsum, blockno);
			signedcksum = TRUE;
			hsum = checksum(ptb);
			if (checks == hsum) {
				printf("Warning: archive uses signed checksums.\n");
				return (FALSE);
			}
			if (blockno != 0) {
				if (is_posix_2001) {
					printf(
					"The archive may either be corrupted or using the POSIX.1-2001 size field.\n");
				} else {
					printf(
					"Warning: Corrupted TAR archive.\n");
				}
			}
			return (FALSE);
		}
		blockno++;

		stolli(ptb->ustar_dbuf.t_size, &size, 12);

		if (ptb->ustar_dbuf.t_prefix[0]) {
			js_snprintf(name, sizeof(name), "%.155s/%.100s",
				ptb->ustar_dbuf.t_prefix,
				ptb->ustar_dbuf.t_name);
		} else {
			strncpy(name, ptb->ustar_dbuf.t_name, 100);
			name[100] = '\0';
		}
		strncpy(lname, ptb->ustar_dbuf.t_linkname, 100);
		lname[100] = '\0';

		r = checkhdr(ptb);
		if (!r)
			ret = FALSE;

		/*
		 * Handle the size field acording to POSIX.1-1990.
		 */
		i = tarblocks(size);
		switch ((int)(ptb->ustar_dbuf.t_typeflag & 0xFF)) {

		case '\0':	/* Old plain file */
		case '0':	/* Ustar plain file */
		case '7':	/* Contiguous file */
			break;

		case '1':	/* Hard link */
		case '2':	/* Symbolic link */
			if (i != 0) {
				printf(
				"Warning: t_size field: %0llu, should be 0 for %s link\n",
					size,
					ptb->ustar_dbuf.t_typeflag == '1'?
					"hard":"symbolic");
				ret = r = FALSE;
			}
			i = 0;
			break;
		case '3':	/* Character special */
		case '4':	/* Block special */
		case '5':	/* Directory */
		case '6':	/* FIFO  (named pipe) */
			i = 0;
			break;
		}

		if (!r || verbose) {
			printf("*** %sFilename '%s'\n",
						r==FALSE?"Failing ":"", name);
			if (lname[0])
				printf("*** %sLinkname '%s'\n",
						r==FALSE?"Failing ":"", lname);
		}

		/*
		 * Skip file content.
		 */
		while (--i >= 0) {
			if (fileread(f, ptb, TBLOCK) != TBLOCK) {
				printf("Hard EOF at block %lld\n", blockno);
				return (FALSE);
			}
			blockno++;
		}
	}

}

LOCAL BOOL
checkhdr(ptb)
	TCB	*ptb;
{
	BOOL	ret = TRUE;
	int	errs = 0;
	Ullong	ll;

	if (ptb->ustar_dbuf.t_name[  0] != '\0' &&
	    ptb->ustar_dbuf.t_name[ 99] != '\0' &&
	    ptb->ustar_dbuf.t_name[100] == '\0') {
		printf("Warning: t_name[100] is a null character.\n");
		errs++;
	}
	if (ptb->ustar_dbuf.t_linkname[  0] != '\0' &&
	    ptb->ustar_dbuf.t_linkname[ 99] != '\0' &&
	    ptb->ustar_dbuf.t_linkname[100] == '\0') {
		printf("Warning: t_linkname[100] is a null character.\n");
		errs++;
	}

	if (!checkoctal(ptb->ustar_dbuf.t_mode, 8, "t_mode"))
		errs++;

	stolli(ptb->ustar_dbuf.t_mode, &ll, 8);
	if (ll & ~07777) {
		printf(
		"Warning: too many bits in t_mode field: 0%llo, should be 0%llo\n",
			ll, ll & 07777);
		errs++;
	}

	if (!checkoctal(ptb->ustar_dbuf.t_uid, 8, "t_uid"))
		errs++;

	if (!checkoctal(ptb->ustar_dbuf.t_gid, 8, "t_gid"))
		errs++;

	if (!checkoctal(ptb->ustar_dbuf.t_size, 12, "t_size"))
		errs++;

	if (!checkoctal(ptb->ustar_dbuf.t_mtime, 12, "t_mtime"))
		errs++;

	if (!checkoctal(ptb->ustar_dbuf.t_chksum, 8, "t_chksum"))
		errs++;

	if (!checktype(ptb))
		errs++;

	if (!checkmagic(ptb->ustar_dbuf.t_magic))
		errs++;

	if (!checkvers(ptb->ustar_dbuf.t_version))
		errs++;

	if (!checkid(ptb->ustar_dbuf.t_uname, "t_uname"))
		errs++;
	if (!checkid(ptb->ustar_dbuf.t_gname, "t_gname"))
		errs++;

	if (!checkoctal(ptb->ustar_dbuf.t_devmajor, 8, "t_devmajor"))
		errs++;

	if (!checkoctal(ptb->ustar_dbuf.t_devminor, 8, "t_devminor"))
		errs++;

#ifdef	__needed__
	/*
	 * The POSIX.1 TAR standard does not mention the last 12 bytes in the
	 * TAR header. They may have any value...
	 */
	if (cmpnullbytes(ptb->ustar_dbuf.t_mfill, 12) < 12) {
		printf(
		"Warning: non null character in last 12 bytes of header\n");
		errs++;
	}
#endif

	if (errs)
		ret= FALSE;
	return (ret);
}

/*
 * Check whether octal numeric fields are according to POSIX.1-1990.
 */
LOCAL BOOL
checkoctal(ptr, len, text)
	char	*ptr;
	int	len;
	char	*text;
{
	BOOL	ret = TRUE;
	BOOL	foundoctal = FALSE;
	int	i;
	int	endoff = 0;
	char	endc = '\0';
	char	cs[4];

	for (i=0; i < len; i++) {
/*		error("%d '%c'\n", i, ptr[i]);*/

#ifdef		END_ALL_THESAME
		if (endoff > 0 && ptr[i] != endc) {
#else
		/*
		 * Ugly, but the standard seems to allow mixins space and null
		 * characters at the end of an octal numeric field.
		 */
		if (endoff > 0 && (ptr[i] != ' ' && ptr[i] != '\0')) {
#endif
			pretty_char(cs, ptr[i] & 0xFF);
			printf(
			"Warning: illegal end character '%s' (0x%02X) found in field '%s[%d]'\n",
					cs,
					ptr[i] & 0xFF,
					text, i);
			ret = FALSE;
		}
		if (endoff > 0)
			continue;
		if (ptr[i] == ' ' || ptr[i] == '\0') {
			if (foundoctal) {
				endoff = i;
				endc = ptr[i];
				continue;
			}
		}
		if (!isoctal(ptr[i])) {
			pretty_char(cs, ptr[i] & 0xFF);
			printf(
			"Warning: non octal character '%s' (0x%02X) found in field '%s[%d]'\n",
					cs,
					ptr[i] & 0xFF,
					text, i);
			ret = FALSE;
		} else {
			foundoctal = TRUE;
		}
	}
	if (foundoctal && endoff == 0) {
		printf("Warning: no end character found in field '%s'\n",
			text);
		ret = FALSE;
	}
	return (ret);
}

/*
 * Check whether the POSIX.1-1990 'typeflag' field contains a valid character.
 */
LOCAL BOOL
checktype(ptb)
	TCB	*ptb;
{
	BOOL	ret = TRUE;
	char	cs[4];

	switch ((int)(ptb->ustar_dbuf.t_typeflag & 0xFF)) {

	case '\0':	/* Old plain file */
	case '0':	/* Ustar plain file */
	case '1':	/* Hard link */
	case '2':	/* Symbolic link */
	case '3':	/* Character special */
	case '4':	/* Block special */
	case '5':	/* Directory */
	case '6':	/* FIFO  (named pipe) */
	case '7':	/* Contiguous file */
		break;

	case 'g':
	case 'x':
		if (!is_posix_2001) {
			error("Archive uses POSIX.1-2001 extensions.\n");
			error("The correctness of the size field cannot be checked for this reason.\n");
			is_posix_2001 = TRUE;
		}
		break;

	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
	case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
	case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
	case 'V': case 'W': case 'X': case 'Y': case 'Z':
		{ static char vend[256];
			if (vend[ptb->ustar_dbuf.t_typeflag & 0xFF] == 0) {
				error(
				"Archive uses Vendor specific extension file type '%c'.\n",
				ptb->ustar_dbuf.t_typeflag & 0xFF);
				vend[ptb->ustar_dbuf.t_typeflag & 0xFF] = 1;
			}
		}
		break;

	default:
		pretty_char(cs, ptb->ustar_dbuf.t_typeflag & 0xFF);
		printf(
		"Warning: Archive uses illegal file type '%s' (0x%02X).\n",
				cs,
				ptb->ustar_dbuf.t_typeflag & 0xFF);
		ret = FALSE;
	}
	return (ret);
}

/*
 * Check whether the POSIX.1-1990 'uid' or 'gid' field contains
 * reasonable things.
 */
LOCAL BOOL
checkid(ptr, text)
	char	*ptr;
	char	*text;
{
	BOOL	ret = TRUE;
	char	cs[4];
	int	len = 32;
	int	i;

	if (ptr[0] == '\0') {
		for (i=0; i < len; i++) {
			if (ptr[i] != '\0') {
				pretty_char(cs, ptr[i] & 0xFF);
				printf(
				"Warning: non null character '%s' (0x%02X) found in field '%s[%d]'\n",
						cs,
						ptr[i] & 0xFF,
						text, i);
				ret = FALSE;
			}
		}
		return (ret);
	}
	i = len - 1;
	if (ptr[i] != '\0') {
		pretty_char(cs, ptr[i] & 0xFF);
		printf(
		"Warning: non null string terminator character '%s' (0x%02X) found in field '%s[%d]'\n",
				cs,
				ptr[i] & 0xFF,
				text, i);
		ret = FALSE;
	}
	return (ret);
}

/*
 * Check whether the POSIX.1-1990 'magic' field contains the
 * two string "ustar" (5 characters and a null byte).
 */
LOCAL BOOL
checkmagic(ptr)
	char	*ptr;
{
	BOOL	ret = TRUE;
	char	mag[6] = "ustar";
	char	cs[4];
	int	i;

	for (i=0; i < 6; i++) {
		if (ptr[i] != mag[i]) {
			pretty_char(cs, ptr[i] & 0xFF);
			printf(
			"Warning: illegal character '%s' (0x%02X) found in field 't_magic[%d]'\n",
					cs,
					ptr[i] & 0xFF, i);
			ret = FALSE;
		}
	}
	return (ret);
}

/*
 * Check whether the POSIX.1-1990 'version' field contains the
 * two characters "00".
 */
LOCAL BOOL
checkvers(ptr)
	char	*ptr;
{
	BOOL	ret = TRUE;
	char	vers[3] = "00";
	char	cs[4];
	int	i;

	for (i=0; i < 2; i++) {
		if (ptr[i] != vers[i]) {
			pretty_char(cs, ptr[i] & 0xFF);
			printf(
			"Warning: illegal character '%s' (0x%02X) found in field 't_version[%d]'\n",
					cs,
					ptr[i] & 0xFF, i);
			ret = FALSE;
		}
	}
	return (ret);
}


/*
 * Convert string -> long long int
 * This is the debug version that stops at "len" size to be safe against
 * field overflow.
 */
EXPORT void /*char **/
stolli(s, ull, len)
	register char	*s;
		 Ullong	*ull;
		 int	len;
{
	register Ullong	ret = (Ullong)0;
	register char	c;
	register int	t;
	
	while(*s == ' ') {
		if (--len < 0)
			break;
		s++;
	}

	for(;;) {
		if (--len < 0)
			break;
		c = *s++;
/*		error("'%c'\n", c);*/
		if(isoctal(c))
			t = c - '0';
		else
			break;
		ret *= 8;
		ret += t;
	}
	*ull = ret;
/*	error("len: %d\n", len);*/
	/*return(s);*/
}

/*
 * Checsum function.
 * Returns 0 if the block contains nothing but null characters.
 */
#define	CHECKS	sizeof(ptb->ustar_dbuf.t_chksum)
/*
 * We know, that sizeof(TCP) is 512 and therefore has no
 * reminder when dividing by 8
 *
 * CHECKS is known to be 8 too, use loop unrolling.
 */
#define	DO8(a)	a;a;a;a;a;a;a;a;

LOCAL Ulong
checksum(ptb)
	register	TCB	*ptb;
{
	register	int	i;
	register	Ulong	sum = 0;
	register	Uchar	*us;

	if (signedcksum) {
		register	char	*ss;

		ss = (char *)ptb;
		for (i=sizeof(*ptb)/8; --i >= 0;) {
			DO8(sum += *ss++);
		}
		if (sum == 0L)		/* Block containing 512 nul's */
			return(sum);

		ss=(char *)ptb->ustar_dbuf.t_chksum;
		DO8(sum -= *ss++);
		sum += CHECKS*' ';
	} else {
		us = (Uchar *)ptb;
		for (i=sizeof(*ptb)/8; --i >= 0;) {
			DO8(sum += *us++);
		}
		if (sum == 0L)		/* Block containing 512 nul's */
			return(sum);

		us=(Uchar *)ptb->ustar_dbuf.t_chksum;
		DO8(sum -= *us++);
		sum += CHECKS*' ';
	}
	return sum;
}

/*
 * Pretty print one character.
 * Quote anything that is not a printable 7 bit ASCII character.
 */
#define	SP	' '
#define	DEL	'\177'
#define	SP8	(SP | 0x80)
#define	DEL8	(DEL | 0x80)

LOCAL void
pretty_char(p, c)
	char		*p;
	unsigned	c;
{
	c &= 0xFF;

	if (c < SP || c == DEL) {			/* ctl char */
		*p++ = '^';
		*p++ = c ^ 0100;
	} else if ((c > DEL && c < SP8) || c == DEL8) {  /* 8 bit ctl */
		*p++ = '~';
		*p++ = '^';
		*p++ = c ^ 0300;
	} else if (c >= SP8) {			      /* 8 bit char */
		*p++ = '~';
		*p++ = c & 0177;
	} else {				     /* normal char */
		*p++ = c;
	}
	*p = '\0';
}