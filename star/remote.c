#define	USE_REMOTE
#ifdef	USE_REMOTE
/* @(#)remote.c	1.12 96/06/26 Copyright 1990 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)remote.c	1.12 96/06/26 Copyright 1990 J. Schilling";
#endif
/*
 *	Remote tape interface
 *
 *	Copyright (c) 1990 J. Schilling
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
#include <sys/types.h>
#ifdef	HAVE_SYS_MTIO_H
#include <sys/mtio.h>
#else
#include "mtio.h"
#endif
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <pwd.h>
#include <standard.h>
#include <stdxlib.h>
#include <unixstd.h>
#include <strdefs.h>

#if	defined(SIGDEFER) || defined(SVR4)
#define	signal	sigset
#endif

#define	CMD_SIZE	80
#define	BAD		(-1)

int	debug;

static	int	rmtsock;
static	char	*rmtpeer;
static	char	rmtuser[64];

LOCAL	void	rmtabrt			__PR((int sig));
EXPORT	int	rmtgetconn		__PR((char* host, int size));
EXPORT	int	rmtopen			__PR((int fd, char* fname, int fmode));
EXPORT	int	rmtclose		__PR((int fd));
EXPORT	int	rmtread			__PR((int fd, char* buf, int count));
EXPORT	int	rmtwrite		__PR((int fd, char* buf, int count));
EXPORT	int	rmtseek			__PR((int fd, long offset, int whence));
EXPORT	int	rmtioctl		__PR((int fd, int cmd, int count));
LOCAL	int	rmtxstatus		__PR((int fd, int cmd));
LOCAL	struct	mtget* rmt_v1_status	__PR((int fd));
EXPORT	struct	mtget* rmtstatus	__PR((int fd));
LOCAL	int	rmtcmd			__PR((int fd, char* name, char* cbuf));
LOCAL	void	rmtsendcmd		__PR((int fd, char* name, char* cbuf));
LOCAL	int	rmtgetline		__PR((int fd, char* line, int count));
LOCAL	int	rmtgetstatus		__PR((int fd, char* name));
LOCAL	int	rmtaborted		__PR((int fd));

LOCAL void
rmtabrt(sig)
	int	sig;
{
	rmtaborted(-1);
}

EXPORT int
rmtgetconn(host, size)
	char	*host;
	int	size;
{
	static	struct servent	*sp = 0;
	static	struct passwd	*pw = 0;
		char		*name = "root";
		char		*p;

	signal(SIGPIPE, rmtabrt);
	if (sp == 0) {
		sp = getservbyname("shell", "tcp");
		if (sp == 0) {
			comerrno(BAD, "shell/tcp: unknown service\n");
			/* NOTREACHED */
		}
		pw = getpwuid(getuid());
		if (pw == 0) {
			comerrno(BAD, "who are you? No passwd entry found.\n");
			/* NOTREACHED */
		}
	}
	if ((p = strchr(host, '@')) != NULL) {
		strncpy(rmtuser, host, p - host);
		name = rmtuser;
		host = &p[1];
	} else {
		name = pw->pw_name;
	}
	if (debug)
		errmsgno(BAD, "locuser: '%s' rmtuser: '%s' host: '%s'\n",
						pw->pw_name, name, host);
	rmtpeer = host;
	rmtsock = rcmd(&rmtpeer, (unsigned short)sp->s_port,
					pw->pw_name, name, "/etc/rmt", 0);
/*					pw->pw_name, name, "/etc/xrmt", 0);*/

	if (rmtsock < 0)
		return (-1);


#ifdef	SO_SNDBUF
	while (size > 512 &&
		setsockopt(rmtsock, SOL_SOCKET, SO_SNDBUF,
					(char *)&size, sizeof (size)) < 0) {
		size -= 512;
	}
	if (debug)
		errmsgno(BAD, "sndsize: %d\n", size);
#endif
#ifdef	SO_RCVBUF
	while (size > 512 &&
		setsockopt(rmtsock, SOL_SOCKET, SO_RCVBUF,
					(char *)&size, sizeof (size)) < 0) {
		size -= 512;
	}
	if (debug)
		errmsgno(BAD, "rcvsize: %d\n", size);
