#ifdef	FIFO
/* @(#)fifo.c	1.9 97/04/28 Copyright 1989 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)fifo.c	1.9 97/04/28 Copyright 1989 J. Schilling";
#endif
/*
 *	A "fifo" that uses shared memory between two processes
 *
 *	Copyright (c) 1989 J. Schilling
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

/*#define	DEBUG*/
#if !defined(USE_MMAP) && !defined(USE_SHM)
#define	USE_MMAP
#endif
#include <mconfig.h>
#include <fctldefs.h>
#include <sys/types.h>
#if defined(HAVE_SMMAP) && defined(USE_MMAP)
#include <sys/mman.h>
#endif
#include <stdio.h>
#include <stdxlib.h>
#include <unixstd.h>
#include <standard.h>
#include <errno.h>
#include "star.h"
#include "fifo.h"
#include "starsubs.h"

#ifndef	HAVE_SMMAP
#	undef	USE_MMAP
#	define	USE_SHM	/* SYSV shared memory is the default */
#endif

#ifdef DEBUG
#define	EDEBUG(a)	if (debug) error a
#else
#define	EDEBUG(a)
#endif

#undef	roundup
#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y))

char	*buf;
m_head	*mp;
int	buflen;

extern	int	debug;
extern	BOOL	shmflag;
extern	BOOL	no_stats;
extern	long	fs;
extern	long	bs;
extern	long	ibs;
extern	long	obs;
extern	int	hiw;
extern	int	low;

extern	m_stats	*stats;
extern	int	pid;

long	ibs;
long	obs;
int	hiw;
int	low;

int	shmflag;

EXPORT	void	initfifo	__PR((void));
LOCAL	void	fifo_setparams	__PR((void));
EXPORT	void	fifo_ibs_shrink	__PR((int newsize));
EXPORT	void	runfifo		__PR((void));
LOCAL	void	prmp		__PR((void));
EXPORT	void	fifo_stats	__PR((void));
LOCAL	int	swait		__PR((int f));
LOCAL	int	swakeup		__PR((int f, int c));
EXPORT	int	fifo_amount	__PR((void));
EXPORT	int	fifo_iwait	__PR((int amount));
EXPORT	void	fifo_owake	__PR((int amount));
EXPORT	void	fifo_oflush	__PR((void));
EXPORT	int	fifo_owait	__PR((int amount));
EXPORT	void	fifo_iwake	__PR((int amt));
EXPORT	void	fifo_resume	__PR((void));
EXPORT	void	fifo_sync	__PR((void));
EXPORT	void	fifo_chtape	__PR((void));
LOCAL	void	do_in		__PR((void));
LOCAL	void	do_out		__PR((void));
#ifdef	USE_MMAP
LOCAL	char*	mkshare		__PR((int size));
#endif
#ifdef	USE_SHM
LOCAL	char*	mkshm		__PR((int size));
#endif

EXPORT void
initfifo()
{
	int	pagesize;

	if (obs == 0)
		obs = bs;
	if (fs == 0)
		fs = 1*1024*1024;
	if (fs < bs + obs)
		fs = bs + obs;
	if (fs < 2*obs)
		fs = 2*obs;
	fs = roundup(fs, obs);
#ifdef	_SC_PAGESIZE
	pagesize = sysconf(_SC_PAGESIZE);
#else
	pagesize = getpagesize();
#endif
	buflen = roundup(fs, pagesize) + pagesize;
	EDEBUG(("bs: %d obs: %d fs: %d buflen: %d\n", bs, obs, fs, buflen));

#if	defined(USE_MMAP) && defined(USE_SHM)
	if (shmflag)
		buf = mkshm(buflen);
	else
		buf = mkshare(buflen);
#endif
#if	defined(USE_MMAP)
	buf = mkshare(buflen);
#endif
#if	defined(USE_SHM)
		buf = mkshm(buflen);
#endif
	mp = (m_head *)buf;
	fillbytes((char *)mp, sizeof(*mp), '\0');
	stats = &mp->stats;
	mp->base = &buf[pagesize];

	fifo_setparams();

	if (pipe(mp->gp) < 0)
		comerr("pipe\n");
	if (pipe(mp->pp) < 0)
		comerr("pipe\n");
	mp->putptr = mp->getptr = mp->base;
	prmp();
	{
		/* Temporary until all modules know about mp->xxx */
		extern int	bufsize;
		extern char*	bigbuf;
		extern char*	bigptr;

		bufsize = mp->size;
		bigptr = bigbuf = mp->base;
	}
}

