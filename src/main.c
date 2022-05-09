/* #includes */ /*{{{C}}}*//*{{{*/
#ifndef NO_POSIX_SOURCE
#undef  _POSIX_SOURCE
#define _POSIX_SOURCE   1
#undef  _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 2
#undef  _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include <assert.h>
#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
extern char *optarg;
extern int optind,opterr,optopt;
int getopt(int argc, char * const *argv, const char *optstring);
#include <string.h>
#include <unistd.h>


#include "default.h"
#include "display.h"
#include "eval.h"
#include "htmlio.h"
#include "latex.h"
#include "context.h"
#include "main.h"
#include "misc.h"
#include "sc.h"
#include "scanner.h"
#include "utf8.h"
#include "parser.h"
#include "sheet.h"
#include "wk1.h"
/*}}}*/

/* variables */ /*{{{*/
char helpfile[PATH_MAX];
int batch=0;
unsigned int batchln=0;
int def_precision=DEF_PRECISION;
int quote=0;
int header=1;
static int usexdr=1;
/*}}}*/

static void get_mark(Sheet *sheet, int *x1, int *x2, int *y1, int *y2, int *z1, int *z2, int everything)
{
    if (sheet->marking) {
        posorder(&sheet->mark1x, &sheet->mark2x);
        posorder(&sheet->mark1y, &sheet->mark2y);
        posorder(&sheet->mark1z, &sheet->mark2z);
        sheet->marking = 0;
    }
    if (x1) {
        if (sheet->mark1x >= 0) {
            *x1 = sheet->mark1x;
            *x2 = sheet->mark2x;
            *y1 = sheet->mark1y;
            *y2 = sheet->mark2y;
            *z1 = sheet->mark1z;
            *z2 = sheet->mark2z;
        } else if (everything) {
            *x1 = 0;
            *x2 = sheet->dimx-1;
            *y1 = 0;
            *y2 = sheet->dimy-1;
            *z1 = 0;
            *z2 = sheet->dimz-1;
        } else {
            *x1 = *x2 = sheet->curx;
            *y1 = *y2 = sheet->cury;
            *z1 = *z2 = sheet->curz;
        }
    }
}

void moveto(Sheet *sheet, int x, int y, int z)
{
    int need_redraw = 0;
    int xdir = x > sheet->curx?1:-1;

    if (x >= 0) sheet->curx = x;
    if (y >= 0) sheet->cury = y;
    if (z >= 0) need_redraw++, sheet->curz = z;
    while (sheet->curx > 0 && shadowed(sheet, sheet->curx, sheet->cury, sheet->curz)) sheet->curx += xdir;

    if (sheet->marking) {
        sheet->mark2x = sheet->curx;
        sheet->mark2y = sheet->cury;
        sheet->mark2z = sheet->curz;
    }

    if (sheet->curx <= sheet->offx && sheet->offx) need_redraw++, sheet->offx = (sheet->curx?sheet->curx-1:0);
    if (sheet->cury <= sheet->offy && sheet->offy) need_redraw++, sheet->offy = (sheet->cury?sheet->cury-1:0);
    if (sheet->curx >= sheet->offx+sheet->maxx) need_redraw++, sheet->offx = sheet->curx-sheet->maxx+2;
    if (sheet->cury >= sheet->offy+sheet->maxy) need_redraw++, sheet->offy = sheet->cury-sheet->maxy+2;

    if (need_redraw) redraw_sheet(sheet);
    else if (x != sheet->curx || y != sheet->cury || z != sheet->curz) redraw_cell(sheet, sheet->curx, sheet->cury, sheet->curz);
}

void relmoveto(Sheet *sheet, int x, int y, int z)
{
    moveto(sheet, sheet->curx+x, sheet->cury+y, (z?sheet->curz+z:-1));
}

/* line_numedit   -- number line editor function */ /*{{{*/
static int line_numedit(int *n, const char *prompt, size_t *x, size_t *offx)
{
  /* variables */ /*{{{*/
  char buf[20];
  const char *s;
  Token **t;
  int c;
  /*}}}*/

  /* asserts */ /*{{{*/
  assert(prompt!=(char*)0);
  assert(x!=(size_t*)0);
  assert(offx!=(size_t*)0);
  /*}}}*/
  t=(Token**)0;
  sprintf(buf,"%d",*n);
  s=buf+strlen(buf);
  do
  {
    tvecfree(t);
    *x=s-buf;
    if ((c=line_edit((Sheet*)0,buf,sizeof(buf),prompt,x,offx))<0) return c;
    s=buf;
    t=scan(&s);
  } while ((*s!='\0' && t==(Token**)0) || !(t!=(Token**)0 && ((*t)==(Token*)0 || ((*t)->type==INT && (*t)->u.integer>=0 && *(t+1)==(Token*)0))));
  if (t==(Token**)0 || *t==(Token*)0) *n=-1;
  else *n=(*t)->u.integer;
  tvecfree(t);
  return 0;
}
/*}}}*/
/* line_lidedit   -- label identifier line editor function */ /*{{{*/
static int line_idedit(char *ident, size_t size, const char *prompt, size_t *x, size_t *offx)
{
  /* variables */ /*{{{*/
  const char *s;
  Token **t;
  int c;
  /*}}}*/

  t=(Token**)0;
  s=ident+strlen(ident);
  do
  {
    tvecfree(t);
    *x=s-ident;
    if ((c=line_edit((Sheet*)0,ident,size,prompt,x,offx))<0) return c;
    s=ident;
    t=scan(&s);
  } while ((*s!='\0' && t==(Token**)0) || !(t!=(Token**)0 && ((*t)==(Token*)0 || ((*t)->type==LIDENT && *(t+1)==(Token*)0))));
  tvecfree(t);
  return 0;
}
/*}}}*/
/* doanyway       -- ask if action should be done despite unsaved changes */ /*{{{*/
int doanyway(Sheet *sheet, const char *msg)
{
  int result;

  if (sheet->changed) {
    result=line_ok(msg,0);
    if (result < 0) return 0;
    return result;
  }
  return 1;
}
/*}}}*/

