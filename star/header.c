/* @(#)header.c	1.43 01/04/07 Copyright 1985, 1995 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)header.c	1.43 01/04/07 Copyright 1985, 1995 J. Schilling";
#endif
/*
 *	Handling routines to read/write, parse/create
 *	archive headers
 *
 *	Copyright (c) 1985, 1995 J. Schilling
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
#include "star.h"
#include "props.h"
#include "table.h"
#include <sys/types.h>
#include <dirdefs.h>
#include <standard.h>
#include <stdxlib.h>
#include <strdefs.h>
#define	__XDEV__	/* Needed to activate _dev_major()/_dev_minor() */
#include <device.h>
#include <schily.h>
#include "starsubs.h"

	/* ustar */
LOCAL	char	magic[TMAGLEN] = TMAGIC;
	/* star */
LOCAL	char	stmagic[STMAGLEN] = STMAGIC;
	/* gnu tar */
LOCAL	char	gmagic[GMAGLEN] = GMAGIC;

LOCAL	char	*hdrtxt[] = {
	/* 0 */	"UNDEFINED",
	/* 1 */	"unknown tar",
	/* 2 */	"old tar",
	/* 3 */	"star",
	/* 4 */	"gnu tar",
	/* 5 */	"ustar",
	/* 6 */	"xstar",
	/* 7 */	"xustar",
	/* 8 */	"res8",		/* Reserved	*/
	/* 9 */	"bar",
	/*10 */	"cpio binary",
	/*11 */	"cpio -c",
	/*12 */	"cpio",
	/*13 */	"cpio crc",
	/*14 */	"cpio ascii",
	/*15 */	"cpio ascii crc",
};

extern	FILE	*tty;
extern	long	hdrtype;
extern	long	chdrtype;
extern	int	version;
extern	int	swapflg;
extern	BOOL	debug;
extern	BOOL	numeric;
extern	BOOL	verbose;
extern	BOOL	xflag;
extern	BOOL	nflag;
extern	BOOL	ignoreerr;
extern	BOOL	signedcksum;
extern	BOOL	nowarn;
extern	BOOL	nullout;
extern	BOOL	modebits;

extern	Ulong	tsize;

extern	char	*bigbuf;
extern	int	bigsize;

LOCAL	Ulong	checksum	__PR((TCB * ptb));
LOCAL	Ulong	bar_checksum	__PR((TCB * ptb));
LOCAL	BOOL	isstmagic	__PR((char* s));
LOCAL	BOOL	isxmagic	__PR((TCB *ptb));
LOCAL	BOOL	ismagic		__PR((char* s));
LOCAL	BOOL	isgnumagic	__PR((char* s));
LOCAL	BOOL	strxneql	__PR((char* s1, char* s2, int l));
LOCAL	BOOL	ustmagcheck	__PR((TCB * ptb));
LOCAL	void	print_hdrtype	__PR((int type));
LOCAL	int	get_hdrtype	__PR((TCB * ptb, BOOL isrecurse));
EXPORT	int	get_tcb		__PR((TCB * ptb));
EXPORT	void	put_tcb		__PR((TCB * ptb, FINFO * info));
EXPORT	void	write_tcb	__PR((TCB * ptb, FINFO * info));
EXPORT	void	put_volhdr	__PR((char* name));
EXPORT	BOOL	get_volhdr	__PR((FINFO * info, char* vhname));
EXPORT	void	info_to_tcb	__PR((FINFO * info, TCB * ptb));
LOCAL	void	info_to_star	__PR((FINFO * info, TCB * ptb));
LOCAL	void	info_to_ustar	__PR((FINFO * info, TCB * ptb));
LOCAL	void	info_to_xstar	__PR((FINFO * info, TCB * ptb));
LOCAL	void	info_to_gnutar	__PR((FINFO * info, TCB * ptb));
EXPORT	int	tcb_to_info	__PR((TCB * ptb, FINFO * info));
LOCAL	void	tar_to_info	__PR((TCB * ptb, FINFO * info));
LOCAL	void	star_to_info	__PR((TCB * ptb, FINFO * info));
LOCAL	void	ustar_to_info	__PR((TCB * ptb, FINFO * info));
LOCAL	void	xstar_to_info	__PR((TCB * ptb, FINFO * info));
LOCAL	void	gnutar_to_info	__PR((TCB * ptb, FINFO * info));
LOCAL	void	cpiotcb_to_info	__PR((TCB * ptb, FINFO * info));
LOCAL	int	ustoxt		__PR((int ustype));
EXPORT	BOOL	ia_change	__PR((TCB * ptb, FINFO * info));
LOCAL	BOOL	checkeof	__PR((TCB * ptb));
LOCAL	BOOL	eofblock	__PR((TCB * ptb));
EXPORT	void	astoo_cpio	__PR((char* s, Ulong * l, int cnt));
EXPORT	void	astoo		__PR((char* s, Ulong * l));
EXPORT	void	otoa		__PR((char* s, Ulong l, int fieldw));
EXPORT	void	astob		__PR((char* s, Ulong * l, int fieldw));
EXPORT	void	btoa		__PR((char* s, Ulong l, int fieldw));

/*
 * XXX Hier sollte eine tar/bar universelle Checksummenfunktion sein!
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
#undef	CHECKS

#define	CHECKS	sizeof(ptb->bar_dbuf.t_chksum)

LOCAL Ulong
bar_checksum(ptb)
	register	TCB	*ptb;
{
	register	int	i;
	register	Ulong	sum = 0;
	register	Uchar	*us;

	if (signedcksum) {
		register	char	*ss;

		ss = (char *)ptb;
		for (i=sizeof(*ptb); --i >= 0;)
			sum += *ss++;
		if (sum == 0L)		/* Block containing 512 nul's */
			return(sum);

		for (i=CHECKS, ss=(char *)ptb->bar_dbuf.t_chksum; --i >= 0;)
			sum -= *ss++;
		sum += CHECKS*' ';
	} else {
		us = (Uchar *)ptb;
		for (i=sizeof(*ptb); --i >= 0;)
			sum += *us++;
		if (sum == 0L)		/* Block containing 512 nul's */
			return(sum);

		for (i=CHECKS, us=(Uchar *)ptb->bar_dbuf.t_chksum; --i >= 0;)
			sum -= *us++;
		sum += CHECKS*' ';
	}
	return sum;
}
#undef	CHECKS