LOCAL void
fifo_setparams()
{
	mp->end = &mp->base[fs];
	mp->size = fs;
	mp->ibs = ibs;
	mp->obs = obs;

	if (hiw)
		mp->hiw = hiw;
	else
		mp->hiw = mp->size / 3 * 2;
	if (low)
		mp->low = low;
	else
		mp->low = mp->size / 3;
	if (mp->low < mp->obs)
		mp->low = mp->obs;
	if (mp->low > mp->hiw)
		mp->low = mp->hiw;

	if (ibs == 0 || mp->ibs > mp->size)
		mp->ibs = mp->size;
}

EXPORT void
fifo_ibs_shrink(newsize)
	int	newsize;
{
	ibs = newsize;
	fs = (fs/newsize)*newsize;
	fifo_setparams();
}

/*---------------------------------------------------------------------------
|
| Der eigentliche star Prozess ist immer im Vordergrund.
| Der Band Prozess ist immer im Hintergrund.
| Band -> fifo -> star
| star -> fifo -> Band
| Beim Lesen ist der star Prozess der get Prozess.
| Beim Schreiben ist der star Prozess der put Prozess.
|
+---------------------------------------------------------------------------*/

EXPORT void
runfifo()
{
	extern	BOOL	cflag;

	if ((pid = fork()) < 0)
		comerr("Cannot fork.\n");

	if ((pid != 0) ^ cflag) {
		EDEBUG(("Get prozess: cflag: %d pid: %d\n", cflag, pid));
		/* Get Prozess */
		close(mp->gpout);
		close(mp->ppin);
	} else {
		EDEBUG(("Put prozess: cflag: %d pid: %d\n", cflag, pid));
		/* Put Prozess */
		close(mp->gpin);
		close(mp->ppout);
	}

	if (pid == 0) {
		if (cflag) {
			mp->ibs = mp->size;
			mp->obs = bs;
			do_out();
		} else {
			mp->flags |= FIFO_IWAIT;
			mp->ibs = bs;
			mp->obs = mp->size;
			do_in();
		}
	} else {
		extern	FILE	*tarf;

		if (tarf)
			fclose(tarf);
	}
}

LOCAL void
prmp()
{
#ifdef	DEBUG
	if (!debug)
		return;
#ifdef	TEST
	error("putptr: %X\n", mp->putptr);
	error("getptr: %X\n", mp->getptr);
#endif
	error("base:  %X\n", mp->base);
	error("end:   %X\n", mp->end);
	error("size:  %d\n", mp->size);
	error("ibs:   %d\n", mp->ibs);
	error("obs:   %d\n", mp->obs);
	error("amt:   %d\n", FIFO_AMOUNT(mp));
	error("hiw:   %d\n", mp->hiw);
	error("low:   %d\n", mp->low);
	error("flags: %X\n", mp->flags);
#ifdef	TEST
	error("wpin:  %d\n", mp->wpin);
	error("wpout: %d\n", mp->wpout);
	error("rpin:  %d\n", mp->rpin);
	error("rpout: %d\n", mp->rpout);
#endif
#endif	/* DEBUG */
}

EXPORT void
fifo_stats()
{
	if (no_stats)
		return;

	errmsgno(BAD, "fifo had %d puts %d gets.\n", mp->puts, mp->gets);
	errmsgno(BAD, "fifo was %d times empty and %d times full.\n",
						mp->empty, mp->full);
	errmsgno(BAD, "fifo held %d bytes max, size was %d bytes\n",
						mp->maxfill, mp->size);
}


/*---------------------------------------------------------------------------
|
| Semaphore wait
|
+---------------------------------------------------------------------------*/
LOCAL int
swait(f)
	int	f;
{
		 int	ret;
	unsigned char	c;

	do {
		ret = read(f, &c, 1);
	} while (ret < 0 && geterrno() == EINTR);
	if (ret < 0 || (ret == 0 && pid)) {
		errmsg("Sync pipe read error on pid %d.\n", pid);
		prstats();
		exit(1);
	}
	if (ret == 0) {
		/*
		 * this is the background process!
		 */
		exit(0);
	}
	return ((int)c);
}

/*---------------------------------------------------------------------------
|
| Semaphore wakeup
|
+---------------------------------------------------------------------------*/
LOCAL int
swakeup(f, c)
	int	f;
	char	c;
{
	return (write(f, &c, 1));
}

