/* @(#)volhdr.c	1.31 07/11/08 Copyright 1994, 2003-2007 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)volhdr.c	1.31 07/11/08 Copyright 1994, 2003-2007 J. Schilling";
#endif
/*
 *	Volume header related routines.
 *
 *	Copyright (c) 1994, 2003-2007 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */


#include <schily/mconfig.h>
#include <stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include "star.h"
#include "props.h"
#include "table.h"
#include <schily/standard.h>
#include <schily/string.h>
#include <schily/schily.h>
#include <schily/libport.h>
#include "starsubs.h"
#include "dumpdate.h"
#include "xtab.h"
#include "fifo.h"

extern	FILE	*vpr;
extern	BOOL	multivol;
extern	long	chdrtype;
extern	char	*vers;
extern	int	verbose;
extern	Ullong	tsize;
extern	BOOL	ghdr;

extern struct timeval	ddate;			/* The current dump date	*/

extern	m_stats	*stats;

extern	GINFO	*gip;				/* Global information pointer	*/
extern	GINFO	*grip;				/* Global read info pointer	*/

EXPORT	void	ginit		__PR((void));
EXPORT	void	grinit		__PR((void));
LOCAL	int	xstrcpy		__PR((char **newp, char *old, char *p, int len));
EXPORT	void	gipsetup	__PR((GINFO *gp));
EXPORT	void	griprint	__PR((GINFO *gp));
EXPORT	BOOL	verifyvol	__PR((char *buf, int amt, int volno, int *skipp));
LOCAL	BOOL	vrfy_gvolhdr	__PR((char *buf, int amt, int volno, int *skipp));
EXPORT	char	*dt_name	__PR((int type));
EXPORT	int	dt_type		__PR((char *name));
EXPORT	void	put_release	__PR((void));
EXPORT	void	put_archtype	__PR((void));
EXPORT	void	put_gvolhdr	__PR((char *name));
EXPORT	void	put_volhdr	__PR((char *name, BOOL putv));
EXPORT	void	put_svolhdr	__PR((char *name));
EXPORT	void	put_multhdr	__PR((off_t size, off_t off));
EXPORT	BOOL	get_volhdr	__PR((FINFO *info, char *vhname));
LOCAL	void	get_label	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_hostname	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_filesys	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_cwd		__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_device	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_dumptype	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_dumplevel	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_reflevel	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_dumpdate	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_refdate	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_volno	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_blockoff	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_blocksize	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_tapesize	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));

/*
 * Important for the correctness of gen_number(): As long as we stay <= 55
 * chars for the keyword, a 128 bit number entry will fit into 100 chars.
 */
EXPORT xtab_t volhtab[] = {
			{ "SCHILY.volhdr.label",	19, get_label,	   0	},
			{ "SCHILY.volhdr.hostname",	22, get_hostname,  0	},
			{ "SCHILY.volhdr.filesys",	21, get_filesys,   0	},
			{ "SCHILY.volhdr.cwd",		17, get_cwd,	   0	},
			{ "SCHILY.volhdr.device",	20, get_device,	   0	},
			{ "SCHILY.volhdr.dumptype",	22, get_dumptype,  0	},
			{ "SCHILY.volhdr.dumplevel",	23, get_dumplevel, 0	},
			{ "SCHILY.volhdr.reflevel",	22, get_reflevel,  0	},
			{ "SCHILY.volhdr.dumpdate",	22, get_dumpdate,  0	},
			{ "SCHILY.volhdr.refdate",	21, get_refdate,   0	},
			{ "SCHILY.volhdr.volno",	19, get_volno,	   0	},
			{ "SCHILY.volhdr.blockoff",	22, get_blockoff,  0	},
			{ "SCHILY.volhdr.blocksize",	23, get_blocksize, 0	},
			{ "SCHILY.volhdr.tapesize",	22, get_tapesize,  0	},

			{ NULL,				0, NULL,	   0	}};

