#ident %W% %E% %Q%
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		star
#HARDLINKS=	ustar
#HARDLINKS=	
CPPOPTS +=	-DBSD4_2 -DSET_CTIME -DFIFO -DUSE_MMAP
#CPPOPTS +=	-DBSD4_2 -DFIFO -DUSE_MMAP
CFILES=		star.c header.c list.c extract.c create.c append.c diff.c \
		star_unix.c \
		buffer.c dirtime.c lhash.c \
		hole.c longnames.c names.c remote.c \
		table.c props.c \
		fifo.c device.c
HFILES=		star.h starsubs.h dirtime.h xutimes.h \
		table.h props.h fifo.h diff.h dir.h
LIBS=		-lschily $(LIB_SOCKET)
#XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
