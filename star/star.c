/* @(#)star.c	1.80 01/04/14 Copyright 1985, 88-90, 92-96, 98, 99, 2000-2001 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)star.c	1.80 01/04/14 Copyright 1985, 88-90, 92-96, 98, 99, 2000-2001 J. Schilling";
#endif
/*
 *	Copyright (c) 1985, 88-90, 92-96, 98, 99, 2000-2001 J. Schilling
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
#include <signal.h>
#include <stdxlib.h>
#include <unixstd.h>
#include <strdefs.h>
#include "star.h"
#include "diff.h"
#include <waitdefs.h>
#include <standard.h>
#include <patmatch.h>
#define	__XDEV__	/* Needed to activate _dev_init() */
#include <device.h>
#include <getargs.h>
#include <schily.h>
#include "starsubs.h"
#include "fifo.h"

EXPORT	int	main		__PR((int ac, char** av));
LOCAL	void	getdir		__PR((int *acp, char *const **avp,
						const char **dirp));
LOCAL	char	*dogetwdir	__PR((void));
LOCAL	BOOL	dochdir		__PR((const char *dir, BOOL doexit));
LOCAL	void	openlist	__PR((void));
LOCAL	void	usage		__PR((int ret));
LOCAL	void	xusage		__PR((int ret));
LOCAL	void	dusage		__PR((int ret));
LOCAL	void	husage		__PR((int ret));
LOCAL	void	gargs		__PR((int ac, char *const* av));
LOCAL	long	number		__PR((char* arg, int* retp));
LOCAL	int	getnum		__PR((char* arg, long* valp));
EXPORT	const char *filename	__PR((const char *name));
LOCAL	BOOL	nameprefix	__PR((const char *patp, const char *name));
LOCAL	int	namefound	__PR((const char* name));
EXPORT	BOOL	match		__PR((const char* name));
LOCAL	int	addpattern	__PR((const char* pattern));
LOCAL	int	addarg		__PR((const char* pattern));
LOCAL	void	closepattern	__PR((void));
LOCAL	void	printpattern	__PR((void));
LOCAL	int	add_diffopt	__PR((char* optstr, long* flagp));
LOCAL	int	gethdr		__PR((char* optstr, long* typep));
#ifdef	USED
LOCAL	int	addfile		__PR((char* optstr, long* dummy));
#endif
LOCAL	void	exsig		__PR((int sig));
LOCAL	void	sighup		__PR((int sig));
LOCAL	void	sigintr		__PR((int sig));
LOCAL	void	sigquit		__PR((int sig));
LOCAL	void	getstamp	__PR((void));
EXPORT	void	*__malloc	__PR((unsigned int size));
EXPORT	char	*__savestr	__PR((char *s));
LOCAL	void	docompat	__PR((int *pac, char *const **pav));

#if	defined(SIGDEFER) || defined(SVR4)
#define	signal	sigset
#endif

#define	QIC_24_TSIZE	122880		/*  61440 kBytes */
#define	QIC_120_TSIZE	256000		/* 128000 kBytes */
#define	QIC_150_TSIZE	307200		/* 153600 kBytes */
#define	QIC_250_TSIZE	512000		/* 256000 kBytes (XXX not verified)*/
#define	TSIZE(s)	((s)*TBLOCK)

#define	SECOND		(1)
#define	MINUTE		(60 * SECOND)
#define	HOUR		(60 * MINUTE)
#define DAY		(24 * HOUR)
#define YEAR		(365 * DAY)
#define LEAPYEAR	(366 * DAY)

char	strvers[] = "1.3";

struct star_stats	xstats;

#define	NPAT	100

EXPORT	BOOL		havepat = FALSE;
LOCAL	int		npat	= 0;
LOCAL	int		narg	= 0;
LOCAL	int		maxplen	= 0;
LOCAL	int		*aux[NPAT];
LOCAL	int		alt[NPAT];
LOCAL	int		*state;
const	unsigned char	*pat[NPAT];
const		char	*dirs[NPAT];

FILE	*tarf;
FILE	*listf;
FILE	*tty;
FILE	*vpr;
char	*tarfile;
char	*listfile;
char	*stampfile;
const	char	*wdir;
const	char	*currdir;
const	char	*dir_flags = NULL;
char	*volhdr;
long	tape_dev;
long	tape_ino;
#ifdef	FIFO
BOOL	use_fifo = TRUE;
#else
BOOL	use_fifo = FALSE;
#endif
BOOL	shmflag	= FALSE;
long	fs;
long	bs;
int	nblocks = 20;
int	uid;
Ulong	curfs;
long	hdrtype	= H_XSTAR;	/* default header format */
long	chdrtype= H_UNDEF;	/* command line hdrtype	 */
int	version	= 0;
int	swapflg	= -1;
BOOL	debug	= FALSE;
BOOL	showtime= FALSE;
BOOL	no_stats= FALSE;
BOOL	do_fifostats= FALSE;
BOOL	numeric	= FALSE;
BOOL	verbose = FALSE;
BOOL	tpath	= FALSE;
BOOL	cflag	= FALSE;
BOOL	uflag	= FALSE;
BOOL	rflag	= FALSE;
BOOL	xflag	= FALSE;
BOOL	tflag	= FALSE;
BOOL	nflag	= FALSE;
BOOL	diff_flag = FALSE;
BOOL	zflag	= FALSE;
BOOL	bzflag	= FALSE;
BOOL	multblk	= FALSE;
BOOL	ignoreerr = FALSE;
BOOL	nodir	= FALSE;
BOOL	nomtime	= FALSE;
BOOL	nochown	= FALSE;
BOOL	acctime	= FALSE;
BOOL	dirmode	= FALSE;
BOOL	nolinkerr = FALSE;
BOOL	follow	= FALSE;
BOOL	nodesc	= FALSE;
BOOL	nomount	= FALSE;
BOOL	interactive = FALSE;
BOOL	signedcksum = FALSE;
BOOL	partial	= FALSE;
BOOL	nospec	= FALSE;
int	Fflag	= 0;
BOOL	uncond	= FALSE;
BOOL	xdir	= FALSE;
BOOL	keep_old= FALSE;
BOOL	refresh_old= FALSE;
BOOL	abs_path= FALSE;
BOOL	notpat	= FALSE;
BOOL	force_hole = FALSE;
BOOL	sparse	= FALSE;
BOOL	to_stdout = FALSE;
BOOL	wready = FALSE;
BOOL	force_remove = FALSE;
BOOL	ask_remove = FALSE;
BOOL	remove_first = FALSE;
BOOL	remove_recursive = FALSE;
BOOL	nullout = FALSE;