EXPORT void
ginit()
{
extern	int	dumplevel;
extern	int	nblocks;

	gip->label	= NULL;
	gip->hostname	= NULL;
	gip->filesys	= NULL;
	gip->cwd	= NULL;
	gip->device	= NULL;
	gip->release	= vers;
	gip->archtype	= chdrtype;
	gip->dumptype	= 0;
	gip->dumplevel	= dumplevel;
	gip->reflevel	= -1;
	gip->dumpdate	= ddate;
	gip->refdate.tv_sec  = 0;
	gip->refdate.tv_usec = 0;
	gip->volno	= 1;
	gip->tapesize	= tsize;
	gip->blockoff	= 0;
	gip->blocksize	= nblocks;
	gip->gflags	=   GF_RELEASE|GF_DUMPLEVEL|GF_REFLEVEL|GF_DUMPDATE|
			    GF_VOLNO|GF_TAPESIZE|GF_BLOCKOFF|GF_BLOCKSIZE;
}

EXPORT void
grinit()
{
	if (grip->label) {
		if ((grip->gflags & GF_NOALLOC) == 0)
			free(grip->label);
		grip->label	= NULL;
	}
	if (grip->hostname) {
		if ((grip->gflags & GF_NOALLOC) == 0)
			free(grip->hostname);
		grip->hostname	= NULL;
	}
	if (grip->filesys) {
		if ((grip->gflags & GF_NOALLOC) == 0)
			free(grip->filesys);
		grip->filesys	= NULL;
	}
	if (grip->cwd) {
		if ((grip->gflags & GF_NOALLOC) == 0)
			free(grip->cwd);
		grip->cwd	= NULL;
	}
	if (grip->device) {
		if ((grip->gflags & GF_NOALLOC) == 0)
			free(grip->device);
		grip->device	= NULL;
	}
	if (grip->release) {
		if ((grip->gflags & GF_NOALLOC) == 0)
			free(grip->release);
		grip->release	= NULL;
	}
	grip->archtype	= H_UNDEF;
	grip->dumptype	= 0;
	grip->dumplevel	= 0;
	grip->reflevel	= 0;
	grip->dumpdate.tv_sec  = 0;
	grip->dumpdate.tv_usec = 0;
	grip->refdate.tv_sec   = 0;
	grip->refdate.tv_usec  = 0;
	grip->volno	= 0;
	grip->tapesize	= 0;
	grip->blockoff	= 0;
	grip->blocksize	= 0;
	grip->gflags	= 0;
}

/*
 * A special string copy that is used copy strings into the limited space
 * in the shared memory.
 */
LOCAL int
xstrcpy(newp, old, p, len)
	char	**newp;
	char	*old;
	char	*p;
	int	len;
{
	int	slen;

	if (old == NULL)
		return (0);

	slen = strlen(old) + 1;
	if (slen > len)
		return (0);
	*newp = p;
	strncpy(p, old, len);
	p[len-1] = '\0';

	return (slen);
}

/*
 * Set up the global GINFO *gip structure from a structure that just
 * has been read from the information on the current medium.
 * This structure is inside the shared memory if we are usinf the fifo.
 */
EXPORT void
gipsetup(gp)
	GINFO	*gp;
{
#ifdef	FIFO
extern	m_head	*mp;
extern	BOOL	use_fifo;
#endif
	if (gip->gflags & GF_MINIT) {
		return;
	}
	*gip = *gp;
	gip->label	= NULL;
	gip->hostname	= NULL;
	gip->filesys	= NULL;
	gip->cwd	= NULL;
	gip->device	= NULL;
	gip->release	= NULL;

#ifdef	FIFO
	if (use_fifo) {
		char	*p = (char *)&gip[1];
		int	len = mp->rsize;
		int	slen;

		slen = xstrcpy(&gip->label, gp->label, p, len);
		p += slen;
		len -= slen;
		slen = xstrcpy(&gip->filesys, gp->filesys, p, len);
		p += slen;
		len -= slen;
		slen = xstrcpy(&gip->cwd, gp->cwd, p, len);
		p += slen;
		len -= slen;
		slen = xstrcpy(&gip->hostname, gp->hostname, p, len);
		p += slen;
		len -= slen;
		slen = xstrcpy(&gip->release, gp->release, p, len);
		p += slen;
		len -= slen;
		slen = xstrcpy(&gip->device, gp->device, p, len);
		p += slen;
		len -= slen;
		gip->gflags |= GF_NOALLOC;
	} else
#endif
	{
		if (gp->label)
			gip->label = __savestr(gp->label);
		if (gp->filesys)
			gip->filesys = __savestr(gp->filesys);
		if (gp->cwd)
			gip->cwd = __savestr(gp->cwd);
		if (gp->hostname)
			gip->hostname = __savestr(gp->hostname);
		if (gp->release)
			gip->release = __savestr(gp->release);
		if (gp->device)
			gip->device = __savestr(gp->device);
	}
	if (gp->volno > 1)		/* Allow to start with vol # != 1 */
		stats->volno = gp->volno;
	gip->gflags |= GF_MINIT;
}