LOCAL BOOL
isstmagic(s)
	char	*s;
{
	return (strxneql(s, stmagic, STMAGLEN));
}

LOCAL BOOL
isxmagic(ptb)
	TCB	*ptb;
{
	/*
	 * prefix[130] is is granted to be '\0' with 'xstar'.
	 */
	if (ptb->xstar_dbuf.t_prefix[130] != '\0')
		return (FALSE);
	/*
	 * If atime[0] or ctime[0] is not an octal number it cannot be 'xstar'.
	 */
	if (ptb->xstar_dbuf.t_atime[0] < '0' ||
	    ptb->xstar_dbuf.t_atime[0] > '7')
		return (FALSE);
	if (ptb->xstar_dbuf.t_ctime[0] < '0' ||
	    ptb->xstar_dbuf.t_ctime[0] > '7')
		return (FALSE);

	/*
	 * We may need to remove this in future.
	 */
	if (ptb->xstar_dbuf.t_atime[11] != ' ' ||
	    ptb->xstar_dbuf.t_ctime[11] != ' ')
		return (FALSE);

	return (TRUE);
}

LOCAL BOOL
ismagic(s)
	char	*s;
{
	return (strxneql(s, magic, TMAGLEN));
}

LOCAL BOOL
isgnumagic(s)
	char	*s;
{
	return (strxneql(s, gmagic, GMAGLEN));
}

LOCAL BOOL
strxneql(s1, s2, l)
	register char	*s1;
	register char	*s2;
	register int	l;
{
	while (--l >= 0)
		if (*s1++ != *s2++)
			return (FALSE);
	return (TRUE);
}

LOCAL BOOL
ustmagcheck(ptb)
	TCB	*ptb;
{
	if (ismagic(ptb->xstar_dbuf.t_magic) &&
				strxneql(ptb->xstar_dbuf.t_version, "00", 2))
		return (TRUE);
	return (FALSE);
}

LOCAL void
print_hdrtype(type)
	int	type;
{
	BOOL	isswapped = H_ISSWAPPED(type);

	if (H_TYPE(type) > H_MAX_ARCH)
		type = H_UNDEF;
	type = H_TYPE(type);

	error("%s%s archive.\n", isswapped?"swapped ":"", hdrtxt[type]);
}

LOCAL int
get_hdrtype(ptb, isrecurse)
	TCB	*ptb;
	BOOL	isrecurse;
{
	Ulong	check;
	Ulong	ocheck;
	int	ret = H_UNDEF;

	astoo(ptb->dbuf.t_chksum, &ocheck);
	check = checksum(ptb);
	if (ocheck != check)
		goto nottar;

	if (isstmagic(ptb->dbuf.t_magic)) {	/* Check for 'tar\0' at end */
		if (ustmagcheck(ptb))
			ret = H_XSTAR;
		else
			ret = H_STAR;
		if (debug) print_hdrtype(ret);
		return (ret);
	}
	if (ustmagcheck(ptb)) {			/* 'ustar\000' POSIX magic */
		if (isxmagic(ptb))
			ret = H_XUSTAR;
		else
			ret = H_USTAR;
		if (debug) print_hdrtype(ret);
		return (ret);
	}
	if (isgnumagic(&ptb->dbuf.t_vers)) {	/* 'ustar  ' GNU magic */
		ret = H_GNUTAR;
		if (debug) print_hdrtype(ret);
		return (ret);
	}
	if ((ptb->dbuf.t_mode[6] == ' ' && ptb->dbuf.t_mode[7] == '\0')) {
		ret = H_OTAR;
		if (debug) print_hdrtype(ret);
		return (ret);
	}
	if (ptb->ustar_dbuf.t_typeflag == LF_VOLHDR ||
			    ptb->ustar_dbuf.t_typeflag == LF_MULTIVOL) {
		/*
		 * Gnu volume headers & multi volume headers
		 * are no real tar headers.
		 */
		if (debug) error("gnutar buggy archive.\n");
		ret = H_GNUTAR;
		if (debug) print_hdrtype(ret);
		return (ret);
	}
	/*
	 * The only thing we know here is:
	 * we found a header with a correct tar checksum.
	 */
	ret = H_TAR;
	if (debug) print_hdrtype(ret);
	return (ret);

nottar:
	if (ptb->bar_dbuf.bar_magic[0] == 'V') {
		astoo(ptb->bar_dbuf.t_chksum, &ocheck);
		check = bar_checksum(ptb);

		if (ocheck == check) {
			ret = H_BAR;
			if (debug) print_hdrtype(ret);
			return (ret);
		}
	}
	if (strxneql((char *)ptb, "070701", 6)) {
		ret = H_CPIO_ASC;
		if (debug) print_hdrtype(ret);
		return (ret);
	}
	if (strxneql((char *)ptb, "070702", 6)) {
		ret = H_CPIO_ACRC;
		if (debug) print_hdrtype(ret);
		return (ret);
	}
	if (strxneql((char *)ptb, "070707", 6)) {
		ret = H_CPIO_CHR;
		if (debug) print_hdrtype(ret);
		return (ret);

	}
	if (strxneql((char *)ptb, "\161\301", 2)) {
		ret = H_CPIO_NBIN;
		if (debug) print_hdrtype(ret);
		return (ret);
	}
	if (strxneql((char *)ptb, "\161\302", 2)) {
		ret = H_CPIO_CRC;
		if (debug) print_hdrtype(ret);
		return (ret);
	}
	if (strxneql((char *)ptb, "\161\307", 2)) {
		ret = H_CPIO_BIN;
		if (debug) print_hdrtype(ret);
		return (ret);
	}
	if (debug) error("no tar archive??\n");

	if (!isrecurse) {
		int	rret;
		swabbytes((char *)ptb, TBLOCK);
		rret = get_hdrtype(ptb, TRUE);
		swabbytes((char *)ptb, TBLOCK);
		rret = H_SWAPPED(rret);
		if (debug) print_hdrtype(rret);
		return (rret);
	}

	if (debug) print_hdrtype(ret);
	return (ret);
}

