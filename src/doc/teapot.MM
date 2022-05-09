.\"{{{roff}}}\"{{{ Macros
.ds HF 3 3 3 3 3
.ds HP +3 +2 +1
.nr Hb 7
.\" variable
.ds V \fI
.ds v \fP
.\" code
.ds C \fB
.ds c \fP
.\" anchor
.ds A \fI
.ds a \fP
.\" emphasize
.ds E \fI
.ds e \fP
.\" keyboard
.ds K \fB
.ds k \fP
.if n .ds DG *
.if t .ds DG *
.\"}}}
.\"{{{ Title
.ND "October 22nd, 2001"
.TL
Teapot User guide
.AU "Michael Haardt (michael@moria.de)"
.AF ""
.AS "" 0.5i
For ages, spread sheet programs have been closely associated with
financial calculations done by typical end-users.  But it has shown
that there is also hacker's work which can be done with them, like
calculate monitor timings for various resolutions, produce convincing
time statistics which justify the lack of documentation or the need for
a budget increase to your employer.  This first part of this user guide
explains how the various functions of teapot are used, whereas the
second part gives an introduction to spread sheets and explains the
expression evaluator and its functions.
.AE
.MT 4 1
.\"}}}
.\"{{{ Copyright
.H 1 "Copyright and usage conditions"
\*Cteapot\*c (Table Editor And Planner, Or: Teapot\*F),
.FS
Back then, around 1992, all IRC servers supported \*C/wallop\*c.  If you
were bored, not knowing what to do and it was just too silent and
peaceful, sending a \*C/wallop pineapple\*c or \*C/wallop teapot\*c was
an effective way to start a silly but entertaining operator
conversation.  The fact that the first versions of this program were
unnamed, because I had no idea how to call it and because all
suggestions sounded boring, is pure coincidence.  :)
.FE
is copyright (c) by Michael Haardt, 1995, 1996, 1997, 2001.  The
implementation of clocked expressions is modelled after the description
of clocked evaluation in the PhD work of Jörg Wittenberger
<joerg.wittenberger@inf.tu-dresden.de> at the University of Technology
in Dresden, Germany.  The German message catalog was contributed by
Guido Müsch, the Dutch
catalog by Wim van Dorst
<baron@clifton.hobby.nl> and the Ukrainian catalog by Volodymyr M. Lisivka <lvm@mystery.lviv.net>.
The trigonometric functions were inspired by Koniorczyk Mátyás <kmatyas@cs.elte.hu>.
The context output format was contributed by Marko Schuetz <MarkoSchuetz@web.de>.
You may use and modify this program or
derived works without charge for personal use and at work under the
condition that no copyright notices are removed or changed.  Distributing
teapot under different conditions and any commercial use like charging
for copying or selling it or derived works, alone or as part of a bigger
product, needs a written license from me.
.P
I can be reached by:
.DS I
Snail mail:
Michael Haardt
An Neuenhofen 40
47800 Krefeld
Germany

