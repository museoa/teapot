..\"{{{roff}}}\"{{{ Title
.TH TEAPOT 1 "August 12, 1995" "" "User commands"
.\"}}}
.\"{{{ Name
.SH NAME
teapot \- arkusz kalkulacyjny
.\"}}}
.\"{{{ Synopsis
.SH SK£ADNIA WYWO£ANIA
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
trójwymiarowym arkuszem kalkulacyjnym. Po uruchomieniu programu
jego menu g³ówne wywo³ujemy klawiszem F10 lub / , kolejne punkty
tego¿ menu (oraz pod-menu) - klawiszami odpowiadaj±cymi wyró¿nionym
literom, za¶ wycofanie siê do menu nadrzêdnego - klawiszem
"strza³ka w górê".
Teapot korzysta tak¿e z innych klawiszy (oraz ich kombinacji), ale
dok³adniejsze przybli¿enie poleceñ interaktywnego trybu pracy
z programem wykracza poza zakres niniejszej strony, szczegó³y
w ``Teapot User Guide''.
.\"}}}
.\"{{{ Options
.SH OPCJE
.\"{{{ -a
.IP \fB\-a\fP
U¿ywaj "przeno¶nych" plików ASCII jako natywnego formatu plików
zamiast (równie¿ "przeno¶nego") formatu XDR.
.\"}}}
.\"{{{ -b
.IP \fB\-b\fP
Wykorzystaj tryb wsadowy, korzystaj±cy z poleceñ przekazywanych
przez "standard input".
.\"}}}
.\"{{{ -n
.IP \fB\-n\fP
Nie pokazuj w arkuszu cudzys³owów zamykaj±cych ³añcuchy znaków.
.\"}}}
.\"{{{ -p cyfry
.IP "\fB\-p\fP \fIcyfry\fP"
Okre¶l domy¶ln± precyzjê jako podan± liczbê \fIznaków\fP po przecinku.
.\"}}}
.\"{{{ -r
.IP "\fB\-r\fP"
Zawsze od¶wie¿aj ekran w ca³o¶ci, zamiast odnawiaæ jego zawarto¶æ
jedynie w wolnym czasie; niektórzy z szybko pisz±cych u¿ytkowników
wybior± raczej tê mo¿liwo¶æ.
.\"}}}
.\"}}}
.\"{{{ Environment
.SH OTOCZENIE
.IP \fBTERM\fP
Typ u¿ywanego terminala.
.IP \fBLC_MESSAGES\fP
Zmienna "locale" u¿yta dla informacji przekazywanych przez program.
.IP \fBLC_CTYPE\fP
Zmienna "locale" wybieraj±ca standard zestawu znaków.
.IP \fBLC_TIME\fP
Zmienna "locale" wybieraj±ca sposób przedstawienia daty i godziny.
.\"}}}
.\"{{{ Author
.SH AUTOR
Michael Haardt (michael@moria.de)
.\"}}}
.\"{{{ See also
.SH ZOBACZ TAK¯E
awk(1), bc(1), dc(1), expr(1), ``Teapot User Guide'', ``External Data
Representation Standard''
.\"}}}