EXPORT int
get_tcb(ptb)
	TCB	*ptb;
{
	Ulong	check;
	Ulong	ocheck;
	BOOL	eof;

	do {
		/*
		 * bei der Option -i wird ein genulltes File
		 * fehlerhaft als EOF Block erkannt !
		 * wenn nicht t_magic gesetzt ist.
		 */
		if (readblock((char *)ptb) == EOF) {
			errmsgno(EX_BAD, "Hard EOF on input, first EOF block is missing.\n");
			return (EOF);
		}
		/*
		 * First tar control block
		 */
		if (swapflg < 0) {
			BOOL	swapped;

			hdrtype = get_hdrtype(ptb, FALSE);
			swapped = H_ISSWAPPED(hdrtype);
			if (chdrtype != H_UNDEF &&
					swapped != H_ISSWAPPED(chdrtype)) {

				swapped = H_ISSWAPPED(chdrtype);
			}
			if (swapped) {
				swapflg = 1;
				swabbytes((char *)ptb, TBLOCK);	/* copy of TCB*/
				swabbytes(bigbuf, bigsize);	/* io buffer */
			} else {
				swapflg = 0;
			}
			/*
			 * wake up fifo (first block ist swapped)
			 */
			buf_resume();
			if (H_TYPE(hdrtype) == H_BAR) {
				comerrno(EX_BAD, "Can't handle bar archives (yet).\n");
			}
			if (H_TYPE(hdrtype) >= H_CPIO) {
/* XXX JS Test */if (H_TYPE(hdrtype) == H_CPIO_CHR) {
/* XXX JS Test */FINFO info;
/* XXX JS Test */tcb_to_info(ptb, &info);
/* XXX JS Test */}
				comerrno(EX_BAD, "Can't handle cpio archives (yet).\n");
			}
			if (H_TYPE(hdrtype) == H_UNDEF) {
				char	*p = (char *)ptb;

				if (p[0] == '\037') {
					if ((p[1] == '\037') ||	/* Packed	     */
					    (p[1] == '\213') ||	/* Gzip compressed   */
					    (p[1] == '\235') ||	/* LZW compressed    */
					    (p[1] == '\236') ||	/* Freezed 	     */
					    (p[1] == '\240'))	/* SCO LZH compressed*/
					comerrno(EX_BAD, "Archive is compressed, try to use the -z option.\n");
				}
				if (p[0] == 'B' && p[1] == 'Z' && p[2] == 'h')
					comerrno(EX_BAD, "Archive is bzip2 compressed, try to use the -bz option.\n");
				if (!ignoreerr) {
					comerrno(EX_BAD,
					"Unknown archive type (neither tar, nor bar/cpio).\n");
				}
			}
			if (chdrtype != H_UNDEF && chdrtype != hdrtype) {
				errmsgno(EX_BAD, "Found: ");
				print_hdrtype(hdrtype);
				errmsgno(EX_BAD, "Warning: extracting as ");
				print_hdrtype(chdrtype);
				hdrtype = chdrtype;
			}
			setprops(hdrtype);
		}
		if ((eof = ptb->dbuf.t_name[0] == '\0' && checkeof(ptb))) {
			if (!ignoreerr)
				return (EOF);
		}
		/*
		 * XXX Hier muß eine Universalchecksummenüberprüfung hin
		 */
		astoo(ptb->dbuf.t_chksum, &ocheck);
		check = checksum(ptb);
		/*
		 * check == 0 : genullter Block.
		 */
		if (check != 0 && ocheck == check) {
			char	*tmagic = ptb->ustar_dbuf.t_magic;

			switch (H_TYPE(hdrtype)) {

			case H_XUSTAR:
				if (ismagic(tmagic) && isxmagic(ptb))
					return (0);
			case H_XSTAR:
				if (ismagic(tmagic) &&
				    isstmagic(ptb->xstar_dbuf.t_xmagic))
					return (0);
				break;
			case H_USTAR:
				if (ismagic(tmagic))
					return (0);
				break;
			case H_GNUTAR:
				if (isgnumagic(tmagic))
					return (0);
				break;
			case H_STAR: 
				tmagic = ptb->star_dbuf.t_magic;
				if (ptb->dbuf.t_vers < STVERSION ||
				    isstmagic(tmagic))
				return (0);
				break;
			default:
			case H_TAR:
			case H_OTAR:
				return (0);
			}
			errmsgno(EX_BAD, "Wrong magic at: %d: '%.8s'.\n",
							tblocks(), tmagic);
			/*
			 * Allow buggy gnu Volheaders & Multivolheaders to work
			 */
			if (H_TYPE(hdrtype) == H_GNUTAR)
				return (0);

		} else if (eof) {
			errmsgno(EX_BAD, "EOF Block at: %d ignored.\n",
							tblocks());
		} else {
			errmsgno(EX_BAD, "Checksum error at: %d: 0%lo should be 0%lo.\n",
							tblocks(),
							ocheck, check);
		}
	} while (ignoreerr);
	prstats();
	exit(EX_BAD);
	/* NOTREACHED */
	return (EOF);		/* Keep lint happy */
}

EXPORT void
put_tcb(ptb, info)
	TCB	*ptb;
	register FINFO	*info;
{
	TCB	tb;
	int	x1 = 0;
	int	x2 = 0;

	if (info->f_flags & (F_LONGNAME|F_LONGLINK))
		x1++;

/*XXX start alter code und Test */
	if (( (info->f_flags & F_ADDSLASH) ? 1:0 +
	    info->f_namelen > props.pr_maxsname &&
	    (ptb->dbuf.t_prefix[0] == '\0' || H_TYPE(hdrtype) == H_GNUTAR)) ||
		    info->f_lnamelen > props.pr_maxslname)
		x2++;

	if (x1 != x2) {
error("type: %ld name: %s x1 %d x2 %d namelen: %ld prefix: %s lnamelen: %ld\n",
info->f_filetype, info->f_name, x1, x2,
info->f_namelen, ptb->dbuf.t_prefix, info->f_lnamelen);

	}
/*XXX ende alter code und Test */

	if (x1 || x2) {
		if ((info->f_flags & F_TCB_BUF) != 0) {	/* TCB is on buffer */
			movebytes(ptb, &tb, TBLOCK);
			ptb = &tb;
			info->f_flags &= ~F_TCB_BUF;
		}
		write_longnames(info);
	}
	write_tcb(ptb, info);
}

