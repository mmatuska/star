#ident %W% %E% %Q%
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

SUBARCHDIR=	/profiled
.SEARCHLIST:	. $(ARCHDIR) stdio $(ARCHDIR)
VPATH=		.:stdio:$(ARCHDIR)
INSDIR=		lib
TARGETLIB=	schily_p
CPPOPTS +=	-DBSD4_2 -DNO_SCANSTACK
COPTS +=	$(COPTGPROF)
include		Targets
LIBS=		

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
# Gmake has a bug with the VPATH= option. Some of the macros are
# not correctly expanded. I had to remove all occurrences of
# $@ $* and $^ on some places for this reason.
###########################################################################

cmpbytes.o fillbytes.o movebytes.o: align.h
$(ARCHDIR)/cmpbytes.o $(ARCHDIR)/fillbytes.o $(ARCHDIR)/movebytes.o: align.h

align_test.o:	align_test.c
		$(CC) -c $(CPPFLAGS) -o $(ARCHDIR)/align_test.o align_test.c

align_test:	align_test.o
		$(LDCC) -o $(ARCHDIR)/align_test $(ARCHDIR)/align_test.o

align.h:	align_test
		$(ARCHDIR)/align_test > $(ARCHDIR)/align.h

getav0.o:	avoffset.h
$(ARCHDIR)/getav0.o:	avoffset.h

avoffset.o:	avoffset.c
		$(CC) -c $(CPPFLAGS) -o $(ARCHDIR)/avoffset.o avoffset.c

getfp_x.o:	getfp.c
		$(CC) -O -c $(CPPFLAGS) -o $(ARCHDIR)/getfp_x.o getfp.c

avoffset:	avoffset.o getfp_x.o
		$(LDCC) -o $(ARCHDIR)/avoffset $(ARCHDIR)/avoffset.o $(ARCHDIR)/getfp_x.o

avoffset.h:	avoffset
		$(ARCHDIR)/avoffset > $(ARCHDIR)/avoffset.h

$(ARCHDIRX)align_test$(DEP_SUFFIX):	$(ARCHDIRX)

include		$(ARCHDIRX)avoffset$(DEP_SUFFIX)
include		$(ARCHDIRX)align_test$(DEP_SUFFIX)

CLEAN_FILEX=	$(ARCHDIR)/align_test.o $(ARCHDIR)/align_test
CLEAN_FILEX +=	$(ARCHDIR)/avoffset.o $(ARCHDIR)/avoffset
CLEAN_FILEX +=	$(ARCHDIR)/getfp_x.o

CLOBBER_FILEX=	$(ARCHDIR)/align_test.d $(ARCHDIR)/align_test.dep \
		$(ARCHDIR)/align.h
CLOBBER_FILEX += $(ARCHDIR)/avoffset.d $(ARCHDIR)/avoffset.dep \
		$(ARCHDIR)/avoffset.h