Ulong	maxsize	= 0L;
Ulong	Newer	= 0L;
Ulong	tsize	= 0L;
long	diffopts= 0L;
BOOL	nowarn	= FALSE;
BOOL	Ctime	= FALSE;

BOOL	listnew	= FALSE;
BOOL	listnewf= FALSE;
BOOL	hpdev	= FALSE;
BOOL	modebits= FALSE;
BOOL	copylinks= FALSE;

BOOL	fcompat	= FALSE;

int	intr	= 0;

char	*opts = "C*,help,xhelp,version,debug,time,no_statistics,no-statistics,fifostats,numeric,v,tpath,c,u,r,x,t,n,diff,diffopts&,H&,force_hole,force-hole,sparse,to_stdout,to-stdout,wready,force_remove,force-remove,ask_remove,ask-remove,remove_first,remove-first,remove_recursive,remove-recursive,nullout,onull,fifo,no_fifo,no-fifo,shm,fs&,VOLHDR*,list*,file*,f*,T,bs&,blocks#,b#,z,bz,B,pattern&,pat&,i,d,m,nochown,a,atime,p,l,L,D,dodesc,M,I,w,O,signed_checksum,signed-checksum,P,S,F+,U,xdir,k,keep_old_files,keep-old-files,refresh_old_files,refresh-old-files,refresh,/,not,V,maxsize#L,newer*,ctime,tsize#L,qic24,qic120,qic150,qic250,nowarn,newest_file,newest-file,newest,hpdev,modebits,copylinks";

EXPORT int
main(ac, av)
	int	ac;
	char	**av;
{
	int		cac  = ac;
	char *const	*cav = av;

	save_args(ac, av);

	if (ac > 1 && av[1][0] != '-')
		docompat(&cac, &cav);

	gargs(cac, cav);
	--cac,cav++;

	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
		(void) signal(SIGHUP, sighup);
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		(void) signal(SIGINT, sigintr);
	if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		(void) signal(SIGQUIT, sigquit);

	file_raise((FILE *)NULL, FALSE);

	initbuf(nblocks);

	(void)openremote();		/* This needs super user privilleges */

	if (geteuid() != getuid()) {	/* AIX does not like to do this */
					/* If we are not root		*/
#ifdef	HAVE_SETREUID
		if (setreuid(-1, getuid()) < 0)
#else
#ifdef	HAVE_SETEUID
		if (seteuid(getuid()) < 0)
#else
		if (setuid(getuid()) < 0)
#endif
#endif
			comerr("Panic cannot set back efective uid.\n");
	}

	opentape();

	uid = geteuid();

	if (stampfile)
		getstamp();

	setprops(chdrtype);	/* Set up properties for archive format */
	dev_init(debug);	/* Init device macro handling */

#ifdef	FIFO
	if (use_fifo)
		runfifo();
#endif

	if (dir_flags)
		wdir = dogetwdir();

	if (xflag || tflag || diff_flag) {
		if (listfile) {
			openlist();
			hash_build(listf, 1000);
			if((currdir = dir_flags) != NULL)
				dochdir(currdir, TRUE);
		} else {
			for (;;--cac,cav++) {
				if (dir_flags)
					getdir(&cac, &cav, &currdir);
				if (getfiles(&cac, &cav, opts) == 0)
					break;
				addarg(cav[0]);
			}
			closepattern();
		}
		if (tflag) {
			list();
		} else {
			/*
			 * xflag || diff_flag
			 * First change dir to the one or last -C arg
			 * in case there is no pattern in list.
			 */
			if((currdir = dir_flags) != NULL)
				dochdir(currdir, TRUE);
			if (xflag)
				extract(volhdr);
			else
				diff();
		}
	}
	closepattern();
	if (uflag || rflag) {
		skipall();
		syncbuf();
		backtape();
	}
	if (cflag) {
		put_volhdr(volhdr);
		if (listfile) {
			openlist();
			if((currdir = dir_flags) != NULL)
				dochdir(currdir, TRUE);
			createlist();
		} else {
			const char	*cdir = NULL;

			for (;;--cac,cav++) {
				if (dir_flags)
					getdir(&cac, &cav, &currdir);
				if (currdir && cdir != currdir) {
					if (!(dochdir(wdir, FALSE) &&
					      dochdir(currdir, FALSE)))
						break;
					cdir = currdir;
				}

				if (getfiles(&cac, &cav, opts) == 0)
					break;
				if (intr)
					break;
				curfs = -1L;
				create(cav[0]);
			}
		}
		weof();
		buf_drain();
	}

	if (!nolinkerr)
		checklinks();
	if (!use_fifo)
		closetape();

	while (wait(0) >= 0)
		;
	prstats();
	if (checkerrs()) {
		if (!nowarn && !no_stats) {
			errmsgno(EX_BAD,
			"Processed all possible files, despite earlier errors.\n");
		}
		exit(-2);
	}
	exit(0);
	/* NOTREACHED */
	return(0);	/* keep lint happy */
}

