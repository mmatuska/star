/* @(#)star.h	1.21 97/04/27 Copyright 1985, 1995 J. Schilling */
/*
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

#define tarblocks(s)	(((s) + (TBLOCK-1)) / TBLOCK)
#define tarsize(s)	(tarblocks(s) * TBLOCK)

/*
 * Defines for header type recognition
 * N.B. these must kept in sync with hdrtxt[] in header.c
 */
#define	H_SWAPPED(t)	((-1)*(t))
#define	H_ISSWAPPED(t)	((t) < H_UNDEF)
#define	H_TYPE(t)	((int)(H_ISSWAPPED(t) ? ((-1)*(t)):(t)))
#define	H_UNDEF		0
#define	H_TAR		1	/* tar unbekanntes format */
#define	H_OTAR		2	/* tar altes format */
#define	H_STAR		3	/* star format */
#define	H_GNUTAR	4	/* gnu tar format */
#define	H_USTAR		5	/* ieee 1003.1 format */
#define	H_XSTAR		6	/* extended 1003.1 format */
#define	H_BAR		7	/* SUN bar format */
#define	H_CPIO		8	/* XXX entfällt */
#define	H_CPIO_BIN	8	/* cpio Binär */
#define	H_CPIO_CHR	9	/* cpio -c format */
#define	H_CPIO_NBIN	10	/* cpio neu Binär */
#define	H_CPIO_CRC	11	/* cpio crc Binär */
#define	H_CPIO_ASC	12	/* cpio ascii expanded maj/min */
#define	H_CPIO_ACRC	13	/* cpio crc expanded maj/min */
#define	H_MAX_ARCH	13	/* Highest possible # */


#define TBLOCK		512
#define NAMSIZ		100
#define	PFXSIZ		155

#define	TMODLEN		8
#define	TUIDLEN		8
#define	TGIDLEN		8
#define	TSIZLEN		12
#define	TMTMLEN		12
#define	TCKSLEN		8

#define	TMAGIC		"ustar"	/* ustar magic */
#define	TMAGLEN		6	/* "ustar" including NULL byte */
#define	TVERSION	"00"
#define	TVERSLEN	2
#define	TUNMLEN		32
#define	TGNMLEN		32
#define	TDEVLEN		8

#define	REGTYPE		'0'
#define	AREGTYPE	'\0'
#define	LNKTYPE		'1'
#define	SYMTYPE		'2'
#define	CHRTYPE		'3'
#define	BLKTYPE		'4'
#define	DIRTYPE		'5'
#define	FIFOTYPE	'6'
#define	CONTTYPE	'7'

/*
 * star/gnu tar extensions:
 */
/* Note that the standards committee allows only capital A through
   capital Z for user-defined expansion.  This means that defining something
   as, say '8' is a *bad* idea. */
#define LF_DUMPDIR	'D'	/* This is a dir entry that contains
					   the names of files that were in
					   the dir at the time the dump
					   was made */
#define LF_LONGLINK	'K'	/* Identifies the NEXT file on the tape
					   as having a long linkname */
#define LF_LONGNAME	'L'	/* Identifies the NEXT file on the tape
					   as having a long name. */
#define LF_MULTIVOL	'M'	/* This is the continuation
					   of a file that began on another
					   volume */
#define LF_NAMES	'N'	/* For storing filenames that didn't
					   fit in 100 characters */
#define LF_SPARSE	'S'	/* This is for sparse files */
#define LF_VOLHDR	'V'	/* This file is a tape/volume header */
				/* Ignore it on extraction */

/*
 * This is the ustar (Posix 1003.1) header.
 */
struct header {
	char t_name[NAMSIZ];	/*   0	Dateiname	*/
	char t_mode[8];		/* 100	Zugriffsrechte 	*/
	char t_uid[8];		/* 108	Benutzernummer	*/
	char t_gid[8];		/* 116	Benutzergruppe	*/
	char t_size[12];	/* 124	Dateigroesze	*/
	char t_mtime[12];	/* 136	Zeit d. letzten Aenderung */
	char t_chksum[8];	/* 148	Checksumme	*/
	char t_typeflag;	/* 156	Typ der Datei	*/
	char t_linkname[NAMSIZ];/* 157	Zielname des Links */
	char t_magic[TMAGLEN];	/* 257	"ustar"		*/
	char t_version[TVERSLEN];/*263	Version v. star	*/
	char t_uname[TUNMLEN];	/* 265	Benutzername	*/
	char t_gname[TGNMLEN];	/* 297	Gruppenname	*/
	char t_devmajor[8];	/* 329	Major bei Geraeten */
	char t_devminor[8];	/* 337	Minor bei Geraeten */
	char t_prefix[PFXSIZ];	/* 345	Prefix fuer t_name */
				/* 500	Ende		*/
	char t_mfill[12];	/* 500	Filler bis 512	*/
};

