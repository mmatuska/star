/*#define	USE_REMOTE*/
#ifdef	USE_REMOTE
/* @(#)remote.c	1.22 00/11/12 Copyright 1990 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)remote.c	1.22 00/11/12 Copyright 1990 J. Schilling";
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
#include <fctldefs.h>
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
#include <schily.h>
#include "remote.h"

#if	defined(SIGDEFER) || defined(SVR4)
#define	signal	sigset
#endif

#define	CMD_SIZE	80

extern	BOOL	debug;

LOCAL	void	rmtabrt			__PR((int sig));
EXPORT	int	rmtgetconn		__PR((char* host, int size));
LOCAL	void	rmtoflags		__PR((int fmode, char *cmode));
EXPORT	int	rmtopen			__PR((int fd, char* fname, int fmode));
EXPORT	int	rmtclose		__PR((int fd));
EXPORT	int	rmtread			__PR((int fd, char* buf, int count));
EXPORT	int	rmtwrite		__PR((int fd, char* buf, int count));
EXPORT	int	rmtseek			__PR((int fd, long offset, int whence));
EXPORT	int	rmtioctl		__PR((int fd, int cmd, int count));
LOCAL	int	rmtmapold		__PR((int cmd));
LOCAL	int	rmtmapnew		__PR((int cmd));
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
		int		rmtsock;
		char		*rmtpeer;
		char		rmtuser[128];


	signal(SIGPIPE, rmtabrt);
	if (sp == 0) {
		sp = getservbyname("shell", "tcp");
		if (sp == 0) {
			comerrno(EX_BAD, "shell/tcp: unknown service\n");
			/* NOTREACHED */
		}
		pw = getpwuid(getuid());
		if (pw == 0) {
			comerrno(EX_BAD, "who are you? No passwd entry found.\n");
			/* NOTREACHED */
		}
	}
	if ((p = strchr(host, '@')) != NULL) {
		js_snprintf(rmtuser, sizeof(rmtuser), "%.*s", p - host, host);
		name = rmtuser;
		host = &p[1];
	} else {
		name = pw->pw_name;
	}
	if (debug)
		errmsgno(EX_BAD, "locuser: '%s' rmtuser: '%s' host: '%s'\n",
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
		errmsgno(EX_BAD, "sndsize: %d\n", size);
#endif
#ifdef	SO_RCVBUF
	while (size > 512 &&
		setsockopt(rmtsock, SOL_SOCKET, SO_RCVBUF,
					(char *)&size, sizeof (size)) < 0) {
		size -= 512;
	}
	if (debug)
		errmsgno(EX_BAD, "rcvsize: %d\n", size);
#endif

	return (rmtsock);
}

