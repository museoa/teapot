.\"{{{roff}}}\"{{{ Titel
.TH TEAPOT 1 "12 augustus 1995" "" "Gebruikercommando's"
.\"}}}
.\"{{{ Naam
.SH NAAM
teapot \- een spreadsheet-programma
.\"}}}
.\"{{{ Synopsis
.SH SYNOPSIS
.ad l
.B teapot
.RB [ \-a ]
.RB [ \-r ]
.RB [ \-b ]
.RB [ \-n ]
.RB [ \-p
.IR cijfers ]
.RI [ bestand ]
.ad b
.\"}}}
.\"{{{ Beschrijving
.SH BESCHRIJVING
\fBteapot\fP (Table Editor And Planner, Or: Teapot) is een
driedimensionaal spreadsheet-programma.  De beschrijving van 
de interactieve commando's valt buiten deze handboekpagina. 
Zie daarvoor de ``Teapot Gebruikersgids''.
.\"}}}
.\"{{{ Opties
.SH OPTIES
.\"{{{ -a
.IP \fB\-a\fP
Gebruik als bronbestanden overdraagbare ASCII-bestanden 
in plaats van (ook overdraagbare) XDR-bestanden.
.\"}}}
.\"{{{ -b
.IP \fB\-b\fP
Niet-interactief, waarbij de commando's gelezen worden 
van de standaardinvoer.
.\"}}}
.\"{{{ -n
.IP \fB\-n\fP
Laat niet de aanhalingstekens zien voor teksten in het spreadsheet.
.\"}}}
.\"{{{ -p cijfers
.IP "\fB\-p\fP \fIcijfers\fP"
Stel de nauwkeurigheid op het gegeven aantal cijfers achter de komma.
.\"}}}
.\"{{{ -r
.IP "\fB\-r\fP"
Herteken het scherm altijd in zijn geheel in plaats van alleen
als er tijd voor is.  Sommige mensen geven hier de voorkeur
aan ten opzichte van blindelings vooruittypen.
.\"}}}
.\"}}}
.\"{{{ Omgeving
.SH OMGEVING
.IP \fBTERM\fP
Het gebruikte terminal-type
.IP \fBLC_MESSAGES\fP
De gebruike 'locale' voor uitvoerberichten van Teapot
.IP \fBLC_CTYPE\fP
De gebruikte 'locale' voor de te gebruiken letterset
.IP \fBLC_TIME\fP
De gebruikte 'locale' voor datum- en tijdweergave
.\"}}}
.\"{{{ Auteur
.SH AUTEUR
Michael Haardt (michael@moria.de)

Vertaling door Wim van Dorst, van Clifton Scientific Text 
Services (clifton@clifton.hobby.nl)
.\"}}}
.\"{{{ Zie ook
.SH "ZIE OOK"
awk(1), bc(1), dc(1), expr(1), ``Teapot Gebruikersgids'', ``External Data
Representation Standard''
.\"}}}