/*
 * star header specific definitions
 */
#define	STMAGIC		"tar"	/* star magic */
#define	STMAGLEN	4	/* "tar" including NULL byte */
#define	STVERSION	'1'	/* current star version */

#define	STUNMLEN	16	/* star user name length */
#define	STGNMLEN	15	/* star group name length */

/*
 * This is the old (pre Posix 1003.1) star header defined in 1985.
 */
struct star_header {
	char t_name[NAMSIZ];	/*   0	Dateiname	*/
	char t_mode[8];		/* 100	Zugriffsrechte 	*/
	char t_uid[8];		/* 108	Benutzernummer	*/
	char t_gid[8];		/* 116	Benutzergruppe	*/
	char t_size[12];	/* 124	Dateigroesze	*/
	char t_mtime[12];	/* 136	Zeit d. letzten Aenderung */
	char t_chksum[8];	/* 148	Checksumme	*/
	char t_linkflag;	/* 156	Linktyp der Datei */
	char t_linkname[NAMSIZ];/* 157	Zielname des Links */
	char t_vers;		/* 257	Version v. star	*/
	char t_filetype[8];	/* 258	Interner Dateityp */
	char t_type[12];	/* 266	Dateityp (UNIX)	*/
#ifdef	no_minor_bits_in_rdev
	char t_rdev[12];	/* 278	Major/Minor bei Geraeten */
#else
	char t_rdev[11];	/* 278	Major/Minor bei Geraeten */
	char t_devminorbits;	/* 298	Anzahl d. Minor Bits in t_rdev */
#endif
	char t_atime[12];	/* 290	Zeit d. letzten Zugriffs */
	char t_ctime[12];	/* 302	Zeit d. letzten Statusaend. */
	char t_uname[STUNMLEN];	/* 314	Benutzername	*/
	char t_gname[STGNMLEN];	/* 330	Gruppenname	*/
	char t_prefix[PFXSIZ];	/* 345	Prefix fuer t_name */
	char t_mfill[8];	/* 500	Filler bis magic */
	char t_magic[4];	/* 508	"tar"		*/
};

/*
 * This is the new (post Posix 1003.1) xstar header.
 */
struct xstar_header {
	char t_name[NAMSIZ];	/*   0	Dateiname	*/
	char t_mode[8];		/* 100	Zugriffsrechte 	*/
	char t_uid[8];		/* 108	Benutzernummer	*/
	char t_gid[8];		/* 116	Benutzergruppe	*/
	char t_size[12];	/* 124	Dateigroesze	*/
	char t_mtime[12];	/* 136	Zeit d. letzten Aenderung */
	char t_chksum[8];	/* 148	Checksumme	*/
	char t_typeflag;	/* 156	Typ der Datei	*/
	char t_linkname[NAMSIZ];/* 157	Zielname des Links */
	char t_magic[TMAGLEN];	/* 257	"ustar"		*/
	char t_version[TVERSLEN];/*263	Version v. star	*/
	char t_uname[TUNMLEN];	/* 265	Benutzername	*/
	char t_gname[TGNMLEN];	/* 297	Gruppenname	*/
	char t_devmajor[8];	/* 329	Major bei Geraeten */
	char t_devminor[8];	/* 337	Minor bei Geraeten */
	char t_prefix[131];	/* 345	Prefix fuer t_name */
	char t_atime[12];	/* 476	Zeit d. letzten Zugriffs */
	char t_ctime[12];	/* 488	Zeit d. letzten Statusaend. */
	char t_mfill[8];	/* 500	Filler bis magic */
	char t_xmagic[4];	/* 508	"tar"		*/
};

struct sparse {
	char t_offset[12];
	char t_numbytes[12];
};

#define SPARSE_EXT_HDR  21
#define SPARSE_IN_HDR	4
#define	SIH		SPARSE_IN_HDR
#define	SEH		SPARSE_EXT_HDR