EXPORT void
write_tcb(ptb, info)
	TCB	*ptb;
	register FINFO	*info;
{
	if (tsize > 0) {
		TCB	tb;
		long	left;
		Ulong	size = info->f_rsize;

		left = tsize - tblocks();

		if (is_link(info))
			size = 0L;
						/* file + tcb + EOF */
		if (left < (long)(tarblocks(size)+1+2)) {
			if ((info->f_flags & F_TCB_BUF) != 0) {
				movebytes(ptb, &tb, TBLOCK);
				ptb = &tb;
				info->f_flags &= ~F_TCB_BUF;
			}
			nexttape();
		}
	}
	if (!nullout) {				/* 17 (> 16) Bit !!! */
		if (props.pr_fillc == '0')
			otoa(ptb->dbuf.t_chksum, checksum(ptb) & 0x1FFFF, 7);
		else
			otoa(ptb->dbuf.t_chksum, checksum(ptb) & 0x1FFFF, 6);
	}
	if ((info->f_flags & F_TCB_BUF) != 0)	/* TCB is on buffer */
		put_block();
	else
		writeblock((char *)ptb);
}

EXPORT void
put_volhdr(name)
	char	*name;
{
	FINFO	finfo;
	TCB	tb;
	struct timeval tv;

	if (name == 0)
		return;
	if ((props.pr_flags & PR_VOLHDR) == 0)
		return;

	gettimeofday(&tv, (struct timezone *)0);

	fillbytes((char *)&finfo, sizeof (FINFO), '\0');
	fillbytes((char *)&tb, TBLOCK, '\0');
	finfo.f_name = name;
	finfo.f_namelen = strlen(name);
	finfo.f_xftype = XT_VOLHDR;
	finfo.f_mtime = tv.tv_sec;
	finfo.f_tcb = &tb;

	if (!name_to_tcb(&finfo, &tb))	/* Name too long */
		return;

	info_to_tcb(&finfo, &tb);
	put_tcb(&tb, &finfo);
	vprint(&finfo);
}

EXPORT BOOL
get_volhdr(info, vhname)
	FINFO	*info;
	char	*vhname;
{
	error("Volhdr: %s\n", info->f_name);

	if (vhname) {
		return (streql(info->f_name, vhname));
	} else { 
		return (TRUE);
	}
}

EXPORT void
info_to_tcb(info, ptb)
	register FINFO	*info;
	register TCB	*ptb;
{
	if (nullout)
		return;

	if (props.pr_fillc == '0') {
		if (modebits)
			otoa(ptb->dbuf.t_mode, (info->f_mode|info->f_type) & 0xFFFF, 7);
		else
			otoa(ptb->dbuf.t_mode, info->f_mode & 0xFFFF, 7);
		otoa(ptb->dbuf.t_uid, info->f_uid & 0xFFFF, 7);
		otoa(ptb->dbuf.t_gid, info->f_gid & 0xFFFF, 7);
	} else {
		if (modebits)
			otoa(ptb->dbuf.t_mode, (info->f_mode|info->f_type) & 0xFFFF, 6);
		else
			otoa(ptb->dbuf.t_mode, info->f_mode & 0xFFFF, 6);
		otoa(ptb->dbuf.t_uid, info->f_uid & 0xFFFF, 6);
		otoa(ptb->dbuf.t_gid, info->f_gid & 0xFFFF, 6);
	}
	otoa(ptb->dbuf.t_size, info->f_rsize, 11);
	otoa(ptb->dbuf.t_mtime, (Ulong)info->f_mtime, 11);
	ptb->dbuf.t_linkflag = XTTOUS(info->f_xftype);

	if (H_TYPE(hdrtype) == H_USTAR) {
		info_to_ustar(info, ptb);
	} else if (H_TYPE(hdrtype) == H_XSTAR) {
		info_to_xstar(info, ptb);
	} else if (H_TYPE(hdrtype) == H_XUSTAR) {
		info_to_xstar(info, ptb);
	} else if (H_TYPE(hdrtype) == H_GNUTAR) {
		info_to_gnutar(info, ptb);
	} else if (H_TYPE(hdrtype) == H_STAR) {
		info_to_star(info, ptb);
	}
}

LOCAL void
info_to_star(info, ptb)
	register FINFO	*info;
	register TCB	*ptb;
{
	ptb->dbuf.t_vers = STVERSION;
	otoa(ptb->dbuf.t_filetype, info->f_filetype & 0xFFFF, 6);	/* XXX -> 7 ??? */
	otoa(ptb->dbuf.t_type, info->f_type & 0xFFFF, 11);
	otoa(ptb->dbuf.t_rdev, info->f_rdev, 11);
#ifdef	DEV_MINOR_NONCONTIG
	ptb->dbuf.t_devminorbits = '@';
#else
	ptb->dbuf.t_devminorbits = '@' + minorbits;
#endif

	otoa(ptb->dbuf.t_atime, (Ulong)info->f_atime, 11);
	otoa(ptb->dbuf.t_ctime, (Ulong)info->f_ctime, 11);
/*	strcpy(ptb->dbuf.t_magic, stmagic);*/
	ptb->dbuf.t_magic[0] = 't';
	ptb->dbuf.t_magic[1] = 'a';
	ptb->dbuf.t_magic[2] = 'r';
	if (!numeric) {
		nameuid(ptb->dbuf.t_uname, STUNMLEN, info->f_uid);
		namegid(ptb->dbuf.t_gname, STGNMLEN, info->f_gid);
		if (*ptb->dbuf.t_uname) {
			info->f_uname = ptb->dbuf.t_uname;
			info->f_umaxlen = STUNMLEN;
		}
		if (*ptb->dbuf.t_gname) {
			info->f_gname = ptb->dbuf.t_gname;
			info->f_gmaxlen = STGNMLEN;
		}
	}

	if (is_sparse(info))
		otoa(ptb->xstar_in_dbuf.t_realsize, info->f_size, 11);
}

