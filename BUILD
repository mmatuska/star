Short overview for those who don't read manuals:

	There is no 'configure', simply call 'make' on the top level
	directory.

	All results in general will be placed into a directory named 
	OBJ/<arch-name>/ in the current projects leaf directory.

	You **need** either my "smake" program, the SunPRO make 
	from /usr/bin/make (SunOS 4.x) or /usr/ccs/bin/make (SunOS 5.x)
	or GNU make to compile this program. Read README.gmake for 
	more information on gmake.

	All other make programs are either not smart enough or have bugs.

	My "smake" source is at:

	ftp://ftp.fokus.gmd.de/pub/unix/smake/alpha/

	It is easy to compile and doesn't need a working make program
	on your machine.

	If you have the choice between all three make programs, the
	preference would be 

		1)	smake		(preferred)
		2)	SunPRO make
		3)	GNU make	(this is the last resort)

	Important notice: "smake" that comes with SGI/IRIX will not work!!!

	Please read the README's for your operating system too.

			WARNING
	Do not use 'mc' to extract the tar file!
	All mc versions before 4.0.14 cannot extract symbolic links correctly.

	The versions of WinZip that support tar archives cannot be used too.
	The reason is that they don't support symbolic links.
	Star and Gnutar do support symbolic links even on win32 systems.
	To support symbolic links on win32, you need to link with the
	Cygwin32 POSIX library.

	To unpack an archive use:

		gzip -d < star.tar.gz | tar -xpf -

	Replace 'star' by the actual archive name.


Here comes the long form:


PREFACE:

	You don't have to call configure with this make file system.

	Calling	'make' or 'make all' on the top level directory will create
	all needed targets. Calling 'make install' will install all needed
	files.

	This program uses a new makefilesystem. The makefilesystem is optimized
	for a program called 'smake' Copyright 1985 by J�rg Schilling, but
	SunPro make (the make program that comes with SunOS >= 4.0 and Solaris)
	as well as newer versions of GNU make will work also.
	BSDmake could be made working, if it supports pattern matching rules
	correctly.

	The makefile system allows simultaneous compilation on a wide
	variety of target systems if the source tree is accessible via NFS.


Finding Compilation Results:

	To allow this, all binaries and results of a 'compilation' in any form
	are placed in sub-directories. This includes automatically generated
	include files. Results in general will be placed into
	a directory named OBJ/<arch-name>/ in the current projects
	leaf directory, libraries will be placed into a directory called
	libs/<arch-name>/ that is located in the source tree root directory.

		<arch-name> will be something like 'sparc-sunos5-cc'


How to compile:

	To compile a system or sub-system, simply enter 'smake', 'make' or 
	'Gmake'. Compilation may be initialized at any point of the source
	tree of a system. If compilation is started in a sub tree, all objects
	in that sub tree will be made.


How to install results:

	To install the product of a compilation in your system, call:

		smake install

	at top level. The binaries will usually be installed in 
	/opt/schily/bin. The directory /opt/<vendor-name>/ has been agreed
	on by all major UNIX vendors in 1989. Unfortunately, not all vendors
	follow this agreement.

	If you want to change the default installation directory, edit the
	appropriate (system dependent) files in the DEFAULTS directory
	(e.g. DEFAULTS/Defaults.sunos5).


Using a different installation directory:

	If your system does not yet use the standard installation path /opt
	or if you don't like this installation directory, you can easily 
	change the installation directory. You may edit the DEFAULTS file 
	for your system and modify the macro INS_BASE.

	You may  use a different installation directory without editing the
	DEFAULTS files. If you like to install everything in /usr/local, call:


	If your make program supports to propagate make macros to sub make programs
	which is the case for recent smake releases as well as for a recent gnumake:

		smake INS_BASE=/usr/local install
	or
		gmake INS_BASE=/usr/local install

	If you make program doesn't propagate make macros (e.g. SunPRO make) call:

		env INS_BASE=/usr/local make -e install


Using a different C-compiler:

	The default C-compiler can be modified in the files in the
	DEFAULT directory too. If you want to have a different compiler
	for one compilation, call:

		make CCOM=gcc
	or
		make CCOM=cc

	This works even when your make program doen't propagate make macros.

Getting help from make:

	For a list of targets call:

		make .help


Getting more information on the make file system:

	The man page makefiles.4 located in man/man4/makefiles.4 contains
	the documentation on general use and for leaf makefiles.

	The man page makerules.4 located in man/man4/makerules.4 contains
	the documentation for system programmers who want to modify
	the make rules of the makefile system.


Hints for compilation:

	The makefile system is optimized for 'smake'. Smake will give the
	fastest processing and best debugging output.

	SunPro make will work as is. GNU make need some special preparation.

	Read README.gmake for more information on gmake.

	To use GNU make create a file called 'Gmake' in your search path
	that contains:

		#!/bin/sh
		MAKEPROG=gmake
		export MAKEPROG
		exec gmake "$@"

	and call 'Gmake' instead of gmake. On Linux there is no gmake, 'make'
	on Linux is really a gmake.

	'Gmake' and 'Gmake.linux' are part of this distribution.

	Some versions of gmake are very buggy. There are e.g. versions of gmake
	on some architectures that will not correctly recognize the default
	target. In this case call 'make all' or ../Gmake all'.

	Note that speudo error messages from gmake similar to:

	gmake[1]: Entering directory `cdrtools-1.10/conf'
	../RULES/rules.cnf:58: ../incs/sparc-sunos5-cc/Inull: No such file or directory
	../RULES/rules.cnf:59: ../incs/sparc-sunos5-cc/rules.cnf: No such file or directory

	Are a result of a bug un GNU make. The make file system itself is
	correct (as you could proove by usingsmake).
	If your gmake version still has this bug, send a bug report to:

		"Paul D. Smith" <psmith@gnu.org>

	He is the current GNU make maintainer.

	If you like to use 'smake', please always compile it from source.
	The packages are located on:

		ftp://ftp.fokus.gmd.de/pub/unix/smake/alpha/

	Smake has a -D flag to see the actual makefile source used
	and a -d flag that gives easy to read debugging info. Use smake -xM
	to get a makefile dependency list. Try smake -help


Compiling the project using engineering defaults:

	The defaults found in the directory DEFAULTS are configured to
	give minimum warnings. This is made because many people will
	be irritated by warning messages and because the GNU c-compiler
	will give warnings for perfectly correct and portable c-code.

	If you want to port code to new platforms or do engineering
	on the code, you should use the alternate set of defaults found
	in the directory DEFAULTS_ENG.
	You may do this permanently by renaming the directories or
	for one compilation by calling:

		make DEFAULTSDIR=DEFAULTS_ENG


Compiling the project to allow debugging with dbx/gdb:

	If you like to compile with debugging information for dbx or gdb,
	call:

		make clean
		make COPTX=-g LDOPTX=-g


	If you want to see an example, please have a look at the "star"
	source. It may be found on:

		ftp://ftp.fokus.gmd.de/pub/unix/star

	Have a look at the manual page, it is included in the distribution.
	Install the manual page with 

	make install first and include /opt/schily/man in your MANPATH

	Note that some systems (e.g. Solaris 2.x) require you either to call
	/usr/lib/makewhatis /opt/schily/man or to call 

		man -F <man-page-name>

Author:

Joerg Schilling
Seestr. 110
D-13353 Berlin
Germany

Email: 	joerg@schily.isdn.cs.tu-berlin.de, js@cs.tu-berlin.de
	schilling@fokus.gmd.de

Please mail bugs and suggestions to me.