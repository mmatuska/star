/* @(#)buffer.c	1.46 01/04/07 Copyright 1985, 1995 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)buffer.c	1.46 01/04/07 Copyright 1985, 1995 J. Schilling";
#endif
/*
 *	Buffer handling routines
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
#include <fctldefs.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <vadefs.h>
#include "star.h"
#include <errno.h>
#include <standard.h>
#include "fifo.h"
#include <stdxlib.h>
#include <unixstd.h>
#include <strdefs.h>
#include <waitdefs.h>
#include <schily.h>
#ifdef	HAVE_SYS_MTIO_H
#include <sys/mtio.h>
#else
#include "mtio.h"
#endif
#include "starsubs.h"
#include "remote.h"

long	bigcnt	= 0;
int	bigsize	= 0;		/* Tape block size */
int	bufsize	= 0;		/* Available buffer size */
char	*bigbuf	= NULL;
char	*bigptr	= NULL;
char	*eofptr	= NULL;

m_stats	bstat;
m_stats	*stats	= &bstat;
int	pid;

#ifdef	timerclear
LOCAL	struct	timeval	starttime;
LOCAL	struct	timeval	stoptime;
#endif

LOCAL	BOOL	isremote = FALSE;
LOCAL	int	remfd	= -1;
LOCAL	char	*remfn;
LOCAL	char	host[128];

extern	FILE	*tarf;
extern	FILE	*tty;
extern	FILE	*vpr;
extern	char	*tarfile;
extern	BOOL	use_fifo;
extern	int	swapflg;
extern	BOOL	debug;
extern	BOOL	showtime;
extern	BOOL	no_stats;
extern	BOOL	do_fifostats;
extern	BOOL	cflag;
extern	BOOL	uflag;
extern	BOOL	rflag;
extern	BOOL	zflag;
extern	BOOL	bzflag;
extern	BOOL	multblk;
extern	BOOL	partial;
extern	BOOL	wready;
extern	BOOL	nullout;
extern	BOOL	nowarn;

extern	int	intr;

EXPORT	BOOL	openremote	__PR((void));
EXPORT	void	opentape	__PR((void));
EXPORT	void	closetape	__PR((void));
EXPORT	void	changetape	__PR((void));
EXPORT	void	nexttape	__PR((void));
EXPORT	void	initbuf		__PR((int nblocks));
EXPORT	void	markeof		__PR((void));
EXPORT	void	syncbuf		__PR((void));
EXPORT	int	readblock	__PR((char* buf));
LOCAL	int	readtblock	__PR((char* buf, int amount));
LOCAL	void	readbuf		__PR((void));
EXPORT	int	readtape	__PR((char* buf, int amount));
EXPORT	void	*get_block	__PR((void));
EXPORT	void	put_block	__PR((void));
EXPORT	void	writeblock	__PR((char* buf));
EXPORT	int	writetape	__PR((char* buf, int amount));
LOCAL	void	writebuf	__PR((void));
LOCAL	void	flushbuf	__PR((void));
EXPORT	void	writeempty	__PR((void));
EXPORT	void	weof		__PR((void));
EXPORT	void	buf_sync	__PR((void));
EXPORT	void	buf_drain	__PR((void));
EXPORT	int	buf_wait	__PR((int amount));
EXPORT	void	buf_wake	__PR((int amount));
EXPORT	int	buf_rwait	__PR((int amount));
EXPORT	void	buf_rwake	__PR((int amount));
EXPORT	void	buf_resume	__PR((void));
EXPORT	void	backtape	__PR((void));
EXPORT	int	mtioctl		__PR((int cmd, int count));
EXPORT	long	mtseek		__PR((long offset, int whence));
EXPORT	int	tblocks		__PR((void));
EXPORT	void	prstats		__PR((void));
EXPORT	BOOL	checkerrs	__PR((void));
EXPORT	void	exprstats	__PR((int ret));
EXPORT	void	excomerrno	__PR((int err, char* fmt, ...)) __printflike__(2, 3);
EXPORT	void	excomerr	__PR((char* fmt, ...)) __printflike__(1, 2);
EXPORT	void	die		__PR((int err));
LOCAL	void	compressopen	__PR((void));