LOCAL void
info_to_ustar(info, ptb)
	register FINFO	*info;
	register TCB	*ptb;
{
/*XXX solaris hat illegalerweise mehr als 12 Bit in t_mode !!!
 *	otoa(ptb->dbuf.t_mode, info->f_mode|info->f_type & 0xFFFF, 6);	XXX -> 7 ???
*/
/*	strcpy(ptb->ustar_dbuf.t_magic, magic);*/
	ptb->ustar_dbuf.t_magic[0] = 'u';
	ptb->ustar_dbuf.t_magic[1] = 's';
	ptb->ustar_dbuf.t_magic[2] = 't';
	ptb->ustar_dbuf.t_magic[3] = 'a';
	ptb->ustar_dbuf.t_magic[4] = 'r';
/*	strncpy(ptb->ustar_dbuf.t_version, TVERSION, TVERSLEN);*/
	/*
	 * strncpy is slow: use handcrafted replacement.
	 */
	ptb->ustar_dbuf.t_version[0] = '0';
	ptb->ustar_dbuf.t_version[1] = '0';

	if (!numeric) {
		nameuid(ptb->ustar_dbuf.t_uname, TUNMLEN, info->f_uid);
		namegid(ptb->ustar_dbuf.t_gname, TGNMLEN, info->f_gid);
		if (*ptb->ustar_dbuf.t_uname) {
			info->f_uname = ptb->ustar_dbuf.t_uname;
			info->f_umaxlen = TUNMLEN;
		}
		if (*ptb->ustar_dbuf.t_gname) {
			info->f_gname = ptb->ustar_dbuf.t_gname;
			info->f_gmaxlen = TGNMLEN;
		}
	}
	otoa(ptb->ustar_dbuf.t_devmajor, info->f_rdevmaj, 7);
#if	DEV_MINOR_BITS > 21
	if (info->f_rdevmin > 07777777) {
		extern	BOOL	hpdev;

		if (!is_special(info)) {
			/*
			 * Until we know how to deal with this, we reduce
			 * the number of files that get non POSIX headers.
			 */
			info->f_rdevmin = 0;
			goto doposix;
		}
		if ((info->f_rdevmin <= 077777777) && hpdev) {
			char	c;

			/*
			 * Implement the method from HP-UX that allows 24 bit
			 * for the device minor number. Note that this method
			 * violates the POSIX specs.
			 */
			c = ptb->ustar_dbuf.t_prefix[0];
			otoa(ptb->ustar_dbuf.t_devminor, info->f_rdevmin, 8);
			ptb->ustar_dbuf.t_prefix[0] = c;
		} else {
			btoa(ptb->ustar_dbuf.t_devminor, info->f_rdevmin, 7);
		}
	} else
#endif
		{
#if	DEV_MINOR_BITS > 21
doposix:
#endif
		otoa(ptb->ustar_dbuf.t_devminor, info->f_rdevmin, 7);
	}
}

LOCAL void
info_to_xstar(info, ptb)
	register FINFO	*info;
	register TCB	*ptb;
{
	info_to_ustar(info, ptb);
	otoa(ptb->xstar_dbuf.t_atime, (Ulong)info->f_atime, 11);
	otoa(ptb->xstar_dbuf.t_ctime, (Ulong)info->f_ctime, 11);

	/*
	 * Help recognition in isxmagic(), make sure that prefix[130] is null.
	 */
	ptb->xstar_dbuf.t_prefix[130] = '\0';

	if (H_TYPE(hdrtype) == H_XSTAR) {
/*		strcpy(ptb->xstar_dbuf.t_xmagic, stmagic);*/
		ptb->xstar_dbuf.t_xmagic[0] = 't';
		ptb->xstar_dbuf.t_xmagic[1] = 'a';
		ptb->xstar_dbuf.t_xmagic[2] = 'r';
	}
	if (is_sparse(info))
		otoa(ptb->xstar_in_dbuf.t_realsize, info->f_size, 11);
}

LOCAL void
info_to_gnutar(info, ptb)
	register FINFO	*info;
	register TCB	*ptb;
{
	strcpy(ptb->gnu_dbuf.t_magic, gmagic);

	if (!numeric) {
		nameuid(ptb->ustar_dbuf.t_uname, TUNMLEN, info->f_uid);
		namegid(ptb->ustar_dbuf.t_gname, TGNMLEN, info->f_gid);
		if (*ptb->ustar_dbuf.t_uname) {
			info->f_uname = ptb->ustar_dbuf.t_uname;
			info->f_umaxlen = TUNMLEN;
		}
		if (*ptb->ustar_dbuf.t_gname) {
			info->f_gname = ptb->ustar_dbuf.t_gname;
			info->f_gmaxlen = TGNMLEN;
		}
	}
	if (info->f_xftype == XT_CHR || info->f_xftype == XT_BLK) {
		otoa(ptb->ustar_dbuf.t_devmajor, info->f_rdevmaj, 6);	/* XXX -> 7 ??? */
		otoa(ptb->ustar_dbuf.t_devminor, info->f_rdevmin, 6);	/* XXX -> 7 ??? */
	}

	/*
	 * XXX GNU tar only fill this if doing a gnudump.
	 */
	otoa(ptb->gnu_dbuf.t_atime, (Ulong)info->f_atime, 11);
	otoa(ptb->gnu_dbuf.t_ctime, (Ulong)info->f_ctime, 11);

	if (is_sparse(info))
		otoa(ptb->gnu_in_dbuf.t_realsize, info->f_size, 11);
}

