/* @(#)names.c	1.6 97/05/29 Copyright 1993 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)names.c	1.6 97/05/29 Copyright 1993 J. Schilling";
#endif
/*
 *	Handle user/group names for archive hheader
 *
 *	Copyright (c) 1993 J. Schilling
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
#include <standard.h>
#include "star.h"
#include <pwd.h>
#include <grp.h>
#include <strdefs.h>
#include "starsubs.h"

/*#define	C_SIZE	10*/

#define TUNMLEN	32
#define TGNMLEN	32

LOCAL	Ulong	lastuid;
LOCAL	char	lastuname[TUNMLEN];
LOCAL	int	uvalid;

LOCAL	Ulong	lastgid;
LOCAL	char	lastgname[TGNMLEN];
LOCAL	int	gvalid;

EXPORT	BOOL	nameuid	__PR((char* name, int namelen, Ulong uid));
EXPORT	BOOL	uidname	__PR((char* name, int namelen, Ulong* uidp));
EXPORT	BOOL	namegid	__PR((char* name, int namelen, Ulong gid));
EXPORT	BOOL 	gidname	__PR((char* name, int namelen, Ulong* gidp));

/*
 * Get name from uid
 */
EXPORT BOOL
nameuid(name, namelen, uid)
	char	*name;
	int	namelen;
	Ulong	uid;
{
	struct passwd	*pw;

	if (uvalid <= 0 || uid != lastuid) {
		lastuid = uid;
		lastuname[0] = '\0';

		if ((pw = getpwuid(uid)) != NULL) {
			strncpy(lastuname, pw->pw_name, TUNMLEN);
			lastuname[namelen-1] = 0;
		}
		uvalid = 1;	/* force not to look again for invalid uid */
	}
	strcpy(name, lastuname);
	return (name[0] != '\0');
}

/*
 * Get uid from name
 */
EXPORT BOOL
uidname(name, namelen, uidp)
	char	*name;
	int	namelen;
	Ulong	*uidp;
{
	struct passwd	*pw;
	char	uname[TUNMLEN+1];
	int	len = namelen>TUNMLEN?TUNMLEN:namelen;

	if (name[0] == '\0')
		return (FALSE);

	if (!uvalid || name[0] != lastuname[0] ||
					strncmp(name, lastuname, len) != 0) {
		
		strncpy(uname, name, len);
		uname[len] = '\0';

		if ((pw = getpwnam(uname)) != NULL) {
			strncpy(lastuname, pw->pw_name, TUNMLEN);
			lastuid = pw->pw_uid;
		}
		uvalid = TRUE;
		if (!pw) {
			*uidp = 0; /* XXX */
			return (FALSE);
		}
	}
	*uidp = lastuid;
	return (TRUE);
}

/*
 * Get name from gid
 */
EXPORT BOOL
namegid(name, namelen, gid)
	char	*name;
	int	namelen;
	Ulong	gid;
{
	struct group	*gr;

	if (!gvalid || gid != lastgid) {
		lastgid = gid;
		lastgname[0] = '\0';

		if ((gr = getgrgid(gid)) != NULL) {
			strncpy(lastgname, gr->gr_name, TGNMLEN);
			lastgname[namelen-1] = 0;
		}
		gvalid = TRUE;	/* force not to look again for invalid gid */
	}
	strcpy(name, lastgname);
	return (name[0] != '\0');
}

/*
 * Get gid from name
 */
EXPORT BOOL
gidname(name, namelen, gidp)
	char	*name;
	int	namelen;
	Ulong	*gidp;
{
	struct group	*gr;
	char	gname[TGNMLEN+1];
	int	len = namelen>TGNMLEN?TGNMLEN:namelen;

	if (name[0] == '\0')
		return (FALSE);
	if (!gvalid || name[0] != lastgname[0] ||
					strncmp(name, lastgname, len) != 0) {
		
		strncpy(gname, name, len);
		gname[len] = '\0';

		if ((gr = getgrnam(gname)) != NULL) {
			strncpy(lastgname, gr->gr_name, TGNMLEN);
			lastgid = gr->gr_gid;
		}
		gvalid = TRUE;
		if (!gr) {
			*gidp = 0; /* XXX */
			return (FALSE);
		}
	}
	*gidp = lastgid;
	return (TRUE);
}