LOCAL void
getdir(acp, avp, dirp)
	int		*acp;
	char *const	**avp;
	const char	**dirp;
{
	/*
	 * Skip all other flags.
	 */
	getfiles(acp, avp, &opts[3]);

	if (debug) /* temporary */
		errmsgno(EX_BAD, "Flag/File: '%s'.\n", *avp[0]);

	if (getargs(acp, avp, "C*", dirp) < 0) {
		/*
		 * Skip all other flags.
		 */
		if (getfiles(acp, avp, &opts[3]) < 0) {
			errmsgno(EX_BAD, "Badly placed Option: %s.\n", *avp[0]);
			usage(EX_BAD);
		}
	}
	if (debug) /* temporary */
		errmsgno(EX_BAD, "Dir: '%s'.\n", *dirp);
}

#include <dirdefs.h>
#include <maxpath.h>
#include <getcwd.h>

LOCAL char *
dogetwdir()
{
	char	dir[PATH_MAX+1];
	char	*ndir;

/* XXX MAXPATHNAME vs. PATH_MAX ??? */

	if (getcwd(dir, PATH_MAX) == NULL)
		comerr("Cannot get working directory\n");
	ndir = malloc(strlen(dir)+1);
	if (ndir == NULL)
		comerr("Cannot alloc space for working dir.\n");
	strcpy(ndir, dir);
	return (ndir);
}

LOCAL BOOL
dochdir(dir, doexit)
	const char	*dir;
	BOOL		doexit;
{
	if (debug) /* temporary */
		error("dochdir(%s) = ", dir);

	if (chdir(dir) < 0) {
		int	ex = geterrno();

		if (debug) /* temporary */
			error("%d\n", ex);

		errmsg("Cannot change directory to '%s'.\n", dir);
		if (doexit)
			exit(ex);
		return (FALSE);
	}
	if (debug) /* temporary */
		error("%d\n", 0);

	return (TRUE);
}

LOCAL void
openlist()
{
	if (streql(listfile, "-")) {
		listf = stdin;
		listfile = "stdin";
	} else if ((listf = fileopen(listfile, "r")) == (FILE *)NULL)
		comerr("Cannot open '%s'.\n", listfile);
}

LOCAL void
usage(ret)
	int	ret;
{
	error("Usage:\tstar cmd [options] file1 ... filen\n");
	error("Cmd:\n");
	error("\t-c/-u/-r\tcreate/update/replace named files to tape\n");
	error("\t-x/-t/-n\textract/list/trace named files from tape\n");
	error("\t-diff\t\tdiff archive against file system (see -xhelp)\n");
	error("Options:\n");
	error("\t-help\t\tprint this help\n");
	error("\t-xhelp\t\tprint extended help\n");
	error("\tblocks=#,b=#\tset blocking factor to #x512 Bytes (default 20)\n"); 
	error("\tfile=nm,f=nm\tuse 'nm' as tape instead of stdin/stdout\n");
	error("\t-T\t\tuse $TAPE as tape instead of stdin/stdout\n");
#ifdef	FIFO
	error("\t-fifo/-no-fifo\tuse/don't use a fifo to optimize data flow from/to tape\n");
#if defined(USE_MMAP) && defined(USE_USGSHM)
	error("\t-shm\t\tuse SysV shared memory for fifo\n");
#endif
#endif
	error("\t-v\t\tbe verbose\n");
	error("\t-tpath\t\tuse with -t to list path names only\n");
	error("\tH=header\tgenerate 'header' type archive (see H=help)\n");
	error("\tC=dir\t\tperform a chdir to 'dir' before storing next file\n");
	error("\t-z\t\tpipe input/output through gzip, does not work on tapes\n");
	error("\t-bz\t\tpipe input/output through bzip2, does not work on tapes\n");
	error("\t-B\t\tperform multiple reads (needed on pipes)\n");
	error("\t-i\t\tignore checksum errors\n");
	error("\t-d\t\tdo not store/create directories\n");
	error("\t-m\t\tdo not restore access and modification time\n");
	error("\t-nochown\tdo not restore owner and group\n");
	error("\t-a,-atime\treset access time after storing file\n");
	error("\t-p\t\trestore filemodes of directories\n");
	error("\t-l\t\tdo not print a message if not all links are dumped\n");
	error("\t-L\t\tfollow symbolic links as if they were files\n");
	error("\t-D\t\tdo not descend directories\n");
	error("\t-M\t\tdo not descend mounting points\n");
	error("\t-I,-w\t\tdo interactive creation/extraction/renaming\n");
	error("\t-O\t\tbe compatible to old tar (except for checksum bug)\n");
	error("\t-P\t\tlast record may be partial (useful on cartridge tapes)\n");
	error("\t-S\t\tdo not store/create special files\n");
	error("\t-F,-FF,-FFF,...\tdo not store/create SCCS/RCS, core and object files\n");
	error("\t-U\t\trestore files unconditionally\n");
	exit(ret);
	/* NOTREACHED */
}