EXPORT void
griprint(gp)
	GINFO	*gp;
{
	register FILE	*f = vpr;

	if (verbose <= 0)
		return;

	if (gp->label)
		fprintf(f, "Label       %s\n", gp->label);

	if (gp->hostname)
		fprintf(f, "Host name   %s\n", gp->hostname);

	if (gp->filesys)
		fprintf(f, "File system %s\n", gp->filesys);

	if (gp->cwd)
		fprintf(f, "Working dir %s\n", gp->cwd);

	if (gp->device)
		fprintf(f, "Device      %s\n", gp->device);

	if (gp->release)
		fprintf(f, "Release     %s\n", gp->release);

	if (gp->archtype != H_UNDEF)
		fprintf(f, "Archtype    %s\n", hdr_name(gp->archtype));

	if (gp->gflags & GF_DUMPTYPE)
		fprintf(f, "Dumptype    %s\n", dt_name(gp->dumptype));

	if (gp->gflags & GF_DUMPLEVEL)
		fprintf(f, "Dumplevel   %d\n", gp->dumplevel);

	if (gp->gflags & GF_REFLEVEL)
		fprintf(f, "Reflevel    %d\n", gp->reflevel);

	if (gp->gflags & GF_DUMPDATE) {
		fprintf(f, "Dumpdate    %lld.%6.6lld (%s)\n",
			(Llong)gp->dumpdate.tv_sec,
			(Llong)gp->dumpdate.tv_usec,
			dumpdate(&gp->dumpdate));
	}
	if (gp->gflags & GF_REFDATE) {
		fprintf(f, "Refdate     %lld.%6.6lld (%s)\n",
			(Llong)gp->refdate.tv_sec,
			(Llong)gp->refdate.tv_usec,
			dumpdate(&gp->refdate));
	}
	if (gp->gflags & GF_VOLNO)
		fprintf(f, "Volno       %d\n", gp->volno);
	if (gp->gflags & GF_BLOCKOFF)
		fprintf(f, "Blockoff    %llu records\n", gp->blockoff);
	if (gp->gflags & GF_BLOCKSIZE)
		fprintf(f, "Blocksize   %d records\n", gp->blocksize);
	if (gp->gflags & GF_TAPESIZE)
		fprintf(f, "Tapesize    %llu records\n", gp->tapesize);
}

EXPORT BOOL
verifyvol(buf, amt, volno, skipp)
	char	*buf;
	int	amt;
	int	volno;
	int	*skipp;
{
	TCB	*ptb = (TCB *)buf;

	*skipp = 0;

	/*
	 * Minimale Blockgroesse ist 2,5k
	 * 'g' Header, 512 Byte Content and a 'V' header
	 */
	if (ptb->ustar_dbuf.t_typeflag == 'g') {
		if (pr_validtype(ptb->ustar_dbuf.t_typeflag) &&
		    tarsum_ok(ptb))
			return (vrfy_gvolhdr(buf, amt, volno, skipp));
	}
	if (ptb->ustar_dbuf.t_typeflag == 'x') {
		if (pr_validtype(ptb->ustar_dbuf.t_typeflag) &&
		    tarsum_ok(ptb)) {
			Ullong	ull;
			int	xlen;

			stolli(ptb->dbuf.t_size, &ull);
			xlen = ull;
			xlen = 1 + tarblocks(xlen);
			*skipp += xlen;
			ptb = (TCB *)&((char *)ptb)[*skipp * TBLOCK];
		}
	}

	if (ptb->ustar_dbuf.t_typeflag == 'V' ||
	    ptb->ustar_dbuf.t_typeflag == 'M') {
		if (pr_validtype(ptb->ustar_dbuf.t_typeflag) &&
		    tarsum_ok(ptb)) {
			*skipp += 1;
			ptb = (TCB *)&buf[*skipp * TBLOCK];
		}
	}
	return (TRUE);
}

