/* @(#)longnames.c	1.23 00/11/09 Copyright 1993, 1995 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)longnames.c	1.23 00/11/09 Copyright 1993, 1995 J. Schilling";
#endif
/*
 *	Handle filenames that cannot fit into a single
 *	string of 100 charecters
 *
 *	Copyright (c) 1993, 1995 J. Schilling
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
#include "star.h"
#include "props.h"
#include "table.h"
#include <standard.h>
#include <strdefs.h>
#include <schily.h>
#include "starsubs.h"

typedef struct {
	char	*m_name;
	int	m_size;
	int	m_add;
} move_t;

LOCAL	void	enametoolong	__PR((char* name, int len, int maxlen));
LOCAL	char*	split_posix_name __PR((char* name, int namlen, int add));
EXPORT	BOOL	name_to_tcb	__PR((FINFO * info, TCB * ptb));
EXPORT	void	tcb_to_name	__PR((TCB * ptb, FINFO * info));
EXPORT	void	tcb_undo_split	__PR((TCB * ptb, FINFO * info));
LOCAL	int	move_from_name	__PR((move_t * move, char* p, int amount));
LOCAL	int	move_to_name	__PR((move_t * move, char* p, int amount));
EXPORT	int	tcb_to_longname	__PR((TCB * ptb, FINFO * info));
EXPORT	void	write_longnames	__PR((FINFO * info));
LOCAL	void	put_longname	__PR((FINFO * info,
					char* name, int namelen, char* tname,
							Ulong  xftype));

LOCAL void
enametoolong(name, len, maxlen)
	char	*name;
	int	len;
	int	maxlen;
{
	xstats.s_toolong++;
	errmsgno(EX_BAD, "%s: Name too long (%d > %d chars)\n",
							name, len, maxlen);
}


LOCAL char *
split_posix_name(name, namlen, add)
	char	*name;
	int	namlen;
	int	add;
{
	register char	*low;
	register char	*high;

	if (namlen+add > props.pr_maxprefix+1+props.pr_maxsname) {
		/*
		 * Cannot split
		 */
		if (props.pr_maxnamelen <= props.pr_maxsname) /* No longnames*/
			enametoolong(name, namlen+add,
				props.pr_maxprefix+1+props.pr_maxsname);
		return (NULL);
	}
	low = &name[namlen+add - props.pr_maxsname];
	if (--low < name)
		low = name;
	high = &name[props.pr_maxprefix>namlen ? namlen:props.pr_maxprefix];

#ifdef	DEBUG
error("low: %d:%s high: %d:'%c',%s\n",
			strlen(low), low, strlen(high), *high, high);
#endif
	high++;
	while (--high >= low)
		if (*high == '/')
			break;
	if (high < low) {
		if (props.pr_maxnamelen <= props.pr_maxsname) {
			xstats.s_toolong++;
			errmsgno(EX_BAD, "%s: Name too long (cannot split)\n",
									name);
		}
		return (NULL);
	}
#ifdef	DEBUG
error("solved: add: %d prefix: %d suffix: %d\n",
			add, high-name, strlen(high+1)+add);
#endif
	return (high);
}

/*
 * Es ist sichergestelt, daß namelen >= 1 ist.
 */
EXPORT BOOL
name_to_tcb(info, ptb)
	FINFO	*info;
	TCB	*ptb;
{
	char	*name = info->f_name;
	int	namelen = info->f_namelen;
	int	add = 0;
	char	*np = NULL;

	if (namelen == 0)
		raisecond("name_to_tcb: namelen", 0L);

	if (is_dir(info) && name[namelen-1] != '/')
		add++;

	if ((namelen+add) <= props.pr_maxsname) {	/* Fits in shortname */
		if (add)
			strcatl(ptb->dbuf.t_name, name, "/", (char *)NULL);
		else
			strcpy(ptb->dbuf.t_name, name);
		return (TRUE);
	}

	if (props.pr_nflags & PR_POSIX_SPLIT)
		np = split_posix_name(name, namelen, add);
	if (np == NULL) {
		/*
		 * cannot split
		 */
		if (namelen+add <= props.pr_maxnamelen) {
			info->f_flags |= F_LONGNAME;
			if (add)
				info->f_flags |= F_ADDSLASH;
			strncpy(ptb->dbuf.t_name, name, props.pr_maxsname);
			return (TRUE);
		} else {
			enametoolong(name, namelen+add, props.pr_maxnamelen);
			return (FALSE);
		}
	}

	if (add)
		strcatl(ptb->dbuf.t_name, &np[1], "/", (char *)NULL);
	else
		strcpy(ptb->dbuf.t_name, &np[1]);
	strncpy(ptb->dbuf.t_prefix, name, np - name);
	info->f_flags |= F_SPLIT_NAME;
	return (TRUE);
}