struct xstar_in_header {
	char t_fill[345];
	char t_fill2[10];
	char t_isextended;	/* 355	*/
	struct sparse t_sp[SIH];/* 356	*/
	char t_realsize[12];	/* 452	Echte Größe bei sparse Dateien */
	char t_offset[12];	/* 464	Offset für Multivol cont. Dateien */
	char t_atime[12];	/* 476	Zeit d. letzten Zugriffs */
	char t_ctime[12];	/* 488	Zeit d. letzten Statusaend. */
	char t_mfill[8];	/* 500	Filler bis magic */
	char t_xmagic[4];	/* 508	"tar"		*/
};

struct star_ext_header {
	struct sparse t_sp[SEH];
	char t_isextended;
};

/*
 * gnu tar header specific definitions
 */

#define	GMAGIC		"ustar  "/* gnu tar magic */
#define	GMAGLEN		8	/* "ustar" two blanks and a NULL */

typedef struct {
	unsigned long	sp_offset;
	unsigned long	sp_numbytes;
} sp_t;

struct gnu_in_header {
	char	t_fill[386];
	struct sparse t_sp[SIH];/* 386	*/
	char t_isextended;	/* 482	*/
	char t_realsize[12];	/* true size of the sparse file *//* 483 */
};

struct gnu_extended_header {
	struct sparse t_sp[SEH];
	char t_isextended;
};

struct gnu_header {
	char t_name[NAMSIZ];	/*   0	Dateiname	*/
	char t_mode[8];		/* 100	Zugriffsrechte	*/
	char t_uid[8];		/* 108	Benutzernummer	*/
	char t_gid[8];		/* 116	Benutzergruppe	*/
	char t_size[12];	/* 124	Dateigroesze	*/
	char t_mtime[12];	/* 136	Zeit d. letzten Aenderung */
	char t_chksum[8];	/* 148	Checksumme	*/
	char t_linkflag;	/* 156	Typ der Datei	*/
	char t_linkname[NAMSIZ];/* 157	Zielname des Links */
	char t_magic[8];	/* 257	"ustar"		*/
	char t_uname[TUNMLEN];	/* 265	Benutzername	*/
	char t_gname[TGNMLEN];	/* 297	Gruppenname	*/
	char t_devmajor[8];	/* 329	Major bei Geraeten */
	char t_devminor[8];	/* 337	Minor bei Geraeten */
	/* these following fields were added by JF for gnu */
	/* and are NOT standard */
	char t_atime[12];	/* 345	*/
	char t_ctime[12];	/* 357	*/
	char t_offset[12];	/* 369	*/
	char t_longnames[4];	/* 381	*/
#ifdef	xxx
#ifdef NEEDPAD
	char pad;		/* 385	*/
#endif
	struct sparse t_sp[SPARSE_IN_HDR];/* 386	*/
	char t_isextended;	/* 482	*/
	char t_realsize[12];	/* true size of the sparse file *//* 483 */
	/* char	ending_blanks[12];*//* number of nulls at the	*//* 495 */
	/* end of the file, if any */
#endif /* xxx */
};
#undef	SIH
#undef	SEH

#define	BAR_UNSPEC	'\0'	/* XXX Volheader ??? */
#define	BAR_REGTYPE	'0'
#define	BAR_LNKTYPE	'1'
#define	BAR_SYMTYPE	'2'
#define	BAR_SPECIAL	'3'

#define	BAR_VOLHEAD	"V"

struct bar_header {
	char mode[8];		/*   0	file type and mode (top bit masked) */
	char uid[8];		/*   8	Benutzernummer	*/
	char gid[8];		/*  16	Benutzergruppe	*/
	char size[12];		/*  24	Dateigroesze	*/
	char mtime[12];		/*  36	Zeit d. letzten Aenderung */
	char t_chksum[8];	/*  48	Checksumme	*/
	char rdev[8];		/*  56	Major/Minor bei Geraeten */
	char linkflag;		/*  64	Linktyp der Datei */
	char bar_magic[2];	/*  65	xxx		*/
	char volume_num[4];	/*  67	Volume Nummer	*/
	char compressed;	/*  71	Compress Flag	*/
	char date[12];		/*  72	Aktuelles Datum	*/
	char start_of_name[1];	/*  84	Dateiname	*/
};

typedef union hblock {
	char dummy[TBLOCK];
	struct star_header dbuf;
	struct star_header star_dbuf;
	struct xstar_header xstar_dbuf;
	struct xstar_in_header xstar_in_dbuf;
	struct header ustar_dbuf;
	struct gnu_header gnu_dbuf;
	struct gnu_in_header gnu_in_dbuf;
	struct gnu_extended_header gnu_ext_dbuf;
	struct bar_header bar_dbuf;
}TCB ;

