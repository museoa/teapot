teapot.mm:	teapot.MM command.tbl.0 editline.tbl.0 unsorted.tbl.0 sorted.tbl.0
		cp command.tbl.0 command.tbl
		cp editline.tbl.0 editline.tbl
		cp unsorted.tbl.0 unsorted.tbl
		cp sorted.tbl.0 sorted.tbl
		soelim teapot.MM | grep -v '^\.lf' >teapot.mm

command.html.0 command.tbl.0:	command command.bat insTH.ed
		../teapot -b command <command.bat
		ed command.tbl.0 <insTH.ed

editline.html.0 editline.tbl.0:	editline editline.bat insTH.ed
		../teapot -b editline <editline.bat
		ed editline.tbl.0 <insTH.ed

unsorted.html.0 unsorted.tbl.0:	unsorted unsorted.bat unsorted.ed
		../teapot -b unsorted <unsorted.bat
		ed unsorted.tbl.0 <unsorted.ed

sorted.html.0 sorted.tbl.0:	unsorted sorted.bat unsorted.ed
		../teapot -b unsorted <sorted.bat
		ed sorted.tbl.0 <unsorted.ed

teapot.ps:	teapot.mm
		groff -Tps -mm -tp teapot.mm > teapot.ps

teapot.doc:	teapot.mm
		groff -Tlatin1 -mm -t -rW7.5i teapot.mm > teapot.doc

teapot.html:	teapot.MM command.html.0 editline.html.0 unsorted.html.0 sorted.html.0 Sr2html Mm2html
		./Sr2html <command.html.0 >command.tbl
		./Sr2html <editline.html.0 >editline.tbl
		./Sr2html <unsorted.html.0 >unsorted.tbl
		./Sr2html <sorted.html.0 >sorted.tbl
		soelim teapot.MM | grep -v '^\.lf' | ./Mm2html | ./Sr2html >teapot.html

clean:
		rm -f *.tbl *.0 *.1 teapot.mm *~

clobber:	clean
		rm -f *.html *.ps *.doc teapot.mm