EXPORT int
tcb_to_info(ptb, info)
	register TCB	*ptb;
	register FINFO	*info;
{
	int	ret = 0;
	char	xname;
	char	xlink;
	Ulong	ul;
static	BOOL	posixwarn = FALSE;
static	BOOL	namewarn = FALSE;

	/*
	 * F_HAS_NAME is only used from list.c when the -listnew option is
	 * present. Keep f_lname and f_name, don't read LF_LONGLINK/LF_LONGNAME
	 * in this case.
	 */
	if ((info->f_flags & F_HAS_NAME) == 0)
		info->f_lname = ptb->dbuf.t_linkname;
	info->f_uname = info->f_gname = NULL;
	info->f_umaxlen = info->f_gmaxlen = 0L;
	info->f_xftype = 0;
	info->f_offset = 0;
	info->f_flags &= F_HAS_NAME;

/* XXX JS Test */if (H_TYPE(hdrtype) >= H_CPIO) {
/* XXX JS Test */cpiotcb_to_info(ptb, info);
/* XXX JS Test */list_file(info);
/* XXX JS Test */return (ret);
/* XXX JS Test */}

	/*
	 * Handle very long names
	 */
	if ((info->f_flags & F_HAS_NAME) == 0 &&
					props.pr_nflags & PR_LONG_NAMES) {
		while (ptb->dbuf.t_linkflag == LF_LONGLINK ||
				    ptb->dbuf.t_linkflag == LF_LONGNAME)
			ret = tcb_to_longname(ptb, info);
	}

	if (ptb->dbuf.t_name[NAMSIZ] == '\0') {
		if (!nowarn && !namewarn) {
			errmsgno(EX_BAD,
			"Warning: Archive violates POSIX 1003.1 (100 char filename is null terminated).\n");
			namewarn = TRUE;
		}
		ptb->dbuf.t_name[NAMSIZ] = ' ';
	}
	astoo(ptb->dbuf.t_mode, &info->f_mode);
	if (info->f_mode & ~07777) {
		if (!nowarn && !modebits && H_TYPE(hdrtype) == H_USTAR && !posixwarn) {
			errmsgno(EX_BAD,
			"Warning: Archive violates POSIX 1003.1 (too many bits in mode field).\n");
			posixwarn = TRUE;
		}
		info->f_mode &= 07777;
	}
	astoo(ptb->dbuf.t_uid, &info->f_uid);
	astoo(ptb->dbuf.t_gid, &info->f_gid);
	astoo(ptb->dbuf.t_size, &info->f_size);
	info->f_rsize = 0L;
/*XXX	if (ptb->dbuf.t_linkflag < LNKTYPE)*/	/* Alte star Version!!! */
	if (ptb->dbuf.t_linkflag != LNKTYPE &&
					ptb->dbuf.t_linkflag != DIRTYPE) {
		/* XXX
		 * XXX Ist das die richtige Stelle um f_rsize zu setzen ??
		 */
		info->f_rsize = info->f_size;
	}
	astoo(ptb->dbuf.t_mtime, &ul);
	info->f_mtime = (time_t)ul;

	info->f_ansec = info->f_mnsec = info->f_cnsec = 0L;


	switch (H_TYPE(hdrtype)) {

	default:
	case H_TAR:
	case H_OTAR:
		tar_to_info(ptb, info);
		break;
	case H_USTAR:
		ustar_to_info(ptb, info);
		break;
	case H_XSTAR:
	case H_XUSTAR:
		xstar_to_info(ptb, info);
		break;
	case H_GNUTAR:
		gnutar_to_info(ptb, info);
		break;
	case H_STAR:
		star_to_info(ptb, info);
		break;
	}

	/*
	 * Hack for list module (option -newest) ...
	 * Save and restore t_name[NAMSIZ] & t_linkname[NAMSIZ]
	 */
	xname = ptb->dbuf.t_name[NAMSIZ];
	ptb->dbuf.t_name[NAMSIZ] = '\0';	/* allow 100 chars in name */
	xlink = ptb->dbuf.t_linkname[NAMSIZ];
	ptb->dbuf.t_linkname[NAMSIZ] = '\0';/* allow 100 chars in linkname */

	/*
	 * Handle long name in posix split form now.
	 */
	tcb_to_name(ptb, info);

	ptb->dbuf.t_name[NAMSIZ] = xname;	/* restore remembered value */
	ptb->dbuf.t_linkname[NAMSIZ] = xlink;	/* restore remembered value */

	return (ret);
}

LOCAL void
tar_to_info(ptb, info)
	register TCB	*ptb;
	register FINFO	*info;
{
	register int	typeflag = ptb->ustar_dbuf.t_typeflag;

	if (ptb->dbuf.t_name[strlen(ptb->dbuf.t_name) - 1] == '/') {
		typeflag = DIRTYPE;
		info->f_filetype = F_DIR;
		info->f_rsize = 0L;	/* XXX hier?? siehe oben */
	} else if (typeflag == SYMTYPE) {
		info->f_filetype = F_SLINK;
	} else if (typeflag != DIRTYPE) {
		info->f_filetype = F_FILE;
	}
	info->f_xftype = USTOXT(typeflag);
	info->f_type = XTTOIF(info->f_xftype);
	info->f_rdevmaj = info->f_rdevmin = info->f_rdev = 0;
	info->f_ctime = info->f_atime = info->f_mtime;
}

LOCAL void
star_to_info(ptb, info)
	register TCB	*ptb;
	register FINFO	*info;
{
	Ulong	id;
	int	mbits;

	version = ptb->dbuf.t_vers;
	if (ptb->dbuf.t_vers < STVERSION) {
		tar_to_info(ptb, info);
		return;
	}
	astoo(ptb->dbuf.t_filetype, &info->f_filetype);
	astoo(ptb->dbuf.t_type, &info->f_type);
	/*
	 * star Erweiterungen sind wieder ANSI kompatibel, d.h. linkflag
	 * hält den echten Dateityp (LONKLINK, LONGNAME, SPARSE ...)
	 */
	if(ptb->dbuf.t_linkflag < '1')
		info->f_xftype = IFTOXT(info->f_type);
	else
		info->f_xftype = USTOXT(ptb->ustar_dbuf.t_typeflag);

	astoo(ptb->dbuf.t_rdev, &info->f_rdev);
	mbits = ptb->dbuf.t_devminorbits - '@';
	if (mbits == 0) {
		static	BOOL	dwarned = FALSE;
		if (!dwarned) {
			errmsgno(EX_BAD,
#ifdef	DEV_MINOR_NONCONTIG
			"Warning: Minor device numbers are non contiguous, devices may not be extracted correctly.\n");
#else
			"Warning: The archiving system used non contiguous minor numbers, cannot extract devices correctly.\n");
#endif
			dwarned = TRUE;
		}
		/*
		 * Let us hope that both, the archiving and the extracting system
		 * use the same major()/minor() mapping.
		 */
		info->f_rdevmaj	= major(info->f_rdev);
		info->f_rdevmin	= minor(info->f_rdev);
	} else {
		/*
		 * Convert from remote major()/minor() mapping to
		 * local major()/minor() mapping.
		 */
		if (mbits < 0)		/* Old star format */
			mbits = 8;
		info->f_rdevmaj	= _dev_major(mbits, info->f_rdev);
		info->f_rdevmin	= _dev_minor(mbits, info->f_rdev);
		info->f_rdev = makedev(info->f_rdevmaj, info->f_rdevmin);
	}

	astoo(ptb->dbuf.t_atime, &id);
	info->f_atime = (time_t)id;
	astoo(ptb->dbuf.t_ctime, &id);
	info->f_ctime = (time_t)id;

	if (!numeric && uidname(ptb->dbuf.t_uname, STUNMLEN, &id))
		info->f_uid = id;
	if (!numeric && gidname(ptb->dbuf.t_gname, STGNMLEN, &id))
		info->f_gid = id;
	if (*ptb->dbuf.t_uname) {
		info->f_uname = ptb->dbuf.t_uname;
		info->f_umaxlen = STUNMLEN;
	}
	if (*ptb->dbuf.t_gname) {
		info->f_gname = ptb->dbuf.t_gname;
		info->f_gmaxlen = STGNMLEN;
	}

	if (is_sparse(info))
		astoo(ptb->xstar_in_dbuf.t_realsize, &info->f_size);
	if (is_multivol(info))
		astoo(ptb->xstar_in_dbuf.t_offset, &info->f_offset);
}