LOCAL void
xusage(ret)
	int	ret;
{
	error("Usage:\tstar cmd [options] file1 ... filen\n");
	error("Extended options:\n");
	error("\tdiffopts=optlst\tcomma separated list of diffopts (see diffopts=help)\n");
	error("\t-not,-V\t\tuse those files which do not match pattern\n");
	error("\tVOLHDR=name\tuse name to generate a volume header\n");
	error("\t-xdir\t\textract dir even if the current is never\n");
	error("\t-keep-old-files,-k\tkeep existing files\n");
	error("\t-refresh-old-files\trefresh existing files, don't create new files\n");
	error("\t-/\t\tdon't strip leading '/'s from file names\n");
	error("\tlist=name\tread filenames from named file\n");
	error("\t-dodesc\t\tdo descend directories found in a list= file\n");
	error("\tpattern=p,pat=p\tset matching pattern\n");
	error("\tmaxsize=#\tdo not store file if it is bigger than # kBytes\n");
	error("\tnewer=name\tstore only files which are newer than 'name'\n");
	error("\t-ctime\t\tuse ctime for newer= option\n");
	error("\tbs=#\t\tset (output) block size to #\n");
#ifdef	FIFO
	error("\tfs=#\t\tset fifo size to #\n");
#endif
	error("\ttsize=#\t\tset tape volume size to # 512 byte blocks\n");
	error("\t-qic24\t\tset tape volume size to %d kBytes\n",
						TSIZE(QIC_24_TSIZE)/1024);
	error("\t-qic120\t\tset tape volume size to %d kBytes\n",
						TSIZE(QIC_120_TSIZE)/1024);
	error("\t-qic150\t\tset tape volume size to %d kBytes\n",
						TSIZE(QIC_150_TSIZE)/1024);
	error("\t-qic250\t\tset tape volume size to %d kBytes\n",
						TSIZE(QIC_250_TSIZE)/1024);
	error("\t-nowarn\t\tdo not print warning messages\n");
	error("\t-time\t\tprint timing info\n");
	error("\t-no-statistics\tdo not print statistics\n");
#ifdef	FIFO
	error("\t-fifostats\tprint fifo statistics\n");
#endif
	error("\t-numeric\tdon't use user/group name from tape\n");
	error("\t-newest\t\tfind newest file on tape\n");
	error("\t-newest-file\tfind newest regular file on tape\n");
	error("\t-hpdev\t\tuse HP's non POSIX compliant method to store dev numbers\n");
	error("\t-modebits\tinclude all 16 bits from stat.st_mode, this violates POSIX-1003.1\n");
	error("\t-copylinks\tCopy hard and symlinks rather than linking\n");
	error("\t-signed-checksum\tuse signed chars to calculate checksum\n");
	error("\t-sparse\t\thandle file with holes effectively on store/create\n");
	error("\t-force-hole\ttry to extract all files with holes\n");
	error("\t-to-stdout\textract files to stdout\n");
	error("\t-wready\t\twait for tape drive to become ready\n");
	error("\t-force-remove\tforce to remove non writable files on extraction\n");
	error("\t-ask-remove\task to remove non writable files on extraction\n");
	error("\t-remove-first\tremove files before extraction\n");
	error("\t-remove-recursive\tremove files recursive\n");
	error("\t-onull,-nullout\tsimulate creating an achive to compute the size\n");
	exit(ret);
	/* NOTREACHED */
}

LOCAL void
dusage(ret)
	int	ret;
{
	error("Diff options:\n");
	error("\tnot\t\tif this option is present, exclude listed options\n");
	error("\tperm\t\tcompare file permissions\n");
	error("\tmode\t\tcompare file permissions\n");
	error("\ttype\t\tcompare file type\n");
	error("\tnlink\t\tcompare linkcount (not supported)\n");
	error("\tuid\t\tcompare owner of file\n");
	error("\tgid\t\tcompare group of file\n");
	error("\tuname\t\tcompare name of owner of file\n");
	error("\tgname\t\tcompare name of group of file\n");
	error("\tid\t\tcompare owner, group, ownername and groupname of file\n");
	error("\tsize\t\tcompare file size\n");
	error("\tdata\t\tcompare content of file\n");
	error("\tcont\t\tcompare content of file\n");
	error("\trdev\t\tcompare rdev of device node\n");
	error("\thardlink\tcompare target of hardlink\n");
	error("\tsymlink\t\tcompare target of symlink\n");
	error("\tatime\t\tcompare access time of file (only star)\n");
	error("\tmtime\t\tcompare modification time of file\n");
	error("\tctime\t\tcompare creation time of file (only star)\n");
	error("\ttimes\t\tcompare all times of file\n");
	exit(ret);
	/* NOTREACHED */
}

LOCAL void
husage(ret)
	int	ret;
{
	error("Header types:\n");
	error("\ttar\t\told tar format\n");
	error("\tstar\t\tstar format\n");
	error("\tgnutar\t\tgnu tar format\n");
	error("\tustar\t\tstandard tar (ieee 1003.1) format\n");
	error("\txstar\t\textended standard tar format\n");
	error("\txustar\t\textended standard tar format without tar signature\n");
	exit(ret);
	/* NOTREACHED */
}