#ifndef	NO_LONGLONG
#	if	defined(__GNUC__)
#		define	LONGLONG
#	endif
#	if	defined(sun) && defined(SVR4)
#		define	LONGLONG
#	endif
#endif

#ifdef	LONGLONG
typedef	long long	Llong;
#else
typedef	unsigned long	Llong;	/* give maximum precision */
#endif
typedef	unsigned long	Ulong;
typedef	unsigned int	Uint;
typedef	unsigned short	Ushort;
typedef	unsigned char	Uchar;

typedef	struct	{
	TCB	*f_tcb;
	char	*f_name;	/* Zeiger auf den langen Dateinamen */
	Ulong	f_namelen;	/* Länge des Dateinamens */
	char	*f_lname;	/* Zeiger auf den langen Linknamen */
	Ulong	f_lnamelen;	/* Länge des Linknamens */
	char	*f_uname;	/* User name oder NULL Pointer */
	Ulong	f_umaxlen;	/* Maximale Länge des Usernamens*/
	char	*f_gname;	/* Group name oder NULL Pointer */
	Ulong	f_gmaxlen;	/* Maximale Länge des Gruppennamens*/
	Ulong	f_dev;		/* Geraet auf dem sich d. Datei befindet */
	Ulong	f_ino;		/* Dateinummer			*/
	Ulong	f_nlink;	/* Anzahl der Links		*/
	Ulong	f_mode;		/* Zugriffsrechte 		*/
	Ulong	f_uid;		/* Benutzernummer		*/
	Ulong	f_gid;		/* Benutzergruppe		*/
	Ulong	f_size;		/* Dateigroesze			*/
	Ulong	f_rsize;	/* Dateigroesze auf Band	*/
	Ulong	f_offset;	/* Offset für Multivol cont. Dateien */
	Ulong	f_flags;	/* Bearbeitungshinweise		*/
	Ulong	f_xftype;	/* Typ der Datei (neu generell)	*/
	Ulong	f_filetype;	/* Typ der Datei (star alt)	*/
	Ulong	f_type;		/* Dateityp			*/
	Ulong	f_rdev;		/* Major/Minor bei Geraeten	*/
	Ulong	f_rdevmaj;	/* Major bei Geraeten		*/
	Ulong	f_rdevmin;	/* Minor bei Geraeten		*/
	Ulong	f_atime;	/* Zeit d. letzten Zugriffs	*/
	Ulong	f_spare1;
	Ulong	f_mtime;	/* Zeit d. letzten Aenderung */
	Ulong	f_spare2;
	Ulong	f_ctime;	/* Zeit d. letzten Statusaend. */
	Ulong	f_spare3;
} FINFO;

#define	F_LONGNAME	0x01	/* Langer Name passt nicht in Header	*/
#define	F_LONGLINK	0x02	/* Langer Linkname passt nicht in Header*/
#define	F_SPLIT_NAME	0x04	/* Langer Name wurde gesplitted		*/
#define	F_HAS_NAME	0x08	/* Langer Name in f_name soll bleiben	*/
#define	F_SPARSE	0x10	/* Datei enthält Löcher		*/

#define	F_SPEC	0
#define	F_FILE	1
#define	F_SLINK	2
#define	F_DIR	3

#define	is_special(i)	((i)->f_filetype == F_SPEC)
#define	is_file(i)	((i)->f_filetype == F_FILE)
#define	is_symlink(i)	((i)->f_filetype == F_SLINK)
#define	is_dir(i)	((i)->f_filetype == F_DIR)

#define	is_link(i)	((i)->f_xftype == XT_LINK)
#define	is_volhdr(i)	((i)->f_xftype == XT_VOLHDR)
#define	is_sparse(i)	((i)->f_xftype == XT_SPARSE)
#define	is_multivol(i)	((i)->f_xftype == XT_MULTIVOL)

#define isoctal(c)	((c) >= '0' && (c) <= '7')
#define	isupper(c)	((c) >= 'A' && (c) <= 'Z')
#define	toupper(c)	(isupper(c) ? (c) : (c) - ('a' - 'A'))
#define	max(a,b)	((a) < (b) ? (b) : (a))
#define	min(a,b)	((a) < (b) ? (a) : (b))

#ifdef	JOS
#	define	BAD	(1)
#else
#	define	BAD	(-1)
#endif

#include <sys/param.h>

#if !defined(PATH_MAX) && defined(MAXPATHLEN)
#define	PATH_MAX	MAXPATHLEN
#endif

#ifndef	PATH_MAX
#define	PATH_MAX	1024
#endif
