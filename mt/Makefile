#ident @(#)Makefile	1.2 02/11/11 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		smt
HARDLINKS=	mt
CPPOPTS +=	-DUSE_REMOTE
CFILES=		mt.c
#HFILES=
LIBS=		-lrmt -lschily $(LIB_SOCKET)
#XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1