EXPORT BOOL
openremote()
{
	register char *hp;
	register char *fp;
	register int  i;

	if ((!nullout || (uflag || rflag)) && strchr(tarfile, ':')) {

#ifdef	USE_REMOTE
		isremote = TRUE;
		remfn = strchr(tarfile, ':');
		for (fp = tarfile, hp = host, i = 1;
				fp < remfn && i < sizeof(host); i++) {
			*hp++ = *fp++;
		}
		*hp = '\0';
		remfn++;
		if (debug)
			errmsgno(EX_BAD, "Remote: %s Host: %s file: %s\n",
							tarfile, host, remfn);

		if ((remfd = rmtgetconn(host, bigsize)) < 0)
			comerrno(EX_BAD, "Cannot get connection to '%s'.\n",
				/* errno not valid !! */		host);
#else
		comerrno(EX_BAD, "Remote tape support not present.\n");
#endif
	}
	return (isremote);
}

EXPORT void
opentape()
{
	int	n = 0;

	if (nullout && !(uflag || rflag)) {
		tarfile = "null";
		tarf = (FILE *)NULL;
	} else if (streql(tarfile, "-")) {
		if (cflag) {
			tarfile = "stdout";
			tarf = stdout;
		} else {
			tarfile = "stdin";
			tarf = stdin;
			multblk = TRUE;
		}
		setbuf(tarf, (char *)NULL);
#if	defined(__CYGWIN32__)
		setmode(fileno(tarf), O_BINARY);
#endif
	} else if (isremote) {
#ifdef	USE_REMOTE
		/*
		 * isremote will always be FALSE if USE_REMOTE is not defined.
		 * NOTE any open flag bejond O_RDWR is not portable across
		 * different platforms. The remote tape library will check
		 * whether the current /etc/rmt server supports symbolic
		 * open flags. If there is no symbolic support in the
		 * remote server, our rmt client code will mask off all
		 * non portable bits.
		 */
		while (rmtopen(remfd, remfn, (cflag ? O_RDWR|O_CREAT:O_RDONLY)|O_BINARY) < 0) {
			if (!wready || n++ > 6 || geterrno() != EIO)
				comerr("Cannot open remote '%s'.\n", tarfile);
			else
				sleep(10);
		}
#endif	
	} else {
		FINFO	finfo;
	extern	BOOL	follow;
	extern	BOOL	fcompat;

		if (fcompat && cflag) {
			/*
			 * The old syntax has a high risk of corrupting
			 * files if the user disorders the args.
			 * For this reason, we do not allow to overwrite
			 * a plain file in compat mode.
			 * XXX What if we implement 'r' & 'u' ???
			 */
			follow++;
			n = getinfo(tarfile, &finfo);
			follow--;
			if (n >= 0 && is_file(&finfo) && finfo.f_size > 0L) {
				comerrno(EX_BAD,
				"Will not overwrite non empty plain files in compat mode.\n");
			}
		}

		n = 0;
		while ((tarf = fileopen(tarfile, cflag?"rwcub":"rub")) ==
								(FILE *)NULL) {
			if (!wready || n++ > 6 || geterrno() != EIO)
				comerr("Cannot open '%s'.\n", tarfile);
			else
				sleep(10);
		}
	}
	if (!isremote && (!nullout || (uflag || rflag))) {
		file_raise(tarf, FALSE);
		checkarch(tarf);
	}
	vpr = tarf == stdout ? stderr : stdout;

	if (zflag || bzflag) {
		extern	long	tape_dev;
		extern	long	tape_ino;

		if (isremote)
			comerrno(EX_BAD, "Cannot compress remote archives (yet).\n");
		if (tape_dev || tape_ino)
			compressopen();
		else
			comerrno(EX_BAD, "Can only compress files.\n");
	}

#ifdef	timerclear
	if (showtime && gettimeofday(&starttime, (struct timezone *)0) < 0)
		comerr("Cannot get starttime\n");
#endif
}

EXPORT void
closetape()
{
	if (isremote) {
#ifdef	USE_REMOTE
		/*
		 * isremote will always be FALSE if USE_REMOTE is not defined.
		 */
		if (rmtclose(remfd) < 0)
			errmsg("Remote close failed.\n");
#endif
	} else {
		if (tarf)
			fclose(tarf);
	}
}

EXPORT void
changetape()
{
	char	ans[2];

	prstats();
	stats->Tblocks += stats->blocks;
	stats->Tparts += stats->parts;
	stats->blocks = 0L;
	stats->parts = 0L;
	closetape();
	errmsgno(EX_BAD, "Mount volume #%d and hit <RETURN>", ++stats->volno);
	fgetline(tty, ans, sizeof(ans));
	if (feof(tty))
		exit(1);
	opentape();
}

