#ident @(#)star_sym.mk	1.9 08/09/10 
###########################################################################
# Sample makefile for general application programs
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		star_sym
CPPOPTS +=	-I../star
CPPOPTS +=	-DSTAR_SYM
CPPOPTS +=	-D__STAR__ 
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DSCHILY_PRINT

CFILES=		star_sym.c restore.c dumpdate.c
LIBS=		-lfind -lschily $(LIB_ACL_TEST) $(LIB_INTL)
#XMK_FILE=	Makefile.man
#XMK_FILE=	tartestman.mk

restore.c dumpdate.c:
	@echo "	==> MAKING SYMLINKS in ." && sh ./MKLINKS
$(ALLTARGETS): restore.c dumpdate.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################

