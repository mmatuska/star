#ident @(#)shlrmt.mk	1.3 02/11/10 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

SUBARCHDIR=	/pic
#.SEARCHLIST:	. $(ARCHDIR) stdio $(ARCHDIR)
#VPATH=		.:stdio:$(ARCHDIR)
INSDIR=		lib
TARGETLIB=	rmt
CPPOPTS +=	-DUSE_REMOTE
CPPOPTS +=	-DUSE_RCMD_RSH
CPPOPTS +=	-DUSE_LARGEFILES
include		Targets
LIBS=		

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.shl
###########################################################################
#CC=		echo "	==> COMPILING \"$@\""; cc
###########################################################################
