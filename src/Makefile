DEBUG= -Os -s
#DEBUG= -g3

#{{{ Solaris 9, gcc, ncurses
#CC=		gcc
#CFLAGS=		-pipe -O2 -Wall -Wmissing-prototypes -Wstrict-prototypes -Wcast-qual -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-declarations -Wnested-externs -pedantic -fno-common
#CPPFLAGS=	-I/opt/FSFgnu/include -DHAVE_TYPEAHEAD
#LDFLAGS=	-L/opt/FSFgnu/lib
#LIBS=		-lcurses -lm -lnsl
#MAKEDEPEND=	gcc -MM
#}}}
#{{{ NetBSD 2.0, Linux
CC=		gcc
CFLAGS=	-pipe $(DEBUG) -Wall -Wcast-qual -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-declarations -pedantic -fno-common
CPPFLAGS=
LDFLAGS= $(DEBUG)
LIBS=		-lm
LIBS_TTY=	-lncurses
MAKEDEPEND=	gcc -MM
#}}}

HELPFILE = doc/teapot.html
#HELPFILE = <b>Sorry, Help not installed!</b>

LIBS_FLTK=	-lfltk -lfltk_images
CXXFLAGS = $(CFLAGS)
CPPFLAGS += -DHELPFILE="\"$(HELPFILE)\""
TEAPOTOBJS=	csv.o complete.o context.o eval.o func.o html.o \
		latex.o main.o misc.o parser.o sc.o scanner.o sheet.o \
		wk1.o xdr.o
FLTKOBJS=	fteapot.o
TTYOBJS=	display.o

teapot: cat.h $(TEAPOTOBJS) $(TTYOBJS)
		$(CC) $(LDFLAGS) -o $@ $^ $(LIBS) $(LIBS_TTY)

fteapot: cat.h $(TEAPOTOBJS) $(FLTKOBJS)
		( make teapot; cd doc; make teapot.html; )
		$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS) $(LIBS_FLTK)

%.cc: %.fl
		fluid -c -o $@ $<

cat.h:		Gencat
		./Gencat
		ln -s cat-en.h cat.h

display.o: version.h
main.o: version.h

graph:		graph.o csv.o
		$(CC) $(LDFLAGS) -o $@ graph.o csv.o -lm

wc:
		@wc -l *.[ch] | tail -1
		@wc -c *.[ch] | tail -1

depend: cat.h
		$(MAKEDEPEND) $(CPPFLAGS) *.c >.depend

clean:
		rm -f $(TEAPOTOBJS) graph.o $(FLTKOBJS) $(TTYOBJS) fteapot.h fteapot.cc cat.h cat-* *~
		(cd doc; make clean)

clobber:	clean
		rm -f core *.out teapot graph fteapot tags
		(cd doc; make clean)

dist:	depend teapot
		(cd doc; make teapot.ps teapot.doc clean)
		make clobber
		./Gencat
		( V=teapot-"`grep \\\" version.h | sed -e 's/.*"\(.*\)"/\1/'`"; rm -r "../$$V/"; mkdir "../$$V"; cp -a * "../$$V"/; cd ..; tar czf "$$V".tar.gz "$$V"; )

include .depend		