#endif

	return (rmtsock);
}

EXPORT int
rmtopen(fd, fname, fmode)
	int	fd;
	char	*fname;
	int	fmode;
{
	char	cbuf[CMD_SIZE];

	sprintf(cbuf, "O%s\n%d\n", fname, fmode);
	return (rmtcmd(fd, "open", cbuf));
}

EXPORT int
rmtclose(fd)
	int	fd;
{
	return (rmtcmd(fd, "close", "C\n"));
}

EXPORT int
rmtread(fd, buf, count)
	int	fd;
	char	*buf;
	int	count;
{
	char	cbuf[CMD_SIZE];
	int	n;
	int	amt = 0;
	int	cnt;

	sprintf(cbuf, "R%d\n", count);
	n = rmtcmd(fd, "read", cbuf);
	if (n < 0)
		return (-1);

	while (amt < n) {
		if ((cnt = read(fd, &buf[amt], n - amt)) <= 0) {
			return (rmtaborted(fd));
		}
		amt += cnt;
	}

	return (amt);
}

EXPORT int
rmtwrite(fd, buf, count)
	int	fd;
	char	*buf;
	int	count;
{
	char	cbuf[CMD_SIZE];

	sprintf(cbuf, "W%d\n", count);
	rmtsendcmd(fd, "write", cbuf);
	write(fd, buf, count);
	return (rmtgetstatus(fd, "write"));
}

EXPORT int
rmtseek(fd, offset, whence)
	int	fd;
	long	offset;
	int	whence;
{
	char	cbuf[CMD_SIZE];

	sprintf(cbuf, "L%ld\n%d\n", offset, whence);
	return (rmtcmd(fd, "seek", cbuf));
}

/*
 * Definitions for the new RMT Protocol version 1
 *
 * The new Protocol version tries to make the use
 * of rmtioctl() more portable between different platforms.
 */
#define	RMTIVERSION	-1
#define	RMT_VERSION	1

/*
 * Support for commands bejond MTWEOF..MTNOP (0..7)
 */
#define	RMTICACHE	0
#define	RMTINOCACHE	1
#define	RMTIRETEN	2
#define	RMTIERASE	3
#define	RMTIEOM		4
#define	RMTINBSF	5

/*
 * Old MTIOCGET copies a binary version of struct mtget back
 * over the wire. This is highly non portable.
 * MTS_* retrieves ascii versions (%d format) of a single
 * field in the struct mtget.
 * NOTE: MTS_ERREG may only be valid on the first call and
 *	 must be retrived first.
 */
#define	MTS_TYPE	'T'		/* mtget.mt_type */
#define	MTS_DSREG	'D'		/* mtget.mt_dsreg */
#define	MTS_ERREG	'E'		/* mtget.mt_erreg */
#define	MTS_RESID	'R'		/* mtget.mt_resid */
#define	MTS_FILENO	'F'		/* mtget.mt_fileno */
#define	MTS_BLKNO	'B'		/* mtget.mt_blkno */
#define	MTS_FLAGS	'f'		/* mtget.mt_flags */
#define	MTS_BF		'b'		/* mtget.mt_bf */

EXPORT int
rmtioctl(fd, cmd, count)
	int	fd;
	int	cmd;
	int	count;
{
	char	cbuf[CMD_SIZE];
	char	c = 'I';

	if (cmd > 7 && (rmtioctl(fd, RMTIVERSION, 0) == RMT_VERSION)) {
		c = 'i';
		switch (cmd) {
#ifdef	MTRETEN
		case MTRETEN:	cmd = RMTIRETEN; break;
#endif
#ifdef	MTERASE
		case MTERASE:	cmd = RMTIERASE; break;
#endif
#ifdef	MTEOM
		case MTEOM:	cmd = RMTIEOM;   break;
#endif
#ifdef	MTNBSF
		case MTNBSF:	cmd = RMTINBSF;  break;
#endif
		default:	c = 'I';
		}
	}
	sprintf(cbuf, "%c%d\n%d\n", c, cmd, count);
	return (rmtcmd(fd, "ioctl", cbuf));
}

static struct	mtget mts;

