#ident @(#)shlschily.mk	1.3 02/08/23 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

SUBARCHDIR=	/pic
.SEARCHLIST:	. $(ARCHDIR) stdio $(ARCHDIR)
VPATH=		.:stdio:$(ARCHDIR)
INSDIR=		lib
TARGETLIB=	schily
CPPOPTS +=	-Istdio
include		Targets
LIBS=		

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.shl
###########################################################################