EXPORT void
nexttape()
{
	weof();
#ifdef	FIFO
	if (use_fifo) {
		fifo_chtape();
	} else
#endif
	changetape();
	if (intr)
		exit(2);
}

EXPORT void
initbuf(nblocks)
	int	nblocks;
{
	pid = getpid();
	bufsize = bigsize = nblocks * TBLOCK;
#ifdef	FIFO
	if (use_fifo) {
		initfifo();
	} else
#endif
	{
		if ((bigptr = bigbuf = malloc((unsigned) bufsize)) == NULL)
			comerr("Cannot alloc buf.\n");
		fillbytes(bigbuf, bufsize, '\0');
	}
	stats->blocksize = bigsize;
	stats->volno = 1;
	stats->swapflg = -1;
}

EXPORT void
markeof()
{
#ifdef	FIFO
	if (use_fifo) {
		/*
		 * Remember current FIFO status.
		 */
	}
#endif
	eofptr = bigptr - TBLOCK;

	if (debug) {
		error("Blocks: %d\n", tblocks());
		error("bigptr - bigbuff: %d %p %p %p lastsize: %ld\n",
			bigptr - bigbuf, bigbuf, bigptr, eofptr, stats->lastsize);
	}
}

EXPORT void
syncbuf()
{
#ifdef	FIFO
	if (use_fifo) {
		/*
		 * Switch FIFO direction.
		 */
		excomerr("Cannot update tape with FIFO.\n");
	}
#endif
	bigptr = eofptr;
	bigcnt = eofptr - bigbuf;
}

EXPORT int
readblock(buf)
	char	*buf;
{
	if (buf_rwait(TBLOCK) == 0)
		return (EOF);
	movebytes(bigptr, buf, TBLOCK);
	buf_rwake(TBLOCK);
	return TBLOCK;
}

LOCAL int
readtblock(buf, amount)
	char	*buf;
	int	amount;
{
	int	cnt;

	stats->reading = TRUE;
	if (isremote) {
#ifdef	USE_REMOTE
		/*
		 * isremote will always be FALSE if USE_REMOTE is not defined.
		 */
		if ((cnt = rmtread(remfd, buf, amount)) < 0)
			excomerr("Error reading '%s'.\n", tarfile);
#endif
	} else {
		if ((cnt = ffileread(tarf, buf, amount)) < 0)
			excomerr("Error reading '%s'.\n", tarfile);
	}
	return (cnt);
}

LOCAL void
readbuf()
{
	bigcnt = readtape(bigbuf, bigsize);	
	bigptr = bigbuf;
}

EXPORT int
readtape(buf, amount)
	char	*buf;
	int	amount;
{
	int	amt;
	int	cnt;
	char	*bp;
	int	size;

	amt = 0;
	bp = buf;
	size = amount;

	do {
		cnt = readtblock(bp, size);

		amt += cnt;
		bp += cnt;
		size -= cnt;
	} while (amt < amount && cnt > 0 && multblk);

	if (amt == 0)
		return (amt);
	if (amt < TBLOCK)
		excomerrno(EX_BAD, "Error reading '%s' size (%d) too small.\n",
							tarfile, amt);
	/*
	 * First block
	 */
	if (stats->swapflg < 0) {
		if ((amt % TBLOCK) != 0)
			comerrno(EX_BAD, "Invalid blocksize %d bytes.\n", amt);
		if (amt < amount) {
			stats->blocksize = bigsize = amt;
#ifdef	FIFO
			if (use_fifo)
				fifo_ibs_shrink(amt);
#endif
			errmsgno(EX_BAD, "Blocksize = %ld records.\n",
						stats->blocksize/TBLOCK);
		}
	}
	if (stats->swapflg > 0)
		swabbytes(buf, amt);

	if (amt == stats->blocksize)
		stats->blocks++;
	else
		stats->parts += amt;
	stats->lastsize = amt;
#ifdef	DEBUG
	error("readbuf: cnt: %d.\n", amt);
#endif
	return (amt);
}

/*#define	MY_SWABBYTES*/
#ifdef	MY_SWABBYTES
#define	DO8(a)	a;a;a;a;a;a;a;a;

void swabbytes(bp, cnt)
	register char	*bp;
	register int	cnt;
{
	register char	c;

	cnt /= 2;	/* even count only */
	while ((cnt -= 8) >= 0) {
		DO8(c = *bp++; bp[-1] = *bp; *bp++ = c;);
	}
	cnt += 8;

	while (--cnt >= 0) {
		c = *bp++; bp[-1] = *bp; *bp++ = c;
	}
}
#endif