/* do_edit        -- set or modify cell contents */ /*{{{*/
static int do_edit(Sheet *cursheet, Key c, const char *expr, int clocked)
{
  /* variables */ /*{{{*/
  char buf[1024];
  const char *s;
  size_t x,offx;
  int curx,cury,curz;
  Token **t;
  /*}}}*/

  if (locked(cursheet,cursheet->curx,cursheet->cury,cursheet->curz)) line_msg(_("Edit cell:"),_("Cell is locked"));
  else
  {
    curx=cursheet->curx;
    cury=cursheet->cury;
    curz=cursheet->curz;
    if (expr)
    {
      s=expr;
      t=scan(&s);
      if (*s!='\0' && t==(Token**)0) line_msg(clocked ? _("Clocked cell contents:") : _("Cell contents:"),"XXX invalid expression");
    }
    else
    {
      offx=0;
      if (c==K_NONE)
      {
        print(buf,sizeof(buf),0,1,getscientific(cursheet,cursheet->curx,cursheet->cury,cursheet->curz),-1,getcont(cursheet,cursheet->curx,cursheet->cury,cursheet->curz,clocked));
        s=buf+strlen(buf);
      }
      else if (c==K_BACKSPACE)
      {
        print(buf,sizeof(buf),0,1,getscientific(cursheet,cursheet->curx,cursheet->cury,cursheet->curz),-1,getcont(cursheet,cursheet->curx,cursheet->cury,cursheet->curz,clocked));
        if (strlen(buf)) *mbspos(buf+strlen(buf),-1)='\0';
        s=buf+strlen(buf);
      }
      else if (c==K_DC)
      {
        print(buf,sizeof(buf),0,1,getscientific(cursheet,cursheet->curx,cursheet->cury,cursheet->curz),-1,getcont(cursheet,cursheet->curx,cursheet->cury,cursheet->curz,clocked));
        memmove(buf,mbspos(buf,1),strlen(mbspos(buf,1))+1);
        s=buf;
      }
      else if (isalpha(c))
      {
        buf[0] = '"';
        buf[1] = c;
        buf[2] = 0;
        s=buf+2;
      }
      else
      {
        if (c < 256) buf[0]=c;
        else buf[0] = 0;
        buf[1]='\0';
        s=buf+1;
      }
      do
      {
        int r;

        x=mbslen(buf)-mbslen(s);
        if ((r=line_edit(cursheet,buf,sizeof(buf),clocked ? _("Clocked cell contents:") : _("Cell contents:"),&x,&offx))<0) return r;
        s=buf;
        if (buf[0] == '"' && buf[strlen(buf)-1] != '"' && strlen(buf)+1 < sizeof(buf)) {
            buf[strlen(buf)+1] = 0;
            buf[strlen(buf)] = '"';
        }
        t=scan(&s);
      } while (*s!='\0' && t==(Token**)0);
    }
    if (t!=(Token**)0 && *t==(Token*)0) { free(t); t=(Token**)0; }
    moveto(cursheet,curx,cury,curz);
    putcont(cursheet,cursheet->curx,cursheet->cury,cursheet->curz,t,clocked);
    forceupdate(cursheet);
  }
  return 0;
}
/*}}}*/
/* do_label       -- modify cell label */ /*{{{*/
static int do_label(Sheet *sheet)
{
  /* variables */ /*{{{*/
  char buf[1024],oldlabel[1024];
  size_t edx,offx,ok;
  Token t;
  int x,y,z,x1,y1,z1,x2,y2,z2;
  int c;
  /*}}}*/

  assert(sheet!=(Sheet*)0);
  if (sheet->mark1x==-1 && locked(sheet,sheet->curx,sheet->cury,sheet->curz)) line_msg(_("Cell label:"),_("Cell is locked"));
  else
  {
    get_mark(sheet, &x1, &x2, &y1, &y2, &z1, &z2, 0);
    ok=edx=offx=0;
    (void)strcpy(buf,getlabel(sheet,sheet->curx,sheet->cury,sheet->curz));
    (void)strcpy(oldlabel,buf);
    do
    {
      if ((c=line_idedit(buf,sizeof(buf),_("Cell label:"),&edx,&offx))<0) return c;
      if (buf[0]=='\0') ok=1;
      else
      {
        ok=((t=findlabel(sheet,buf)).type==EEK || (t.type==LOCATION && t.u.location[0]==sheet->curx && t.u.location[1]==sheet->cury && t.u.location[2]==sheet->curz));
        tfree(&t);
      }
    } while (!ok);
    setlabel(sheet,sheet->curx,sheet->cury,sheet->curz,buf,1);
    if (buf[0]!='\0') for (x=x1; x<=x2; ++x) for (y=y1; y<=y2; ++y) for (z=z1; z<=z2; ++z) relabel(sheet,oldlabel,buf,x,y,z);
  }
  return -1;
}
/*}}}*/
/* do_columnwidth -- set the column width */ /*{{{*/
static int do_columnwidth(Sheet *cursheet)
{
  /* variables */ /*{{{*/
  size_t edx,offx;
  int n;
  int x,x1,x2,z,z1,z2;
  int c;
  /*}}}*/

  offx=0;
  edx=0;
  n=columnwidth(cursheet,cursheet->curx,cursheet->curz);
  do if ((c=line_numedit(&n,_("Column width:"),&edx,&offx))<0) return c; while (n<=0);
  if (cursheet->mark1x==-1)
  /* range is the current cell */ /*{{{*/
  {
    x1=x2=cursheet->curx;
    z1=z2=cursheet->curz;
  }
  /*}}}*/
  else
  /* range is the marked cube */ /*{{{*/
  {
    x1=cursheet->mark1x; x2=cursheet->mark2x;
    z1=cursheet->mark1z; z2=cursheet->mark2z;
  }
  /*}}}*/
  for (x=x1; x<=x2; ++x) for (z=z1; z<=z2; ++z) setwidth(cursheet,x,z,n);
  return -1;
}
/*}}}*/
/* do_attribute   -- set cell attributes */ /*{{{*/
static void do_attribute(Sheet *cursheet, Key action)
{
  /* variables */ /*{{{*/
  int x,y,z;
  int x1,y1,z1;
  int x2,y2,z2;
  int c = 0;
  /*}}}*/

  get_mark(cursheet, &x1, &x2, &y1, &y2, &z1, &z2, 0);

  if (action != ADJUST_LOCK && cursheet->mark1x==-1 &&  action != ADJUST_LOCK && locked(cursheet,cursheet->curx,cursheet->cury,cursheet->curz))
  {
    line_msg(_("Cell attribute:"),_("Cell is locked"));
    return;
  }
  switch ((int)action)
  {
    /* 0       -- adjust left */ /*{{{*/
    case ADJUST_LEFT:
    {
      if (cursheet->mark1x != -1 && line_ok(_("Make block left-adjusted:"), 0) <= 0) break;
      for (x=x1; x<=x2; ++x) for (y=y1; y<=y2; ++y) for (z=z1; z<=z2; ++z) setadjust(cursheet,x,y,z,LEFT);
      break;
    }
    /*}}}*/
    /* 1       -- adjust right */ /*{{{*/
    case ADJUST_RIGHT:
    {
      if (cursheet->mark1x != -1 && line_ok(_("Make block right-adjusted:"), 0) <= 0) break;
      for (x=x1; x<=x2; ++x) for (y=y1; y<=y2; ++y) for (z=z1; z<=z2; ++z) setadjust(cursheet,x,y,z,RIGHT);
      break;
    }
    /*}}}*/
    /* 2       -- adjust centered */ /*{{{*/
    case ADJUST_CENTER:
    {
      if (cursheet->mark1x != -1 && line_ok(_("Make block centered:"), 0) <= 0) break;
      for (x=x1; x<=x2; ++x) for (y=y1; y<=y2; ++y) for (z=z1; z<=z2; ++z) setadjust(cursheet,x,y,z,CENTER);
      break;
    }
    /*}}}*/
    /* 3       -- set scientific notation flag */ /*{{{*/
    case ADJUST_SCIENTIFIC:
    {
      int n;

      if (cursheet->mark1x==-1) n = !getscientific(cursheet,x1,y1,z1);
      else n = line_ok(_("Make block notation scientific:"), getscientific(cursheet,x1,y1,z1));
      if (n >= 0) for (x=x1; x<=x2; ++x) for (y=y1; y<=y2; ++y) for (z=z1; z<=z2; ++z) setscientific(cursheet,x,y,z,n);
      break;
    }
    /*}}}*/
    /* 5       -- set precision */ /*{{{*/
    case ADJUST_PRECISION:
    {
      size_t ex,offx;
      int n;

      offx=0;
      ex=0;
      n=getprecision(cursheet,x1,y1,z1);
      do if (line_numedit(&n,cursheet->mark1x==-1 ? _("Precision for cell:") : _("Precision for block:"),&ex,&offx)==-1) return; while (n!=-1 && (n==0 || n>20));
      for (x=x1; x<=x2; ++x) for (y=y1; y<=y2; ++y) for (z=z1; z<=z2; ++z) setprecision(cursheet,x,y,z,n);
      break;
    }
    /*}}}*/
    /* 6       -- shadow */ /*{{{*/
    case ADJUST_SHADOW:
    {
      int n;

      if (cursheet->mark1x==-1) n = !shadowed(cursheet,x1,y1,z1);
      else n = line_ok(_("Shadow block:"), shadowed(cursheet,x1,y1,z1));
      if (x1 == 0 && n == 1) {
        line_msg(_("Shadow cell:"),_("You can not shadow cells in column 0"));
        break;
      }

      if (n >= 0) {
        for (x=x1; x<=x2; ++x) for (y=y1; y<=y2; ++y) for (z=z1; z<=z2; ++z)
        {
          int rx;

          if (n==0) for (rx=x+1; shadowed(cursheet,rx,y,z); ++rx) shadow(cursheet,rx,y,z,0);
          else if (x>0) shadow(cursheet,x,y,z,1);
        }
      }
      break;
    }
    /*}}}*/
    /* 7       -- transparent */ /*{{{*/
    case ADJUST_TRANSPARENT:
    {
      int n;

      if (cursheet->mark1x==-1) n = !transparent(cursheet,x1,y1,z1);
      else n = line_ok(_("Make block transparent:"), transparent(cursheet,x1,y1,z1));
      if (n >= 0) for (x=x1; x<=x2; ++x) for (y=y1; y<=y2; ++y) for (z=z1; z<=z2; ++z) maketrans(cursheet,x,y,z,n);
      break;
    }
    /*}}}*/
    /* 8       -- bold */ /*{{{*/
    case ADJUST_BOLD:
    {
      int n;

      if (cursheet->mark1x==-1) n = !isbold(cursheet,x1,y1,z1);
      else n = line_ok(_("Make block bold:"), isbold(cursheet,x1,y1,z1));
      if (n >= 0) for (x=x1; x<=x2; ++x) for (y=y1; y<=y2; ++y) for (z=z1; z<=z2; ++z) bold(cursheet,x,y,z,n);
      break;
    }
    /*}}}*/
    /* 9       -- underline */ /*{{{*/
    case ADJUST_UNDERLINE:
    {
      int n;

      if (cursheet->mark1x==-1) n = !underlined(cursheet,x1,y1,z1);
      else n = line_ok(_("Make block underlined:"), underlined(cursheet,x1,y1,z1));
      if (n >= 0) for (x=x1; x<=x2; ++x) for (y=y1; y<=y2; ++y) for (z=z1; z<=z2; ++z) underline(cursheet,x,y,z,n);
      break;
    }
    /*}}}*/
    /* 1       -- edit label and goto end */ /*{{{*/
    case ADJUST_LABEL:
    {
      do_label(cursheet);
      return;
    }
    /*}}}*/
    /* 2       -- lock */ /*{{{*/
    case ADJUST_LOCK:
    {
      int n;

      if (cursheet->mark1x==-1) n = !locked(cursheet,x1,y1,z1);
      else n = line_ok(_("Lock block:"), locked(cursheet,x1,y1,z1));
      if (n >= 0) for (x=x1; x<=x2; ++x) for (y=y1; y<=y2; ++y) for (z=z1; z<=z2; ++z) lockcell(cursheet,x,y,z,n);

      break;
    }
    /*}}}*/
    /* 3       -- ignore */ /*{{{*/
    case ADJUST_IGNORE:
    {
      int n;

      if (cursheet->mark1x==-1) n = !ignored(cursheet,x1,y1,z1);
      else n = line_ok(_("Ignore values of all cells in this block:"), ignored(cursheet,x1,y1,z1));
      if (n >= 0) for (x=x1; x<=x2; ++x) for (y=y1; y<=y2; ++y) for (z=z1; z<=z2; ++z) igncell(cursheet,x,y,z,n);
      break;
    }
    /*}}}*/
    /* default -- should not happen */ /*{{{*/
      default: assert(0);
    /*}}}*/
  }
  if (c>=0)
  {
    if (cursheet->mark1x==-1) redraw_cell(cursheet, cursheet->curx, cursheet->cury, cursheet->curz);
    else redraw_sheet(cursheet);
  }
  forceupdate(cursheet);
  return;
}
/*}}}*/
/* do_savexdr     -- save sheet as XDR file */ /*{{{*/
static int do_savexdr(Sheet *cursheet, const char *name)
{
  char buf[PATH_MAX];
  const char *msg;
  unsigned int count;

  if (!name) {
      name = cursheet->name;

      if (strcmp(name+strlen(name)-3,".tp")) {
        snprintf(buf, sizeof(buf), "%s.tp", name);
        name = buf;
      }
  }

  if ((msg = savexdr(cursheet, name, &count))) {
    line_msg(_("Save sheet to XDR file:"), msg);
    return -2;
  }

  snprintf(buf, sizeof(buf), _("%u cells written"), count);
  if (!batch) line_msg(_("Save sheet to XDR file:"),buf);
  return -1;
}
/*}}}*/
/* do_saveport    -- save sheet as portable ASCII file */ /*{{{*/
static int do_saveport(Sheet *cursheet, const char *name)
{
  char buf[PATH_MAX];
  const char *msg;
  unsigned int count;

  if (!name) name = cursheet->name;

  if ((msg = saveport(cursheet, name, &count))) {
    line_msg(_("Save sheet to ASCII file:"),msg);
    return -2;
  }

  snprintf(buf, sizeof(buf), _("%u cells written"), count);
  if (!batch) line_msg(_("Save sheet to ASCII file:"),buf);
  return -1;
}
/*}}}*/
/* do_savetbl     -- save sheet as tbl file */ /*{{{*/
static int do_savetbl(Sheet *cursheet, const char *name)
{
  char buf[PATH_MAX];
  const char *msg;
  int standalone=0;
  int x1,y1,z1,x2,y2,z2;
  unsigned int count;

  get_mark(cursheet, &x1, &x2, &y1, &y2, &z1, &z2, 1);
  if (!name) {
    name = cursheet->name;
    if ((standalone=line_ok(_("Save as stand-alone document:"),1))<0) return standalone;
  }

  if ((msg = savetbl(cursheet, name, !standalone, x1, y1, z1, x2, y2, z2, &count))) {
    line_msg(_("Save in tbl format to file:"),msg);
    return -2;
  }

  snprintf(buf, sizeof(buf), _("%u cells written"), count);
  if (!batch) line_msg(_("Save in tbl format to file:"), buf);
  return -1;
}
/*}}}*/
/* do_savelatex   -- save sheet as LaTeX file */ /*{{{*/
static int do_savelatex(Sheet *cursheet, const char *name)
{
  char buf[PATH_MAX];
  const char *msg;
  int standalone=0;
  int x1,y1,z1,x2,y2,z2;
  unsigned int count;

  get_mark(cursheet, &x1, &x2, &y1, &y2, &z1, &z2, 1);
  if (!name) {
    name = cursheet->name;
    if ((standalone=line_ok(_("Save as stand-alone document:"),1))<0) return standalone;
  }

  if ((msg = savelatex(cursheet, name, !standalone, x1, y1, z1, x2, y2, z2, &count))) {
    line_msg(_("Save in LaTeX format to file:"),msg);
    return -2;
  }

  snprintf(buf, sizeof(buf), _("%u cells written"), count);
  if (!batch) line_msg(_("Save in LaTeX format to file:"), buf);
  return -1;
}
/*}}}*/
/* do_savecontext   -- save sheet as ConTeXt file */ /*{{{*/
static int do_savecontext(Sheet *cursheet, const char *name)
{
  char buf[PATH_MAX];
  const char *msg;
  int standalone=0;
  int x1,y1,z1,x2,y2,z2;
  unsigned int count;

  get_mark(cursheet, &x1, &x2, &y1, &y2, &z1, &z2, 1);
  if (!name) {
    name = cursheet->name;
    if ((standalone=line_ok(_("Save as stand-alone document:"),1))<0) return standalone;
  }

  if ((msg = savecontext(cursheet, name, !standalone, x1, y1, z1, x2, y2, z2, &count))) {
    line_msg(_("Save in ConTeXt format to file:"),msg);
    return -2;
  }

  snprintf(buf, sizeof(buf), _("%u cells written"), count);
  if (!batch) line_msg(_("Save in ConTeXt format to file:"), buf);
  return -1;
}
/*}}}*/
/* do_savehtml    -- save sheet as HTML file */ /*{{{*/
static int do_savehtml(Sheet *cursheet, const char *name)
{
  char buf[PATH_MAX];
  const char *msg;
  int standalone=0;
  int x1,y1,z1,x2,y2,z2;
  unsigned int count;

  get_mark(cursheet, &x1, &x2, &y1, &y2, &z1, &z2, 1);
  if (!name) {
    name = cursheet->name;
    if ((standalone=line_ok(_("Save as stand-alone document:"),1))<0) return standalone;
  }

  if ((msg = savehtml(cursheet, name, !standalone, x1, y1, z1, x2, y2, z2, &count))) {
    line_msg(_("Save in HTML format to file:"),msg);
    return -2;
  }

  snprintf(buf, sizeof(buf), _("%u cells written"), count);
  if (!batch) line_msg(_("Save in HTML format to file:"), buf);
  return -1;
}
/*}}}*/
/* do_savetext    -- save sheet as formatted text file */ /*{{{*/
static int do_savetext(Sheet *cursheet, const char *name)
{
  char buf[PATH_MAX];
  const char *msg;
  int x1,y1,z1,x2,y2,z2;
  unsigned int count;

  get_mark(cursheet, &x1, &x2, &y1, &y2, &z1, &z2, 1);
  if (!name) name = cursheet->name;

  if ((msg = savetext(cursheet, name, x1, y1, z1, x2, y2, z2, &count))) {
    line_msg(_("Save in plain text format to file:"),msg);
    return -2;
  }

  snprintf(buf, sizeof(buf), _("%u cells written"), count);
  if (!batch) line_msg(_("Save in plain text format to file:"), buf);
  return -1;
}
/*}}}*/
/* do_savecsv     -- save sheet as CSV file */ /*{{{*/
static int do_savecsv(Sheet *cursheet, const char *name)
{
  char buf[PATH_MAX];
  const char *msg;
  int x1,y1,z1,x2,y2,z2;
  unsigned int count;
  int sep = 0;
  const char seps[4] = ",;\t";

  get_mark(cursheet, &x1, &x2, &y1, &y2, &z1, &z2, 1);
  if (!name) {
    MenuChoice menu[4];
    name = cursheet->name;

    menu[0].str=mystrmalloc(_("cC)omma (,)")); menu[0].c='\0';
    menu[1].str=mystrmalloc(_("sS)emicolon (;)")); menu[1].c='\0';
    menu[2].str=mystrmalloc(_("tT)ab (\\t)")); menu[2].c='\0';
    menu[3].str=(char*)0;
    sep=line_menu(_("Choose separator:"),menu,0);
    if (sep < 0) return sep;
  }

  if ((msg = savecsv(cursheet, name, seps[sep], x1, y1, z1, x2, y2, z2, &count))) {
    line_msg(_("Save in CSV format to file:"),msg);
    return -2;
  }

  snprintf(buf, sizeof(buf), _("%u cells written"), count);
  if (!batch) line_msg(_("Save in CSV format to file:"), buf);
  return -1;
}
/*}}}*/
/* do_loadxdr     -- load sheet from XDR file */ /*{{{*/
static int do_loadxdr(Sheet *cursheet)
{
  const char *msg;

  if ((msg=loadxdr(cursheet,cursheet->name))!=(const char*)0) line_msg(_("Load sheet from XDR file:"),msg);
  return -1;
}
/*}}}*/
/* do_loadport    -- load sheet from portable ASCII file */ /*{{{*/
static int do_loadport(Sheet *cursheet)
{
  const char *msg;
  /*}}}*/

  if ((msg=loadport(cursheet,cursheet->name))!=(const char*)0) line_msg(_("Load sheet from ASCII file:"),msg);
  return -1;
}
/*}}}*/
/* do_loadsc      -- load sheet from SC file */ /*{{{*/
static int do_loadsc(Sheet *cursheet)
{
  const char *msg;

  if ((msg=loadsc(cursheet,cursheet->name))!=(const char*)0) line_msg(_("Load sheet from SC file:"),msg);
  return -1;
}
/*}}}*/
/* do_loadwk1     -- load sheet from WK1 file */ /*{{{*/
static int do_loadwk1(Sheet *cursheet)
{
  const char *msg;

  if ((msg=loadwk1(cursheet,cursheet->name))!=(const char*)0) line_msg(_("Load sheet from WK1 file:"),msg);
  return -1;
}
/*}}}*/
/* do_loadcsv     -- load/merge sheet from CSV file */ /*{{{*/
static int do_loadcsv(Sheet *cursheet)
{
  const char *msg;

  if ((msg=loadcsv(cursheet,cursheet->name))!=(const char*)0) line_msg(_("Load sheet from CSV file:"),msg);
  return -1;
}
/*}}}*/
/* do_mark        -- set mark */ /*{{{*/
void do_mark(Sheet *cursheet, int force)
{
  if (force==0)
  {
    if (!cursheet->marking && cursheet->mark1x==-1) force=1;
    else if (cursheet->marking) force=2;
    else force=3;
  }
  switch (force)
  {
    case 1:
    {
      cursheet->mark1x=cursheet->mark2x=cursheet->curx;
      cursheet->mark1y=cursheet->mark2y=cursheet->cury;
      cursheet->mark1z=cursheet->mark2z=cursheet->curz;
      cursheet->marking=1;
      break;
    }
    case 2:
    {
      cursheet->marking=0;
      break;
    }
    case 3:
    {
      cursheet->mark1x=-1;
      break;
    }
    default: assert(0);
  }
}
/*}}}*/
static int do_name(Sheet *cursheet);
/* do_save        -- save sheet */ /*{{{*/
static int do_save(Sheet *cursheet)
{
    const char *ext = cursheet->name;
    if (ext==(char*)0) return do_name(cursheet);

    ext += strlen(ext)-1;

    if (!strcmp(ext-3, ".tpa")) return do_saveport(cursheet, NULL);
    if (!strcmp(ext-3, ".tbl")) return do_savetbl(cursheet, NULL);
    if (!strcmp(ext-5, ".latex")) return do_savelatex(cursheet, NULL);
    if (!strcmp(ext-4, ".html")) return do_savehtml(cursheet, NULL);
    if (!strcmp(ext-3, ".csv")) return do_savecsv(cursheet, NULL);
    if (!strcmp(ext-3, ".txt")) return do_savetext(cursheet, NULL);
    if (!strcmp(ext-3, ".tex")) return do_savecontext(cursheet, NULL);
    return do_savexdr(cursheet, NULL);
}
/*}}}*/
/* do_name        -- (re)name sheet */ /*{{{*/
static int do_name(Sheet *cursheet)
{
    const char *name;

    name = line_file(cursheet->name, _("Teapot \t*.tp\nTeapot ASCII \t*.tpa\ntbl \t*.tbl\nLaTeX \t*.latex\nHTML \t*.html\nCSV \t*.csv\nFormatted ASCII \t*.txt\nConTeXt \t*.tex"), _("New file name:"), 1);
    if (!name) return -1;

    if (cursheet->name!=(char*)0) free(cursheet->name);
    cursheet->name=strdup(name);
    return do_save(cursheet);
}
/*}}}*/
/* do_load        -- load sheet */ /*{{{*/
static int do_load(Sheet *cursheet)
{
    const char *name, *ext;

    if (doanyway(cursheet, _("Sheet modified, load new file anyway?")) != 1) return -1;

    name = line_file(cursheet->name, _("Teapot \t*.tp\nTeapot ASCII \t*.tpa\nSC Spreadsheet Calculator \t*.sc\nLotus 1-2-3 \t*.wk1\nCSV \t*.csv"), _("Load sheet:"), 0);
    if (!name) return -1;
    if (cursheet->name!=(char*)0) free(cursheet->name);
    cursheet->name=strdup(name);

    ext = name+strlen(name)-1;
    if (!strcmp(ext-3, ".tpa")) return do_loadport(cursheet);
    if (!strcmp(ext-2, ".sc")) return do_loadsc(cursheet);
    if (!strcmp(ext-3, ".wk1")) return do_loadwk1(cursheet);
    if (!strcmp(ext-3, ".csv")) return do_loadcsv(cursheet);
    return do_loadxdr(cursheet);
}
/*}}}*/