LOCAL BOOL
vrfy_gvolhdr(buf, amt, volno, skipp)
	char	*buf;
	int	amt;
	int	volno;
	int	*skipp;
{
	TCB	*ptb = (TCB *)buf;
	FINFO	finfo;
	char	name[PATH_MAX+1];
	char	lname[PATH_MAX+1];
	Ullong	ull;
	int	xlen = amt - TBLOCK - 1;
	char	*p = &buf[TBLOCK];
	char	*ep;
	char	ec;
	Llong	bytes;
	Llong	blockoff;

	fillbytes((char *)&finfo, sizeof (finfo), '\0');
	finfo.f_tcb = ptb;
	finfo.f_name = name;
	finfo.f_lname = lname;

	/*
	 * File size is strlen of extended header
	 */
	stolli(ptb->dbuf.t_size, &ull);
	if (xlen > ull)
		xlen = ull;

	grinit();	/* Clear/initialize current GINFO read struct */
	ep = p+xlen;
	ec = *ep;
	*ep = '\0';
	xhparse(&finfo, p, p+xlen);
	*ep = ec;
	griprint(grip);

	/*
	 * Return TRUE (no skip) if this was not a volume continuation header.
	 */
	if ((grip->gflags & GF_VOLNO) == 0)
		return (TRUE);

	if ((gip->dumpdate.tv_sec != grip->dumpdate.tv_sec) ||
	    (gip->dumpdate.tv_usec != grip->dumpdate.tv_usec)) {
		errmsgno(EX_BAD,
			"Dump date %s does not match expected",
					dumpdate(&grip->dumpdate));
		error(" %s\n", dumpdate(&gip->dumpdate));
		return (FALSE);
	}
	if (volno != grip->volno) {
		errmsgno(EX_BAD,
			"Volume number %d does not match expected %d\n",
					grip->volno, volno);
		return (FALSE);
	}
	bytes = stats->Tblocks * (Llong)stats->blocksize + stats->Tparts;
	blockoff = bytes / TBLOCK;
	/*
	 * In case we did start past Volume #1, we need to add
	 * the offset of the first volume we did see.
	 */
	blockoff += gip->blockoff;

	if (grip->blockoff != 0 &&
	    blockoff != grip->blockoff) {
		comerrno(EX_BAD,
			"Volume offset %lld does not match expected %lld\n",
				grip->blockoff, blockoff);
			/* NOTREACHED */
	}

	*skipp += 1 + tarblocks(xlen);

	/*
	 * Hier mu� noch ein 'V' Header hin, sonst ist das Archiv nicht
	 * konform zum POSIX Standard.
	 * Alternativ kann aber auch ein 'M'ultivol continuation Header stehen.
	 */
	ptb = (TCB *)&buf[(1 + tarblocks(xlen)) * TBLOCK];

	if (ptb->ustar_dbuf.t_typeflag == 'x') {
		if (pr_validtype(ptb->ustar_dbuf.t_typeflag) &&
		    tarsum_ok(ptb)) {
			stolli(ptb->dbuf.t_size, &ull);
			xlen = ull;
			xlen = 1 + tarblocks(xlen);
			*skipp += xlen;
			ptb = (TCB *)&((char *)ptb)[xlen * TBLOCK];
		}
	}

	if (ptb->ustar_dbuf.t_typeflag == 'V' ||
	    ptb->ustar_dbuf.t_typeflag == 'M') {
		if (pr_validtype(ptb->ustar_dbuf.t_typeflag) &&
		    tarsum_ok(ptb)) {
			*skipp += 1;
		}
	}
	return (TRUE);
}