EXPORT void *
get_block()
{
	buf_wait(TBLOCK);
	return ((void *)bigptr);
}

EXPORT void
put_block()
{
	buf_wake(TBLOCK);
}

EXPORT void
writeblock(buf)
	char	*buf;
{
	buf_wait(TBLOCK);
	movebytes(buf, bigptr, TBLOCK);
	buf_wake(TBLOCK);
}

EXPORT int
writetape(buf, amount)
	char	*buf;
	int	amount;
{
	int	cnt;
	int	err = 0;
					/* hartes oder weiches EOF ??? */
					/* d.h. < 0 oder <= 0          */
	stats->reading = FALSE;
	if (nullout) {
		cnt = amount;
#ifdef	USE_REMOTE
	} else if (isremote) {
		cnt = rmtwrite(remfd, buf, amount);
#endif
	} else {
		cnt = ffilewrite(tarf, buf, amount);
	}
	if (cnt == 0) {
		err = EFBIG;
	} else if (cnt < 0) {
		err = geterrno();
	}
	if (cnt <= 0)
		excomerrno(err, "Error writing '%s'.\n", tarfile);

	if (cnt == stats->blocksize)
		stats->blocks++;
	else
		stats->parts += cnt;
	return (cnt);
}

LOCAL void
writebuf()
{
	long	cnt;

	cnt = writetape(bigbuf, bigsize);

	bigptr = bigbuf;
	bigcnt = 0;
}

LOCAL void
flushbuf()
{
	long	cnt;

#ifdef	FIFO
	if (!use_fifo)
#endif
	{
		cnt = writetape(bigbuf, bigcnt);

		bigptr = bigbuf;
		bigcnt = 0;
	}
}

EXPORT void
writeempty()
{
	char	buf[TBLOCK];

	fillbytes(buf, TBLOCK, '\0');
	writeblock(buf);
}

EXPORT void
weof()
{
	writeempty();
	writeempty();
	if (!partial)
		buf_sync();
	flushbuf();
}

EXPORT void
buf_sync()
{
#ifdef	FIFO
	if (use_fifo) {
		fifo_sync();
	} else
#endif
	{
		fillbytes(bigptr, bigsize - bigcnt, '\0');
		bigcnt = bigsize;
	}
}

EXPORT void
buf_drain()
{
#ifdef	FIFO
	if (use_fifo) {
		fifo_oflush();
		wait(0);
	}
#endif
}

EXPORT int
buf_wait(amount)
	int	amount;
{
#ifdef	FIFO
	if (use_fifo) {
		return (fifo_iwait(amount));
	} else
#endif
	{
		if (bigcnt >= bigsize)
			writebuf();
		return (bigsize - bigcnt);
	}
}

EXPORT void
buf_wake(amount)
	int	amount;
{
#ifdef	FIFO
	if (use_fifo) {
		fifo_owake(amount);
	} else
#endif
	{
		bigptr += amount;
		bigcnt += amount;
	}
}

EXPORT int
buf_rwait(amount)
	int	amount;
{
#ifdef	FIFO
	if (use_fifo) {
		return (fifo_owait(amount));
	} else
#endif
	{
/*		if (bigcnt < amount)*/ /* neu ?? */
		if (bigcnt <= 0)
			readbuf();
		return (bigcnt);
	}
}

EXPORT void
buf_rwake(amount)
	int	amount;
{
#ifdef	FIFO
	if (use_fifo) {
		fifo_iwake(amount);
	} else
#endif
	{
		bigptr += amount;
		bigcnt -= amount;
	}
}

EXPORT void
buf_resume()
{
	stats->swapflg = swapflg;	/* copy over for fifo process */
	bigsize = stats->blocksize;	/* copy over for tar process */
#ifdef	FIFO
	if (use_fifo)
		fifo_resume();
#endif
}

EXPORT void
backtape()
{
	long	ret;

	if (debug) {
		error("Blocks: %d\n", tblocks());
		error("filepos: %ld seeking to: %ld bigsize: %d\n",
		mtseek(0L, SEEK_CUR), mtseek(0L, SEEK_CUR)-stats->lastsize, bigsize);
	}

	if (mtioctl(MTNOP, 0) >= 0) {
		if (debug)
			error("Is a tape: BSR 1...\n");
		ret = mtioctl(MTBSR, 1);
	} else {
		if (debug)
			error("Is a file: lseek()\n");
		ret = mtseek(-stats->lastsize, SEEK_CUR);
	}
	if (ret < 0)
		excomerr("Cannot backspace tape.\n");

	if (stats->lastsize == stats->blocksize)
		stats->blocks--;
	else
		stats->parts -= stats->lastsize;
}