LOCAL int
rmtxstatus(fd, cmd)
	int	fd;
	char	cmd;
{
	char	cbuf[CMD_SIZE];

	sprintf(cbuf, "s%c", cmd);
	return (rmtcmd(fd, "extended status", cbuf));
}

LOCAL struct mtget *
rmt_v1_status(fd)
	int	fd;
{
	mts.mt_erreg  = rmtxstatus(fd, MTS_ERREG); /* must be first */
	mts.mt_type   = rmtxstatus(fd, MTS_TYPE);
#ifdef	HAVE_MTGET_DSREG	/* doch immer vorhanden ??? */
	mts.mt_dsreg  = rmtxstatus(fd, MTS_DSREG);
#endif
#ifdef	HAVE_MTGET_RESID
	mts.mt_resid  = rmtxstatus(fd, MTS_RESID);
#endif
#ifdef	HAVE_MTGET_FILENO
	mts.mt_fileno = rmtxstatus(fd, MTS_FILENO);
#endif
#ifdef	HAVE_MTGET_BLKNO
	mts.mt_blkno  = rmtxstatus(fd, MTS_BLKNO);
#endif
#if	defined(sun)
	mts.mt_flags  = rmtxstatus(fd, MTS_FLAGS);
	mts.mt_bf     = rmtxstatus(fd, MTS_BF);
#endif
	return (&mts);
}

EXPORT struct mtget *
rmtstatus(fd)
	int	fd;
{
	register int i;
	register char *cp;
	int	n;

	if (rmtioctl(fd, RMTIVERSION, 0) == RMT_VERSION)
		return (rmt_v1_status(fd));

	if ((n = rmtcmd(fd, "status", "S\n")) < 0)
		return (0);

	if (sizeof(mts) < n)
		n = sizeof(mts);

	for (i = 0, cp = (char *)&mts; i < sizeof(mts); i++)
		*cp++ = 0;
	for (i = 0, cp = (char *)&mts; i < n; i++)
		if (read(fd, cp++, 1) != 1)
			return ((struct mtget *)rmtaborted(fd));
	return (&mts);
}

LOCAL int
rmtcmd(fd, name, cbuf)
	int	fd;
	char	*name;
	char	*cbuf;
{
	rmtsendcmd(fd, name, cbuf);
	return (rmtgetstatus(fd, name));
}

LOCAL void
rmtsendcmd(fd, name, cbuf)
	int	fd;
	char	*name;
	char	*cbuf;
{
	int	buflen = strlen(cbuf);

	errno = 0;
	if (write(fd, cbuf, buflen) != buflen)
		rmtaborted(fd);
}

LOCAL int
rmtgetline(fd, line, count)
	int	fd;
	char	*line;
	int	count;
{
	register char	*cp;

	for (cp = line; cp < &line[count]; cp++) {
		if (read(fd, cp, 1) != 1)
			return (rmtaborted(fd));

		if (*cp == '\n') {
			*cp = '\0';
			return (cp - line);
		}
	}
	return (rmtaborted(fd));
}

LOCAL int
rmtgetstatus(fd, name)
	int	fd;
	char	*name;
{
	char	cbuf[CMD_SIZE];
	char	code;
	int	number;

	rmtgetline(fd, cbuf, sizeof(cbuf));
	code = cbuf[0];
	number = atoi(&cbuf[1]);

	if (code == 'E' || code == 'F') {
		rmtgetline(fd, cbuf, sizeof(cbuf));
		if (code == 'F')	/* should close file ??? */
			rmtaborted(fd);
		if (debug)
			errmsgno(number, "Remote status(%s): %d '%s'.\n",
							name, number, cbuf);
		errno = number;
		return (-1);
	}
	if (code != 'A') {
		/* XXX Hier kommt evt Command not found ... */
		if (debug)
			errmsgno(BAD, "Protocol error (got %s).\n", cbuf);
		return (rmtaborted(fd));
	}
	return (number);
}

LOCAL int
rmtaborted(fd)
	int	fd;
{
	if (debug)
		errmsgno(BAD, "Lost connection to remote host ??\n");
	/* if fd >= 0 */
	/* close file */
	if (errno == 0)
		errno = EIO;
	return (-1);
}
#endif	/* USE_REMOTE */