LOCAL void
rmtoflags(fmode, cmode)
	int	fmode;
	char	*cmode;
{
	register char	*p;
	register int	amt;
	register int	maxcnt = CMD_SIZE;

	switch (fmode & O_ACCMODE) {

	case O_RDONLY:	p = "O_RDONLY";	break;
	case O_RDWR:	p = "O_RDWR";	break;
	case O_WRONLY:	p = "O_WRONLY";	break;

	default:	p = "Cannot Happen";
	}
	amt = js_snprintf(cmode, maxcnt, p); if (amt < 0) return;
	p = cmode;
	p += amt;
	maxcnt -= amt;
#ifdef	O_TEXT
	if (fmode & O_TEXT) {
		amt = js_snprintf(p, maxcnt, "|O_TEXT"); if (amt < 0) return;
		p += amt;
		maxcnt -= amt;
	}
#endif
#ifdef	O_NDELAY
	if (fmode & O_NDELAY) {
		amt = js_snprintf(p, maxcnt, "|O_NDELAY"); if (amt < 0) return;
		p += amt;
		maxcnt -= amt;
	}
#endif
#ifdef	O_APPEND
	if (fmode & O_APPEND) {
		amt = js_snprintf(p, maxcnt, "|O_APPEND"); if (amt < 0) return;
		p += amt;
		maxcnt -= amt;
	}
#endif
#ifdef	O_SYNC
	if (fmode & O_SYNC) {
		amt = js_snprintf(p, maxcnt, "|O_SYNC"); if (amt < 0) return;
		p += amt;
		maxcnt -= amt;
	}
#endif
#ifdef	O_DSYNC
	if (fmode & O_DSYNC) {
		amt = js_snprintf(p, maxcnt, "|O_DSYNC"); if (amt < 0) return;
		p += amt;
		maxcnt -= amt;
	}
#endif
#ifdef	O_RSYNC
	if (fmode & O_RSYNC) {
		amt = js_snprintf(p, maxcnt, "|O_RSYNC"); if (amt < 0) return;
		p += amt;
		maxcnt -= amt;
	}
#endif
#ifdef	O_NONBLOCK
	if (fmode & O_NONBLOCK) {
		amt = js_snprintf(p, maxcnt, "|O_NONBLOCK"); if (amt < 0) return;
		p += amt;
		maxcnt -= amt;
	}
#endif
#ifdef	O_PRIV
	if (fmode & O_PRIV) {
		amt = js_snprintf(p, maxcnt, "|O_PRIV"); if (amt < 0) return;
		p += amt;
		maxcnt -= amt;
	}
#endif
#ifdef	O_LARGEFILE
	if (fmode & O_LARGEFILE) {
		amt = js_snprintf(p, maxcnt, "|O_LARGEFILE"); if (amt < 0) return;
		p += amt;
		maxcnt -= amt;
	}
#endif
#ifdef	O_CREAT
	if (fmode & O_CREAT) {
		amt = js_snprintf(p, maxcnt, "|O_CREAT"); if (amt < 0) return;
		p += amt;
		maxcnt -= amt;
	}
#endif
#ifdef	O_TRUNC
	if (fmode & O_TRUNC) {
		amt = js_snprintf(p, maxcnt, "|O_TRUNC"); if (amt < 0) return;
		p += amt;
		maxcnt -= amt;
	}
#endif
#ifdef	O_EXCL
	if (fmode & O_EXCL) {
		amt = js_snprintf(p, maxcnt, "|O_EXCL"); if (amt < 0) return;
		p += amt;
		maxcnt -= amt;
	}
#endif
#ifdef	O_NOCTTY
	if (fmode & O_NOCTTY) {
		amt = js_snprintf(p, maxcnt, "|O_NOCTTY"); if (amt < 0) return;
		p += amt;
		maxcnt -= amt;
	}
#endif
}

EXPORT int
rmtopen(fd, fname, fmode)
	int	fd;
	char	*fname;
	int	fmode;
{
	char	cbuf[CMD_SIZE];
	char	cmode[CMD_SIZE];
	int	ret;

	/*
	 * Convert all fmode bits into the symbolic fmode.
	 * only send the lowest 2 bits in numeric mode as it would be too
	 * dangerous because the apropriate bits differ between different
	 * operating systems.
	 */
	rmtoflags(fmode, cmode);
	js_snprintf(cbuf, CMD_SIZE, "O%s\n%d %s\n", fname, fmode & O_ACCMODE, cmode);
	ret = rmtcmd(fd, "open", cbuf);

	/*
	 * Tell the rmt server that we are aware of Version 1 commands.
	 */
/*	(void)rmtioctl(fd, RMTIVERSION, 0);*/
	(void)rmtioctl(fd, -1, 0);

	return (ret);
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

	js_snprintf(cbuf, CMD_SIZE, "R%d\n", count);
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

	js_snprintf(cbuf, CMD_SIZE, "W%d\n", count);
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

	js_snprintf(cbuf, CMD_SIZE, "L%ld\n%d\n", offset, whence);
	return (rmtcmd(fd, "seek", cbuf));
}

/*
 * Definitions for the new RMT Protocol version 1
 *
 * The new Protocol version tries to make the use
 * of rmtioctl() more portable between different platforms.
 */
#define	RMTIVERSION	-1
#define	RMT_NOVERSION	-1
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
	int	rmtversion = RMT_NOVERSION;
	int	i;

	if (cmd != RMTIVERSION)
		rmtversion = rmtioctl(fd, RMTIVERSION, 0);

	if (cmd >= 0 && (rmtversion == RMT_VERSION)) {
		/*
		 * Opcodes 0..7 are unique across different architectures.
		 * But as in many cases Linux does not even follow this rule.
		 * If we know that we are calling a VERSION 1 client, we may 
		 * safely assume that the client is not using Linux mapping
		 * but the standard mapping.
		 */
		i = rmtmapold(cmd);
		if (cmd <= 7 && i  < 0) {
			/*
			 * We cannot map the current command but it's value is
			 * within the range 0..7. Do not send it over the wire.
			 */
			errno = EINVAL;
			return (-1);
		}
		if (i >= 0)
			cmd = i;
	}
	if (cmd > 7 && (rmtversion == RMT_VERSION)) {
		i = rmtmapnew(cmd);
		if (i >= 0) {
			cmd = i;
			c = 'i';
		}
	}

	js_snprintf(cbuf, CMD_SIZE, "%c%d\n%d\n", c, cmd, count);
	return (rmtcmd(fd, "ioctl", cbuf));
}