#define	sgetwait(m)		swait((m)->gpin)
#define	sgetwakeup(m, c)	swakeup((m)->gpout, (c))

#define	sputwait(m)		swait((m)->ppin)
#define	sputwakeup(m, c)	swakeup((m)->ppout, (c))

EXPORT int
fifo_amount()
{
	return (FIFO_AMOUNT(mp));
}


/*---------------------------------------------------------------------------
|
| wait until at least amount bytes my be put into the fifo
|
+---------------------------------------------------------------------------*/
EXPORT int
fifo_iwait(amount)
	int	amount;
{
	register int	cnt;
	register m_head *rmp = mp;

	while ((cnt = rmp->size - FIFO_AMOUNT(rmp)) < amount) {
		if (rmp->flags & FIFO_MERROR) {
			fifo_stats();
			exit(1);
		}
		rmp->full++;
		rmp->flags |= FIFO_IBLOCKED;
		EDEBUG(("i"));
		sputwait(rmp);
	}
	if (cnt > rmp->ibs)
		cnt = rmp->ibs;
	if ((rmp->end - rmp->putptr) < cnt) {
		EDEBUG(("at end: cnt: %d max: %d\n",
						cnt, rmp->end - rmp->putptr));
		cnt = rmp->end - rmp->putptr;
	}
	{
		/* Temporary until all modules know about mp->xxx */
		extern char *bigptr;

		bigptr = rmp->putptr;
	}
	return (cnt);
}


/*---------------------------------------------------------------------------
|
| add amount bytes to putcount and wake up get side if necessary
|
+---------------------------------------------------------------------------*/
EXPORT void
fifo_owake(amount)
	int	amount;
{
	register m_head *rmp = mp;

	if (amount == 0)
		return;
	rmp->puts++;
	rmp->putptr += amount;
	rmp->icnt += amount;
	if (rmp->putptr >= rmp->end)
		rmp->putptr = rmp->base;

	if ((rmp->flags & FIFO_OBLOCKED) &&
			((rmp->flags & FIFO_IWAIT) ||
					(FIFO_AMOUNT(rmp) >= rmp->low))) {
		rmp->flags &= ~FIFO_OBLOCKED;
		EDEBUG(("d"));
		sgetwakeup(rmp, 'd');
	}
	if ((rmp->flags & FIFO_IWAIT)) {
		rmp->flags &= ~FIFO_IWAIT;

		EDEBUG(("I"));
		sputwait(rmp);
	}
}


/*---------------------------------------------------------------------------
|
| send EOF condition to get side
|
+---------------------------------------------------------------------------*/
EXPORT void
fifo_oflush()
{
	mp->flags |= FIFO_MEOF;
	if (mp->flags & FIFO_OBLOCKED) {
		mp->flags &= ~FIFO_OBLOCKED;
		EDEBUG(("e"));
		sgetwakeup(mp, 'e');
	}
}

/*---------------------------------------------------------------------------
|
| wait until at least obs bytes may be taken out of fifo
|
+---------------------------------------------------------------------------*/
EXPORT int
fifo_owait(amount)
	int	amount;
{
	int	c;
	register int	cnt;
	register m_head *rmp = mp;

again:
	cnt = FIFO_AMOUNT(rmp);
	if (cnt == 0 && (rmp->flags & FIFO_MEOF))
		return (cnt);

	if (cnt < amount && (rmp->flags & (FIFO_MEOF|FIFO_O_CHREEL)) == 0) {
		rmp->empty++;
		rmp->flags |= FIFO_OBLOCKED;
		EDEBUG(("o"));
		c = sgetwait(rmp);
		cnt = FIFO_AMOUNT(rmp);
	}
	if (cnt == 0 && (rmp->flags & FIFO_O_CHREEL)) {
		pid = 1;
		changetape();
		pid = 0;
		rmp->flags &= ~FIFO_O_CHREEL;
		EDEBUG(("T"));
		sputwakeup(mp, 'T');
		goto again;
	}

	if (rmp->maxfill < cnt)
		rmp->maxfill = cnt;

	if (cnt > rmp->obs)
		cnt = rmp->obs;

	c = rmp->end - rmp->getptr;
	if (cnt > c && c >= amount)
		cnt = c;

	if (rmp->getptr + cnt > rmp->end) {
		errmsgno(BAD, "getptr >: %X %X %d end: %X\n",
				rmp->getptr, &rmp->getptr[cnt], cnt, rmp->end);
	}
	{
		/* Temporary until all modules know about mp->xxx */
		extern char *bigptr;

		bigptr = rmp->getptr;
	}
	return (cnt);
}


