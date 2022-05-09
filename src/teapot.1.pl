..\"{{{roff}}}\"{{{ Title
.TH TEAPOT 1 "August 12, 1995" "" "User commands"
.\"}}}
.\"{{{ Name
.SH NAME
teapot \- arkusz kalkulacyjny
.\"}}}
.\"{{{ Synopsis
.SH SK�ADNIA WYWO�ANIA
.ad l
.B teapot
.RB [ \-a ]
.RB [ \-r ]
.RB [ \-b ]
.RB [ \-n ]
.RB [ \-p
.IR cyfry ]
.RI [ file ]
.ad b
.\"}}}
.\"{{{ Description
.SH OPIS
\fBteapot\fP (Table Editor And Planner, czyli: Teapot) jest
tr�jwymiarowym arkuszem kalkulacyjnym. Po uruchomieniu programu
jego menu g��wne wywo�ujemy klawiszem F10 lub / , kolejne punkty
tego� menu (oraz pod-menu) - klawiszami odpowiadaj�cymi wyr�nionym
literom, za� wycofanie si� do menu nadrz�dnego - klawiszem
"strza�ka w g�r�".
Teapot korzysta tak�e z innych klawiszy (oraz ich kombinacji), ale
dok�adniejsze przybli�enie polece� interaktywnego trybu pracy
z programem wykracza poza zakres niniejszej strony, szczeg�y
w ``Teapot User Guide''.
.\"}}}
.\"{{{ Options
.SH OPCJE
.\"{{{ -a
.IP \fB\-a\fP
U�ywaj "przeno�nych" plik�w ASCII jako natywnego formatu plik�w
zamiast (r�wnie� "przeno�nego") formatu XDR.
.\"}}}
.\"{{{ -b
.IP \fB\-b\fP
Wykorzystaj tryb wsadowy, korzystaj�cy z polece� przekazywanych
przez "standard input".
.\"}}}
.\"{{{ -n
.IP \fB\-n\fP
Nie pokazuj w arkuszu cudzys�ow�w zamykaj�cych �a�cuchy znak�w.
.\"}}}
.\"{{{ -p cyfry
.IP "\fB\-p\fP \fIcyfry\fP"
Okre�l domy�ln� precyzj� jako podan� liczb� \fIznak�w\fP po przecinku.
.\"}}}
.\"{{{ -r
.IP "\fB\-r\fP"
Zawsze od�wie�aj ekran w ca�o�ci, zamiast odnawia� jego zawarto��
jedynie w wolnym czasie; niekt�rzy z szybko pisz�cych u�ytkownik�w
wybior� raczej t� mo�liwo��.
.\"}}}
.\"}}}
.\"{{{ Environment
.SH OTOCZENIE
.IP \fBTERM\fP
Typ u�ywanego terminala.
.IP \fBLC_MESSAGES\fP
Zmienna "locale" u�yta dla informacji przekazywanych przez program.
.IP \fBLC_CTYPE\fP
Zmienna "locale" wybieraj�ca standard zestawu znak�w.
.IP \fBLC_TIME\fP
Zmienna "locale" wybieraj�ca spos�b przedstawienia daty i godziny.
.\"}}}
.\"{{{ Author
.SH AUTOR
Michael Haardt (michael@moria.de)
.\"}}}
.\"{{{ See also
.SH ZOBACZ TAK�E
awk(1), bc(1), dc(1), expr(1), ``Teapot User Guide'', ``External Data
Representation Standard''
.\"}}}