EXPORT char *
dt_name(type)
	int	type;
{
	switch (type) {

	case DT_NONE:		return ("none");
	case DT_FULL:		return ("full");
	case DT_PARTIAL:	return ("partial");
	default:		return ("unknown");
	}
}

EXPORT int
dt_type(name)
	char	*name;
{
	if (streql(name, "none")) {
		return (DT_NONE);
	} else if (streql(name, "full")) {
		return (DT_FULL);
	} else if (streql(name, "partial")) {
		return (DT_PARTIAL);
	} else {
		return (DT_UNKN);
	}
}


EXPORT void
put_release()
{
	if ((props.pr_flags & PR_VU_XHDR) == 0 || props.pr_xc != 'x')
		return;

	/*
	 * We may change this in future when more tar implementations
	 * implement POSIX.1-2001
	 */
	if (H_TYPE(chdrtype) == H_XUSTAR)
		return;

	gen_text("SCHILY.release", vers, -1, 0);

	ghdr = TRUE;
}

EXPORT void
put_archtype()
{
	if ((props.pr_flags & PR_VU_XHDR) == 0 || props.pr_xc != 'x')
		return;

	/*
	 * We may change this in future when more tar implementations
	 * implement POSIX.1-2001
	 */
	if (H_TYPE(chdrtype) == H_XUSTAR)
		return;

	gen_text("SCHILY.archtype", hdr_name(chdrtype), -1, 0);

	ghdr = TRUE;
}

EXPORT void
put_gvolhdr(name)
	char	*name;
{
	char	nbuf[1024];

	if ((props.pr_flags & PR_VU_XHDR) == 0 || props.pr_xc != 'x')
		return;

	/*
	 * We may change this in future when more tar implementations
	 * implement POSIX.1-2001
	 */
	if (H_TYPE(chdrtype) == H_XUSTAR)
		return;

	gip->label = name;
	if (gip->dumplevel >= 0) {
		nbuf[0] = '\0';
		gethostname(nbuf, sizeof (nbuf));
		gip->hostname = __savestr(nbuf);
	}

	if (gip->label)
		gen_text("SCHILY.volhdr.label", gip->label, -1, 0);
	if (gip->hostname)
		gen_text("SCHILY.volhdr.hostname", gip->hostname, -1, 0);
	if (gip->filesys)
		gen_text("SCHILY.volhdr.filesys", gip->filesys, -1, 0);
	if (gip->cwd)
		gen_text("SCHILY.volhdr.cwd", gip->cwd, -1, 0);
	if (gip->device)
		gen_text("SCHILY.volhdr.device", gip->device, -1, 0);

	if (gip->dumptype > 0)
		gen_text("SCHILY.volhdr.dumptype", dt_name(gip->dumptype), -1, 0);
	if (gip->dumplevel >= 0)
		gen_number("SCHILY.volhdr.dumplevel", gip->dumplevel);
	if (gip->reflevel >= 0)
		gen_number("SCHILY.volhdr.reflevel", gip->reflevel);

	gen_xtime("SCHILY.volhdr.dumpdate", gip->dumpdate.tv_sec, gip->dumpdate.tv_usec * 1000);
	if (gip->refdate.tv_sec)
		gen_xtime("SCHILY.volhdr.refdate", gip->refdate.tv_sec, gip->refdate.tv_usec * 1000);

	if (gip->volno > 0)
		gen_number("SCHILY.volhdr.volno", gip->volno);
	if (gip->blockoff > 0)
		gen_number("SCHILY.volhdr.blockoff", gip->blockoff);
	if (gip->blocksize > 0)
		gen_number("SCHILY.volhdr.blocksize", gip->blocksize);
	if (gip->tapesize > 0)
		gen_number("SCHILY.volhdr.tapesize", gip->tapesize);

	if ((xhsize() + 2 * TBLOCK) > (gip->blocksize * TBLOCK)) {
		errmsgno(EX_BAD, "Panic: Tape record size too small.\n");
		comerrno(EX_BAD, "Panic: Shorten label or increase tape block size.\n");
	}
	ghdr = TRUE;
}