LOCAL void
ustar_to_info(ptb, info)
	register TCB	*ptb;
	register FINFO	*info;
{
	Ulong	id;
	char	c;

	info->f_xftype = USTOXT(ptb->ustar_dbuf.t_typeflag);
	info->f_filetype = XTTOST(info->f_xftype);
	info->f_type = XTTOIF(info->f_xftype);

	if (!numeric && uidname(ptb->ustar_dbuf.t_uname, TUNMLEN, &id))
		info->f_uid = id;
	if (!numeric && gidname(ptb->ustar_dbuf.t_gname, TGNMLEN, &id))
		info->f_gid = id;
	if (*ptb->ustar_dbuf.t_uname) {
		info->f_uname = ptb->ustar_dbuf.t_uname;
		info->f_umaxlen = TUNMLEN;
	}
	if (*ptb->ustar_dbuf.t_gname) {
		info->f_gname = ptb->ustar_dbuf.t_gname;
		info->f_gmaxlen = TGNMLEN;
	}

	astoo(ptb->ustar_dbuf.t_devmajor, &info->f_rdevmaj);
	if (ptb->ustar_dbuf.t_devminor[0] & 0x80) {
		astob(ptb->ustar_dbuf.t_devminor, &info->f_rdevmin, 7);
	} else {
		/*
		 * The 'tar' that comes with HP-UX writes illegal tar archives.
		 * It includes 8 characters in the minor field and allows
		 * to archive 24 bits for the minor device which are used by HP-UX.
		 * As we like to be able to read these archives, we need to convert
		 * the number carefully by temporarily writing a NULL to the next
		 * character and restoring the right content afterwards.
		 */
		c = ptb->ustar_dbuf.t_prefix[0];
		ptb->ustar_dbuf.t_prefix[0] = '\0';
		astoo(ptb->ustar_dbuf.t_devminor, &info->f_rdevmin);
		ptb->ustar_dbuf.t_prefix[0] = c;
	}

	info->f_rdev = makedev(info->f_rdevmaj, info->f_rdevmin);

	/*
	 * ANSI Tar hat keine atime & ctime im Header!
	 */
	info->f_ctime = info->f_atime = info->f_mtime;
}

LOCAL void
xstar_to_info(ptb, info)
	register TCB	*ptb;
	register FINFO	*info;
{
	Ulong	ul;

	ustar_to_info(ptb, info);

	astoo(ptb->xstar_dbuf.t_atime, &ul);
	info->f_atime = (time_t)ul;
	astoo(ptb->xstar_dbuf.t_ctime, &ul);
	info->f_ctime = (time_t)ul;

	if (is_sparse(info))
		astoo(ptb->xstar_in_dbuf.t_realsize, &info->f_size);
	if (is_multivol(info))
		astoo(ptb->xstar_in_dbuf.t_offset, &info->f_offset);
}

LOCAL void
gnutar_to_info(ptb, info)
	register TCB	*ptb;
	register FINFO	*info;
{
	Ulong	ul;

	ustar_to_info(ptb, info);

	astoo(ptb->gnu_dbuf.t_atime, &ul);
	info->f_atime = (time_t)ul;

	if (info->f_atime == 0 && ptb->gnu_dbuf.t_atime[0] == '\0')
		info->f_atime = info->f_mtime;

	astoo(ptb->gnu_dbuf.t_ctime, &ul);
	info->f_ctime = (time_t)ul;

	if (info->f_ctime == 0 && ptb->gnu_dbuf.t_ctime[0] == '\0')
		info->f_ctime = info->f_mtime;

	if (is_sparse(info))
		astoo(ptb->gnu_in_dbuf.t_realsize, &info->f_size);
	if (is_multivol(info))
		astoo(ptb->gnu_dbuf.t_offset, &info->f_offset);
}

/*
 * XXX vorerst nur zum Test!
 */
LOCAL void
cpiotcb_to_info(ptb, info)
	register TCB	*ptb;
	register FINFO	*info;
{
	Ulong	ul;

	astoo_cpio(&((char *)ptb)[6], &info->f_dev, 6);
	astoo_cpio(&((char *)ptb)[12], &info->f_ino, 6);
error("ino: %ld\n", info->f_ino);
	astoo_cpio(&((char *)ptb)[18], &info->f_mode, 6);
error("mode: %lo\n", info->f_mode);
	info->f_type = info->f_mode & S_IFMT;
	info->f_mode = info->f_mode & 07777;
	info->f_xftype = IFTOXT(info->f_type);
	info->f_filetype = XTTOST(info->f_xftype);
	astoo_cpio(&((char *)ptb)[24], &info->f_uid, 6);
	astoo_cpio(&((char *)ptb)[30], &info->f_gid, 6);
	astoo_cpio(&((char *)ptb)[36], &info->f_nlink, 6);
	astoo_cpio(&((char *)ptb)[42], &info->f_rdev, 6);

	astoo_cpio(&((char *)ptb)[48], &ul, 11);
	info->f_atime = (time_t)ul;

	astoo_cpio(&((char *)ptb)[59], &info->f_namelen, 6);

	astoo_cpio(&((char *)ptb)[65], &info->f_size, 11);
info->f_rsize = info->f_size;
	info->f_name = &((char *)ptb)[76];
}