EXPORT int
mtioctl(cmd, count)
	int	cmd;
	int	count;
{
	int	ret;

	if (nullout && !(uflag || rflag)) {
		return (0);
#ifdef	USE_REMOTE
	} else if (isremote) {
		ret = rmtioctl(remfd, cmd, count);
#endif
	} else {
#ifdef	MTIOCTOP
		struct mtop mtop;

		mtop.mt_op = cmd;
		mtop.mt_count = count;

		ret = ioctl(fdown(tarf), MTIOCTOP, &mtop);
#else
		return (-1);
#endif
	}
	if (ret < 0 && debug)
		errmsg("Error sending mtioctl(%d, %d) to '%s'.\n", cmd, count, tarfile);
	return (ret);
}

EXPORT long
mtseek(offset, whence)
	long	offset;
	int	whence;
{
	if (nullout && !(uflag || rflag)) {
		return (0L);
#ifdef	USE_REMOTE
	} else if (isremote) {
		return (rmtseek(remfd, offset, whence));
#endif
	} else {
		return (fseek(tarf, offset, whence));
	}
}

EXPORT int
tblocks()
{
	long	fifo_cnt = 0;

#ifdef	FIFO
	if (use_fifo)
		fifo_cnt = fifo_amount()/TBLOCK;
#endif
	if (debug) {
		error("blocks: %ld blocksize: %ld parts: %ld bigcnt: %ld fifo_cnt: %ld\n", 
		stats->blocks, stats->blocksize, stats->parts, bigcnt, fifo_cnt);
	}
	if (stats->reading)
		return (-fifo_cnt + (stats->blocks * stats->blocksize +
					stats->parts - (bigcnt+TBLOCK))/TBLOCK);
	else
		return (fifo_cnt + (stats->blocks * stats->blocksize +
					stats->parts + bigcnt)/TBLOCK);
}

EXPORT void
prstats()
{
	Llong	bytes;
	long	kbytes;
	long	hibytes;
	long	lobytes;
	char	cbytes[32];
	int	per;
#ifdef	timerclear
	int	sec;
	int	usec;
	int	tmsec;
#endif

	if (no_stats)
		return;
	if (pid == 0)	/* child */
		return;

#ifdef	timerclear
	if (showtime && gettimeofday(&stoptime, (struct timezone *)0) < 0)
		comerr("Cannot get stoptime\n");
#endif
#ifdef	FIFO
	if (use_fifo && do_fifostats)
		fifo_stats();
#endif

	bytes = (Llong)stats->blocks * (Llong)stats->blocksize + stats->parts;
	kbytes = bytes >> 10;
	per = ((bytes&1023)<<10)/10485;

	cbytes[0] = '\0';
	hibytes = bytes / 1000000000;
	lobytes = bytes % 1000000000;
	if (hibytes)
		sprintf(cbytes, "%ld%09ld", hibytes, lobytes);
	else
		sprintf(cbytes, "%ld", lobytes);

	errmsgno(EX_BAD,
		"%ld blocks + %ld bytes (total of %s bytes = %ld.%02dk).\n",
		stats->blocks, stats->parts, cbytes, kbytes, per);

	if (stats->Tblocks + stats->Tparts) {
		bytes = (Llong)stats->Tblocks * (Llong)stats->blocksize +
								stats->Tparts;
		kbytes = bytes >> 10;
		per = ((bytes&1023)<<10)/10485;

		cbytes[0] = '\0';
		hibytes = bytes / 1000000000;
		lobytes = bytes % 1000000000;
		if (hibytes)
			sprintf(cbytes, "%ld%09ld", hibytes, lobytes);
		else
			sprintf(cbytes, "%ld", lobytes);

		errmsgno(EX_BAD,
		"Total %ld blocks + %ld bytes (total of %s bytes = %ld.%02dk).\n",
		stats->Tblocks, stats->Tparts, cbytes, kbytes, per);
	}
#ifdef	timerclear
	if (showtime) {
		Llong	kbs;

		sec = stoptime.tv_sec - starttime.tv_sec;
		usec = stoptime.tv_usec - starttime.tv_usec;
		tmsec = sec*1000 + usec/1000;
		if (usec < 0) {
			sec--;
			usec += 1000000;
		}
		if (tmsec == 0)
			tmsec++;

		kbs = (Llong)kbytes*(Llong)1000/tmsec;
		lobytes = kbs;

		errmsgno(EX_BAD, "Total time %d.%03dsec (%ld kBytes/sec)\n",
				sec, usec/1000, lobytes);
	}
#endif
}