EXPORT void
put_volhdr(name, putv)
	char	*name;
	BOOL	putv;
{
extern	Ullong	tsize;

	if ((multivol || tsize > 0) && name == 0)
		name = "<none>";

	put_gvolhdr(name);

	if (name == 0)
		return;

	if (!putv)
		return;	/* Only a 'g' header is needed */

	put_svolhdr(name);
}

EXPORT void
put_svolhdr(name)
	char	*name;
{
	FINFO	finfo;
	TCB	tb;

	if ((props.pr_flags & PR_VOLHDR) == 0)
		return;

	if (name == 0 || *name == '\0')
		name = "<none>";

	fillbytes((char *)&finfo, sizeof (FINFO), '\0');
	filltcb(&tb);
	finfo.f_name = name;
	finfo.f_namelen = strlen(name);
	finfo.f_xftype = XT_VOLHDR;
	finfo.f_rxftype = XT_VOLHDR;
	finfo.f_mtime = ddate.tv_sec;
	finfo.f_mnsec = ddate.tv_usec*1000;
	finfo.f_atime = gip->volno;
	finfo.f_ansec = 0;
	finfo.f_tcb = &tb;
	finfo.f_xflags = XF_NOTIME;

	if (!name_to_tcb(&finfo, &tb))	/* Name too long */
		return;

	info_to_tcb(&finfo, &tb);
	put_tcb(&tb, &finfo);
	vprint(&finfo);
}

EXPORT void
put_multhdr(size, off)
	off_t	size;
	off_t	off;
{
extern	BOOL dodump;
	FINFO	finfo;
	TCB	tb;
	TCB	*mptb;
	BOOL	ododump = dodump;

	fillbytes((char *)&finfo, sizeof (finfo), '\0');

	if ((mptb = (TCB *)get_block(TBLOCK)) == NULL)
		mptb = &tb;
	else
		finfo.f_flags |= F_TCB_BUF;
	filltcb(mptb);
	strcpy(mptb->dbuf.t_name, "././@MultHeader");
	finfo.f_mode = TUREAD|TUWRITE;
	finfo.f_size = size;
	finfo.f_rsize = size - off;
	finfo.f_contoffset = off;
	finfo.f_xftype = XT_MULTIVOL;
	finfo.f_rxftype = XT_MULTIVOL;
	finfo.f_xflags = XF_NOTIME|XF_REALSIZE|XF_OFFSET;

	info_to_tcb(&finfo, mptb);

	dodump = FALSE;
	put_tcb(mptb, &finfo);
	dodump = ododump;
}

EXPORT BOOL
get_volhdr(info, vhname)
	FINFO	*info;
	char	*vhname;
{
	error("Volhdr: %s\n", info->f_name);

	if (vhname) {
		if (!streql(info->f_name, vhname))
			return (FALSE);
	}

	return (TRUE);
}

/* ARGSUSED */
LOCAL void
get_label(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	grip->gflags |= GF_LABEL;
	grip->label = __savestr(arg);
}

/* ARGSUSED */
LOCAL void
get_hostname(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	grip->gflags |= GF_HOSTNAME;
	grip->hostname = __savestr(arg);
}

/* ARGSUSED */
LOCAL void
get_filesys(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	grip->gflags |= GF_FILESYS;
	grip->filesys = __savestr(arg);
}

/* ARGSUSED */
LOCAL void
get_cwd(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	grip->gflags |= GF_CWD;
	grip->cwd    = __savestr(arg);
}

/* ARGSUSED */
LOCAL void
get_device(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	grip->gflags |= GF_DEVICE;
	grip->device = __savestr(arg);
}

/* ARGSUSED */
LOCAL void
get_dumptype(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	if (len == 0) {
		grip->gflags &= ~GF_DUMPTYPE;
		grip->dumptype = 0;
		return;
	}
	grip->dumptype = dt_type(arg);
	if (grip->dumptype == DT_UNKN)
		errmsgno(EX_BAD, "Unknown dump type '%s'\n", arg);
	else
		grip->gflags |= GF_DUMPTYPE;
}

