#ident @(#)librmt_p.mk	1.2 02/11/10 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

SUBARCHDIR=	/profiled
#.SEARCHLIST:	. $(ARCHDIR) stdio $(ARCHDIR)
#VPATH=		.:stdio:$(ARCHDIR)
INSDIR=		lib
TARGETLIB=	rmt_p
CPPOPTS +=	-DUSE_REMOTE
CPPOPTS +=	-DUSE_RCMD_RSH
CPPOPTS +=	-DUSE_LARGEFILES
COPTS +=	$(COPTGPROF)
include		Targets
LIBS=		

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
#CC=		echo "	==> COMPILING \"$@\""; cc
###########################################################################
