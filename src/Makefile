#{{{script}}}#{{{ Linux at moria
#CFLAGS=		-g -pipe -O2 -Wall -Wmissing-prototypes -Wstrict-prototypes -Wcast-qual -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-declarations -Wnested-externs -pedantic -fno-common
#CPPFLAGS=	-DHAVE_TYPEAHEAD
##CPPFLAGS=	-DHAVE_TYPEAHEAD -DTHE_ELECTRIC_FENCE
##CPPFLAGS=	-I/usr/dmalloc/include -DDMALLOC
#LDFLAGS=	-g
##LDFLAGS=	-g -L/usr/efence/lib
##LDFLAGS=	-g -L/usr/dmalloc/lib
#LIBS=		-lcurses -lm
##LIBS=		-lcurses -lm -lefence
##LIBS=		-lcurses -lm -ldmalloc
#MAKEDEPEND=	mkdep -d
#}}}
#{{{ Linux, SuSE 8.1, ncurses
#CC=		gcc
#CFLAGS=		-pipe -O2 -Wall -Wmissing-prototypes -Wstrict-prototypes -Wcast-qual -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-declarations -Wnested-externs -pedantic -fno-common
#CPPFLAGS=	-I/usr/include/ncurses -DHAVE_TYPEAHEAD
#LDFLAGS=
#LIBS=		-lncurses -lm
#MAKEDEPEND=	gcc -MM
#}}}
#{{{ Solaris 9, gcc, ncurses
#CC=		gcc
#CFLAGS=		-pipe -O2 -Wall -Wmissing-prototypes -Wstrict-prototypes -Wcast-qual -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-declarations -Wnested-externs -pedantic -fno-common
#CPPFLAGS=	-I/opt/FSFgnu/include -DHAVE_TYPEAHEAD
#LDFLAGS=	-L/opt/FSFgnu/lib
#LIBS=		-lcurses -lm -lnsl
#MAKEDEPEND=	gcc -MM
#}}}
#{{{ NetBSD 2.0
CC=		gcc
CFLAGS=		-pipe -O2 -Wall -Wmissing-prototypes -Wstrict-prototypes -Wcast-qual -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-declarations -Wnested-externs -pedantic -fno-common
CPPFLAGS=
LDFLAGS=
LIBS=		-lcurses -lm
MAKEDEPEND=	gcc -MM
#}}}

TEAPOTOBJS=	csv.o complete.o context.o display.o eval.o func.o html.o \
		latex.o main.o misc.o parser.o sc.o scanner.o sheet.o \
		wgetc.o wk1.o version.o xdr.o

teapot:		$(TEAPOTOBJS)
		$(CC) $(LDFLAGS) -o $@ $(TEAPOTOBJS) $(LIBS)

cat.h:		Gencat
		./Gencat

version.c:	Genversion
		./Genversion

graph:		graph.o csv.o
		$(CC) $(LDFLAGS) -o $@ graph.o csv.o -lm

wc:
		@wc -l *.[ch] | tail -1
		@wc -c *.[ch] | tail -1

depend:
		$(MAKEDEPEND) $(CPPFLAGS) *.c >.depend

clean:
		rm -f $(TEAPOTOBJS) graph.o
		(cd doc; make clean)

clobber:	clean
		rm -f core *.out teapot graph
		(cd doc; make clobber)

tar:		depend
		./Gencat
		(cd doc; make teapot.ps teapot.doc clean)
		(b=`pwd`; b=`basename $$b`; cd ..; tar zcvf $$b.tar.gz $$b/ANNOUNCE $$b/INSTALL $$b/doc $$b/magic $$b/teapot.1.* $$b/Makefile $$b/Gencat $$b/Genversion $$b/.depend $$b/.curses.h $$b/.nl_types.h $$b/*.c $$b/*.h $$b/examples)

include .depend		