/*
 * Map all old opcodes that should be in range 0..7 to numbers /etc/rmt expects
 * This is needed because Linux does not follow the UNIX conventions.
 */
LOCAL int
rmtmapold(cmd)
	int	cmd;
{
	switch (cmd) {

#ifdef	MTWEOF
	case  MTWEOF:	return (0);
#endif

#ifdef	MTFSF
	case MTFSF:	return (1);
#endif

#ifdef	MTBSF
	case MTBSF:	return (2);
#endif

#ifdef	MTFSR
	case MTFSR:	return (3);
#endif

#ifdef	MTBSR
	case MTBSR:	return (4);
#endif

#ifdef	MTREW
	case MTREW:	return (5);
#endif

#ifdef	MTOFFL
	case MTOFFL:	return (6);
#endif

#ifdef	MTNOP
	case MTNOP:	return (7);
#endif
	}
	return (-1);
}

/*
 * Map all new opcodes that should be in range above 7 to the 
 * values expected by the 'i' command of /etc/rmt.
 */
LOCAL int
rmtmapnew(cmd)
	int	cmd;
{
	switch (cmd) {

#ifdef	MTCACHE
	case MTCACHE:	return (RMTICACHE);
#endif

#ifdef	MTNOCACHE
	case MTNOCACHE:	return (RMTINOCACHE);
#endif

#ifdef	MTRETEN
	case MTRETEN:	return (RMTIRETEN);
#endif

#ifdef	MTERASE
	case MTERASE:	return (RMTIERASE);
#endif

#ifdef	MTEOM
	case MTEOM:	return (RMTIEOM);
#endif

#ifdef	MTNBSF
	case MTNBSF:	return (RMTINBSF);
#endif
	}
	return (-1);
}

static struct	mtget mts;

LOCAL int
rmtxstatus(fd, cmd)
	int	fd;
	char	cmd;
{
	char	cbuf[CMD_SIZE];

			/* No newline */
	js_snprintf(cbuf, CMD_SIZE, "s%c", cmd);
	return (rmtcmd(fd, "extended status", cbuf));
}

LOCAL struct mtget *
rmt_v1_status(fd)
	int	fd;
{
	mts.mt_erreg = mts.mt_type = 0;

#ifdef	HAVE_MTGET_ERREG
	mts.mt_erreg  = rmtxstatus(fd, MTS_ERREG); /* must be first */
#endif
#ifdef	HAVE_MTGET_TYPE
	mts.mt_type   = rmtxstatus(fd, MTS_TYPE);
#endif
	if (mts.mt_erreg == -1 || mts.mt_type == -1)
		return (0);

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
#ifdef	HAVE_MTGET_FLAGS
	mts.mt_flags  = rmtxstatus(fd, MTS_FLAGS);
#endif
#ifdef	HAVE_MTGET_BF
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
	char	c;
	int	n;

	if (rmtioctl(fd, RMTIVERSION, 0) == RMT_VERSION)
		return (rmt_v1_status(fd));

				/* No newline */
	if ((n = rmtcmd(fd, "status", "S")) < 0)
		return (0);

	for (i = 0, cp = (char *)&mts; i < sizeof(mts); i++)
		*cp++ = 0;
	for (i = 0, cp = (char *)&mts; i < n; i++) {
		/*
		 * Make sure to read all bytes because we otherwise
		 * would confuse the protocol. Do not copy more
		 * than the size of our local struct mtget.
		 */
		if (read(fd, &c, 1) != 1) {
			rmtaborted(fd);
			return (0);
		}
		if (i < sizeof(mts))
			*cp++ = c;
	}
	/*
	 * The GNU remote tape lib tries to swap the structure based on the
	 * value of mt_type. While this makes sense for UNIX, it will not
	 * work if one system is running Linux. The Linux mtget structure
	 * is completely incompatible (mt_type is long instead of short).
	 */
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
			errmsgno(EX_BAD, "Protocol error (got %s).\n", cbuf);
		return (rmtaborted(fd));
	}
	return (number);
}

LOCAL int
rmtaborted(fd)
	int	fd;
{
	if (debug)
		errmsgno(EX_BAD, "Lost connection to remote host ??\n");
	/* if fd >= 0 */
	/* close file */
	if (errno == 0)
		errno = EIO;
	return (-1);
}
#endif	/* USE_REMOTE */