Email:
michael@moria.de
.DE
Disclaimer: I am in no way responsible for anything caused by using this
program, so if your computer or house blows up, that's your problem.
.\"}}}
.\"{{{ Introduction to spread sheets
.H 1 "Introduction to spread sheets"
.\"{{{ General introduction
.H 2 "General introduction"
A spread sheet consists of cells formed by rows and columns. 
Additionally, in many spread sheets you have a third dimension, which
you can imagine as various sheets laying on top of each other.  The
third dimension allows you to hide intermediate results, show you
additional results you do not want to appear in the ``official''
tables, keep sheets per time period (like 12 sheets for each month in a
year) while allowing you to make calculations over the entire time
interval and much more.  The figure below shows the three dimensions:
.\"{{{ picture
.DS CB
.\"{{{ with pic for troff
.if t \{
.PS 5i
cols=20
rows=10
layers=4
dist=0.6
[
  SHEET: box invis wid 4 ht 1
  for x=0 to cols do { line from x/cols <SHEET.nw,SHEET.ne> down SHEET.ht }
  for y=0 to rows do { line from y/rows <SHEET.nw,SHEET.sw> right SHEET.wid }
  for i=1 to layers do
  {
    for x=0 to cols-1 do
    {
      line from x/cols <last box.nw,last box.ne> + (i*dist*last box.wid/cols,i*dist*last box.ht/rows) to x/cols <last box.nw,last box.ne> + (i*dist * last box.wid/cols,(i-1)*dist*last box.ht/rows)
    }
    L: line from last box.ne + (i*dist*last box.wid/cols,i*dist*last box.ht/rows) to last box.se + (i*dist*last box.wid/cols,dist*i*last box.ht/rows)
    line from L.end left dist*last box.wid/cols
    line from L.start left last box.wid
    for y=0 to rows-1 do
    {
      line from y/rows <last box.ne,last box.se> + (i*dist*last box.wid/cols,i*dist*last box.ht/rows) to y/rows <last box.ne,last box.se> + ((i-1)*dist * last box.wid/cols,i*dist*last box.ht/rows)
    }  
  }
]
SHEET: last [].SHEET
S: last [].se + (0.3,SHEET.ht)
arrow from S right SHEET.wid
" x" ljust at last arrow.end
arrow from S down SHEET.ht
"y" below at last arrow.end
arrow from S to S +(layers*dist*SHEET.wid/cols,layers*dist*SHEET.ht/rows)
" z" ljust at last arrow.end
.PE
\}
.\"}}}
.\"{{{ with ASCII art for nroff
.if n \{
                                         _.
    +-----+-----+-----+-----+-----+      /|  z
  +-----+-----+-----+-----+-----+ |     /
+-----+-----+-----+-----+-----+ |-+    +------------->  x
|     |     |     |     |     |-+ |    |
+-----+-----+-----+-----+-----+ |-+    |
|     |     |     |     |     |-+ |    |
+-----+-----+-----+-----+-----+ |-+    |
|     |     |     |     |     |-+ |    |
+-----+-----+-----+-----+-----+ |-+    |
|     |     |     |     |     |-+      |
+-----+-----+-----+-----+-----+        v  y
\}
.\"}}}
.DE
.\"}}}
You can think of cells as variables, which value is the value of an
associated expression.  The expression may be constant, like 1.23, or
it may be a function of other cell values.  The advantage compared to a
programmable calculator is that if you change a number, you directly
see all changes in other cells caused by that.  Often this allows you
to get a feeling how much you may change basic sizes with still getting
satisfying results without having to solve the problem analytically.
.P
Spread sheets offer many editing functions in order to modify, clear,
copy and move cells or blocks of cells.  Besides the usual mathematical
functions, there are functions which work on blocks of cells, like
calculating the sum of a block or counting all non-empty elements. 
Further there are functions working on character strings, because most
likely you also want text besides numbers.  The next section will
introduce you to some of these by examples.
.P
teapot is a traditional spread sheet and a typical UNIX program, because
it does just one thing: Calculations.  It does not include any graphics
functions and never will, but it allows to export data in many formats,
so you can use your favourite graphics software.
.\"}}}
.P
.\"{{{ First steps
.H 2 "The first steps"
.\"{{{ start it
Now that you should have an idea, it is probably a good time to make
your first steps.  This section will show you how to create and save a
sheet which contains two numbers and their sum.  If your system supports
message catalogs, please make sure that \*CNLSPATH\*c and the \*CLC_*\*c variables
are set right for your locale.  Now start the program without any
arguments:
.DS I
\*Kteapot\*k
.DE
.\"}}}
.\"{{{ make a sheet which adds two numbers
You see an empty sheet with the cell cursor being at the upper left
corner.  Further, the status line tells you that this cell is really empty:
.DS I
\*CE @(0,0,0)=\*c
.DE
The \*CE\*c means that you can edit the sheet.  A \*CV\*c would mean
that you could only view its contents.  The meaning of \*C@()\*c will be
explained soon.  You are now in the command mode of teapot.  Now type
return to edit this cell.  A complete list of command mode functions
will be given later.  A prompt will appear below the status line:
.DS I
\*CCell contents:\*c \*K1\*k
.DE
Now the cell at position 0,0,0 has the integer
constant 1.  The status line shows you the cell contents, whereas in the
sheet you see its value.  Since constants are identical with their values,
both are 1.  Now move the cell cursor down one row and edit that cell,
giving it the integer constant \*C41\*c.
.P
Now that you have two numbers, move the cell cursor to cell 0,2,0 and
give that cell the following contents:
.DS I
\*CCell contents:\*c \*K@(0,0,0)+@(0,1,0)\*k
.DE
If you were confused about the difference between contents and value of
a cell, it should become more clear now: The status line shows the contents,
which is the arithmetic expression to calculate the sum of two cells,
whereas in the sheet you see the value of that expression: 42, which
was to be expected.  \*C@(\*c\*Vx\*v\*C,\*c\*Vy\*v\*C,\*c\*Vz\*v\*C)\*c
is a function which takes three coordinates and returns the value of the
cell at the given position.
.\"}}}
.P
.\"{{{ label your cells
As you can see, the arithmetic expression is not too readable.  If you
would move cells around, it would not even work any more.  For these
reasons, you can use symbolic names instead of coordinates, called
labels.  When used in an expression, a label is like a pointer to a
cell, its data type is \*Elocation\*e.  Move to cell 0,0,0 and use
\*K/\*k (slash) in command mode to get into the main menu.  Depending on
your screen size, you may not see all of it.  In this case, move the
highlighted block right (or left) to scroll through it and to see all
items.  Now change its label attribute: A)ttributes, L)abel:
.DS I
\*CCell label:\*c \*KPaper\*k
.DE
Then go one cell down and change its label to \*KTapes\*k.  After,
move again one cell down and change the expression to:
.DS I
\*CCell contents:\*c \*K@(Paper)+@(Tapes)\*k
.DE
.\"}}}
.\"{{{ use strings to explain numbers
As you see, you can call the function \*C@\*c with three integer values
or with one location value.  Now the expression is more understandable,
at least to you.  To someone else, the sheet only contained three numbers,
so a little text should be added.  To accomplish that, a new column needs to
be inserted: B)lock, I)insert, C)olumn, W)hole column.  The last menu
item means that you want to insert a whole new column, not only a partial
column.  If you move the cursor around, you will see that everything is
still fine, because you used labels.  Go to cell 0,0,0 and edit it:
.DS I
\*CCell contents:\*c \*K"Paper:"\*k
.DE
This is how you enter strings.  A string is a data type on its own, don't
confuse this with labels.  If you feel like it, leave the quotes and the
colon away, and you will see the difference, because the result will not
be a string, but the value of the label \*CPaper\*c, which is \*C&(1,0,0)\*c.
Now change the cells below to \*K"Tapes:"\*k and \*K"Result:"\*k.  This
is something that is understandable to others, too.
.\"}}}
.P
.\"{{{ save the sheet
As the last step, save your work sheet to a file: F)ile, X)DR.
The native file format is XDR, so chose that.  Up to 
now, your sheet does not have a name, so you will be prompted for one:
.DS I
\*CSave sheet to XDR file:\*c \*Kfirst_step\*k
.DE
Unless you see an error message after,
your sheet is written to a file.
.P
If you have come this far, quit (from the main menu) and you have
successfully completed your first steps on using teapot.\*F
.FS
It may be a good idea to make and enjoy some tea at this point.  :)
.FE
Now you know cells, the difference between contents and values, you
learned that labels are a good thing and you can do simple
cell modifications as well as saving your work.  This is enough for
most applications.  If the capabilities described in the next section
confuse you, then it is unlikely that you need them really.  Just skip
that section and don't worry about it.
.\"}}}
.P
.\"{{{ iterative expressions
You may wonder what happens if you have circular dependencies, i.e.\&
you have a cell which evaluates to its own value plus one.
Well, the answer is that it depends on the order in which
you create this cell.  If you first give it the value 1 and after edit
it to contain the expression which refers to itself plus 1, then you
will find that each recalculation, like after editing other cells,
will increase the value.  While this may be funny, it is certainly
not useful as you can not reset the cell and you have little control
of its development.
.P
What you really want is a base value and an iterative expression along
with a way to control the recalculations.  teapot supports this by
allowing two expressions per cell.  The expressions you have used so
far are the ones which evaluate to the base values.  Each time you
edit a cell, the whole sheet will be reset, which means that all
results are recalculated using the base values.  After, you can
clock the sheet, which is why the iterative part is also called clocked
expression.  A clock is an atomic operation, which means that
all cell results will be recalculated in a way that the new result
will only show after the entire recalculation.
.P
An examples will demonstrate how to make use of this feature.
The notation \*Vx\*v \(-> \*Vy\*v means that \*Vx\*v is the base
expression and \*Vy\*v is the clocked expression.  Don't let
this confuse you, as both are entered separately: teapot does
not have an \(-> operator, but it displays the cell contents this
way for increased overview.  So, give the cell a base expression
of \*C1\*c and a clocked expression of \*C@(0,0,0)+1\*c (using
\*KESC-Enter\*k or \*KMeta-Enter\*k) and you
will see:
.DS I
\*C@(0,0,0)=1 -> @(0,0,0)+1\*c
.DE
The sheet is currently in reset condition and the result is 1.  Now
clock it and you will see how the value increases.
.\"}}}
.P
After this introductional chapter, you should be familiar with the basic
concepts in spread sheets.  The next chapters explain all functions
available in detail.  You should read them to get an overview of the
possibilities offered by teapot.  Finally, we will come back to using
teapot by showing some common problems and their solutions.
.\"}}}
.\"}}}
.\"{{{ The command mode
.H 1 "The command mode"
Right after starting teapot, you are in the command mode.  Many
functions from the command mode are also available from menues, but
using keys is faster and some things, like moving the cell cursor,
are only available through keys.  The command mode accepts:
.TS H
center,allbox;
.so command.tbl
.TE
.sp
\&.\& (dot) marks blocks: The first time it marks the beginning of a
block, which is then extended by moving the cell cursor.  The next time,
it marks the end of the block which lets you move the cell cursor after
without changing the block.  The third time, it removes the block marks
again.
.\"}}}
.\"{{{ The line editor
.H 1 "The line editor"
Many functions in teapot require editing a line of text, e.g. editing
cell contents, typing file names and the line.  Similar to the command
mode, all things can be reached by control codes and most by function keys.
The line editor accepts:
.TS H
center,allbox;
.so editline.tbl
.TE
.sp
Besides the regular line editor functions as explained previously, you
may use ^o to temporarily leave the editor in order to move around in
the sheet if you are editing cell contents.  Another ^o brings you back
to the line editor.  While moving around in the sheet, you can insert
the value (v) or position (p) at the cursor position in the edited
cell.
.P
Aborting line editing means that you will get right back to command mode,
whatever you started doing will have no effect.
.\"}}}
.\" ----- fine until here
.\"{{{ Interactive functions
.H 1 "Interactive functions"
If a block of cells is marked, many interactive functions will be applied to
all cells within that block.  Such functions are denoted by * for this chapter.
.\"{{{ Setting the precision
.H 2 "Setting the precision"
The precision only changes what is printed, teapot always uses the
maximum precision for calculations.  It also only affects the output,
if the cell value is a floating point number.  The precision of printed
floating point numbers.  Entering an empty precision means to set it to
the default value.
.\"}}}
.\"{{{ Changing the cell label
.H 2 "Changing the cell label *"
This function lets you edit the cell label of the current cell.
Further it changes all occurences of it in the cell contents to the new
value, unless you erased the cell label.  If a block has been marked by
the time you edit the cell label, all occurences of the label in contents
of cells in that block will be changed.
.\"}}}
.\"{{{ Locking cells
.H 2 "Locking cells *"
You can lock cells to protect them from accidental editing.  Note
that this protects you from modifying single cells.  If you modify
a block of cells which contains locked cells, those will be modified
as well.  This has been done because when using block commands, you
usually know what you are doing.
.\"}}}
.\"{{{ Setting the column width
.H 2 "Setting the column width *"
The column width only affects the screen display, not the formatting of
the final output with the expectation of saved formatted ASCII files.  It
is intended to let you make better usage of the screen for more
overview.  If the width is too small to display the cell value, stars
will be displayed.
.\"}}}
.\"{{{ Copying or moving a block of cells
.H 2 "Copying or moving a block of cells"
To copy a block of cells, mark it, then move the cell cursor to where
the upper left corner of the copy should be and issue the copy command.
Moving works similar, just use the move command.  Of course you can mark
three-dimensional blocks and copy them anywhere in the
three-dimensional sheet, but doing so requires a good three-dimensional
imagination to get what you want.
.\"}}}
.\"{{{ Filling a block of cells
.H 2 "Filling a block of cells"
To fill a block of cells, first mark a the block it should be filled
with.  This may be just one cell!  Then move the cell cursor to where
the upper left corner of the block to be filled should be and issue the
fill command.  You will be prompted for how often the marked block should
be repeated in each dimension.  For example, you may to repeat a cell 9
times below.  Mark it, then move down one row.  Issue the fill command and
answer 1 to the number of column repetitions, 9 to rows and 1 to layers.
.\"}}}
.\"{{{ Clear cells
.H 2 "Clearing cells *"
Clearing means to delete the cell contents and set all attributes to
the default value.  If you want to preserve the attributes, just edit
the contents of a cell and delete them.
.\"}}}
.\"{{{ Insert cells
.H 2 "Insert cells *"
Since work sheets can be three-dimensional, you can insert cells in all
three dimensional, too.  The inserted cells will be empty and their
attributes have the default values.  Cells will always be moved away
from the front upper left corner to make room for the inserted cells.
If no block is marked, you will be asked if you really only want to
insert a cell of if you want to insert a whole row, line or sheet.
.\"}}}
.\"{{{ Delete cells
.H 2 "Delete cells *"
Deleting works contrary to inserting.  The deleted cells will be filled
by moving neighbour cells to their positions.  You will be prompted for
the direction from where those cells will be taken.  Deleting an entire
column column-wise is done by marking the column, use the delete command
and chose X direction.
.\"}}}
.\"{{{ Sort cells
.H 2 "Sort cells"
Marked blocks of cells can be sorted after one or multiple keys, either
column-wise, row-wise or depth-wise.  Sorting a two dimensional block
row-wise will sort lines, but if a three dimensional block is sorted
row-wise, then horizontal layers will be sorted.  The sort key is
specified as vector which is orthogonal to the sorted elements, either
in ascending or descending order.  The following example illustrates
the sort function.  The upper left part of the screen should look like
this:
.DS CB
.TS
.so unsorted.tbl
.TE
.DE
.sp
The box shows you which block to mark.  Now this block should be sorted
row-wise, with the sort key being the numbers in descending order, i.e.\&
we want the lines being numbered 4,3,2,1.  Go to the block menu, then select
sort.  Use \*CR)ow\*c, because that is how we want to sort this block.
The X position of the sort key vector is 0, because the column 0 contains
the numbers.  The Z position is 0, too, because those numbers are on sheet 0.
Now chose \*CD)escending\*c as direction.  At this point, you could add
a secondary key or decide to sort the block by the keys entered so far.
Use \*CS)ort region\*c to sort it.  That's it, the screen should look like
this now:
.DS CB
.TS
.so sorted.tbl
.TE
.DE
.\"}}}
.\"{{{ Mirror cells
.H 2 "Mirror cells"
Mirroring a marked block of cells can be done in three directions:
Left/right, upside/down and front/back.
.\"}}}
.\"{{{ Saving and loading sheets
.H 2 "Saving and loading sheets"
.\"{{{ Name or rename a sheet
.H 3 "Name or rename a sheet"
Occasionally, you may want to rename a sheet, like before making critical
changes or when you load something an existing sheet to have a start for
making a new one.  This function allows to change the filename which is
associated with a sheet.
.\"}}}
.\"{{{ Save and load in XDR format
.H 3 "Save and load in XDR format"
XDR (eXternal Data Representation) is a standard invented by Sun
Microsystems which defines a canonical way of storing/transporting data
on external media.  Its advantage is that it is widely available and
that it defines a portable floating point number format.  The native
teapot file format uses XDR so it is portable across different machine
architectures and operating systems.  The advantage of this over the
portable ASCII format is that due to the (usually) missing conversion
calculations any floating point constants will be saved/loaded exactly
without conversion errors.  XDR files have no default extension.
.\"}}}
.\"{{{ Save and load in ASCII format
.H 3 "Save and load in ASCII format"
The ASCII file format allows easy generation/modification of saved
sheets by shell scripts.  Due to binary/ASCII conversion, there may
be conversion errors in floating point constants.  The default
extension is \*C.asc\*c.
.\"}}}
.\"{{{ Save and load in CSV/GCSV format
.H 3 "Save and load in CSV/GCSV format *"
CSV (comma seperated value) files only contain the data, not the
expressions calculating it.  Many spread sheets can generate this file
format and many graphics programs, like \*Agnuplot\*a(1) can read it.
PlotPlus can read it if you specify \*CFORMAT FREE\*c.
The field separator is a tab or comma, strings are preferably but
not necessarily enclosed in double quotes and decimal numbers have a
dot to mark the fractional part.  The default extension is \*C.txt\*c.
When loading CSV files, the sheet will not be cleared and the data will
be load relative to the current cursor position.  GCSV is the German
variation, which uses semicolons for separating fields and a decimal
comma instead of a decimal point.
On load, strings without quotes and with a \fI0x\fP prefix followed by
hexadecimal digits will be converted to integers.
.\"}}}
.\"{{{ Load in SC format
.H 3 "Load in SC format"
teapot can load simple SC sheets to convert them to teapot's native
format.  While loading, teapot converts all references to absolute cell
positions to labels.  This allows to insert and delete in such sheets
without screwing the whole sheet up.  teapot can not save sheets in SC
format, because SC lacks many features.  For now, only the most basic
SC features are supported.  The default extension is \*C.sc\*c.
.\"}}}
.\"{{{ Load in WK1 format
.H 3 "Load in WK1 format"
teapot can load simple WK1 sheets to convert them to teapot's native
format.  By default, 1-2-3 cell references are relative, so don't be
surprised by a big amount of relative references in the resulting teapot
sheet.  For now, only the most basic 1-2-3 features are supported.  The
default extension is \*C.wk1\*c.
.\"}}}
.\"{{{ Save in formatted ASCII format
.H 3 "Save in formatted ASCII format *"
The generated formatted ASCII files contain about what you see on the
screen.  If your sheet has more than one layer, then the various layers
will be saved separated by form feeds.  The default extension is \*C.doc\*c.
.\"}}}
.\"{{{ Save in tbl format
.H 3 "Save in tbl format *"
teapot can generate \*Atbl\*a(1) table bodies in single files which are
supposed to be used like this:
.DS I
\&\*C.TS\*c
\*Voptions\*v\*C;\*c
\&\*C.so\*c \*Vfilename\*v
\&\*C.TE\*c
.DE
You will have to use \*Asoelim\*a(1) to eliminate the \*C\&.so\*c
requests before the tbl run.  The \*Voptions\*v\*C;\*c are optional. 
If you use GNU roff, you will need to eliminate \*C\&.lf\*c requests,
because this GNU roff extension confuses GNU tbl:
.DS I
\*Csoelim\*c \*Vfile\*v \*C| grep -v '^\e.lf'\*c
.DE
Alternatively, you can generate a stand-alone document, which needs no
further operations to format and print.  Note: If no block is marked,
the whole sheet will be saved, not only the current cell.  The default
extension is \*C.tbl\*c.
.\"}}}
.\"{{{ Save in LaTeX format
.H 3 "Save in LaTeX format *"
If you generate LaTeX 2e tables in single files, you include them in
documents like this:
.DS I
\*C\einclude{\*c\*Vfilename\*v\*C}\*c
.DE
Alternatively, you can generate a stand-alone document, which needs no
further operations to format and print.  Note: If no block is marked,
the whole sheet will be saved, not only the current cell.  The default
extension is \*C.tex\*c.
.\"}}}
.\"{{{ Save in HTML format
.H 3 "Save in HTML format *"
You can generate html table bodies in single files which could be used
in combination with server-side includes.  This feature differs between
the various servers, so refer to the manual for your web server for
details, please.
.P
Alternatively, you can generate a stand-alone document.  Note: If no
block is marked, the whole sheet will be saved, not only the current cell.
The default file extension is \*C.html\*c.
.\"}}}
.\"}}}
.\"{{{ Go to location
.H 2 "Go to location"
Sometimes, you directly want to go to a specific position, either to
change its contents to see which cell a location expression refers to.
This function lets you enter an expression, which must evaluate to
a value of the type location.  If so, the cursor is positioned to that
location.  For example, you could enter \*C&(10,2)\*c to go to cell
10,2 of the current layer or you could enter the name of a label you
want to go to.  Relative movements are no problem, either.
.\"}}}
.\"{{{ Shell
.H 2 "Shell"
Start a sub shell.  Exiting from that sub shell will bring you back
into teapot.
.\"}}}
.\"{{{ Version information
.H 2 "Version information"
teapot will display its version number and other entertaining things.  :)
If you send me email, please always write which version you are using.
.\"}}}
.\"}}}
.\"{{{ Batch functions
.H 1 "Batch functions"
Besides interactive facilities, teapot has a batch mode.  Using this
batch mode, shell scripts can generate output from teapot sheets.  This
is handy if you use \*Amake\*a(1) to generate a bigger document
containing tables, because you don't have to generate a tbl or LaTeX file
each time you modified a sheet: make will do so.  In batch mode, teapot
reads batch commands from standard input.  The following commands are
available:
.VL \n(Pi
.\"{{{ goto location
.LI "\*Cgoto\*c \*Vlocation\*v"
.br
Go to the specified \*Vlocation\*v.
.\"}}}
.\"{{{ from location
.LI "\*Cfrom\*c \*Vlocation\*v"
.br
Start marking a block.
.\"}}}
.\"{{{ to location
.LI "\*Cto\*c \*Vlocation\*v"
.br
End marking a block.
.\"}}}
.\"{{{ sort-x, sort-y, sort-z
.LI "\fBsort-x\fP d|a \fIy z\fP [ d|a \fIy z\fP ... ]"
.br
.sp -0.5
.LI "\fBsort-y\fP d|a \fIx z\fP [ d|a \fIx z\fP ... ]"
.br
.sp -0.5
.LI "\fBsort-z\fP d|a \fIx y\fP [ d|a \fIx y\fP ... ]"
.br
Sorts the marked block as described in section \fI5.10\fP,
column-wise, row-wise or depth-wise, respectivly.
"d" or "a" specify the sort order to be descending or ascending.
\fIx\fP, \fIy\fP and \fIz\fP specify the position of the sort key
\fBrelative\fP to the first cell of the marked block.
Up to eight sort keys can be specified.
.br
This example reproduces the result from \fI5.10\fP:
.br
echo -e "from &(1,1,0)\\n to &(2,4,0)\\n sort-y d 0 0\\n save-csv
result_num.txt" \\
.br
| teapot -b doc/unsorted
.\"}}}
.\"{{{ save-tbl, save-csv, save-latex, save-context, save-html filename
.LI "\fBsave-tbl\fP \fIfile\fP"
.br
.sp -0.5
.LI "\fBsave-csv\fP \fIfile\fP"
.br
.sp -0.5
.LI "\fBsave-latex\fP \fIfile\fP"
.br
.sp -0.5
.LI "\fBsave-context\fP \fIfile\fP"
.br
.sp -0.5
.LI "\fBsave-html\fP \fIfile\fP"
.br
Save the marked block in the specified format as \fIfile\fP.
.\"}}}
.\"{{{ load-csv, load-gcsv filename
.LI "\fBload-csv\fP \fIfile\fP"
.br
.sp -0.5
.LI "\fBload-gcsv\fP \fIfile\fP"
.br
Load \fIfile\fP in the specified format to the last \fBgoto\fP
location.
This is the same functionality as the interactive load described in
subsection \fI5.12.4\fP.
.\"}}}
.LE
.\"}}}
.\"{{{ Cells
.H 1 "Cells"
In this chapter, \fBbold\fP marks literals which have to be entered
exactly as shown.  \fIItalic\fP mark place holders, they are not to be
taken literally.
.\"{{{ Attributes
.H 2 "Attributes"
Each cell consists of a base/reset expression, a clocked expression,
the result of one of these expressions (depending on the cell being
reset or clocked) and several attributes:
.BL
.LI
A cell label, which is useful because it avoids to directly address
cells by their position.  A cell label must be different from function
names.
.LI
The cell adjustment, which determines if the cell value is printed left
adjusted, right adjusted or centered.
.LI
The precision for the output of floating point values.  The default is
2 digits after the dot.
.LI
If floating point numbers should be printed in scientific notation
(0.123e1) or as decimal number (1.23).  It only affects the output, if
the cell value is a floating point number.
.LI
If the cell is shadowed by its left neighbour.  This means that the
left neighbour cell additionally uses the room of the shadowed cell.
.LI
If the cell is locked which prevents to accidentally edit or clear it. 
Note that block operations override this attribute, because when you
deal with blocks, you usually know what you are doing.
.LI
If special characters for e.g.\& roff and LaTeX should be quoted
(default) or not.  Not quoting them allows special effects (if you know
roff or LaTeX), but is of course not portable.
.LE
.\"}}}
.\"{{{ Data Types
.H 2 "Data Types"
In teapot, each value has an associated data type.  The following
data types exist:
.BL
.\"{{{ empty
.LI
empty.  Empty cells have this as value.
.\"}}}
.\"{{{ string
.LI
string.  A string is a sequence of characters enclosed by double
quotes: \fB"This is a string"\fP.  A double quote can be part of the
string, if it is quoted using a backslash: \fB"\e""\fP.  If you want
the backslash to appear in the output instead of quoting the next
character, use it to quote itself: \fB"\e\e"\fP.  A single minus
character on the screen will be output as a minus.  If you want a
hyphen character in the output, then use two minus characters for that.
.\"}}}
.\"{{{ floating point
.LI
floating point.  Floating point values are inexact, their precision and
range depends on the implementation of the C type double on your
system.  An example is: \fB42.0\fP
.\"}}}
.\"{{{ integer
.LI
integer.  Integer values are exact, their range depends on the C type
long on your system.  An example is: \fB42\fP
.\"}}}
.\"{{{ location
.LI
location.  Cell labels and the \fB&()\fP function have this type, but
there are no location constant literals.
.\"}}}
.\"{{{ error
.LI
error.  Syntactical, semantical (type mismatch) errors cause this value,
as well as division by 0 and the function \fBerror()\fP.  An error
always has an assigned error message.  Functions and operators, when
applied to a value of the type error, evaluate to just that value.  That
way, the first error which was found deep inside a complicated
expression will be showed.
.\"}}}
.LE
.\"}}}
.\"}}}
.\"{{{ Operators
.H 1 "Operators"
Unlike other spread sheets, the operators in teapot check the type
of the values they are applied to, which means the try to add a string
to a floating point number will result in an type error.  The following
operators are available, listed in ascending precendence:
.VL \n(Pi
.\"{{{ x<y
.LI \fIx\fP\fB<\fP\fIy\fP
evaluates to 1 if \fIx\fP is less than \fIy\fP.  If \fIx\fP or \fIy\fP
are empty, they are considered to be 0 if the other is an integer
number, 0.0 if it is a floating point number and the empty string if it
is a string.
.\"}}}
.\"{{{ x<=y
.LI \fIx\fP\fB<=\fP\fIy\fP
evaluates to 1 if \fIx\fP is less than or equal to \fIy\fP.  Empty
values are dealt with as described above.
.\"}}}
.\"{{{ x>=y
.LI \fIx\fP\fB>=\fP\fIy\fP
evaluates to 1 if \fIx\fP is greater than or equal to \fIy\fP.  Empty
values are dealt with as described above.
.\"}}}
.\"{{{ x>y
.LI \fIx\fP\fB>\fP\fIy\fP
evaluates to 1 if \fIx\fP is greater than \fIy\fP.  Empty values are
dealt with as described above.
.\"}}}
.\"{{{ x==y
.LI \fIx\fP\fB==\fP\fIy\fP
evaluates to 1 if \fIx\fP is equal to \fIy\fP.  Empty values are dealt
with as described above.
.\"}}}
.\"{{{ x~=y
.LI \fIx\fP\fB~=\fP\fIy\fP
evaluates to 1 if the floating point value \fIx\fP is almost to the
floating point value \fIy\fP.  Empty values are dealt with as described
above.  Almost equal means, the numbers are at most neighbours.
.\"}}}
.\"{{{ x!=y
.LI \fIx\fP\fB!=\fP\fIy\fP
evaluates to 1 if \fIx\fP is not equal to \fIy\fP.  Empty values are
dealt with as described above.
.\"}}}
.\" ----
.\"{{{ x+y
.LI \fIx\fP\fB+\fP\fIy\fP
evaluates to the sum if \fIx\fP and \fIy\fP are numbers.  If \fIx\fP and
\fIy\fP are strings, the result is the concatenated string.  If \fIx\fP
or \fIy\fP is empty, the result is the other element.  There is no
dedicated logical or operation, so use \fB+\fP for that.
.\"}}}
.\"{{{ x-y
.LI \fIx\fP\fB\-\fP\fIy\fP
evaluates to the difference if \fIx\fP and \fIy\fP are numbers.  If
\fIx\fP is empty, the result is \fB\-\fP\fIy\fP.  If \fIy\fP is empty, the
result is \fIx\fP.
.\"}}}
.\" ----
.\"{{{ x*y
.LI \fIx\fP\fB*\fP\fIy\fP
evaluates to the product if \fIx\fP and \fIy\fP are numbers.  If \fIx\fP
or \fIy\fP is empty, the result is 0.  There is no dedicated logical
and operation, so use \fB*\fP for that.
.\"}}}
.\"{{{ x/y
.LI \fIx\fP\fB/\fP\fIy\fP
evaluates to the quotient if \fIx\fP and \fIy\fP are numbers.  If
\fIx\fP is empty, the result is 0.  If \fIy\fP is empty, the result is
the error ``division by 0''.
.\"}}}
.\"{{{ x%y
.LI \fIx\fP\fB%\fP\fIy\fP
evaluates to the remainder of the division if \fIx\fP and \fIy\fP are numbers.  If
\fIx\fP is empty, the result is 0.  If \fIy\fP is empty, the result is
the error ``modulo 0''.
.\"}}}
.\" ----
.\"{{{ x^y
.LI \fIx\fP\fB^\fP\fIy\fP
evaluates to \fIx\fP to the power of \fIy\fP.  If \fIx\fP or \fIy\fP are
empty, they will be considered to be the integer value 0.
.\"}}}
.\" ----
.\"{{{ -x
.LI \fB\-\fP\fIx\fP
evaluates to \fB\-\fP\fIx\fP if \fIx\fP is a number.  If \fIx\fP is
empty, the result will be empty, too.
.\"}}}
.\"{{{ (expression)
.LI \fB(\fP\fIexpression\fP\fB)\fP
.br
evaluates to the expression.
.\"}}}
.\"{{{ function()
.LI \fIfunction\fP\fB(\fP\fIargument\fP\fB,\fP...\fB)\fP
.br
evaluates to the value of the function applied to the values resulting
from evaluating the argument expressions.
.\"}}}
.LE
.\"}}}
.\"{{{ Functions
.H 1 "Functions"
This section documents all available functions in alphabetical order.
The functions are given in a C-like notation, so use \fB@(0,0,0)\fP
instead of \fB@(integer 0, integer 0, integer 0)\fP.  If no type is
given for the result of a function, it means the result type depends on
the arguments.  Brackets mark optional arguments.
.VL \n(Pi
.\"{{{ @
.LI "\fB@(\fP[integer \fIx\fP][\fB,\fP[integer \fIy\fP][\fB,\fP[integer \fIz\fP]]]\fB)\fP"
.br
.sp -0.5
.LI "\fB@(\fPlocation \fIl\fP\fB)\fP"
.br
returns the value of the cell at position \fIx\fP, \fIy\fP, \fIz\fP.  If
any of \fIx\fP, \fIy\fP or \fIz\fP is omitted, the coordinate of the
cell is used.
.\"}}}
.\"{{{ &
.LI "location \fB&(\fP[integer \fIx\fP][\fB,\fP [integer \fIy\fP][\fB,\fP [integer \fIz\fP]]]\fB)\fP"
.br
returns a pointer to the cell at location \fIx\fP, \fIy\fP, \fIz\fP.  If
\fIz\fP is omitted, the \fIz\fP position of the cell is used.  If
\fIy\fP is missing as well, the \fIy\fP position (row) of the cell is
used.
.\"}}}
.\"{{{ $
.LI "string \fB$(\fPstring \fIenv\fP\fB)\fP"
.br
evaluates to the contents of the specified environment variable.
If the variable does not exist, then an empty string will be returned.
.\"}}}
.\"{{{ abs
.LI "float \fBabs(\fPfloat \fIx\fP\fB)\fP"
.br
.sp -0.5
.LI "int \fBabs(\fPint \fIx\fP\fB)\fP"
.br
evaluates to the absolute value of \fIx\fP.
.\"}}}
.\"{{{ acos
.LI "float \fBacos(\fPdouble|int \fIx\fP\fB)\fP"
.br
evaluates to the arc cosine of \fIx\fP, where \fIx\fP is given in radians.
.\"}}}
.\"{{{ arcosh
.LI "float \fBarcosh(\fPdouble|int \fIx\fP\fB)\fP"
.br
evaluates to the arc hyperbolic cosine of \fIx\fP, where \fIx\fP is given in radians.
.\"}}}
.\"{{{ arsinh
.LI "float \fBarsinh(\fPdouble|int \fIx\fP\fB)\fP"
.br
evaluates to the arc hyperbolic sine of \fIx\fP, where \fIx\fP is given in radians.
.\"}}}
.\"{{{ artanh
.LI "float \fBartanh(\fPdouble|int \fIx\fP\fB)\fP"
.br
evaluates to the arc hyperbolic tangent of \fIx\fP, where \fIx\fP is given in radians.
.\"}}}
.\"{{{ asin
.LI "float \fBasin(\fPdouble|int \fIx\fP\fB)\fP"
.br
evaluates to the arc sine of \fIx\fP, where \fIx\fP is given in radians.
.\"}}}
.\"{{{ atan
.LI "float \fBatan(\fPdouble|int \fIx\fP\fB)\fP"
.br
evaluates to the arc tangent of \fIx\fP, where \fIx\fP is given in radians.
.\"}}}
.\"{{{ clock
.LI "\fBclock(\fPinteger \fIcondition\fP\fB,\fP[location[\fB,\fPlocation]\fB)\fP"
.br
conditionally clocks the specified cell if the condition is not 0.  If
two locations are given, all cells in that range will be clocked.  The
return value of clock is empty.
.\"}}}
.\"{{{ cos
.LI "float \fBcos(\fPdouble|int \fIx\fP\fB)\fP"
.br
evaluates to the cosine of \fIx\fP, where \fIx\fP is given in radians.
.\"}}}
.\"{{{ cosh
.LI "float \fBcosh(\fPdouble|int \fIx\fP\fB)\fP"
.br
evaluates to the hyperbolic cosine of \fIx\fP, where \fIx\fP is given in radians.
.\"}}}
.\"{{{ deg2rad
.LI "float \fBdeg2rad(\fPdouble|int \fIx\fP\fB)\fP"
.br
evaluates to the degrees that are equivalent to \fIx\fP radians.
.\"}}}
.\"{{{ e
.LI "float \fBe()\fP"
.br
evaluates to the Euler constant \fIe\fP.
.\"}}}
.\"{{{ error
.LI "error \fBerror(\fPstring \fImessage\fP\fB)\fP"
.br
evaluates to an error with the specified message.
.\"}}}
.\"{{{ eval
.LI "\fBeval(\fPlocation\fB)\fP"
.br
evaluates to the value of the expression in the cell at the given
\fIlocation\fP, but evaluated in the context of the cell using
\fBeval()\fP.  This function may not be used nested any deeper than
32 times.
.\"}}}
.\"{{{ float
.LI "float \fBfloat(\fPstring \fIs\fP\fB)\fP"
.br
converts the given string into a floating point number.
.\"}}}
.\"{{{ frac
.LI "float \fBfrac(\fPfloat \fIx\fP\fB)\fP"
.br
evaluates to the fractional part of \fIx\fP.
.\"}}}
.\"{{{ int
.LI "int \fBint(\fPfloat \fIx\fP[\fB,\fPinteger \fIneg\fP\fB,\fP integer \fIpos\fP]\fB)\fP"
.br
converts \fIx\fP to an integer value by cutting off the fractional
part.  If given, the value of \fIneg\fP and \fIpos\fP determines how
negative and non-negative numbers will be converted:
.DS CB
.TS
box;
l c l
r c l.
neg,pos		result
_
<\-1	\(->	next smaller integer value
\-1	\(->	round downward
0	\(->	cut fractional part off (default)
1	\(->	round upward
>1	\(->	next larger integer value
.TE
.DE
.LI "int \fBint(\fPstring \fIs\fP\fB)\fP"
.br
converts \fIs\fP to an integer number.
.\"}}}
.\"{{{ len
.LI "string \fBlen(\fPstring \fIs\fP\fB)\fP"
.br
evaluates to the length of \fIs\fP.
.\"}}}
.\"{{{ log
.LI "float \fBlog(\fPdouble|int \fIx\fP[\fB,\fPdouble|int \fIy\fP\fB)\fP"
.br
evaluates to the logarithm of \fIx\fP.  If \fIy\fP is specified, the
result will be the natural logarithm, otherwise it will be the logarithm
to the base of \fIy\fP.
.\"}}}
.\"{{{ max
.LI "location \fBmax(\fPlocation \fIl1\fP\fB,\fP location \fIl2\fP\fB)\fP"
.br
evaluates to the maximum in the same way \fBmin\fP does for the minimum.
.\"}}}
.\"{{{ min
.LI "location \fBmin(\fPlocation \fIl1\fP\fB,\fP location \fIl2\fP\fB)\fP"
.br
evaluates to the location of the minimum of all values in the block
marked by the corners pointed to by \fIl1\fP and \fIl2\fP.  Note that
the empty cell is equal to 0, 0.0 and "", so if the first minimum is an
empty cell, the result will be a pointer to this cell, too.  If you are
not interested in the location of the minimum but the value itself, use
\fB@(min(\fP\fIl1\fP\fB, \fP\fIl2\fP\fB))\fP.
.\"}}}
.\"{{{ n
.LI "int \fBn(\fPlocation \fIl1\fP\fB,\fP location \fIl2\fP\fB)\fP"
.br
evaluates to the number of non-empty cells in the block marked by the
corners pointed to by \fIl1\fP and \fIl2\fP.
.\"}}}
.\"{{{ poly
.LI "\fBpoly(\fPfloat|integer \fIx\fP\fB,\fP float|integer \fIc\s-2\dn\u\s0\fP[\fB,\fP ...]\fB)\fP"
.br
evaluates the polynome \fIc\s-2\dn\u\s0*x\s-2\un\d\s0\fP+...+\fIc\s-2\d0\u\s0*x\s-2\u0\d\s0\fP.
.\"}}}
.\"{{{ rad2deg
.LI "float \fBrad2deg(\fPdouble|int \fIx\fP\fB)\fP"
.br
evaluates to the radians that are equivalent to \fIx\fP degrees.
.\"}}}
.\"{{{ rnd
.LI "float \fBrnd()\fP"
.br
evaluates to a pseudo-random number between 0.0 and 1.0.
.\"}}}
.\"{{{ sin
.LI "float \fBsin(\fPdouble|int \fIx\fP\fB)\fP"
.br
evaluates to the sine of \fIx\fP, where \fIx\fP is given in radians.
.\"}}}
.\"{{{ sinh
.LI "float \fBsinh(\fPdouble|int \fIx\fP\fB)\fP"
.br
evaluates to the hyperbolic sine of \fIx\fP, where \fIx\fP is given in radians.
.\"}}}
.\"{{{ strftime
.LI "string \fBstrftime(\fPstring \fIf\fP [\fB,\fP integer \fIt\fP])"
.br
evaluates to the time \fIt\fP formatted according to the format
specified in \fIf\fP. Times in \fIt\fP are counted in seconds since
epoch (1970-1-1 0:00). If \fIt\fP is empty or 0, the actual time is
used.
For the format specifications consult the man page of your c library,
strftime (3).
Example: \fI@(now)=int(strftime("%s"))\fP sets the field with label
now to the actual time.
.\"}}}
.\"{{{ string
.LI "string \fBstring(\fPlocation \fIl\fP\fB)\fP"
.br
.sp -0.5
.LI "string \fBstring(\fPinteger \fIx\fP\fB)\fP"
.br
.sp -0.5
.LI "string \fBstring(\fPfloat \fIx\fP[\fB,\fP [integer \fIprecision\fP][\fB,\fP \fIscientific\fP]]\fB)\fP"
.br
evaluates to a string containing the current value of the given cell at
location \fIl\fP, or to the numeric value \fIx\fP with the given
\fIprecision\fP.  The \fIscientific\fP flag determines if decimal (0)
or scientific (unequal 0) representation is used.
.\"}}}
.\"{{{ strptime
.LI "int \fBstrptime(\fPstring \fIf\fP \fB,\fP string \fIdatetime\fP)"
.br
evaluates to the seconds since epoch (1970-1-1 0:00) of the
\fIdatetime\fP string, parsed according to the format
specified in \fIf\fP.
For the format specifications consult the man page of your c library,
strptime (3).
.\"}}}
.\"{{{ substr
.LI "string \fBsubstr(\fPstring \fIs\fP\fI,\fPinteger \fIx\fP\fI,\fPinteger \fIy\fP\fB)\fP"
.br
evaluates to the substring of \fIs\fP between \fIx\fP and \fIy\fP,
which start at 0.
.\"}}}
.\"{{{ sum
.LI "\fBsum(\fPlocation \fIl1\fP\fB,\fP location \fIl2\fP\fB)\fP"
.br
evaluates to the sum of all values in the block marked by the corners
pointed to by \fIl1\fP and \fIl2\fP.
.\"}}}
.\"{{{ tan
.LI "float \fBtan(\fPdouble|int \fIx\fP\fB)\fP"
.br
evaluates to the tangent of \fIx\fP, where \fIx\fP is given in radians.
.\"}}}
.\"{{{ tanh
.LI "float \fBtanh(\fPdouble|int \fIx\fP\fB)\fP"
.br
evaluates to the hyperbolic tangent of \fIx\fP, where \fIx\fP is given in radians.
.\"}}}
.\"{{{ x, y, z
.LI "int \fBx(\fP[location \fIl\fP]\fB)\fP"
.br
.sp -0.5
.LI "int \fBy(\fP[location \fIl\fP]\fB)\fP"
.br
.sp -0.5
.LI "int \fBz(\fP[location \fIl\fP]\fB)\fP"
.br
evaluate to the \fIx\fP, \fIy\fP and \fIz\fP position of the given
location, of the currently updated cell if none is given.  These
functions are usually used in combination with the \fB@\fP function for
relative relations to other cells.
.\"}}}
.LE
.\"}}}
.\"{{{ Problems and solutions
.H 1 "Problems and solutions"
.\"{{{ Why is 1.0 unequal 1.0?
.H 2 "Why is 1.0 unequal 1.0?"
If your machine uses binary floating point arithmetic, and chances
are that it does, you may eventually find yourself in the following
situation:
.DS I
0.1+0.1+0.1+0.1+0.1+0.1+0.1+0.1+0.1+0.1
.DE
You expect to see 1.0 as result, and indeed that is what you get.
Now you compare this result to the constant 1.0, but surprisingly for
many users, the result is 0.  Appearantly, 1.0 is unequal 1.0 for
teapot.
.P
This is not a bug in teapot, in fact it is not a bug at all.  The
problem is, that 0.1 (1.0/10.0) does not have an exact representation in binary
floating point arithmetic, similar to how 1.0/3.0 does not have an
exact representation in decimal arithmetic (or binary, for that matter).
As such, a value very close to 0.1 is used, but when displaying it, it
will be rounded to 0.1.  The result is obvious, adding this number which
is a little different from 0.1 ten times leads to a result very close to but
not quite
1.0.  Since it is so close, displaying it rounded to only a few digits precision
shows 1.0.
.P
To solve the comparison problem, teapot has the operator \fB~=\fP, which compares
two floating point values apart from the last significant bit.  Use this
operator to compare the two values from above and the result will be 1,
meaning they are about equal.  Don't assume that a number which can be
expressed with a finite number of decimal digits will be represented exactly
in binary floating point arithmetic.
.\"}}}
.\"{{{ How do I hide intermediate results?
.H 2 "How do I hide intermediate results?"
If you used flat, two-dimensional spread sheets before, you
are probably used to hidden cells which contain intermediate results,
global constants, scratch areas and the like.  teapot has no way to
hide cells, but you have three dimensions.  Just use one or more
layers for such cells and give each cell a label in order
to reference and find it easily.
.\"}}}
.\"{{{ Why is there no conditional evaluation?
.H 2 "Why is there no conditional evaluation?"
There is no special operator or function for conditional evaluation.
I could add one easily, but then next someone would ask for loops and
someone else for user-defined functions, variables and so on.  If you
need a programming language, you know where to find it.
.P
But don't worry.  :) The answer is, that conditional evaluation
comes for free with teapot's orthogonal cell addressing.
As an example, depending on the cell
labelled \fBX\fP being negative or not, you want the result to be the
string \fB"BAD\fP or \fB"GOOD"\fP.  This is the solution:
.DS I
eval(&((@(X)>=0)+x(BAD),y(BAD),z(BAD)))
.DE
The cell labelled \fBBAD\fP contains the string \fB"BAD"\fP, its right
neighbour contains the string \fB"GOOD"\fP.  If you have nested
conditions, you could weight them with 1, 2, 4 and so on to address
a bigger range of cells.  Alternatively, you could make use of all
three dimensions for nested conditions.
.\"}}}
.\"}}}
.\"{{{ Expression grammar
.H 1 "Expression grammar"
.TS
lfI c l.
digit	::=	\fB0\fP | .. | \fB9\fP
hex_digit	::=	\fB0\fP | .. | \fB9\fP | \fBa\fP | .. | \fBf\fP
octal_digit	::=	\fB0\fP | .. | \fB7\fP
decimal_integer	::=	\fIdigit\fP { \fIdigit\fP }
hex_integer	::=	\fB0x\fP \fIhex_digit\fP { \fIhexdigit\fP }
octal_integer	::=	\fB0\fP \fIoctal_digit\fP { \fIoctdigit\fP }
integer	::=	\fIdecimal_integer\fP | \fIhex_integer\fP | \fIoctal_integer\fP
float	::=	\fIdigit\fP { \fIdigit\fP } [ \fB.\fP ] { \fIdigit\fP } [ \fBe\fP | \fBE\fP [ \fB+\fP | \fB\-\fP ] \fIdigit\fP { \fIdigit\fP } ]
quoted_character	::=	\fB\e\fP \fIany_character\fP
character	::=	\fIany_character\fP | \fIquoted_character\fP
string	::=	\fB"\fP { \fIcharacter\fP } \fB"\fP
identifier_character	::=	\fB_\fP | \fB@\fP | \fB&\fP | \fB.\fP | \fB$\fP | \fIalpha_character\fP
identifier	::=	\fIidentifier_character\fP { \fIidentifier_character\fP | \fIdigit\fP }
function	::=	\fIidentifier\fP \fB(\fP [ \fIterm\fP ] { \fB,\fP [ \fIterm\fP ] } \fB)\fP
label	::=	\fIidentifier\fP
parenterm	::=	\fB(\fP \fIterm\fP \fB)\fP
negterm	::=	\fB\-\fP \fIprimary\fP
primary	::=	\fIfunction\fP | \fIlabel\fP | \fIparenterm\fP | \fInegterm\fP
powterm	::=	\fIprimary\fP { \fB^\fP \fIprimary\fP }
mathterm	::=	\fIpowterm\fP { \fB/\fP | \fB*\fP | \fB%\fP \fIpowterm\fP }
factor	::=	\fImathterm\fP { \fB+\fP | \fB\-\fP \fImathterm\fP }
term	::=	\fIfactor\fP {\fB<\fP | \fB<=\fP | \fB>=\fP | \fB>\fP | \fB==\fP | \fB!=\fP \fIfactor\fP }
.TE
.\"}}}
