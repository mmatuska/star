#ident %W% %E% %Q%
###########################################################################
SRCROOT=	.
DIRNAME=	SRCROOT
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

#include		Targetdirs.$(M_ARCH)

DIRS=	conf inc lib libdeflt librmt star mt rmt tartest man

###########################################################################
# Due to a bug in SunPRO make we need special rules for the root makefile
#
include		$(SRCROOT)/$(RULESDIR)/rules.rdi
###########################################################################