/* ARGSUSED */
LOCAL void
get_dumplevel(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	Ullong	ull;

	if (len == 0) {
		grip->gflags &= ~GF_DUMPLEVEL;
		grip->dumplevel = 0;
		return;
	}
	if (get_unumber(keyword, arg, &ull, 1000)) {
		grip->gflags |= GF_DUMPLEVEL;
		grip->dumplevel = ull;
		if (grip->dumplevel != ull) {
			xh_rangeerr(keyword, arg, len);
			grip->dumplevel = 0;
		}
	}
}

/* ARGSUSED */
LOCAL void
get_reflevel(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	Ullong	ull;

	if (len == 0) {
		grip->gflags &= ~GF_REFLEVEL;
		grip->reflevel = 0;
		return;
	}
	if (get_unumber(keyword, arg, &ull, 1000)) {
		grip->gflags |= GF_REFLEVEL;
		grip->reflevel = ull;
		if (grip->reflevel != ull) {
			xh_rangeerr(keyword, arg, len);
			grip->reflevel = 0;
		}
	}
}

/* ARGSUSED */
LOCAL void
get_dumpdate(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	long	nsec;
	time_t	t;	/* FreeBSD/MacOS X have broken tv_sec/time_t */

	if (len == 0) {
		grip->gflags &= ~GF_DUMPDATE;
		grip->dumpdate.tv_sec  = 0;
		grip->dumpdate.tv_usec = 0;
		return;
	}
	if (get_xtime(keyword, arg, len, &t, &nsec)) {
		grip->gflags |= GF_DUMPDATE;
		grip->dumpdate.tv_sec  = t;
		grip->dumpdate.tv_usec = nsec/1000;
	}
}

/* ARGSUSED */
LOCAL void
get_refdate(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	long	nsec;
	time_t	t;	/* FreeBSD/MacOS X have broken tv_sec/time_t */

	if (len == 0) {
		grip->gflags &= ~GF_REFDATE;
		grip->refdate.tv_sec  = 0;
		grip->refdate.tv_usec = 0;
		return;
	}
	if (get_xtime(keyword, arg, len, &t, &nsec)) {
		grip->gflags |= GF_REFDATE;
		grip->refdate.tv_sec  = t;
		grip->refdate.tv_usec = nsec/1000;
	}
}

/* ARGSUSED */
LOCAL void
get_volno(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	Ullong	ull;

	if (len == 0) {
		grip->gflags &= ~GF_VOLNO;
		grip->volno = 0;
		return;
	}
	if (get_unumber(keyword, arg, &ull, INT_MAX)) {
		grip->gflags |= GF_VOLNO;
		grip->volno = ull;
		if (grip->volno != ull) {
			xh_rangeerr(keyword, arg, len);
			grip->volno = 0;
		}
	}
}

/* ARGSUSED */
LOCAL void
get_blockoff(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	Ullong	ull;

	if (len == 0) {
		grip->gflags &= ~GF_BLOCKOFF;
		grip->blockoff = 0;
		return;
	}
	if (get_unumber(keyword, arg, &ull, ULLONG_MAX)) {
		grip->gflags |= GF_BLOCKOFF;
		grip->blockoff = ull;
		if (grip->blockoff != ull) {
			xh_rangeerr(keyword, arg, len);
			grip->blockoff = 0;
		}
	}
}

/* ARGSUSED */
LOCAL void
get_blocksize(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	Ullong	ull;

	if (len == 0) {
		grip->gflags &= ~GF_BLOCKSIZE;
		grip->blocksize = 0;
		return;
	}
	if (get_unumber(keyword, arg, &ull, INT_MAX)) {
		grip->gflags |= GF_BLOCKSIZE;
		grip->blocksize = ull;
		if (grip->blocksize != ull) {
			xh_rangeerr(keyword, arg, len);
			grip->blocksize = 0;
		}
	}
}

/* ARGSUSED */
LOCAL void
get_tapesize(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	Ullong	ull;

	if (len == 0) {
		grip->gflags &= ~GF_TAPESIZE;
		grip->tapesize = 0;
		return;
	}
	if (get_unumber(keyword, arg, &ull, ULLONG_MAX)) {
		grip->gflags |= GF_TAPESIZE;
		grip->tapesize = ull;
		if (grip->tapesize != ull) {
			xh_rangeerr(keyword, arg, len);
			grip->tapesize = 0;
		}
	}
}