/* do_clear       -- clear block */ /*{{{*/
static int do_clear(Sheet *sheet)
{
  /* variables */ /*{{{*/
  int x,y,z;
  int x1,y1,z1;
  int x2,y2,z2;
  int c;
  /*}}}*/

  if (sheet->mark1x==-1 && locked(sheet,sheet->curx,sheet->cury,sheet->curz)) line_msg(_("Clear cell:"),_("Cell is locked"));
  else
  {
    if (sheet->mark1x!=-1)
    {
      if ((c=line_ok(_("Clear block:"),0))<0) return c;
      else if (c!=1) return -1;
    }
    get_mark(sheet, &x1, &x2, &y1, &y2, &z1, &z2, 0);
    for (x=x1; x<=x2; ++x) for (y=y1; y<=y2; ++y) for (z=z1; z<=z2; ++z) freecell(sheet,x,y,z);
    cachelabels(sheet);
    forceupdate(sheet);
  }
  return -1;
}
/*}}}*/
/* do_insert      -- insert block */ /*{{{*/
static int do_insert(Sheet *sheet)
{
  /* variables */ /*{{{*/
  int x1,y1,z1,x2,y2,z2,reply;
  /*}}}*/

  /* ask for direction of insertation */ /*{{{*/
  {
    MenuChoice menu[4];

    menu[0].str=mystrmalloc(_("cC)olumn")); menu[0].c='\0';
    menu[1].str=mystrmalloc(_("rR)ow")); menu[1].c='\0';
    menu[2].str=mystrmalloc(_("dD)epth")); menu[2].c='\0';
    menu[3].str=(char*)0;
    reply=line_menu(_("Insert:"),menu,0);
    free(menu[0].str);
    free(menu[1].str);
    free(menu[2].str);
    if (reply<0) return reply;
  }
  /*}}}*/
  if (sheet->mark1x==-1)
  /* ask if current cell or whole dimension should be used */ /*{{{*/
  {
    /* variables */ /*{{{*/
    MenuChoice menu[3];
    int r;
    /*}}}*/

    /* show menu */ /*{{{*/
    switch (reply)
    {
      case 0: menu[0].str=mystrmalloc(_("wW)hole column")); break;
      case 1: menu[0].str=mystrmalloc(_("wW)hole line")); break;
      case 2: menu[0].str=mystrmalloc(_("wW)hole sheet")); break;
      default: assert(0);
    }
    menu[1].str=mystrmalloc(_("sS)ingle cell")); menu[1].c='\0';
    menu[2].str=(char*)0;
    r=line_menu(_("Insert:"),menu,0);
    free(menu[0].str);
    free(menu[1].str);
    /*}}}*/
    switch (r)
    {
      /*  0 -- use whole dimension */ /*{{{*/
      case 0:
      {
        switch (reply)
        {
          /*  0 -- use whole column */ /*{{{*/
          case 0:
          {
            x1=x2=sheet->curx;
            y1=0; y2=sheet->dimy;
            z1=z2=sheet->curz;
            break;
          }
          /*}}}*/
          /*  1 -- use whole line */ /*{{{*/
          case 1:
          {
            x1=0; x2=sheet->dimx;
            y1=y2=sheet->cury;
            z1=z2=sheet->curz;
            break;
          }
          /*}}}*/
          /*  2 -- use whole layer */ /*{{{*/
          case 2:
          {
            x1=0; x2=sheet->dimx;
            y1=0; y2=sheet->dimy;
            z1=z2=sheet->curz;
            break;
          }
          /*}}}*/
          /*  default -- should not happen */ /*{{{*/
          default: assert(0);
          /*}}}*/
        }
        break;
      }
      /*}}}*/
      /*  1 -- use current cell */ /*{{{*/
      case 1:
      {
        x1=x2=sheet->curx;
        y1=y2=sheet->cury;
        z1=z2=sheet->curz;
        break;
      }
      /*}}}*/
      /* -2,-1 -- go up or abort */ /*{{{*/
      case -2:
      case -1: return r;
      /*}}}*/
      /* default -- should not happen */ /*{{{*/
      default: assert(0);
      /*}}}*/
    }
  }
  /*}}}*/
  else
  /* range is the marked cube */ /*{{{*/
  {
      get_mark(sheet, &x1, &x2, &y1, &y2, &z1, &z2, 0);
  }
  /*}}}*/
  switch (reply)
  {
    /*  0      -- columns */ /*{{{*/
    case 0: insertcube(sheet,x1,y1,z1,x2,y2,z2,IN_X); break;
    /*}}}*/
    /*  1      -- rows */ /*{{{*/
    case 1: insertcube(sheet,x1,y1,z1,x2,y2,z2,IN_Y); break;
    /*}}}*/
    /*  2      -- depth */ /*{{{*/
    case 2: insertcube(sheet,x1,y1,z1,x2,y2,z2,IN_Z); break;
    /*}}}*/
  }
  return 0;
}
/*}}}*/
/* do_delete      -- delete block */ /*{{{*/
static int do_delete(Sheet *sheet)
{
  /* variables */ /*{{{*/
  int x1,y1,z1,x2,y2,z2,reply;
  /*}}}*/

  firstmenu:
  /* ask for direction of deletion */ /*{{{*/
  {
    MenuChoice menu[4];

    menu[0].str=mystrmalloc(_("cC)olumn")); menu[0].c='\0';
    menu[1].str=mystrmalloc(_("rR)ow")); menu[1].c='\0';
    menu[2].str=mystrmalloc(_("dD)epth")); menu[2].c='\0';
    menu[3].str=(char*)0;
    reply=line_menu(_("Delete:"),menu,0);
    free(menu[0].str);
    free(menu[1].str);
    free(menu[2].str);
    if (reply<0) return reply;
  }
  /*}}}*/
  if (sheet->mark1x==-1)
  /* ask if range is the current cell or whole dimension should be used */ /*{{{*/
  {
    /* variables */ /*{{{*/
    MenuChoice menu[3];
    int r;
    /*}}}*/

    /* show menu */ /*{{{*/
    switch (reply)
    {
      case 0: menu[0].str=mystrmalloc(_("wW)hole column")); break;
      case 1: menu[0].str=mystrmalloc(_("wW)hole line")); break;
      case 2: menu[0].str=mystrmalloc(_("wW)hole sheet")); break;
      default: assert(0);
    }
    menu[1].str=mystrmalloc(_("sS)ingle cell")); menu[1].c='\0';
    menu[2].str=(char*)0;
    r=line_menu(_("Delete:"),menu,0);
    free(menu[0].str);
    free(menu[1].str);
    /*}}}*/
    switch (r)
    {
      /*  0 -- use whole dimension */ /*{{{*/
      case 0:
      {
        switch (reply)
        {
          /*  0 -- use whole column */ /*{{{*/
          case 0:
          {
            x1=x2=sheet->curx;
            y1=0; y2=sheet->dimy;
            z1=z2=sheet->curz;
            break;
          }
          /*}}}*/
          /*  1 -- use whole line */ /*{{{*/
          case 1:
          {
            x1=0; x2=sheet->dimx;
            y1=y2=sheet->cury;
            z1=z2=sheet->curz;
            break;
          }
          /*}}}*/
          /*  2 -- use whole layer */ /*{{{*/
          case 2:
          {
            x1=0; x2=sheet->dimx;
            y1=0; y2=sheet->dimy;
            z1=z2=sheet->curz;
            break;
          }
          /*}}}*/
          /*  default -- should not happen */ /*{{{*/
          default: assert(0);
          /*}}}*/
        }
        break;
      }
      /*}}}*/
      /*  1 -- use current cell */ /*{{{*/
      case 1:
      {
        x1=x2=sheet->curx;
        y1=y2=sheet->cury;
        z1=z2=sheet->curz;
        break;
      }
      /*}}}*/
      /* -1 -- abort */ /*{{{*/
      case -1: return -1;
      /*}}}*/
      /* -2 -- go back to previous menu */ /*{{{*/
      case -2: goto firstmenu;
      /*}}}*/
      /* default -- should not happen */ /*{{{*/
      default: assert(0);
      /*}}}*/
    }
  }
  /*}}}*/
  else
  /* range is the marked cube */ /*{{{*/
  {
      get_mark(sheet, &x1, &x2, &y1, &y2, &z1, &z2, 0);
  }
  /*}}}*/
  switch(reply)
  {
    /*  0      -- columns */ /*{{{*/
    case 0: deletecube(sheet,x1,y1,z1,x2,y2,z2,IN_X); break;
    /*}}}*/
    /*  1      -- rows */ /*{{{*/
    case 1: deletecube(sheet,x1,y1,z1,x2,y2,z2,IN_Y); break;
    /*}}}*/
    /*  2      -- depth */ /*{{{*/
    case 2: deletecube(sheet,x1,y1,z1,x2,y2,z2,IN_Z); break;
    /*}}}*/
  }
  return -1;
}
/*}}}*/
/* do_move        -- copy or move a block */ /*{{{*/
static int do_move(Sheet *sheet, int copy, int force)
{
  int c;

  c=-1;
  if (sheet->mark1x==-1) line_msg(copy ? _("Copy block:") : _("Move block:"),_("No block marked"));
  else if (force || (c=line_ok(copy ? _("Copy block:") : _("Move block:"),0))==1)
  {
    int x1,y1,z1;
    int x2,y2,z2;

    get_mark(sheet, &x1, &x2, &y1, &y2, &z1, &z2, 0);
    moveblock(sheet,x1,y1,z1,x2,y2,z2,sheet->curx,sheet->cury,sheet->curz,copy);
    if (!copy) sheet->mark1x=-1;
  }
  if (c<0) return c; else return -1;
}
/*}}}*/
/* do_fill        -- fill a block */ /*{{{*/
static int do_fill(Sheet *sheet)
{
  /* variables */ /*{{{*/
  size_t offx,edx;
  int cols,rows,layers;
  int x,y,z;
  int x1,y1,z1;
  int x2,y2,z2;
  int c;
  /*}}}*/

  if (sheet->mark1x==-1) line_msg(_("Fill block:"),_("No block marked"));
  else
  {
    get_mark(sheet, &x1, &x2, &y1, &y2, &z1, &z2, 0);
    cols=rows=layers=1;
    firstmenu:
    offx=0;
    edx=0;
    do if ((c=line_numedit(&cols,_("Number of column-wise repetitions:"),&edx,&offx))<0) return c; while (cols<=0);
    secondmenu:
    offx=0;
    edx=0;
    do
    {
      c=line_numedit(&rows,_("Number of row-wise repetitions:"),&edx,&offx);
      if (c==-1) return -1;
      else if (c==-2) goto firstmenu;
    } while (rows<=0);
    offx=0;
    edx=0;
    do
    {
      c=line_numedit(&layers,_("Number of depth-wise repetitions:"),&edx,&offx);
      if (c==-1) return -1;
      else if (c==-2) goto secondmenu;
    } while (layers<=0);
    for (x=0; x<cols; ++x) for (y=0; y<rows; ++y) for (z=0; z<layers; ++z) moveblock(sheet,x1,y1,z1,x2,y2,z2,sheet->curx+x*(x2-x1+1),sheet->cury+y*(y2-y1+1),sheet->curz+z*(z2-z1+1),1);
  }
  return -1;
}
/*}}}*/
/* do_sort        -- sort block */ /*{{{*/
static int do_sort(Sheet *sheet)
{
  /* variables */ /*{{{*/
  MenuChoice menu1[4],menu2[3],menu3[3];
  Sortkey sk[MAX_SORTKEYS];
  unsigned int key;
  size_t x,offx;
  const char *msg;
  Direction in_dir=(Direction)-2; /* cause run time error */
  int x1,y1,z1,x2,y2,z2;
  int doit=-1;
  int c;
  int last;
  /*}}}*/

  /* note and order block coordinates */ /*{{{*/
  get_mark(sheet, &x1, &x2, &y1, &y2, &z1, &z2, 1);
  /*}}}*/
  /* build menues */ /*{{{*/
  menu1[0].str=mystrmalloc(_("cC)olumn"));     menu1[0].c='\0';
  menu1[1].str=mystrmalloc(_("rR)ow"));     menu1[1].c='\0';
  menu1[2].str=mystrmalloc(_("dD)epth"));     menu1[2].c='\0';
  menu1[3].str=(char*)0;
  menu2[0].str=mystrmalloc(_("sS)ort region"));  menu2[0].c='\0';
  menu2[1].str=mystrmalloc(_("aA)dd key"));  menu2[1].c='\0';
  menu2[2].str=(char*)0;
  menu3[0].str=mystrmalloc(_("aA)scending"));  menu3[0].c='\0';
  menu3[1].str=mystrmalloc(_("dD)escending")); menu3[0].c='\0';
  menu3[2].str=(char*)0;
  /*}}}*/

  last=-1;
  /* ask for sort direction */ /*{{{*/
  zero: switch (c=line_menu(_("Sort block:"),menu1,0))
  {
    /*  0      -- in X */ /*{{{*/
    case 0: in_dir=IN_X; break;
    /*}}}*/
    /*  1      -- in Y */ /*{{{*/
    case 1: in_dir=IN_Y; break;
    /*}}}*/
    /*  2      -- in Z */ /*{{{*/
    case 2: in_dir=IN_Z; break;
    /*}}}*/
    /* -2,-1   -- abort */ /*{{{*/
    case -2:
    case -1: goto greak;
    /*}}}*/
    /* default -- should not happen */ /*{{{*/
    default: assert(0);
    /*}}}*/
  }
  last=0;
  /*}}}*/
  key=0;
  do
  {
    /* ask for positions */ /*{{{*/
    one: if (in_dir==IN_X) sk[key].x=0; else /* ask for x position */ /*{{{*/
    {
      x=0;
      offx=0;
      sk[key].x=0;
      do
      {
        c=line_numedit(&(sk[key].x),_("X position of key vector:"),&x,&offx);
        if (c==-1) goto greak;
        else if (c==-2) switch (last)
        {
          case -1: goto greak;
          case 0: goto zero;
          case 2: goto two;
          case 3: goto three;
          case 5: goto five;
        }
      } while (sk[key].x<0);
      last=1;
    }
    /*}}}*/
    two: if (in_dir==IN_Y) sk[key].y=0; else /* ask for y position */ /*{{{*/
    {
      x=0;
      offx=0;
      sk[key].y=0;
      do
      {
        c=line_numedit(&(sk[key].y),_("Y position of key vector:"),&x,&offx);
        if (c==-1) goto greak;
        else if (c==-2) switch (last)
        {
          case -1: goto greak;
          case 0: goto zero;
          case 1: goto one;
          case 3: goto three;
          case 5: goto five;
          default: assert(0);
        }
      } while (sk[key].y<0);
      last=2;
    }
    /*}}}*/
    three: if (in_dir==IN_Z) sk[key].z=0; else /* ask for z position */ /*{{{*/
    {
      x=0;
      offx=0;
      sk[key].z=0;
      do
      {
        c=line_numedit(&(sk[key].z),_("Z position of key vector:"),&x,&offx);
        if (c==-1) goto greak;
        else if (c==-2) switch (last)
        {
          case -1: goto greak;
          case 0: goto zero;
          case 1: goto one;
          case 2: goto two;
          case 5: goto five;
          default: assert(0);
        }
      } while (sk[key].z<0);
      last=3;
    }
    /*}}}*/
    /*}}}*/
    /* ask for sort key */ /*{{{*/
    four: sk[key].sortkey=0;
    switch (c=line_menu(_("Sort block:"),menu3,0))
    {
      /*  0      -- ascending */ /*{{{*/
      case 0: sk[key].sortkey|=ASCENDING; break;
      /*}}}*/
      /*  1      -- descending */ /*{{{*/
      case 1: sk[key].sortkey&=~ASCENDING; break;
      /*}}}*/
      /* -1      -- abort */ /*{{{*/
      case -1: goto greak;
      /*}}}*/
      /* -2      -- go to first menu */ /*{{{*/
      case -2: switch (last)
      {
        case -1: goto greak;
        case 1: goto one;
        case 2: goto two;
        case 3: goto three;
        default: assert(0);
      }
      /*}}}*/
      /* default -- should not happen */ /*{{{*/
      default: assert(0);
      /*}}}*/
    }
    last=4;
    /*}}}*/
    ++key;
    five:
    if (key==MAX_SORTKEYS) /* ask for sort comfirmation */ /*{{{*/
    {
      c=line_ok(_("Sort block:"),0);
      if (c==-1) goto greak;
      else if (c==-2) goto four;
      else if (c==0) doit=1;
    }
    /*}}}*/
    else /* ask for sort or adding another key */ /*{{{*/
    switch (line_menu(_("Sort block:"),menu2,0))
    {
      /*       0 -- sort it */ /*{{{*/
      case 0: doit=1; break;
      /*}}}*/
      /*       1 -- add another key */ /*{{{*/
      case 1: doit=0; break;
      /*}}}*/
      /*      -1 -- abort */ /*{{{*/
      case -1: goto greak;
      /*}}}*/
      case -2: goto four;
      /* default -- should not happen */ /*{{{*/
      default: assert(0);
      /*}}}*/
    }
    /*}}}*/
    last=5;
  } while (!doit);
  c=-1;
  if ((msg=sortblock(sheet,x1,y1,z1,x2,y2,z2,in_dir,sk,key))!=(const char*)0) line_msg(_("Sort block:"),msg);
  greak:
  /* free menues */ /*{{{*/
  free((char*)menu1[0].str);
  free((char*)menu1[1].str);
  free((char*)menu1[2].str);
  free((char*)menu2[0].str);
  free((char*)menu2[1].str);
  free((char*)menu2[2].str);
  free((char*)menu3[0].str);
  free((char*)menu3[1].str);
  free((char*)menu3[2].str);
  /*}}}*/
  return c;
}
/*}}}*/
/* do_batchsort -- sort block in a batch*/ /*{{{*/
static void do_batchsort(Sheet *sheet, Direction dir, char* arg)
{
  Sortkey sk[MAX_SORTKEYS];
  int x1,y1,z1,x2,y2,z2;
  unsigned int key = 0;
  char* next;
  while( *arg != '\0' )
  {
    while (isspace((int)*arg)) arg++;
    sk[key].x=sk[key].y=sk[key].z=sk[key].sortkey=0;
    switch (*arg)
    {
      case 'a': sk[key].sortkey|=ASCENDING; arg++; break;
      case 'd': sk[key].sortkey&=~ASCENDING; arg++; break;
    }
    if ( *arg != '\0' && dir != IN_X ) { sk[key].x=strtol(arg, &next, 10); arg = next; }
    if ( *arg != '\0' && dir != IN_Y ) { sk[key].y=strtol(arg, &next, 10); arg = next; }
    if ( *arg != '\0' && dir != IN_Z ) { sk[key].z=strtol(arg, &next, 10); arg = next; }
    key++;
  }
  get_mark(sheet, &x1, &x2, &y1, &y2, &z1, &z2, 1);
  sortblock(sheet, x1, y1, z1, x2, y2, z2, dir, sk, key);
}
/*}}}*/
/* do_mirror      -- mirror block */ /*{{{*/
static int do_mirror(Sheet *sheet)
{
  /* variables */ /*{{{*/
  int x1,y1,z1,x2,y2,z2,reply;
  /*}}}*/

  /* note and order block coordinates */ /*{{{*/
  get_mark(sheet, &x1, &x2, &y1, &y2, &z1, &z2, 1);
  /*}}}*/
  /* ask for direction of mirroring */ /*{{{*/
  {
    MenuChoice menu[4];

    menu[0].str=mystrmalloc(_("lL)eft-right")); menu[0].c='\0';
    menu[1].str=mystrmalloc(_("uU)pside-down")); menu[1].c='\0';
    menu[2].str=mystrmalloc(_("fF)ront-back")); menu[2].c='\0';
    menu[3].str=(char*)0;
    reply=line_menu(_("Mirror block:"),menu,0);
    free(menu[0].str);
    free(menu[1].str);
    free(menu[2].str);
    if (reply<0) return reply;
  }
  /*}}}*/
  switch (reply)
  {
    /*  0      -- left-right */ /*{{{*/
    case 0: mirrorblock(sheet,x1,y1,z1,x2,y2,z2,IN_X); break;
    /*}}}*/
    /*  1      -- upside-down */ /*{{{*/
    case 1: mirrorblock(sheet,x1,y1,z1,x2,y2,z2,IN_Y); break;
    /*}}}*/
    /*  2      -- front-back */ /*{{{*/
    case 2: mirrorblock(sheet,x1,y1,z1,x2,y2,z2,IN_Z); break;
    /*}}}*/
    default: assert(0);
  }
  return 0;
}
/*}}}*/
/* do_goto        -- go to a specific cell */ /*{{{*/
static int do_goto(Sheet *sheet, const char *expr)
{
  /* variables */ /*{{{*/
  char buf[1024];
  const char *s;
  size_t x,offx;
  Token **t;
  int c;
  /*}}}*/

  assert(sheet!=(Sheet*)0);
  buf[0]='\0';
  x=offx=0;
  if (expr) strcpy(buf,expr);
  else if ((c=line_edit(sheet,buf,sizeof(buf),_("Go to location:"),&x,&offx))<0) return c;
  s=buf;
  t=scan(&s);
  if (t!=(Token**)0)
  {
    Token value;

    upd_x=sheet->curx;
    upd_y=sheet->cury;
    upd_z=sheet->curz;
    upd_sheet=sheet;
    value=eval(t);
    tvecfree(t);
    if (value.type==LOCATION && value.u.location[0]>=0 && value.u.location[1]>=0 && value.u.location[2]>=0)
        moveto(sheet,value.u.location[0],value.u.location[1],value.u.location[2]);
    else
        line_msg(_("Go to location:"),_("Not a valid location"));
    tfree(&value);
  }
  return -1;
}
/*}}}*/

