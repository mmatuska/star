#ident %W% %E% %Q%
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		star
HARDLINKS=	ustar
CPPOPTS +=	-DSET_CTIME -DFIFO -DUSE_MMAP -DUSE_REMOTE
#CPPOPTS +=	-DFIFO -DUSE_MMAP
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DUSE_ACL
CPPOPTS +=	-DUSE_FFLAGS
CFILES=		star.c header.c xheader.c \
		list.c extract.c create.c append.c diff.c \
		remove.c star_unix.c acl_unix.c fflags.c \
		buffer.c dirtime.c lhash.c \
		hole.c longnames.c names.c remote.c \
		movearch.c table.c props.c \
		unicode.c \
		fifo.c device.c
HFILES=		star.h starsubs.h dirtime.h xutimes.h \
		remote.h movearch.h table.h props.h fifo.h diff.h mtio.h
LIBS=		-lschily $(LIB_ACL) $(LIB_SOCKET)
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