EXPORT BOOL
checkerrs()
{
	if (xstats.s_staterrs	||
	    xstats.s_openerrs	||
	    xstats.s_rwerrs	||
	    xstats.s_misslinks	||
	    xstats.s_toolong	||
	    xstats.s_toobig	||
	    xstats.s_isspecial	||
	    xstats.s_sizeerrs) {
		if (nowarn || no_stats || (pid == 0)/* child */)
			return (TRUE);

		errmsgno(EX_BAD, "The following problems occurred during archive processing:\n");
		errmsgno(EX_BAD, "Cannot: stat %d, open %d, read/write %d. Size changed %d.\n",
				xstats.s_staterrs,
				xstats.s_openerrs,
				xstats.s_rwerrs,
				xstats.s_sizeerrs);
		errmsgno(EX_BAD, "Missing links %d, Name too long %d, File too big %d, Not dumped %d.\n",
				xstats.s_misslinks,
				xstats.s_toolong,
				xstats.s_toobig,
				xstats.s_isspecial);
		return (TRUE);
	}
	return (FALSE);
}

EXPORT void
exprstats(ret)
	int	ret;
{
	prstats();
	exit(ret);
}

/* VARARGS2 */
#ifdef	PROTOTYPES
EXPORT void
excomerrno(int err, char *fmt, ...)
#else
EXPORT void
excomerrno(err, fmt, va_alist)
	int	err;
	char	*fmt;
	va_dcl
#endif
{
	va_list	args;

#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
	errmsgno(err, "%r", fmt, args);
	va_end(args);
#ifdef	FIFO
	fifo_exit();
#endif
	exprstats(err);
	/* NOTREACHED */
}

/* VARARGS1 */
#ifdef	PROTOTYPES
EXPORT void
excomerr(char *fmt, ...)
#else
EXPORT void
excomerr(fmt, va_alist)
	char	*fmt;
	va_dcl
#endif
{
	va_list	args;
	int	err = geterrno();

#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
	errmsgno(err, "%r", fmt, args);
	va_end(args);
#ifdef	FIFO
	fifo_exit();
#endif
	exprstats(err);
	/* NOTREACHED */
}

EXPORT void
die(err)
	int	err;
{
	excomerrno(err, "Cannot recover from error - exiting.\n");
}

/*
 * Quick hack to implement a -z flag. May be changed soon.
 */
#include <signal.h>
#if	defined(SIGDEFER) || defined(SVR4)
#define	signal	sigset
#endif
LOCAL void
compressopen()
{
	FILE	*pp[2];
	int	mypid;
	char	*zip_prog = "gzip";
        
	if (bzflag)
		zip_prog = "bzip2";

	multblk = TRUE;

	if (fpipe(pp) == 0)
		comerr("Compress pipe failed\n");
	mypid = fork();
	if (mypid < 0)
		comerr("Compress fork failed\n");
	if (mypid == 0) {
		FILE	*null;
		char	*flg = getenv("STAR_COMPRESS_FLAG"); /* Temporary ? */

		signal(SIGQUIT, SIG_IGN);
		if (cflag)
			fclose(pp[1]);
		else
			fclose(pp[0]);

#if	defined(__CYGWIN32__)
		if (cflag)
			setmode(fileno(pp[0]), O_BINARY);
		else
			setmode(fileno(pp[1]), O_BINARY);
#endif

		/* We don't want to see errors */
		null = fileopen("/dev/null", "rw");

		if (cflag)
			fexecl(zip_prog, pp[0], tarf, null, zip_prog, flg, NULL);
		else
			fexecl(zip_prog, tarf, pp[1], null, zip_prog, "-d", NULL);
		errmsg("Compress: exec of '%s' failed\n", zip_prog);
		_exit(-1);
	}
	fclose(tarf);
	if (cflag) {
		tarf = pp[1];
		fclose(pp[0]);
	} else {
		tarf = pp[0];
		fclose(pp[1]);
	}
}