/* do_sheetcmd    -- process one key press */ /*{{{*/
int do_sheetcmd(Sheet *cursheet, Key c, int moveonly)
{
  switch ((int)c)
  {
    case K_GOTO: do_goto(cursheet, (const char *)0); break;
    case K_COLWIDTH: do_columnwidth(cursheet); break;
    case BLOCK_CLEAR: do_clear(cursheet); redraw_sheet(cursheet); break;
    case BLOCK_INSERT: do_insert(cursheet); redraw_sheet(cursheet); break;
    case BLOCK_DELETE: do_delete(cursheet); redraw_sheet(cursheet); break;
    case BLOCK_MOVE: do_move(cursheet,0,0); redraw_sheet(cursheet); break;
    case BLOCK_COPY: do_move(cursheet,1,0); redraw_sheet(cursheet); break;
    case BLOCK_FILL: do_fill(cursheet); redraw_sheet(cursheet); break;
    case BLOCK_SORT: do_sort(cursheet); redraw_sheet(cursheet); break;
    case BLOCK_MIRROR: do_mirror(cursheet); redraw_sheet(cursheet); break;
    case ADJUST_LEFT:
    case ADJUST_RIGHT:
    case ADJUST_CENTER:
    case ADJUST_SCIENTIFIC:
    case ADJUST_PRECISION:
    case ADJUST_SHADOW:
    case ADJUST_BOLD:
    case ADJUST_UNDERLINE:
    case ADJUST_TRANSPARENT:
    case ADJUST_LABEL:
    case ADJUST_LOCK:
    case ADJUST_IGNORE: do_attribute(cursheet, c); break;
    /* UP         -- move up */ /*{{{*/
    case K_UP:
    {
      relmoveto(cursheet, 0, -1, 0);
      break;
    }
    /*}}}*/
    /* DOWN       -- move down */ /*{{{*/
    case K_DOWN:
    {
      relmoveto(cursheet, 0, 1, 0);
      break;
    }
    /*}}}*/
    /* LEFT       -- move left */ /*{{{*/
    case K_LEFT:
    {
      relmoveto(cursheet, -1, 0, 0);
      break;
    }
    /*}}}*/
    /* RIGHT      -- move right */ /*{{{*/
    case K_RIGHT:
    {
      relmoveto(cursheet, 1, 0, 0);
      break;
    }
    /*}}}*/
    /* FIRSTL     -- move to first line */ /*{{{*/
    case K_FIRSTL:
    case '<':
    {
      moveto(cursheet, -1, 0, -1);
      break;
    }
    /*}}}*/
    /* LASTL      -- move to last line */ /*{{{*/
    case K_LASTL:
    case '>':
    {
      moveto(cursheet, -1, (cursheet->dimy ? cursheet->dimy-1 : 0), -1);
      break;
    }
    /*}}}*/
    /* HOME       -- move to beginning of line */ /*{{{*/
    case K_HOME:
    {
      moveto(cursheet, 0, -1, -1);
      break;
    }
    /*}}}*/
    /* END        -- move to end of line */ /*{{{*/
    case K_END:
    {
      moveto(cursheet, (cursheet->dimx ? cursheet->dimx-1 : 0), -1, -1);
      break;
    }
    /*}}}*/
    /* +          -- move one sheet down */ /*{{{*/
    case K_NSHEET:
    case '+':
    {
      relmoveto(cursheet, 0, 0, 1);
      break;
    }
    /*}}}*/
    /* -          -- move one sheet up */ /*{{{*/
    case K_PSHEET:
    case '-':
    {
      relmoveto(cursheet, 0, 0, -1);
      break;
    }
    /*}}}*/
    /* *          -- move to bottom sheet */ /*{{{*/
    case K_LSHEET:
    case '*':
    {
      moveto(cursheet, -1, -1, (cursheet->dimz ? cursheet->dimz-1 : 0));
      break;
    }
    /*}}}*/
    /* _          -- move to top sheet */ /*{{{*/
    case K_FSHEET:
    case '_':
    {
      moveto(cursheet, -1, -1, 0);
      break;
    }
    /*}}}*/
    /* ENTER      -- edit current cell */ /*{{{*/
    case K_ENTER: if (moveonly) break; do_edit(cursheet,'\0',(const char*)0,0); break;
    /*}}}*/
    /* MENTER     -- edit current clocked cell */ /*{{{*/
    case K_MENTER: if (moveonly) break; do_edit(cursheet,'\0',(const char*)0,1); break;
    /*}}}*/
    /* ", @, digit -- edit current cell with character already in buffer */ /*{{{*/
    case K_BACKSPACE:
    case K_DC:
    case '"':
    case '@':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': if (moveonly) break; do_edit(cursheet,c,(const char*)0,0); break;
    /*}}}*/
    /* MARK       -- toggle block marking */ /*{{{*/
    case '.':
    case K_MARK: if (moveonly) break; do_mark(cursheet,0); break;
    /*}}}*/
    /* _("Save sheet file format:")   -- save menu */ /*{{{*/
    case K_SAVEMENU: if (moveonly) break; do_save(cursheet); break;
    /*}}}*/
    /* _("Load sheet file format:")   -- load menu */ /*{{{*/
    case K_LOAD:
    case K_LOADMENU: if (moveonly) break; do_load(cursheet); break;
    /*}}}*/
    /* _("nN)ame")       -- set name */ /*{{{*/
    case K_NAME: if (moveonly) break; do_name(cursheet); break;
    /*}}}*/
#ifdef ENABLE_HELP
    case K_HELP: show_text(helpfile); break;
#else
    case K_HELP: show_text(_("Sorry, manual is not installed.")); break;
#endif
    case K_ABOUT: show_text(_("<html><head><title>About teapot</title></head><body><center><pre>\n\n"
        "               ` ',`    '  '                   \n"
        "                `   '  ` ' '                   \n"
        "                 `' '   '`'                    \n"
        "                 ' `   ' '`                    \n"
        "    '           '` ' ` '`` `                   \n"
        "    `.   Table Editor And Planner, or:         \n"
        "      ,         . ,   ,  . .                   \n"
        "                ` '   `  ' '                   \n"
        "     `::\\    /:::::::::::::::::\\   ___         \n"
        "      `::\\  /:::::::::::::::::::\\,'::::\\       \n"
        "       :::\\/:::::::::::::::::::::\\/   \\:\\      \n"
        "       :::::::::::::::::::::::::::\\    :::     \n"
        "       ::::::::::::::::::::::::::::;  /:;'     \n"
        "       `::::::::::::::::::::::::::::_/:;'      \n"
        "         `::::::::::::::::::::::::::::'        \n"
        "          `////////////////////////'           \n"
        "           `:::::::::::::::::::::'             \n"
        "</pre>\n"
        "<p>Teapot " VERSION "</p>\n"
        "\n"
        "<p>Original Version: Michael Haardt<br>\n"
        "Current Maintainer: Joerg Walter<br>\n"
        "Home Page: <a href='http://www.syntax-k.de/projekte/teapot/'>http://www.syntax-k.de/projekte/teapot/</a></p>\n"
        "\n"
        "<p>Copyright 1995-2006 Michael Haardt,<br>\n"
        "Copyright 2009-2010 Joerg Walter (<a href='mailto:info@syntax-k.de'>info@syntax-k.de</a>)</p></center>\n"
        "\f"
        "<p>This program is free software: you can redistribute it and/or modify\n"
        "it under the terms of the GNU General Public License as published by\n"
        "the Free Software Foundation, either version 3 of the License, or\n"
        "(at your option) any later version.</p>\n"
        "\n"
        "<p>This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.</p>\n"
        "\n"
        "<p>You should have received a copy of the GNU General Public License\n"
        "along with this program.  If not, see <a href='http://www.gnu.org/licenses/'>http://www.gnu.org/licenses/</a>.</p>"
        "</body></html>"));
        break;
    /* MENU, / -- main menu */ /*{{{*/
    case '/': if (!moveonly && do_sheetcmd(cursheet, show_menu(cursheet), 0)) return 1; break;
    /*}}}*/
    /* _("sS)ave")       -- save in current native format */ /*{{{*/
    case K_SAVE: do_save(cursheet); break;
    /*}}}*/
    /* _("cC)opy")       -- copy block */ /*{{{*/
    case K_COPY: if (moveonly) break; do_move(cursheet,1,1); break;
    /*}}}*/
    /* RECALC     -- recalculate */ /*{{{*/
    case K_RECALC: if (moveonly) break; forceupdate(cursheet); break;
    /*}}}*/
    /* _("Usage: clock(condition,location[,location])")      -- clock */ /*{{{*/
    case K_CLOCK:
    {
      int x,y,z;

      for (x=0; x<cursheet->dimx; ++x)
      for (y=0; y<cursheet->dimy; ++y)
      for (z=0; z<cursheet->dimz; ++z)
      clk(cursheet,x,y,z);
      update(cursheet);
      break;
    }
    /*}}}*/
    /* NPAGE      -- page down    */ /*{{{*/
    case K_NPAGE:
    {
      cursheet->offy+=(cursheet->maxy-3);
      relmoveto(cursheet, 0, cursheet->maxy-3, 0);
      break;
    }
    /*}}}*/
    /* PPAGE      -- page up    */ /*{{{*/
    case K_PPAGE:
    {
      cursheet->offy = (cursheet->offy>=(cursheet->maxy-3) ? cursheet->offy-(cursheet->maxy-3) : 0);
      relmoveto(cursheet, 0, (cursheet->cury>=(cursheet->maxy-3) ? -(cursheet->maxy-3) : -cursheet->cury), 0);
      break;
    }
    /*}}}*/
    /* FPAGE      -- page right    */ /*{{{*/
    case K_FPAGE:
    {
      cursheet->offx+=cursheet->width;
      relmoveto(cursheet, cursheet->width, 0, 0);
      break;
    }
    /*}}}*/
    /* BPAGE      -- page left */ /*{{{*/
    case K_BPAGE:
    {
      cursheet->offx=(cursheet->offx>=cursheet->width ? cursheet->offx-cursheet->width : 0);
      relmoveto(cursheet, (cursheet->curx>=cursheet->width ? -cursheet->width : -cursheet->curx), 0, 0);
      break;
    }
    /*}}}*/
    /* SAVEQUIT   -- save and quit */ /*{{{*/
    case K_SAVEQUIT:
    {
      if (moveonly) break;
      if (do_save(cursheet)!=-2) return 1;
      break;
    }
    /*}}}*/
    /* _("qQ)uit")       -- quit */ /*{{{*/
    case K_QUIT: if (moveonly) break; return 1;
    /*}}}*/
    default:
        if (isalpha(c) && !moveonly) do_edit(cursheet,c,(const char*)0,0);
        break;
  }
  return 0;
}
/*}}}*/