LOCAL void
gargs(ac, av)
	int		ac;
	char	*const *av;
{
	BOOL	help	= FALSE;
	BOOL	xhelp	= FALSE;
	BOOL	prvers	= FALSE;
	BOOL	oldtar	= FALSE;
	BOOL	no_fifo	= FALSE;
	BOOL	usetape	= FALSE;
	BOOL	dodesc	= FALSE;
	BOOL	qic24	= FALSE;
	BOOL	qic120	= FALSE;
	BOOL	qic150	= FALSE;
	BOOL	qic250	= FALSE;
	const	char	*p;

/*char	*opts = "C*,help,xhelp,version,debug,time,no_statistics,no-statistics,fifostats,numeric,v,tpath,c,u,r,x,t,n,diff,diffopts&,H&,force_hole,force-hole,sparse,to_stdout,to-stdout,wready,force_remove,force-remove,ask_remove,ask-remove,remove_first,remove-first,remove_recursive,remove-recursive,nullout,onull,fifo,no_fifo,no-fifo,shm,fs&,VOLHDR*,list*,file*,f*,T,bs&,blocks#,b#,z,bz,B,pattern&,pat&,i,d,m,nochown,a,atime,p,l,L,D,dodesc,M,I,w,O,signed_checksum,signed-checksum,P,S,F+,U,xdir,k,keep_old_files,keep-old-files,refresh_old_files,refresh-old-files,refresh,/,not,V,maxsize#L,newer*,ctime,tsize#L,qic24,qic120,qic150,qic250,nowarn,newest_file,newest-file,newest,hpdev,modebits,copylinks";*/

	p = filename(av[0]);
	if (streql(p, "ustar")) {
		hdrtype = H_USTAR;
	}
	--ac,++av;
	if (getallargs(&ac, &av, opts,
				&dir_flags,
				&help, &xhelp, &prvers, &debug,
				&showtime, &no_stats, &no_stats, &do_fifostats,
				&numeric, &verbose, &tpath,
#ifndef	lint
				&cflag,
				&uflag,
				&rflag,
				&xflag,
				&tflag,
				&nflag,
				&diff_flag, add_diffopt, &diffopts,
				gethdr, &chdrtype,
				&force_hole, &force_hole, &sparse, &to_stdout, &to_stdout, &wready,
				&force_remove, &force_remove, &ask_remove, &ask_remove,
				&remove_first, &remove_first, &remove_recursive, &remove_recursive,
				&nullout, &nullout,
				&use_fifo, &no_fifo, &no_fifo, &shmflag,
				getnum, &fs,
				&volhdr,
				&listfile,
				&tarfile, &tarfile,
				&usetape,
				getnum, &bs,
				&nblocks, &nblocks,
				&zflag, &bzflag, &multblk,
				addpattern, NULL,
				addpattern, NULL,
				&ignoreerr,
				&nodir,
				&nomtime, &nochown,
				&acctime, &acctime,
				&dirmode,
				&nolinkerr,
				&follow,
				&nodesc,
				&dodesc,
				&nomount,
				&interactive, &interactive,
				&oldtar, &signedcksum, &signedcksum,
				&partial,
				&nospec, &Fflag,
				&uncond, &xdir,
				&keep_old, &keep_old, &keep_old,
				&refresh_old, &refresh_old, &refresh_old,
				&abs_path,
				&notpat, &notpat,
				&maxsize,
				&stampfile,
				&Ctime,
				&tsize,
				&qic24,
				&qic120,
				&qic150,
				&qic250,
				&nowarn,
#endif /* lint */
				&listnewf, &listnewf,
				&listnew,
				&hpdev, &modebits, &copylinks) < 0){
		errmsgno(EX_BAD, "Bad Option: %s.\n", av[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (xhelp)
		xusage(0);
	if (prvers) {
		printf("star %s (%s-%s-%s)\n\n", strvers, HOST_CPU, HOST_VENDOR, HOST_OS);
		printf("Copyright (C) 1985, 88-90, 92-96, 98, 99, 2000-2001 Jörg Schilling\n");
		printf("This is free software; see the source for copying conditions.  There is NO\n");
		printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
		exit(0);
	}

	if ((xflag + cflag + uflag + rflag + tflag + nflag + diff_flag) > 1) {
		errmsgno(EX_BAD, "Only one of -x -c -u -r -t or -n.\n");
		usage(EX_BAD);
	}
	if (!(xflag | cflag | uflag | rflag | tflag | nflag | diff_flag)) {
		errmsgno(EX_BAD, "Must specify -x -c -u -r -t -n -diff.\n");
		usage(EX_BAD);
	}
	if (uflag || rflag) {
		cflag = TRUE;
		no_fifo = TRUE;	/* Until we are able to reverse the FIFO */
	}
	if (nullout && !cflag) {
		errmsgno(EX_BAD, "-nullout only make sense in create mode.\n");
		usage(EX_BAD);
	}
	if (no_fifo || nullout)
		use_fifo = FALSE;
#ifndef	FIFO
	if (use_fifo) {
		errmsgno(EX_BAD, "Fifo not configured in.\n");
		usage(EX_BAD);
	}
#endif
	if (oldtar)
		chdrtype = H_OTAR;
	if (chdrtype != H_UNDEF) {
		if (H_TYPE(chdrtype) == H_OTAR)
			oldtar = TRUE;	/* XXX hack */
	}
	if (cflag) {
		if (chdrtype != H_UNDEF)
			hdrtype = chdrtype;
		chdrtype = hdrtype;	/* wegen setprops in main() */

		/*
		 * hdrtype und chdrtype
		 * bei uflag, rflag sowie xflag, tflag, nflag, diff_flag
		 * in get_tcb vergleichen !
		 */
	}
	if (diff_flag) {
		if (diffopts == 0)
			diffopts = D_DEFLT;
	} else if (diffopts != 0) {
		errmsgno(EX_BAD, "diffopts= only makes sense with -diff\n");
		usage(EX_BAD);
	}
	if (fs == 0L) {
		char	*ep = getenv("STAR_FIFO_SIZE");

		if (ep) {
			if (getnum(ep, &fs) != 1)
				comerr("Bad fifo size environment '%s'.\n",
									ep);
		}
	}
	if (bs % TBLOCK) {
		errmsgno(EX_BAD, "Invalid blocksize %ld.\n", bs);
		usage(EX_BAD);
	}
	if (bs)
		nblocks = bs / TBLOCK;
	if (nblocks <= 0) {
		errmsgno(EX_BAD, "Invalid blocksize %d blocks.\n", nblocks);
		usage(EX_BAD);
	}
	bs = nblocks * TBLOCK;
	if (tsize > 0 && tsize < 3) {
		errmsgno(EX_BAD, "Tape size must be at least 3 blocks.\n");
		usage(EX_BAD);
	}
	if (tsize == 0) {
		if (qic24)  tsize = QIC_24_TSIZE;
		if (qic120) tsize = QIC_120_TSIZE;
		if (qic150) tsize = QIC_150_TSIZE;
		if (qic250) tsize = QIC_250_TSIZE;
	}
	if (listfile != NULL && !dodesc)
		nodesc = TRUE;
	if (oldtar)
		nospec = TRUE;
	if (!tarfile) {
		if (usetape) {
			tarfile = getenv("TAPE");
		}
		if (!tarfile)
			tarfile = "-";
	}
	if (interactive || ask_remove || tsize > 0) {
#ifdef	JOS
		tty = stderr;
#else
		if ((tty = fileopen("/dev/tty", "r")) == (FILE *)NULL)
			comerr("Cannot open '/dev/tty'.\n");
#endif
	}
	if (nflag) {
		xflag = TRUE;
		interactive = TRUE;
		verbose = TRUE;
	}
	if (to_stdout) {
		force_hole = FALSE;
	}
	if (remove_recursive)
		comerrno(EX_BAD, "-remove_recursive not implemented\n");
	if (keep_old && refresh_old)
		comerrno(EX_BAD, "Cannot use -keep_old_files and -refresh_old_files together.\n");
}

LOCAL long
number(arg, retp)
	register char	*arg;
		int	*retp;
{
	long	val	= 0;

	if (*retp != 1)
		return (val);
	if (*arg == '\0')
		*retp = -1;
	else if (*(arg = astol(arg, &val))) {
		if (*arg == 'm' || *arg == 'M') {
			val *= (1024*1024);
			arg++;
		}
		else if (*arg == 'k' || *arg == 'K') {
			val *= 1024;
			arg++;
		}
		else if (*arg == 'b' || *arg == 'B') {
			val *= TBLOCK;
			arg++;
		}
		else if (*arg == 'w' || *arg == 'W') {
			val *= 2;
			arg++;
		}
		if (*arg == '*' || *arg == 'x')
			val *= number(++arg, retp);
		else if (*arg != '\0')
			*retp = -1;
	}
	return (val);
}

LOCAL int
getnum(arg, valp)
	char	*arg;
	long	*valp;
{
	int	ret = 1;

	*valp = number(arg, &ret);
	return (ret);
}

EXPORT const char *
filename(name)
	const char	*name;
{
	char	*p;

	if ((p = strrchr(name, '/')) == NULL)
		return (name);
	return (++p);
}

LOCAL BOOL
nameprefix(patp, name)
	register const char	*patp;
	register const char	*name;
{
	while (*patp) {
		if (*patp++ != *name++)
			return (FALSE);
	}
	if (*name) {
		return (*name == '/');	/* Directory tree match	*/
	}
	return (TRUE);			/* Names are equal	*/
}

LOCAL int
namefound(name)
	const	char	*name;
{
	register int	i;

	for (i=npat; i < narg; i++) {
		if (nameprefix((const char *)pat[i], name)) {
			return (i);
		}
	}
	return (-1);
}

EXPORT BOOL
match(name)
	const	char	*name;
{
	register int	i;
		char	*ret = NULL;

	if (!cflag && narg > 0) {
		if ((i = namefound(name)) < 0)
			return (FALSE);
		if (npat == 0)
			goto found;
	}

	for (i=0; i < npat; i++) {
		ret = (char *)patmatch(pat[i], aux[i],
					(const unsigned char *)name, 0,
					strlen(name), alt[i], state);
		if (ret != NULL && *ret == '\0')
			break;
	}
	if (notpat ^ (ret != NULL && *ret == '\0')) {
found:
		if (!(xflag || diff_flag))	/* Chdir only on -x or -diff */
			return (TRUE);
		if (dirs[i] != NULL && currdir != dirs[i]) {
			currdir = dirs[i];
			dochdir(wdir, TRUE);
			dochdir(currdir, TRUE);
		}
		return TRUE;
	}
	return FALSE;
}

LOCAL int
addpattern(pattern)
	const char	*pattern;
{
	int	plen;

/*	if (debug)*/
/*		error("Add pattern '%s'.\n", pattern);*/

	if (npat >= NPAT)
		comerrno(EX_BAD, "Too many patterns (max is %d).\n", NPAT);
	plen = strlen(pattern);
	pat[npat] = (const unsigned char *)pattern;

	if (plen > maxplen)
		maxplen = plen;

	if ((aux[npat] = malloc(plen*sizeof(int))) == NULL)
		comerr("Cannot alloc space for compiled pattern.\n");
	if ((alt[npat] = patcompile((const unsigned char *)pattern,
							plen, aux[npat])) == 0)
		comerrno(EX_BAD, "Bad pattern: '%s'.\n", pattern);
	dirs[npat] = currdir;
	npat++;
	return (TRUE);
}

LOCAL int
addarg(pattern)
	const char	*pattern;
{
	if (narg == 0)
		narg = npat;

/*	if (debug)*/
/*		error("Add arg '%s'.\n", pattern);*/

	if (narg >= NPAT)
		comerrno(EX_BAD, "Too many patterns (max is %d).\n", NPAT);

	pat[narg] = (const unsigned char *)pattern;
	dirs[narg] = currdir;
	narg++;
	return (TRUE);
}

/*
 * Close pattern list: insert useful default directories.
 */
LOCAL void
closepattern()
{
	register int	i;

	if (debug) /* temporary */
		printpattern();

	for (i=0; i < npat; i++) {
		if (dirs[i] != NULL)
			break;
	}
	while (--i >= 0)
		dirs[i] = wdir;

	if (debug) /* temporary */
		printpattern();

	if (npat > 0 || narg > 0)
		havepat = TRUE;

	if (npat > 0) {
		if ((state = malloc((maxplen+1)*sizeof(int))) == NULL)
			comerr("Cannot alloc space for pattern state.\n");
	}
}

LOCAL void
printpattern()
{
	register int	i;

	error("npat: %d narg: %d\n", npat, narg);
	for (i=0; i < npat; i++) {
		error("pat %s dir %s\n", pat[i], dirs[i]);
	}
	for (i=npat; i < narg; i++) {
		error("arg %s dir %s\n", pat[i], dirs[i]);
	}
}

LOCAL int
add_diffopt(optstr, flagp)
	char	*optstr;
	long	*flagp;
{
	char	*ep;
	char	*np;
	int	optlen;
	long	optflags = 0;
	BOOL	not = FALSE;

	while (*optstr) {
		if ((ep = strchr(optstr, ',')) != NULL) {
			optlen = ep - optstr;
			np = &ep[1];
		} else {
			optlen = strlen(optstr);
			np = &optstr[optlen];
		}
		if (optstr[0] == '!') {
			optstr++;
			optlen--;
			not = TRUE;
		}
		if (strncmp(optstr, "not", optlen) == 0 ||
				strncmp(optstr, "!", optlen) == 0) {
			not = TRUE;
		} else if (strncmp(optstr, "all", optlen) == 0) {
			optflags |= D_ALL;
		} else if (strncmp(optstr, "perm", optlen) == 0) {
			optflags |= D_PERM;
		} else if (strncmp(optstr, "mode", optlen) == 0) {
			optflags |= D_PERM;
		} else if (strncmp(optstr, "type", optlen) == 0) {
			optflags |= D_TYPE;
		} else if (strncmp(optstr, "nlink", optlen) == 0) {
			optflags |= D_NLINK;
			errmsgno(EX_BAD, "nlink not supported\n");
			dusage(EX_BAD);
		} else if (strncmp(optstr, "uid", optlen) == 0) {
			optflags |= D_UID;
		} else if (strncmp(optstr, "gid", optlen) == 0) {
			optflags |= D_GID;
		} else if (strncmp(optstr, "uname", optlen) == 0) {
			optflags |= D_UNAME;
		} else if (strncmp(optstr, "gname", optlen) == 0) {
			optflags |= D_GNAME;
		} else if (strncmp(optstr, "id", optlen) == 0) {
			optflags |= D_ID;
		} else if (strncmp(optstr, "size", optlen) == 0) {
			optflags |= D_SIZE;
		} else if (strncmp(optstr, "data", optlen) == 0) {
			optflags |= D_DATA;
		} else if (strncmp(optstr, "cont", optlen) == 0) {
			optflags |= D_DATA;
		} else if (strncmp(optstr, "rdev", optlen) == 0) {
			optflags |= D_RDEV;
		} else if (strncmp(optstr, "hardlink", optlen) == 0) {
			optflags |= D_HLINK;
		} else if (strncmp(optstr, "symlink", optlen) == 0) {
			optflags |= D_SLINK;
		} else if (strncmp(optstr, "sparse", optlen) == 0) {
			optflags |= D_SPARS;
		} else if (strncmp(optstr, "atime", optlen) == 0) {
			optflags |= D_ATIME;
		} else if (strncmp(optstr, "mtime", optlen) == 0) {
			optflags |= D_MTIME;
		} else if (strncmp(optstr, "ctime", optlen) == 0) {
			optflags |= D_CTIME;
		} else if (strncmp(optstr, "times", optlen) == 0) {
			optflags |= D_TIMES;
		} else if (strncmp(optstr, "help", optlen) == 0) {
			dusage(0);
		} else {
			error("Illegal diffopt.\n");
			dusage(EX_BAD);
			return (-1);
		}
		optstr = np;
	}
	if (not) {
		*flagp = ~optflags;
	} else {
		*flagp = optflags;
	}
	return (TRUE);
}

LOCAL int
gethdr(optstr, typep)
	char	*optstr;
	long	*typep;
{
	BOOL	swapped = FALSE;
	long	type	= H_UNDEF;

	if (*optstr == 'S') {
		swapped = TRUE;
		optstr++;
	}
	if (streql(optstr, "tar")) {
		type = H_OTAR;
	} else if (streql(optstr, "star")) {
		type = H_STAR;
	} else if (streql(optstr, "gnutar")) {
		type = H_GNUTAR;
	} else if (streql(optstr, "ustar")) {
		type = H_USTAR;
	} else if (streql(optstr, "xstar")) {
		type = H_XSTAR;
	} else if (streql(optstr, "xustar")) {
		type = H_XUSTAR;
	} else if (streql(optstr, "help")) {
		husage(0);
	} else {
		error("Illegal header type '%s'.\n", optstr);
		husage(EX_BAD);
		return (-1);
	}
	if (swapped)
		*typep = H_SWAPPED(type);
	else
		*typep = type;
	return (TRUE);
}

#ifdef	USED
/*
 * Add archive file.
 * May currently not be activated:
 *	If the option string ends with ",&", the -C option will not work
 *	anymore.
 */
LOCAL int
addfile(optstr, dummy)
	char	*optstr;
	long	*dummy;
{
	char	*p;

/*	error("got_it: %s\n", optstr);*/

	if (!strchr("01234567", optstr[0]))
		return (NOTAFILE);/* Tell getargs that this may be a flag */

	for (p = &optstr[1]; *p; p++) {
		if (*p != 'l' && *p != 'm' && *p != 'h')
			return (BADFLAG);
	}
/*	error("is_tape: %s\n", optstr);*/

	comerrno(EX_BAD, "Options [0-7][lmh] currently not supported.\n");
	/*
	 * The tape device should be determined from the defaults file
	 * in the near future.
	 * Search for /etc/opt/schily/star, /etc/default/star, /etc/default/tar
	 */

	return (1);		/* Success */
}
#endif

LOCAL void
exsig(sig)
	int	sig;
{
	signal(sig, SIG_DFL);
	kill(getpid(), sig);
}

/* ARGSUSED */
LOCAL void
sighup(sig)
	int	sig;
{
	signal(SIGHUP, sighup);
	prstats();
	intr++;
	if (!cflag)
		exsig(sig);
}

/* ARGSUSED */
LOCAL void
sigintr(sig)
	int	sig;
{
	signal(SIGINT, sigintr);
	prstats();
	intr++;
	if (!cflag)
		exsig(sig);
}

/* ARGSUSED */
LOCAL void
sigquit(sig)
	int	sig;
{
	signal(SIGQUIT, sigquit);
	prstats();
}

LOCAL void
getstamp()
{
	FINFO	finfo;
	BOOL	ofollow = follow;

	follow = TRUE;
	if (!getinfo(stampfile, &finfo))
		comerr("Cannot stat '%s'.\n", stampfile);
	follow = ofollow;

	Newer = finfo.f_mtime;
}

EXPORT void *
__malloc(size)
	unsigned int size;
{
	void	*ret;

	ret = malloc(size);
	if (ret == NULL) {
		comerr("No memory.\n");
		/* NOTREACHED */
	}
	return (ret);
}

EXPORT char *
__savestr(s)
	char	*s;
{
	char	*ret = __malloc(strlen(s)+1);

	strcpy(ret, s);
	return (ret);
}

/*
 * Convert old tar type syntax into the new UNIX option syntax.
 * Allow only a limited subset of the single character options to avoid
 * collisions between interpretation of options in different
 * tar implementations. The old syntax has a risk to damage files
 * which is avoided with the 'fcompat' flag (see opentape()).
 *
 * Problems:
 *	The 'e' and 'X' option are currently not implemented.
 *	The 'h' option can only be implemented if the -help 
 *	shortcut in star is removed.
 *	There is a collision between the BSD -I (include) and
 *	star's -I (interactive) which may be solved by using -w instead.
 */
LOCAL void
docompat(pac, pav)
	int	*pac;
	char	*const **pav;
{
	int	ac	= *pac;
	char	*const *av	= *pav;
	int	nac;
	char	**nav;
	char	nopt[3];
	char	*copt = "crtuxbfXBFTLdeiklmnopvwz01234567";
	char	*p;
	char	c;
	char	*const *oa;
	char	**na;

	if (strchr(av[1], '=') != NULL)		/* Do not try to convert bs= */
		return;

	nac = ac + strlen(av[1]);
	nav = __malloc(nac-- * sizeof(char *));	/* keep space for NULL ptr */
	oa = av;
	na = nav;
	*na++ = *oa++;
	oa++;					/* Skip over av[1] */

	nopt[0] = '-';
	nopt[2] = '\0';

	for (p=av[1]; (c = *p) != '\0'; p++) {
		if (strchr(copt, c) == NULL) {
			errmsgno(EX_BAD, "Illegal option '%c' for compat mode.\n", c);
			usage(EX_BAD);
		}
		nopt[1] = c;
		*na++ = __savestr(nopt);
		if (c == 'f' || c == 'b' || c == 'X') {
			*na++ = *oa++;
			/*
			 * The old syntax has a high risk of corrupting
			 * files if the user disorders the args.
			 */
			if (c == 'f')
				fcompat = TRUE;
		}
	}

	/*
	 * Now copy over the rest...
	 */
	while ((av + ac) > oa)
		*na++ = *oa++;
	*na = NULL;

	*pac = nac;
	*pav = nav;
}