/*---------------------------------------------------------------------------
|
| add amount bytes to getcount and wake up put side if necessary
|
+---------------------------------------------------------------------------*/
EXPORT void
fifo_iwake(amt)
	int	amt;
{
	register m_head *rmp = mp;

	if (amt == 0) {
		rmp->flags |= FIFO_MERROR;
		exit(1);
	}

	rmp->gets++;
	rmp->getptr += amt;
	rmp->ocnt += amt;

	if (rmp->getptr >= rmp->end)
		rmp->getptr = rmp->base;

	if ((FIFO_AMOUNT(rmp) <= rmp->hiw) && (rmp->flags & FIFO_IBLOCKED)) {
		rmp->flags &= ~FIFO_IBLOCKED;
		EDEBUG(("s"));
		sputwakeup(rmp, 's');
	}
}

EXPORT void
fifo_resume()
{
	EDEBUG(("S"));
	sputwakeup(mp, 'S');
}

EXPORT void
fifo_sync()
{
	register m_head *rmp = mp;
		 int	rest = rmp->obs - FIFO_AMOUNT(rmp)%rmp->obs;

	fifo_iwait(rest);
	fillbytes(rmp->putptr, rest, '\0');
	fifo_owake(rest);
}

EXPORT void
fifo_chtape()
{
	char	c;

	mp->flags |= FIFO_O_CHREEL;
	if (mp->flags & FIFO_OBLOCKED) {
		mp->flags &= ~FIFO_OBLOCKED;
		EDEBUG(("N"));
		sgetwakeup(mp, 'N');
	}
	EDEBUG(("W"));
	c = sputwait(mp);
}

LOCAL void
do_in()
{
	int	amt;
	int	cnt;

	do {
		cnt = fifo_iwait(mp->ibs);

		amt = readtape(mp->putptr, cnt);

		fifo_owake(amt);
	} while (amt > 0);

	fifo_oflush();
	exit(0);
}

LOCAL void
do_out()
{
	int	cnt;
	int	amt;

	for (;;) {
		cnt = fifo_owait(mp->obs);
		if (cnt == 0)
			break;

		amt = writetape(mp->getptr, cnt);

		if (amt < 0)
			comerr("write error getptr: %X, cnt: %d %X\n",
					mp->getptr, cnt, &mp->getptr[cnt]);
		if (amt < cnt)
			error("wrote: %d (%d)\n", amt, cnt);

		fifo_iwake(amt);
	}
	exit(0);
}

#ifdef	USE_MMAP
LOCAL char *
mkshare(size)
	int	size;
{
	int	f;
	char	*addr;

#ifdef	MAP_ANONYMOUS	/* HP/UX */
	f = -1;
	addr = mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, f, 0);
#else
	if ((f = open("/dev/zero", O_RDWR)) < 0)
		comerr("Cannot open '/dev/zero'.\n");
	addr = mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, f, 0);
#endif
	if (addr == (char *)-1)
		comerr("Cannot get mmap for %d Bytes on /dev/zero.\n", size);
	close(f);

	if (debug) errmsgno(BAD, "shared memory segment attached: %x\n", addr);

	return (addr);
}
#endif

#ifdef	USE_SHM
#include <sys/ipc.h>
#include <sys/shm.h>
LOCAL char *
mkshm(size)
	int	size;
{
	int	id;
	char	*addr;
	/*
	 * Unfortunately, a declaration of shmat() is missing in old
	 * implementations such as AT&T SVr0 and SunOS.
	 * We cannot add this definition here because the return-type
	 * changed on newer systems.
	 *
	 * We will get a warning like this:
	 *
	 * warning: assignment of pointer from integer lacks a cast
	 * or
	 * warning: illegal combination of pointer and integer, op =
	 */
/*	extern	char *shmat();*/

	if ((id = shmget(IPC_PRIVATE, size, IPC_CREAT|0600)) == -1)
		comerr("shmget failed\n");

	if (debug) errmsgno(BAD, "shared memory segment allocated: %d\n", id);

	if ((addr = shmat(id, (char *)0, 0600)) == (char *)-1)
		comerr("shmat failed\n");

	if (debug) errmsgno(BAD, "shared memory segment attached: %x\n", addr);

	if (shmctl(id, IPC_RMID, 0) < 0)
		comerr("shmctl failed\n");

	return (addr);
}
#endif

#endif	/* FIFO */