EXPORT void
tcb_to_name(ptb, info)
	TCB	*ptb;
	FINFO	*info;
{
	/*
	 * Name has already been set up because it is a very long name or
	 * because it has been setup from somwhere else.
	 * We have nothing to do.
	 */
	if (info->f_flags & (F_LONGNAME|F_HAS_NAME))
		return;

	if (props.pr_nflags & PR_POSIX_SPLIT) {
		strcatl(info->f_name, ptb->dbuf.t_prefix,
						*ptb->dbuf.t_prefix?"/":"",
						ptb->dbuf.t_name, NULL);
	} else {
		strcpy(info->f_name, ptb->dbuf.t_name);
	}
}

EXPORT void
tcb_undo_split(ptb, info)
	TCB	*ptb;
	FINFO	*info;
{
	fillbytes(ptb->dbuf.t_name, NAMSIZ, '\0');
	fillbytes(ptb->dbuf.t_prefix, props.pr_maxprefix, '\0');

	info->f_flags &= ~F_SPLIT_NAME;
	info->f_flags |= F_LONGNAME;

	strncpy(ptb->dbuf.t_name, info->f_name, props.pr_maxsname);
}

#define	vp_move_from_name ((int(*)__PR((void *, char *, int)))move_from_name)

/*
 * Move name from archive.
 */
LOCAL int
move_from_name(move, p, amount)
	move_t	*move;
	char	*p;
	int	amount;
{
	movebytes(p, move->m_name, amount);
	move->m_name += amount;
	move->m_name[0] = '\0';
	return (amount);
}

#define	vp_move_to_name	((int(*)__PR((void *, char *, int)))move_to_name)

/*
 * Move name to archive.
 */
LOCAL int
move_to_name(move, p, amount)
	move_t	*move;
	char	*p;
	int	amount;
{
	if (amount > move->m_size)
		amount = move->m_size;
	movebytes(move->m_name, p, amount);
	move->m_name += amount;
	move->m_size -= amount;
	if (move->m_add) {
		if (move->m_size == 1) {
			p[amount-1] = '/';
		} else if (move->m_size == 0) {
			if (amount > 1)
				p[amount-2] = '/';
			p[amount-1] = '\0';
		}
	}
	return (amount);
}

/*
 * A bad idea to do this here!
 * We have to set up a more generalized pool of namebuffers wich are allocated
 * on an actual MAX_PATH base.
 */
char	longlinkname[PATH_MAX+1];

EXPORT int
tcb_to_longname(ptb, info)
	register TCB	*ptb;
	register FINFO	*info;
{
	move_t	move;

	/*
	 * File size is strlen of name
	 */
	astoo(ptb->dbuf.t_size, &info->f_size);
	info->f_rsize = info->f_size;
	if (info->f_size > PATH_MAX) {
		xstats.s_toolong++;
		errmsgno(EX_BAD, "Long name too long (%ld) ignored.\n",
							info->f_size);
		void_file(info);
		return (get_tcb(ptb));
	}
	if (ptb->dbuf.t_linkflag == LF_LONGNAME) {
		info->f_namelen = info->f_size -1;
		info->f_flags |= F_LONGNAME;
		move.m_name = info->f_name;
	} else {
		info->f_lname = longlinkname;
		info->f_lnamelen = info->f_size -1;
		info->f_flags |= F_LONGLINK;
		move.m_name = info->f_lname;
	}
	if (xt_file(info, vp_move_from_name, &move, 0, "moving long name") < 0)
		die(EX_BAD);

	return (get_tcb(ptb));
}

EXPORT void
write_longnames(info)
	register FINFO	*info;
{
	/*
	 * XXX Should test for F_LONGNAME & F_FLONGLINK
	 */
	if (info->f_namelen > props.pr_maxsname) {
		put_longname(info, info->f_name, info->f_namelen+1,
						"././@LongName", XT_LONGNAME);
	}
	if (info->f_lnamelen > props.pr_maxslname) {
		put_longname(info, info->f_lname, info->f_lnamelen+1,
						"././@LongLink", XT_LONGLINK);
	}
}

LOCAL void
put_longname(info, name, namelen, tname, xftype)
	FINFO	*info;
	char	*name;
	int	namelen;
	char	*tname;
	Ulong	xftype;
{
	FINFO	finfo;
	TCB	*ptb;
	move_t	move;

	fillbytes((char *)&finfo, sizeof(finfo), '\0');

	ptb = (TCB *)get_block();
	finfo.f_flags |= F_TCB_BUF;
	fillbytes((char *)ptb, TBLOCK, '\0');

	strcpy(ptb->dbuf.t_name, tname);

	move.m_add = 0;
	if ((info->f_flags & F_ADDSLASH) != 0 && xftype == XT_LONGNAME) {
		move.m_add = 1;
		namelen++;
	}
	finfo.f_rsize = finfo.f_size = namelen;
	finfo.f_xftype = xftype;
	info_to_tcb(&finfo, ptb);
	write_tcb(ptb, &finfo);

	move.m_name = name;
	move.m_size = finfo.f_size;
	cr_file(&finfo, vp_move_to_name, &move, 0, "moving long name");
}
