/* @(#)starsubs.h	1.1 96/06/13 Copyright 1996 J. Schilling */
/*
 *	Prototypes for star subroutines
 *
 *	Copyright (c) 1996 J. Schilling
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

/*
 * star.c
 */
extern	int	main	__PR((int ac, char** av));
extern	BOOL	match	__PR((const char* name));

/*
 * buffer.c
 */
extern	BOOL	openremote	__PR((void));
extern	void	opentape	__PR((void));
extern	void	closetape	__PR((void));
extern	void	changetape	__PR((void));
extern	void	nexttape	__PR((void));
extern	void	initbuf		__PR((int nblocks));
extern	void	syncbuf		__PR((void));
extern	int	readblock	__PR((char* buf));
extern	int	readtape	__PR((char* buf, int amount));
extern	void	writeblock	__PR((char* buf));
extern	int	writetape	__PR((char* buf, int amount));
extern	void	writeempty	__PR((void));
extern	void	weof		__PR((void));
extern	void	buf_sync	__PR((void));
extern	void	buf_drain	__PR((void));
extern	int	buf_wait	__PR((int amount));
extern	void	buf_wake	__PR((int amount));
extern	int	buf_rwait	__PR((int amount));
extern	void	buf_rwake	__PR((int amount));
extern	void	buf_resume	__PR((void));
extern	int	tblocks		__PR((void));
extern	void	prstats		__PR((void));
extern	void	exprstats	__PR((int ret));
extern	void	excomerrno	__PR((int err, char* fmt, ...));
extern	void	excomerr	__PR((char* fmt, ...));

/*
 * append.c
 */
extern	void	skipall	__PR((void));

/*
 * create.c
 */
extern	void	checklinks	__PR((void));
extern	void	create	__PR((register char* name));
extern	void	createlist	__PR((void));
extern	BOOL	read_symlink	__PR((char* name, register FINFO * info, TCB * ptb));
#ifdef	EOF
extern	void	put_file	__PR((register FILE * f, register FINFO * info));
#endif
extern	void	cr_file		__PR((FINFO * info, int (*)(void *, char *, int), register void *arg, int amt, char* text));

/*
 * diff.c
 */
extern	void	diff		__PR((void));
#ifdef	EOF
extern	void	prdiffopts	__PR((FILE * f, char* label, int flags));
#endif

/*
 * dirtime.c
 */
extern	void	sdirtimes	__PR((char* name, FINFO* info));
/*extern	void	dirtimes	__PR((char* name, struct timeval* tp));*/

/*
 * extract.c
 */
extern	void	extract		__PR((void));
extern	BOOL	newer		__PR((FINFO * info));
extern	BOOL	void_file	__PR((FINFO * info));
extern	BOOL	xt_file		__PR((FINFO * info, int (*)(void *, char *, int), void *arg, int amt, char* text));
extern	void	skip_slash	__PR((FINFO * info));

/*
 * fifo.c
 */
extern	void	initfifo	__PR((void));
extern	void	fifo_ibs_shrink	__PR((int newsize));
extern	void	runfifo		__PR((void));
extern	void	fifo_stats	__PR((void));
extern	int	fifo_amount	__PR((void));
extern	int	fifo_iwait	__PR((int amount));
extern	void	fifo_owake	__PR((int amount));
extern	void	fifo_oflush	__PR((void));
extern	int	fifo_owait	__PR((int amount));
extern	void	fifo_iwake	__PR((int amt));
extern	void	fifo_resume	__PR((void));
extern	void	fifo_sync	__PR((void));
extern	void	fifo_chtape	__PR((void));

/*
 * header.c
 */
extern	int	get_tcb		__PR((TCB * ptb));
extern	void	put_tcb		__PR((TCB * ptb, register FINFO * info));
extern	void	write_tcb	__PR((TCB * ptb, register FINFO * info));
extern	void	put_volhdr	__PR((char* name));
extern	void	get_volhdr	__PR((FINFO * info));
extern	void	info_to_tcb	__PR((register FINFO * info, register TCB * ptb));
extern	int	tcb_to_info	__PR((register TCB * ptb, register FINFO * info));
extern	BOOL	ia_change	__PR((TCB * ptb, FINFO * info));
extern	void	astoo_cpio	__PR((register char* s, Ulong * l, register int cnt));
extern	void	astoo		__PR((register char* s, Ulong * l));
extern	void	otoa		__PR((char* s, register Ulong  l, register int fieldw));

/*
 * hole.c
 */
#ifdef	EOF
extern	BOOL	get_forced_hole	__PR((FILE * f, FINFO * info));
extern	BOOL	get_sparse	__PR((FILE * f, FINFO * info));
extern	BOOL	cmp_sparse	__PR((FILE * f, FINFO * info));
extern	void	put_sparse	__PR((FILE * f, FINFO * info));
#endif
extern	int	gnu_skip_extended	__PR((TCB * ptb));

/*
 * lhash.c
 */
#ifdef	EOF
extern	void	hash_build	__PR((FILE * fp, unsigned int size));
#endif
extern	BOOL	hash_lookup	__PR((char* str));

/*
 * list.c
 */
extern	void	list	__PR((void));
extern	void	list_file __PR((register FINFO * info));
extern	void	vprint	__PR((FINFO * info));

/*
 * longnames.c
 */
extern	BOOL	name_to_tcb	__PR((FINFO * info, TCB * ptb));
extern	void	tcb_to_name	__PR((TCB * ptb, FINFO * info));
extern	void	tcb_undo_split	__PR((TCB * ptb, FINFO * info));
extern	int	tcb_to_longname	__PR((register TCB * ptb, register FINFO * info));
extern	void	write_longnames	__PR((register FINFO * info));

/*
 * names.c
 */
extern	BOOL	nameuid	__PR((char* name, int namelen, Ulong uid));
extern	BOOL	uidname	__PR((char* name, int namelen, Ulong* uidp));
extern	BOOL	namegid	__PR((char* name, int namelen, Ulong gid));
extern	BOOL 	gidname	__PR((char* name, int namelen, Ulong* gidp));

/*
 * props.c
 */
extern	void	setprops	__PR((long htype));
extern	void	printprops	__PR((void));

/*
 * remote.c
 */
extern	int	rmtgetconn	__PR((char* host, int size));
extern	int	rmtopen		__PR((int fd, char* fname, int fmode));
extern	int	rmtclose	__PR((int fd));
extern	int	rmtread		__PR((int fd, char* buf, int count));
extern	int	rmtwrite	__PR((int fd, char* buf, int count));
extern	int	rmtseek		__PR((int fd, long offset, int whence));
extern	int	rmtioctl	__PR((int fd, int cmd, int count));
extern	struct	mtget* rmtstatus	__PR((int fd));

/*
 * star_unix.c
 */
extern	BOOL	getinfo		__PR((char* name, FINFO * info));
extern	void	setmodes	__PR((FINFO * info));
extern	int	sxsymlink	__PR((FINFO * info));
#ifdef	EOF
extern	int	rs_acctime	__PR((FILE * f, FINFO * info));
#endif