/* main */ /*{{{*/
int main(int argc, char *argv[])
{
  /* variables */ /*{{{*/
  Sheet sheet,*cursheet;
  int o;
  const char *loadfile;
  int always_redraw=0;
  char ln[1024];
  /*}}}*/

  setlocale(LC_ALL, "");
  find_helpfile(helpfile, sizeof(helpfile), argv[0]);

  /* parse options */ /*{{{*/
  while ((o=getopt(argc,argv,"abhnrqHp:?"))!=EOF) switch (o)
  {
    /* a       -- use ascii as default */ /*{{{*/
    case 'a':
    {
      usexdr=0;
      break;
    }
    /*}}}*/
    /* b       -- run batch */ /*{{{*/
    case 'b': batch=1; break;
    /*}}}*/
    /* n       -- no quoted strings */ /*{{{*/
    case 'n': quote=0; break;
    /*}}}*/
    /* q       -- force quoted strings */ /*{{{*/
    case 'q': quote=1; break;
    /*}}}*/
    /* H       -- no row/column headers */ /*{{{*/
    case 'H': header=0; break;
    /*}}}*/
    /* r       -- always redraw */ /*{{{*/
    case 'r':
    {
      always_redraw=1;
      break;
    }
    /*}}}*/
    /* p       -- precision */ /*{{{*/
    case 'p':
    {
      long n;
      char *end;

      n=strtol(optarg,&end,0);
      if (*end || n<0 || n>DBL_DIG)
      {
        fprintf(stderr,_("teapot: precision must be between 0 and %d.\n"),DBL_DIG);
        exit(1);
      }
      def_precision=n;
      break;
    }
    /*}}}*/
    /* default -- includes ? and h */ /*{{{*/
    default:
    {
      fprintf(stderr,_(
          "Usage: %s [-a] [-b] [-n] [-H] [-r] [-p digits] [file]\n"
          "       -a: use ASCII file format as default\n"
          "       -b: batch mode\n"
          "       -q: display strings in quotes\n"
          "       -H: hide row/column headers\n"
          "       -r: redraw more often\n"
          "       -p: set decimal precision\n"
          ), argv[0]);
      exit(1);
    }
    /*}}}*/
  }
  loadfile=(optind<argc ? argv[optind] : (const char*)0);
  /*}}}*/
  /* create empty sheet */ /*{{{*/
  cursheet=&sheet;
  cursheet->curx=cursheet->cury=cursheet->curz=0;
  cursheet->offx=cursheet->offy=0;
  cursheet->dimx=cursheet->dimy=cursheet->dimz=0;
  cursheet->sheet=(Cell**)0;
  cursheet->column=(int*)0;
  cursheet->orix=0;
  cursheet->oriy=0;
  cursheet->maxx=0;
  cursheet->maxy=0;
  cursheet->name=(char*)0;
  cursheet->mark1x=-1;
  cursheet->marking=0;
  cursheet->changed=0;
  cursheet->moveonly=0;
  cursheet->clk=0;
  (void)memset(cursheet->labelcache,0,sizeof(cursheet->labelcache));
  /*}}}*/
  /* start display */ /*{{{*/
  if (!batch) {
    display_init(&sheet, always_redraw);
    line_msg((const char*)0,"");
  }
  /*}}}*/
  if (loadfile) /* load given sheet */ /*{{{*/
  {
    const char *msg;

    cursheet->name=mystrmalloc(loadfile);
    if (usexdr)
    {
      if ((msg=loadxdr(cursheet,cursheet->name))!=(const char*)0) line_msg(_("Load sheet from XDR file:"),msg);
    }
    else
    {
      if ((msg=loadport(cursheet,cursheet->name))!=(const char*)0) line_msg(_("Load sheet from ASCII file:"),msg);
    }
  }
  /*}}}*/
  if (batch) /* process batch */ /*{{{*/
  while (fgets(ln,sizeof(ln),stdin)!=(char*)0)
  {
    /* variables */ /*{{{*/
    size_t len;
    char *cmd,*arg;
    /*}}}*/

    /* set cmd and arg */ /*{{{*/
    ++batchln;
    len=strlen(ln);
    if (len && ln[len-1]=='\n') ln[len-1]='\0';
    cmd=ln; while (isspace((int)*cmd)) ++cmd;
    arg=cmd;
    while (*arg && !isspace((int)*arg)) ++arg;
    while (isspace((int)*arg)) *arg++='\0';
    /*}}}*/

    /* goto location */ /*{{{*/
    if (strcmp(cmd,"goto")==0) do_goto(cursheet,arg);
    /*}}}*/
    /* from location */ /*{{{*/
    else if (strcmp(cmd,"from")==0)
    {
      do_goto(cursheet,arg);
      do_mark(cursheet,1);
    }
    /*}}}*/
    /* to location */ /*{{{*/
    else if (strcmp(cmd,"to")==0)
    {
      do_goto(cursheet,arg);
      do_mark(cursheet,2);
    }
    /*}}}*/
    /* save-tbl file  */ /*{{{*/
    else if (strcmp(cmd,"save-tbl")==0) do_savetbl(cursheet,arg);
    /*}}}*/
    /* save-latex file  */ /*{{{*/
    else if (strcmp(cmd,"save-latex")==0) do_savelatex(cursheet,arg);
    /*}}}*/
    /* save-context file  */ /*{{{*/
    else if (strcmp(cmd,"save-context")==0) do_savecontext(cursheet,arg);
    /*}}}*/
    /* save-csv file  */ /*{{{*/
    else if (strcmp(cmd,"save-csv")==0) do_savecsv(cursheet,arg);
    /*}}}*/
    /* save-html file  */ /*{{{*/
    else if (strcmp(cmd,"save-html")==0) do_savehtml(cursheet,arg);
    /*}}}*/
    /* load-csv file  */ /*{{{*/
    else if (strcmp(cmd,"load-csv")==0) { loadcsv(cursheet,arg); forceupdate(cursheet); }
    /*}}}*/
    /* sort in x direction */ /*{{{*/
    else if (strcmp(cmd,"sort-x")==0) do_batchsort(cursheet, IN_X, arg);
    /*}}}*/
    /* sort in y direction */ /*{{{*/
    else if (strcmp(cmd,"sort-y")==0) do_batchsort(cursheet, IN_Y, arg);
    /*}}}*/
    /* sort in z direction */ /*{{{*/
    else if (strcmp(cmd,"sort-z")==0) do_batchsort(cursheet, IN_Z, arg);
    /*}}}*/
    /* this is an unknown command */ /*{{{*/
    else line_msg(_("Unknown batch command:"),cmd);
    /*}}}*/
  }
  /*}}}*/
  else /* process interactive input */ /*{{{*/
  {
    display_main(cursheet);
    display_end();
  }
  /*}}}*/
  freesheet(cursheet,1);
  fclose(stdin);
  return 0;
}
/*}}}*/