LOCAL int
ustoxt(ustype)
	char	ustype;
{
	/*
	 * Map ANSI types
	 */
	if (ustype >= REGTYPE && ustype <= CONTTYPE)
		return _USTOXT(ustype);

	/*
	 * Map Gnu tar & Star types ANSI: "local enhancements"
	 */
	if ((props.pr_flags & (PR_LOCAL_STAR|PR_LOCAL_GNU)) &&
					ustype >= 'A' && ustype <= 'Z')
		return _GTTOXT(ustype);

	/*
	 * treat unknown types as regular files conforming to standard
	 */
	return (XT_FILE);
}

EXPORT BOOL
ia_change(ptb, info)
	TCB	*ptb;
	FINFO	*info;
{
	char	buf[NAMSIZ+1];	/* XXX nur 100 chars ?? */
	char	ans;
	int	len;

	if (verbose)
		list_file(info);
	else
		vprint(info);
	if (nflag)
		return (FALSE);
	printf("get/put ? Y(es)/N(o)/C(hange name) :");flush();
	fgetline(tty, buf, 2);
	if ((ans = toupper(buf[0])) == 'Y')
		return (TRUE);
	else if (ans == 'C') {
		for(;;) {
			printf("Enter new name:");
			flush();
			if ((len = fgetline(tty, buf, sizeof buf)) == 0)
				continue;
			if (len > sizeof(buf) - 1)
				errmsgno(EX_BAD, "Name too long.\n");
			else
				break;
		}
		strcpy(info->f_name, buf);	/* XXX nur 100 chars ?? */
		if (xflag && newer(info))
			return (FALSE);
		return (TRUE);
	}
	return (FALSE);
}

LOCAL BOOL
checkeof(ptb)
	TCB	*ptb;
{
	if (!eofblock(ptb))
		return (FALSE);
	if (debug)
		errmsgno(EX_BAD, "First  EOF Block OK\n");
	markeof();

	if (readblock((char *)ptb) == EOF) {
		errmsgno(EX_BAD, "Incorrect EOF, second EOF block is missing.\n");
		return (TRUE);
	}
	if (!eofblock(ptb)) {
		if (!nowarn)
			errmsgno(EX_BAD, "WARNING: Partial (single block) EOF detected.\n");
		return (FALSE);
	}
	if (debug)
		errmsgno(EX_BAD, "Second EOF Block OK\n");
	return (TRUE);
}

LOCAL BOOL
eofblock(ptb)
	TCB	*ptb;
{
	register short	i;
	register char	*s = (char *) ptb;

	if (props.pr_nflags & PR_DUMB_EOF)
		return (ptb->dbuf.t_name[0] == '\0');

	for (i=0; i < TBLOCK; i++)
		if (*s++ != '\0')
			return (FALSE);
	return (TRUE);
}

EXPORT void /*char **/
astoo_cpio(s,l, cnt)
	register char	*s;
		 Ulong	*l;
	register int	cnt;
{
	register Ulong	ret = 0L;
	register char	c;
	register int	t;
	
	for(;cnt > 0; cnt--) {
		c = *s++;
		if(isoctal(c))
			t = c - '0';
		else
			break;
		ret *= 8;
		ret += t;
	}
	*l = ret;
	/*return(s);*/
}

EXPORT void /*char **/
astoo(s,l)
	register char	*s;
		 Ulong	*l;
{
	register Ulong	ret = 0L;
	register char	c;
	register int	t;
	
	while(*s == ' ')
		s++;

	for(;;) {
		c = *s++;
		if(isoctal(c))
			t = c - '0';
		else
			break;
		ret *= 8;
		ret += t;
	}
	*l = ret;
	/*return(s);*/
}

EXPORT void
otoa(s, l, fieldw)
		 char	*s;
	register Ulong	l;
	register int	fieldw;
{
	register char	*p	= &s[fieldw+1];
	register char	fill	= props.pr_fillc;

	/*
	 * Bei 12 Byte Feldern würde hier das Nächste Feld überschrieben, wenn
	 * entgegen der normalen Reihenfolge geschrieben wird!
	 * Da der TCB sowieso vorher genullt wird ist es aber kein Problem
	 * das bei 8 Bytes Feldern notwendige Nullbyte wegzulassen.
	 */
/*XXX	*p = '\0';*/
	/*
	 * Das Zeichen nach einer Zahl.
	 * XXX Soll hier besser ein NULL Byte bei POSIX Tar hin?
	 * XXX Wuerde das Probleme mit einer automatischen Erkennung geben?
	 */
	*--p = ' ';
/*???	*--p = '\0';*/

	do {
		*--p = (l%8) + '0';	/* Compiler optimiert */
		fieldw--;
	} while ((l /= 8) > 0);

	switch (fieldw) {
	case 11:	*--p = fill;
	case 10:	*--p = fill;
	case 9:		*--p = fill;
	case 8:		*--p = fill;
	case 7:		*--p = fill;
	case 6:		*--p = fill;
	case 5:		*--p = fill;
	case 4:		*--p = fill;
	case 3:		*--p = fill;
	case 2:		*--p = fill;
	case 1:		*--p = fill;
	}
}

EXPORT void /*char **/
astob(s, l, fieldw)
	register char	*s;
		 Ulong	*l;
	register int	fieldw;
{
	register Ulong	ret = 0L;
	register Uchar	c;
	
	c = *s++ & 0x7F;
	ret = c * 256;

	while (--fieldw >= 0) {
		c = *s++;
		ret *= 256;
		ret += c;
	}
	*l = ret;
	/*return(s);*/
}

EXPORT void
btoa(s, l, fieldw)
		 char	*s;
	register Ulong	l;
	register int	fieldw;
{
	register char	*p	= &s[fieldw+1];

	do {
		*--p = l%256;	/* Compiler optimiert */
		fieldw--;
	} while ((l /= 256) > 0);

	s[0] |= 0x80;
}
