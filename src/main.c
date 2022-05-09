/* #includes */ /*{{{C}}}*//*{{{*/
#ifndef NO_POSIX_SOURCE
#undef  _POSIX_SOURCE
#define _POSIX_SOURCE   1
#undef  _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 2
#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <locale.h>
#include <nl_types.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
extern char *optarg;
extern int optind,opterr,optopt;
int getopt(int argc, char * const *argv, const char *optstring);
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "cat.h"
#include "default.h"
#include "display.h"
#include "eval.h"
#include "html.h"
#include "latex.h"
#include "context.h"
#include "main.h"
#include "misc.h"
#include "sc.h"
#include "scanner.h"
#include "parser.h"
#include "sheet.h"
#include "wgetc.h"
#include "wk1.h"
#include "version.h"
/*}}}*/

/* variables */ /*{{{*/
nl_catd catd;
int batch=0;
unsigned int batchln=0;
int def_precision=DEF_PRECISION;
int quote=1;
static int usexdr=1;
/*}}}*/

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
/* line_ok        -- one line yes/no menu */ /*{{{*/
static int line_ok(const char *prompt, int curx)
{
  /* variables */ /*{{{*/
  MenuChoice menu[3];
  int result;
  /*}}}*/

  /* asserts */ /*{{{*/
  assert(curx==0 || curx==1);
  /*}}}*/
  menu[0].str=mystrmalloc(NO); menu[0].c='\0';
  menu[1].str=mystrmalloc(YES); menu[1].c='\0';
  menu[2].str=(char*)0;
  result=line_menu(prompt,menu,curx);
  free(menu[0].str);
  free(menu[1].str);
  return (result);
}
/*}}}*/
/* doanyway       -- ask if action should be done despite unsaved changes */ /*{{{*/
static int doanyway(Sheet *sheet, const char *msg)
{
  int result;
  
  if (sheet->changed)
  {
    result=line_ok(msg,0);
    return result;
  }
  return 1;
}
/*}}}*/

/* do_edit        -- set or modify cell contents */ /*{{{*/
static int do_edit(Sheet *cursheet, chtype c, const char *expr, int clocked)
{
  /* variables */ /*{{{*/
  char buf[1024];
  const char *s;
  size_t x,offx;
  int curx,cury,curz;
  Token **t;
  /*}}}*/
    
  if (locked(cursheet,cursheet->curx,cursheet->cury,cursheet->curz)) line_msg(MODIFYCELL,ISLOCKED);
  else
  {
    curx=cursheet->curx;
    cury=cursheet->cury;
    curz=cursheet->curz;
    if (expr)
    {
      s=expr;
      t=scan(&s);
      if (*s!='\0' && t==(Token**)0) line_msg(clocked ? CCONTENTS : CONTENTS,"XXX invalid expression");
    }
    else
    {
      offx=0;
      if (c=='\0')
      {
        print(buf,sizeof(buf),0,1,getscientific(cursheet,cursheet->curx,cursheet->cury,cursheet->curz),-1,getcont(cursheet,cursheet->curx,cursheet->cury,cursheet->curz,clocked));
        s=buf+strlen(buf);
      }
      else if (c==KEY_BACKSPACE)
      {
        print(buf,sizeof(buf),0,1,getscientific(cursheet,cursheet->curx,cursheet->cury,cursheet->curz),-1,getcont(cursheet,cursheet->curx,cursheet->cury,cursheet->curz,clocked));
        if (strlen(buf)) buf[strlen(buf)-1]='\0';
        s=buf+strlen(buf);
      }
      else if (c==KEY_DC)
      {
        print(buf,sizeof(buf),0,1,getscientific(cursheet,cursheet->curx,cursheet->cury,cursheet->curz),-1,getcont(cursheet,cursheet->curx,cursheet->cury,cursheet->curz,clocked));
        memmove(buf,buf+1,strlen(buf));
        s=buf;
      }
      else
      {
        buf[0]=c;
        buf[1]='\0';
        s=buf+1;
      }
      do
      {
        int r;

        x=s-buf;
        if ((r=line_edit(cursheet,buf,sizeof(buf),clocked ? CCONTENTS : CONTENTS,&x,&offx))<0) return r;
        s=buf;
        t=scan(&s);
      } while (*s!='\0' && t==(Token**)0);
    }
    if (t!=(Token**)0 && *t==(Token*)0) { free(t); t=(Token**)0; }
    cursheet->curx=curx;
    cursheet->cury=cury;
    cursheet->curz=curz;
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
  if (sheet->mark1x==-1 && locked(sheet,sheet->curx,sheet->cury,sheet->curz)) line_msg(DOLABEL,ISLOCKED);
  else
  {
    if (sheet->mark1x==-1)
    /* range is the current cell */ /*{{{*/
    {
      x1=x2=sheet->curx;
      y1=y2=sheet->cury;
      z1=z2=sheet->curz;
    }
    /*}}}*/
    else
    /* range is the marked cube */ /*{{{*/
    {
      x1=sheet->mark1x; x2=sheet->mark2x; posorder(&x1,&x2);
      y1=sheet->mark1y; y2=sheet->mark2y; posorder(&y1,&y2);
      z1=sheet->mark1z; z2=sheet->mark2z; posorder(&z1,&z2);
    }
    /*}}}*/
    ok=edx=offx=0;
    (void)strcpy(buf,getlabel(sheet,sheet->curx,sheet->cury,sheet->curz));
    (void)strcpy(oldlabel,buf);
    do
    {
      if ((c=line_idedit(buf,sizeof(buf),DOLABEL,&edx,&offx))<0) return c;
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
  do if ((c=line_numedit(&n,COLWIDTH,&edx,&offx))<0) return c; while (n<=0);
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
    x1=cursheet->mark1x; x2=cursheet->mark2x; posorder(&x1,&x2);
    z1=cursheet->mark1z; z2=cursheet->mark2z; posorder(&z1,&z2);
  }
  /*}}}*/
  for (x=x1; x<=x2; ++x) for (z=z1; z<=z2; ++z) setwidth(cursheet,x,z,n);
  return -1;
}
/*}}}*/
/* do_attribute   -- set cell attributes */ /*{{{*/
static int do_attribute(Sheet *cursheet)
{
  /* variables */ /*{{{*/
  MenuChoice mainmenu[5];
  MenuChoice adjmenu[9];
  int c;
  int x,y,z;
  int x1,y1,z1;
  int x2,y2,z2;
  /*}}}*/
      
  /* create menus */ /*{{{*/
  adjmenu[0].str=mystrmalloc(ADLEFT);     adjmenu[0].c='\0';
  adjmenu[1].str=mystrmalloc(ADRIGHT);    adjmenu[1].c='\0';
  adjmenu[2].str=mystrmalloc(ADCENTERED); adjmenu[2].c='\0';
  adjmenu[3].str=mystrmalloc(DOSCI);      adjmenu[3].c='\0';
  adjmenu[4].str=mystrmalloc(NOSCI);      adjmenu[4].c='\0';
  adjmenu[5].str=mystrmalloc(PRECISION);  adjmenu[5].c='\0';
  adjmenu[6].str=mystrmalloc(SHADOW);     adjmenu[6].c='\0';
  adjmenu[7].str=mystrmalloc(TRANSPAR);   adjmenu[7].c='\0';
  adjmenu[8].str=(char*)0;

  mainmenu[0].str=mystrmalloc(ADJUST);    mainmenu[0].c='\0';
  mainmenu[1].str=mystrmalloc(LABEL);     mainmenu[1].c='\0';
  mainmenu[2].str=mystrmalloc(DOLOCK);    mainmenu[2].c='\0';
  mainmenu[3].str=mystrmalloc(DOIGNORE);  mainmenu[3].c='\0';
  mainmenu[4].str=(char*)0;
  /*}}}*/
  if (cursheet->mark1x==-1)
  /* range is the current cell */ /*{{{*/
  {
    x1=x2=cursheet->curx;
    y1=y2=cursheet->cury;
    z1=z2=cursheet->curz;
  }
  /*}}}*/
  else
  /* range is the marked cube */ /*{{{*/
  {
    x1=cursheet->mark1x; x2=cursheet->mark2x; posorder(&x1,&x2);
    y1=cursheet->mark1y; y2=cursheet->mark2y; posorder(&y1,&y2);
    z1=cursheet->mark1z; z2=cursheet->mark2z; posorder(&z1,&z2);
  }
  /*}}}*/
  do
  {
    if ((c=line_menu(cursheet->mark1x==-1 ? CELLATTR : BLOCKATTR,mainmenu,0))>=0)
    {
      if (cursheet->mark1x==-1 && c!=2 && locked(cursheet,cursheet->curx,cursheet->cury,cursheet->curz)) line_msg(CELLATTR,ISLOCKED);
      else
      {
        switch (c)
        {
          /* 0       -- representation */ /*{{{*/
          case 0:
          {
            do 
            {
              switch (c=line_menu(cursheet->mark1x==-1 ? CELLATTR : BLOCKATTR,adjmenu,(int)getadjust(cursheet,x1,y1,z1)))
              {
                /* -2,-1   -- go up, abort */ /*{{{*/
                case -2:
                case -1: break;
                /*}}}*/
                /* 0       -- adjust left */ /*{{{*/
                case 0:
                {
                  for (x=x1; x<=x2; ++x) for (y=y1; y<=y2; ++y) for (z=z1; z<=z2; ++z) setadjust(cursheet,x,y,z,LEFT);
                  break;
                }
                /*}}}*/
                /* 1       -- adjust right */ /*{{{*/
                case 1:
                {
                  for (x=x1; x<=x2; ++x) for (y=y1; y<=y2; ++y) for (z=z1; z<=z2; ++z) setadjust(cursheet,x,y,z,RIGHT);
                  break;
                }
                /*}}}*/
                /* 2       -- adjust centered */ /*{{{*/
                case 2:
                {
                  for (x=x1; x<=x2; ++x) for (y=y1; y<=y2; ++y) for (z=z1; z<=z2; ++z) setadjust(cursheet,x,y,z,CENTER);
                  break;
                }
                /*}}}*/
                /* 3       -- set scientific notation flag */ /*{{{*/
                case 3:
                {
                  for (x=x1; x<=x2; ++x) for (y=y1; y<=y2; ++y) for (z=z1; z<=z2; ++z) setscientific(cursheet,x,y,z,1);
                  break;
                }
                /*}}}*/
                /* 4       -- clear scientific notation flag */ /*{{{*/
                case 4:
                {
                  for (x=x1; x<=x2; ++x) for (y=y1; y<=y2; ++y) for (z=z1; z<=z2; ++z) setscientific(cursheet,x,y,z,0);
                  break;
                }
                /*}}}*/
                /* 5       -- set precision */ /*{{{*/
                case 5:
                {
                  size_t ex,offx;
                  int n;

                  offx=0;
                  ex=0;
                  n=getprecision(cursheet,x1,y1,z1);
                  do if (line_numedit(&n,cursheet->mark1x==-1 ? PRECCELL : PRECBLOCK,&ex,&offx)==-1) goto end; while (n!=-1 && (n==0 || n>20));
                  for (x=x1; x<=x2; ++x) for (y=y1; y<=y2; ++y) for (z=z1; z<=z2; ++z) setprecision(cursheet,x,y,z,n);
                  break;
                }
                /*}}}*/
                /* 6       -- shadow */ /*{{{*/
                case 6:
                {
                  int n;

                  if ((n=line_ok(cursheet->mark1x==-1 ? SHADOWCELL : SHADOWCUBE,shadowed(cursheet,x1,y1,z1)))!=-1)
                  {          
                    for (x=x1; x<=x2; ++x) for (y=y1; y<=y2; ++y) for (z=z1; z<=z2; ++z)
                    {
                      int rx;

                      if (n==0) for (rx=x+1; shadowed(cursheet,rx,y,z); ++rx) shadow(cursheet,rx,y,z,0);
                      else if (x>0) shadow(cursheet,x,y,z,1);
                    }
                  }
                  if (x1==0 && n==1) line_msg(SHADOWCELL,SHADOWBOO);
                  break;
                }
                /*}}}*/
                /* 7       -- transparent */ /*{{{*/
                case 7:
                {
                  int n;

                  if ((n=line_ok(cursheet->mark1x==-1 ? TRANSCELL : TRANSCUBE,transparent(cursheet,x1,y1,z1)))!=-1)
                  for (x=x1; x<=x2; ++x) for (y=y1; y<=y2; ++y) for (z=z1; z<=z2; ++z) maketrans(cursheet,x,y,z,n);
                  break;
                }
                /*}}}*/
                default: assert(0);
              }
              if (c>=0) redraw_sheet(cursheet);
            } while (c>=0);
            break;
          }
          /*}}}*/
          /* 1       -- edit label and goto end */ /*{{{*/
          case 1:
          {
            if (do_label(cursheet)==-1) c=-1;
            goto end;
          }
          /*}}}*/
          /* 2       -- lock */ /*{{{*/
          case 2:
          {
            int n;
          
            if ((n=line_ok(cursheet->mark1x==-1 ? LOCKCELL : LOCKBLOCK,locked(cursheet,x1,y1,z1)))==-1) c=-1;
            else if (n!=-1) for (x=x1; x<=x2; ++x) for (y=y1; y<=y2; ++y) for (z=z1; z<=z2; ++z) lockcell(cursheet,x,y,z,n);
            break;
          }
          /*}}}*/
          /* 3       -- ignore */ /*{{{*/
          case 3:
          {
            int n;
                  
            if ((n=line_ok(cursheet->mark1x==-1 ? IGNCELL : IGNBLOCK,ignored(cursheet,x1,y1,z1)))==-1) c=-1;
            else for (x=x1; x<=x2; ++x) for (y=y1; y<=y2; ++y) for (z=z1; z<=z2; ++z) igncell(cursheet,x,y,z,n);
            break;
          }
          /*}}}*/
          /* default -- should not happen */ /*{{{*/
          default: assert(0);
          /*}}}*/
        }
      }
    }
    if (c>=0) redraw_sheet(cursheet);
  } while (c>=0);
  forceupdate(cursheet);
  cursheet->marking=0;
  end:
  /* free menus */ /*{{{*/
  free(mainmenu[0].str);
  free(mainmenu[1].str);
  free(mainmenu[2].str);
  free(mainmenu[3].str);
  free(adjmenu[0].str);
  free(adjmenu[1].str);
  free(adjmenu[2].str);
  free(adjmenu[3].str);
  free(adjmenu[4].str);
  free(adjmenu[5].str);
  free(adjmenu[6].str);
  free(adjmenu[7].str);
  /*}}}*/
  return c;
}
/*}}}*/
/* do_savexdr     -- save sheet as XDR file */ /*{{{*/
static int do_savexdr(Sheet *cursheet)
{
  /* variables */ /*{{{*/
  char buf[_POSIX_PATH_MAX];
  const char *msg;
  unsigned int count;
  int c;
  /*}}}*/    

  if (cursheet->name==(char*)0)
  {
    /* variables */ /*{{{*/
    size_t x,offx;
    /*}}}*/

    x=0;
    offx=0;
    buf[0]='\0';
    if ((c=line_edit((Sheet*)0,buf,sizeof(buf),SAVEXSHEET,&x,&offx))<0) return c;
    cursheet->name=strcpy(malloc(strlen(buf)+1),buf);
  }
  if ((msg=savexdr(cursheet,cursheet->name,&count)))
  {
    line_msg(SAVEXSHEET,msg);
    return -2;
  }
  else
  {
    sprintf(buf,WROTECELLS,count);
    assert(strlen(buf)<sizeof(buf));
    line_msg(SAVEXSHEET,buf);
  }
  return -1;
}
/*}}}*/
/* do_saveport    -- save sheet as portable ASCII file */ /*{{{*/
static int do_saveport(Sheet *cursheet, const char *extension)
{
  /* variables */ /*{{{*/
  char buf[_POSIX_PATH_MAX];
  const char *msg;
  size_t x,offx,extlen;
  int c;
  unsigned int count;
  /*}}}*/
        
  extlen=strlen(extension)+1;
  if (cursheet->name!=(char*)0) (void)strcpy(buf,cursheet->name); else buf[0]='\0';
  (void)strncat(buf,extension,sizeof(buf)-strlen(buf)-extlen);
  buf[sizeof(buf)-1]='\0';
  x=strlen(buf);
  offx=0;
  if ((c=line_edit((Sheet*)0,buf,sizeof(buf),SAVEPSHEET,&x,&offx))<0) return c;
  if ((msg=saveport(cursheet,buf,&count))) line_msg(SAVEPSHEET,msg);
  else
  {
    sprintf(buf,WROTECELLS,count);
    assert(strlen(buf)<sizeof(buf));
    line_msg(SAVEPSHEET,buf);
  }
  return -1;
}
/*}}}*/
/* do_savetbl     -- save sheet as tbl file */ /*{{{*/
static int do_savetbl(Sheet *cursheet, const char *name)
{
  /* variables */ /*{{{*/
  char buf[_POSIX_PATH_MAX];
  const char *msg;
  size_t x,offx;
  int standalone=0;
  int x1,y1,z1,x2,y2,z2;
  int c;
  unsigned int count;
  /*}}}*/
      
  if (cursheet->mark1x==-1)
  /* range is the whole sheet */ /*{{{*/
  {
    x1=0; x2=cursheet->dimx-1;
    y1=0; y2=cursheet->dimy-1;
    z1=0; z2=cursheet->dimz-1;
  }
  /*}}}*/
  else
  /* range is the marked cube */ /*{{{*/
  {
    x1=cursheet->mark1x; x2=cursheet->mark2x; posorder(&x1,&x2);
    y1=cursheet->mark1y; y2=cursheet->mark2y; posorder(&y1,&y2);
    z1=cursheet->mark1z; z2=cursheet->mark2z; posorder(&z1,&z2);
  }
  /*}}}*/
  if (name) (void)strcpy(buf,name);
  else
  {
    if (cursheet->name==(char*)0) buf[0]='\0';
    else
    {
      (void)strcpy(buf,cursheet->name);
      (void)strncat(buf,".tbl",sizeof(buf)-strlen(buf)-5);
      buf[sizeof(buf)-1]='\0';
    }
    x=0;
    offx=0;
    if ((c=line_edit((Sheet*)0,buf,sizeof(buf),SAVETBLF,&x,&offx))<0) return c;
    if (buf[0]=='\0') return -1;
    if ((standalone=line_ok(STANDALONE,1))<0) return standalone;
  }
  if ((msg=savetbl(cursheet,buf,!standalone,x1,y1,z1,x2,y2,z2,&count))) line_msg(SAVETBLF,msg);
  else if (name==(const char*)0)
  {
    sprintf(buf,WROTECELLS,count);
    assert(strlen(buf)<sizeof(buf));
    line_msg(SAVETBLF,buf);
  }
  return -1;
}
/*}}}*/
/* do_savelatex   -- save sheet as LaTeX file */ /*{{{*/
static int do_savelatex(Sheet *cursheet, const char *name)
{
  /* variables */ /*{{{*/
  char buf[_POSIX_PATH_MAX];
  const char *msg;
  size_t x,offx;
  int standalone=0;
  int x1,y1,z1,x2,y2,z2;
  int c;
  unsigned int count;
  /*}}}*/
      
  if (cursheet->mark1x==-1)
  /* range is the whole sheet */ /*{{{*/
  {
    x1=0; x2=cursheet->dimx-1;
    y1=0; y2=cursheet->dimy-1;
    z1=0; z2=cursheet->dimz-1;
  }
  /*}}}*/
  else
  /* range is the marked cube */ /*{{{*/
  {
    x1=cursheet->mark1x; x2=cursheet->mark2x; posorder(&x1,&x2);
    y1=cursheet->mark1y; y2=cursheet->mark2y; posorder(&y1,&y2);
    z1=cursheet->mark1z; z2=cursheet->mark2z; posorder(&z1,&z2);
  }
  /*}}}*/
  if (name) (void)strcpy(buf,name);
  else
  {
    if (cursheet->name==(char*)0) buf[0]='\0';
    else
    {
      (void)strcpy(buf,cursheet->name);
      (void)strncat(buf,".tex",sizeof(buf)-strlen(buf)-5);
      buf[sizeof(buf)-1]='\0';
    }
    x=0;
    offx=0;
    if ((c=line_edit((Sheet*)0,buf,sizeof(buf),SAVELATEXF,&x,&offx))<0) return c;
    if (buf[0]=='\0') return -1;
    if ((standalone=line_ok(STANDALONE,1))<0) return standalone;
  }
  if ((msg=savelatex(cursheet,buf,!standalone,x1,y1,z1,x2,y2,z2,&count))!=(const char*)0) line_msg(SAVELATEXF,msg);
  else if (name==(const char*)0)
  {
    sprintf(buf,WROTECELLS,count);
    assert(strlen(buf)<sizeof(buf));
    line_msg(SAVELATEXF,buf);
  }
  return -1;
}
/*}}}*/
/* do_savecontext   -- save sheet as ConTeXt file */ /*{{{*/
static int do_savecontext(Sheet *cursheet, const char *name)
{
  /* variables */ /*{{{*/
  char buf[_POSIX_PATH_MAX];
  const char *msg;
  size_t x,offx;
  int standalone=0;
  int x1,y1,z1,x2,y2,z2;
  int c;
  unsigned int count;
  /*}}}*/
      
  if (cursheet->mark1x==-1)
  /* range is the whole sheet */ /*{{{*/
  {
    x1=0; x2=cursheet->dimx-1;
    y1=0; y2=cursheet->dimy-1;
    z1=0; z2=cursheet->dimz-1;
  }
  /*}}}*/
  else
  /* range is the marked cube */ /*{{{*/
  {
    x1=cursheet->mark1x; x2=cursheet->mark2x; posorder(&x1,&x2);
    y1=cursheet->mark1y; y2=cursheet->mark2y; posorder(&y1,&y2);
    z1=cursheet->mark1z; z2=cursheet->mark2z; posorder(&z1,&z2);
  }
  /*}}}*/
  if (name) (void)strcpy(buf,name);
  else
  {
    if (cursheet->name==(char*)0) buf[0]='\0';
    else
    {
      (void)strcpy(buf,cursheet->name);
      (void)strncat(buf,".tex",sizeof(buf)-strlen(buf)-5);
      buf[sizeof(buf)-1]='\0';
    }
    x=0;
    offx=0;
    if ((c=line_edit((Sheet*)0,buf,sizeof(buf),SAVECONTEXTF,&x,&offx))<0) return c;
    if (buf[0]=='\0') return -1;
    if ((standalone=line_ok(STANDALONE,1))<0) return standalone;
  }
  if ((msg=savecontext(cursheet,buf,!standalone,x1,y1,z1,x2,y2,z2,&count))!=(const char*)0) line_msg(SAVECONTEXTF,msg);
  else if (name==(const char*)0)
  {
    sprintf(buf,WROTECELLS,count);
    assert(strlen(buf)<sizeof(buf));
    line_msg(SAVECONTEXTF,buf);
  }
  return -1;
}
/*}}}*/
/* do_savehtml    -- save sheet as HTML file */ /*{{{*/
static int do_savehtml(Sheet *cursheet, const char *name)
{
  /* variables */ /*{{{*/
  char buf[_POSIX_PATH_MAX];
  const char *msg;
  size_t x,offx;
  int standalone=0;
  int x1,y1,z1,x2,y2,z2;
  int c;
  unsigned int count;
  /*}}}*/
      
  if (cursheet->mark1x==-1)
  /* range is the whole sheet */ /*{{{*/
  {
    x1=0; x2=cursheet->dimx-1;
    y1=0; y2=cursheet->dimy-1;
    z1=0; z2=cursheet->dimz-1;
  }
  /*}}}*/
  else
  /* range is the marked cube */ /*{{{*/
  {
    x1=cursheet->mark1x; x2=cursheet->mark2x; posorder(&x1,&x2);
    y1=cursheet->mark1y; y2=cursheet->mark2y; posorder(&y1,&y2);
    z1=cursheet->mark1z; z2=cursheet->mark2z; posorder(&z1,&z2);
  }
  /*}}}*/
  if (name) (void)strcpy(buf,name);
  else
  {
    if (cursheet->name==(char*)0) buf[0]='\0';
    else
    {
      (void)strcpy(buf,cursheet->name);
      (void)strncat(buf,".html",sizeof(buf)-strlen(buf)-6);
      buf[sizeof(buf)-1]='\0';
    }
    x=0;
    offx=0;
    if ((c=line_edit((Sheet*)0,buf,sizeof(buf),SAVEHTMLF,&x,&offx))<0) return c;
    if (buf[0]=='\0') return -1;
    if ((standalone=line_ok(STANDALONE,1))<0) return standalone;
  }
  if ((msg=savehtml(cursheet,buf,!standalone,x1,y1,z1,x2,y2,z2,&count))) line_msg(SAVEHTMLF,msg);
  else if (name==(const char*)0)
  {
    sprintf(buf,WROTECELLS,count);
    assert(strlen(buf)<sizeof(buf));
    line_msg(SAVEHTMLF,buf);
  }
  return -1;
}
/*}}}*/
/* do_savetext    -- save sheet as formatted text file */ /*{{{*/
static int do_savetext(Sheet *cursheet)
{
  /* variables */ /*{{{*/
  char buf[_POSIX_PATH_MAX];
  const char *msg;
  size_t x,offx;
  int x1,y1,z1,x2,y2,z2;
  int c;
  unsigned int count;
  /*}}}*/
      
  if (cursheet->mark1x==-1)
  /* range is the whole sheet */ /*{{{*/
  {
    x1=0; x2=cursheet->dimx-1;
    y1=0; y2=cursheet->dimy-1;
    z1=0; z2=cursheet->dimz-1;
  }
  /*}}}*/
  else
  /* range is the marked cube */ /*{{{*/
  {
    x1=cursheet->mark1x; x2=cursheet->mark2x; posorder(&x1,&x2);
    y1=cursheet->mark1y; y2=cursheet->mark2y; posorder(&y1,&y2);
    z1=cursheet->mark1z; z2=cursheet->mark2z; posorder(&z1,&z2);
  }
  /*}}}*/
  if (cursheet->name==(char*)0) buf[0]='\0';
  else
  {
    (void)strcpy(buf,cursheet->name);
    (void)strncat(buf,".doc",sizeof(buf)-strlen(buf)-5);
    buf[sizeof(buf)-1]='\0';
  }
  x=0;
  offx=0;
  if ((c=line_edit((Sheet*)0,buf,sizeof(buf),SAVETEXTF,&x,&offx))<0) return c;
  if (buf[0]=='\0') return -1;
  if ((msg=savetext(cursheet,buf,x1,y1,z1,x2,y2,z2,&count))) line_msg(SAVETEXTF,msg);
  else
  {
    sprintf(buf,WROTECELLS,count);
    assert(strlen(buf)<sizeof(buf));
    line_msg(SAVETEXTF,buf);
  }
  return -1;
}
/*}}}*/
/* do_savecsv     -- save sheet as CSV file */ /*{{{*/
static int do_savecsv(Sheet *cursheet, const char *name)
{
  /* variables */ /*{{{*/
  char buf[_POSIX_PATH_MAX];
  const char *msg;
  size_t x,offx;
  int x1,y1,z1,x2,y2,z2;
  int c;
  unsigned int count;
  /*}}}*/
      
  if (cursheet->mark1x==-1)
  /* range is the whole sheet */ /*{{{*/
  {
    x1=0; x2=cursheet->dimx-1;
    y1=0; y2=cursheet->dimy-1;
    z1=0; z2=cursheet->dimz-1;
  }
  /*}}}*/
  else
  /* range is the marked cube */ /*{{{*/
  {
    x1=cursheet->mark1x; x2=cursheet->mark2x; posorder(&x1,&x2);
    y1=cursheet->mark1y; y2=cursheet->mark2y; posorder(&y1,&y2);
    z1=cursheet->mark1z; z2=cursheet->mark2z; posorder(&z1,&z2);
  }
  /*}}}*/
  if (name) (void)strcpy(buf,name);
  else
  {
    if (cursheet->name==(char*)0) buf[0]='\0';
    else
    {
      (void)strcpy(buf,cursheet->name);
      (void)strncat(buf,".txt",sizeof(buf)-strlen(buf)-5);
      buf[sizeof(buf)-1]='\0';
    }
    x=0;
    offx=0;
    if ((c=line_edit((Sheet*)0,buf,sizeof(buf),SAVECSVF,&x,&offx))<0) return c;
    if (buf[0]=='\0') return -1;
  }
  if ((msg=savecsv(cursheet,buf,x1,y1,z1,x2,y2,z2,&count))) line_msg(SAVECSVF,msg);
  else if (name==(const char*)0)
  {
    sprintf(buf,WROTECELLS,count);
    assert(strlen(buf)<sizeof(buf));
    line_msg(SAVECSVF,buf);
  }
  return -1;
}
/*}}}*/
/* do_loadxdr     -- load sheet from XDR file */ /*{{{*/
static int do_loadxdr(Sheet *cursheet)
{
  /* variables */ /*{{{*/
  size_t x,offx;
  char buf[_POSIX_PATH_MAX];
  const char *msg;
  int c;
  /*}}}*/
      
  x=0;
  offx=0;
  buf[0]='\0';
  if ((c=line_edit((Sheet*)0,buf,sizeof(buf),LOADXSHEET,&x,&offx))<0) return c;
  if (buf[0]=='\0') return (-1);
  if (cursheet->name!=(char*)0) free(cursheet->name);
  cursheet->name=strcpy(malloc(strlen(buf)+1),buf);
  if ((msg=loadxdr(cursheet,cursheet->name))!=(const char*)0) line_msg(LOADXSHEET,msg);
  return -1;
}
/*}}}*/
/* do_loadport    -- load sheet from portable ASCII file */ /*{{{*/
static int do_loadport(Sheet *cursheet)
{
  /* variables */ /*{{{*/
  size_t x,offx;
  char buf[_POSIX_PATH_MAX];
  const char *msg;
  int c;
  /*}}}*/
      
  x=0;
  offx=0;
  (void)strcpy(buf,".asc");
  if ((c=line_edit((Sheet*)0,buf,sizeof(buf),LOADPSHEET,&x,&offx))<0) return c;
  if (buf[0]=='\0') return -1;
  if (cursheet->name!=(char*)0) free(cursheet->name);
  cursheet->name=strcpy(malloc(strlen(buf)+1),buf);
  if ((msg=loadport(cursheet,cursheet->name))!=(const char*)0) line_msg(LOADPSHEET,msg);
  if (strlen(cursheet->name)>(size_t)4 && strcmp(cursheet->name+strlen(cursheet->name)-4,".asc")==0) *(cursheet->name+strlen(cursheet->name)-4)='\0';
  return -1;
}
/*}}}*/
/* do_loadsc      -- load sheet from SC file */ /*{{{*/
static int do_loadsc(Sheet *cursheet)
{
  /* variables */ /*{{{*/
  size_t x,offx;
  char buf[_POSIX_PATH_MAX];
  const char *msg;
  int c;
  /*}}}*/
      
  x=0;
  offx=0;
  (void)strcpy(buf,".sc");
  if ((c=line_edit((Sheet*)0,buf,sizeof(buf),LOADSCSHEET,&x,&offx))<0) return c;
  if (buf[0]=='\0') return -1;
  if (cursheet->name!=(char*)0) free(cursheet->name);
  cursheet->name=strcpy(malloc(strlen(buf)+1),buf);
  if ((msg=loadsc(cursheet,cursheet->name))!=(const char*)0) line_msg(LOADSCSHEET,msg);
  if (strlen(cursheet->name)>(size_t)3 && strcmp(cursheet->name+strlen(cursheet->name)-3,".sc")==0) *(cursheet->name+strlen(cursheet->name)-3)='\0';
  return -1;
}
/*}}}*/
/* do_loadwk1     -- load sheet from WK1 file */ /*{{{*/
static int do_loadwk1(Sheet *cursheet)
{
  /* variables */ /*{{{*/
  size_t x,offx;
  char buf[_POSIX_PATH_MAX];
  const char *msg;
  int c;
  /*}}}*/
      
  x=0;
  offx=0;
  (void)strcpy(buf,".wk1");
  if ((c=line_edit((Sheet*)0,buf,sizeof(buf),LOADWK1SHEET,&x,&offx))<0) return c;
  if (buf[0]=='\0') return -1;
  if (cursheet->name!=(char*)0) free(cursheet->name);
  cursheet->name=strcpy(malloc(strlen(buf)+1),buf);
  if ((msg=loadwk1(cursheet,cursheet->name))!=(const char*)0) line_msg(LOADWK1SHEET,msg);
  if (strlen(cursheet->name)>(size_t)4 && strcmp(cursheet->name+strlen(cursheet->name)-4,".sc")==0) *(cursheet->name+strlen(cursheet->name)-4)='\0';
  return -1;
}
/*}}}*/
/* do_loadcsv     -- load/merge sheet from CSV file */ /*{{{*/
static int do_loadcsv(Sheet *cursheet, int semicolon)
{
  /* variables */ /*{{{*/
  size_t x,offx;
  char buf[_POSIX_PATH_MAX];
  const char *msg;
  int c;
  /*}}}*/
      
  x=0;
  offx=0;
  (void)strcpy(buf,".txt");
  if ((c=line_edit((Sheet*)0,buf,sizeof(buf),LOADCSVSHEET,&x,&offx))<0) return c;
  if (buf[0]=='\0') return -1;
  if (cursheet->name==(char*)0)
  {
    cursheet->name=strcpy(malloc(strlen(buf)+1),buf);
    if (strlen(cursheet->name)>(size_t)4 && strcmp(cursheet->name+strlen(cursheet->name)-4,".asc")==0) *(cursheet->name+strlen(cursheet->name)-4)='\0';
      }
  if ((msg=loadcsv(cursheet,buf,semicolon))!=(const char*)0) line_msg(LOADCSVSHEET,msg);
  return -1;
}
/*}}}*/
/* do_name        -- (re)name sheet */ /*{{{*/
static int do_name(Sheet *cursheet)
{
  /* variables */ /*{{{*/
  size_t x,offx;
  char buf[_POSIX_PATH_MAX];
  int c;
  /*}}}*/
      
  x=0;
  offx=0;
  if (usexdr) buf[0]='\0'; else strcpy(buf,".asc");
  while (buf[0]=='\0') if ((c=line_edit((Sheet*)0,buf,sizeof(buf),NAMESHEET,&x,&offx))<0) return c;
  if (buf[0]=='\0') return -1;
  if (cursheet->name!=(char*)0) free(cursheet->name);
  cursheet->name=strcpy(malloc(strlen(buf)+1),buf);
  return -1;
}
/*}}}*/
/* do_mark        -- set mark */ /*{{{*/
static void do_mark(Sheet *cursheet, int force)
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
/* do_save        -- save sheet */ /*{{{*/
static int do_save(Sheet *cursheet)
{
  /* variables */ /*{{{*/
  MenuChoice menu[9];
  int c;
  /*}}}*/
  
  menu[0].str=mystrmalloc(ASXDR);    menu[0].c='\0';
  menu[1].str=mystrmalloc(ASASCII);  menu[1].c='\0';  
  menu[2].str=mystrmalloc(TBL);      menu[2].c='\0';
  menu[3].str=mystrmalloc(LATEX);    menu[3].c='\0';
  menu[4].str=mystrmalloc(HTML);     menu[4].c='\0';
  menu[5].str=mystrmalloc(CSV);      menu[5].c='\0';
  menu[6].str=mystrmalloc(SAVETEXT); menu[6].c='\0';
  menu[7].str=mystrmalloc(CONTEXT);  menu[7].c='\0';
  menu[8].str=(char*)0;
  c=(usexdr ? 0 : 1);
  do
  {
    switch (c=line_menu(SAVEMENU,menu,c))
    {
      /* -2,-1   -- go up or abort                */ /*{{{*/
      case -2:
      case -1: break;
      /*}}}*/
      /*  0      -- save in XDR                   */ /*{{{*/
      case 0: if ((c=do_savexdr(cursheet))>=0) c=0; break;
      /*}}}*/
      /*  1      -- save in ASCII format          */ /*{{{*/
      case 1: if (do_saveport(cursheet,".asc")==-1) c=-1; break;
      /*}}}*/
      /*  2      -- save in tbl format            */ /*{{{*/
      case 2: if (do_savetbl(cursheet,(const char*)0)==-1) c=-1; break;
      /*}}}*/
      /*  3      -- save in LaTeX format          */ /*{{{*/
      case 3: if (do_savelatex(cursheet,(const char*)0)==-1) c=-1; break;
      /*}}}*/
      /*  4      -- save in HTML format           */ /*{{{*/
      case 4: if (do_savehtml(cursheet,(const char*)0)==-1) c=-1; break;
      /*}}}*/
      /*  5      -- save in CSV format            */ /*{{{*/
      case 5: if (do_savecsv(cursheet,(const char*)0)==-1) c=-1; break;
      /*}}}*/
      /*  6      -- save in formatted text format */ /*{{{*/
      case 6: if (do_savetext(cursheet)==-1) c=-1; break;
      /*}}}*/
      /*  7      -- save in ConTeXt format        */ /*{{{*/
      case 7: if (do_savecontext(cursheet,(const char*)0)==-1) c=-1; break;
      /*}}}*/
      /* default -- should not happen             */ /*{{{*/
      default: assert(0);
      /*}}}*/
    }
  } while (c>=0);
  free(menu[0].str);
  free(menu[1].str);
  free(menu[2].str);
  free(menu[3].str);
  free(menu[4].str);
  free(menu[5].str);
  free(menu[6].str);
  return c;
}
/*}}}*/
/* do_load        -- load sheet */ /*{{{*/
static int do_load(Sheet *cursheet)
{
  /* variables */ /*{{{*/
  MenuChoice menu[7];
  int c;
  /*}}}*/
  
  menu[0].str=mystrmalloc(ASXDR);   menu[0].c='\0';
  menu[1].str=mystrmalloc(ASASCII); menu[1].c='\0';
  menu[2].str=mystrmalloc(ASSC);    menu[2].c='\0';
  menu[3].str=mystrmalloc(ASWK1);   menu[3].c='\0';
  menu[4].str=mystrmalloc(CSV);     menu[4].c='\0';
  menu[5].str=mystrmalloc(GCSV);    menu[5].c='\0';
  menu[6].str=(char*)0;
  c=(usexdr ? 0 : 1);
  do
  {
    switch (c=line_menu(LOADMENU,menu,c))
    {
      /* -2,-1   -- go up or abort    */ /*{{{*/
      case -2:
      case -1: break;
      /*}}}*/
      /*  0      -- load in XDR       */ /*{{{*/
      case 0:
      {
        if (doanyway(cursheet,LOADANYWAY)!=1 || do_loadxdr(cursheet)==-1) c=-1;
        break;
      }
      /*}}}*/
      /*  1      -- load in ASCII     */ /*{{{*/
      case 1:
      {
        if (doanyway(cursheet,LOADANYWAY)!=1 || do_loadport(cursheet)==-1) c=-1;
        break;
      }
      /*}}}*/
      /*  2      -- load in SC        */ /*{{{*/
      case 2: if (doanyway(cursheet,LOADANYWAY)!=1 || do_loadsc(cursheet)==-1) c=-1; break;
      /*}}}*/
      /*  3      -- load in WK1       */ /*{{{*/
      case 3: if (doanyway(cursheet,LOADANYWAY)!=1 || do_loadwk1(cursheet)==-1) c=-1; break;
      /*}}}*/
      /*  4      -- load in CSV       */ /*{{{*/
      case 4: if (do_loadcsv(cursheet,0)==-1) c=-1; forceupdate(cursheet); break;
      /*}}}*/
      /*  5      -- load in GCSV      */ /*{{{*/
      case 5: if (do_loadcsv(cursheet,1)==-1) c=-1; forceupdate(cursheet); break;
      /*}}}*/
      /* default -- should not happen */ /*{{{*/
      default: assert(0);
      /*}}}*/
    }
  } while (c>=0);
  free(menu[0].str);
  free(menu[1].str);
  free(menu[2].str);
  free(menu[3].str);
  free(menu[4].str);
  free(menu[5].str);
  return c;
}
/*}}}*/
/* do_file        -- file menu */ /*{{{*/
static int do_file(Sheet *cursheet)
{
  /* variables */ /*{{{*/
  MenuChoice menu[4];
  int c;
  /*}}}*/
  
  menu[0].str=mystrmalloc(LOAD); menu[0].c='\0';
  menu[1].str=mystrmalloc(SAVE); menu[1].c='\0';
  menu[2].str=mystrmalloc(NAME); menu[2].c='\0';
  menu[3].str=(char*)0;
  c=0;
  do
  {
  switch (c=line_menu(FILERMENU,menu,c))
  {
    /* -2,-1   -- go up or abort */ /*{{{*/
    case -2:
    case -1: break;
    /*}}}*/
    /*  0      -- load */ /*{{{*/
    case 0: if (do_load(cursheet)==-1) c=-1; break;
    /*}}}*/
    /*  1      -- save */ /*{{{*/
    case 1: if (do_save(cursheet)==-1) c=-1; break;
    /*}}}*/
    /*  2      -- name */ /*{{{*/
    case 2: if (do_name(cursheet)==-1) c=-1; break;
    /*}}}*/
    /* default -- should not happen */ /*{{{*/
    default: assert(0);
    /*}}}*/
  }
  } while (c>=0);
  free(menu[0].str);
  free(menu[1].str);
  free(menu[2].str);
  return c;
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

  if (sheet->mark1x==-1 && locked(sheet,sheet->curx,sheet->cury,sheet->curz)) line_msg(CLEARCELL,ISLOCKED);
  else
  {
    if (sheet->mark1x!=-1) 
    {
      if ((c=line_ok(CLEARBLOCK,0))<0) return c;
      else if (c!=1) return -1;
    }
    if (sheet->mark1x==-1) /* range is the current cell */ /*{{{*/
    {
      x1=x2=sheet->curx;
      y1=y2=sheet->cury;
      z1=z2=sheet->curz;
    }
    /*}}}*/
    else /* range is the marked cube */ /*{{{*/
    {
      x1=sheet->mark1x; x2=sheet->mark2x; posorder(&x1,&x2);
      y1=sheet->mark1y; y2=sheet->mark2y; posorder(&y1,&y2);
      z1=sheet->mark1z; z2=sheet->mark2z; posorder(&z1,&z2);
    }
    /*}}}*/
    for (x=x1; x<=x2; ++x) for (y=y1; y<=y2; ++y) for (z=z1; z<=z2; ++z) freecell(sheet,x,y,z);
    cachelabels(sheet);
    forceupdate(sheet);
    sheet->marking=0;
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

    menu[0].str=mystrmalloc(INX); menu[0].c='\0';
    menu[1].str=mystrmalloc(INY); menu[1].c='\0';
    menu[2].str=mystrmalloc(INZ); menu[2].c='\0';
    menu[3].str=(char*)0;
    reply=line_menu(INSERTBLOCK,menu,0);
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
      case 0: menu[0].str=mystrmalloc(WHOLECOL); break;
      case 1: menu[0].str=mystrmalloc(WHOLELINE); break;
      case 2: menu[0].str=mystrmalloc(WHOLELAYER); break;
      default: assert(0);
    }
    menu[1].str=mystrmalloc(SINGLECELL); menu[1].c='\0';
    menu[2].str=(char*)0;
    r=line_menu(INSERTBLOCK,menu,0);
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
    x1=sheet->mark1x; x2=sheet->mark2x; posorder(&x1,&x2);
    y1=sheet->mark1y; y2=sheet->mark2y; posorder(&y1,&y2);
    z1=sheet->mark1z; z2=sheet->mark2z; posorder(&z1,&z2);
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
  sheet->marking=0;
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

    menu[0].str=mystrmalloc(INX); menu[0].c='\0';
    menu[1].str=mystrmalloc(INY); menu[1].c='\0';
    menu[2].str=mystrmalloc(INZ); menu[2].c='\0';
    menu[3].str=(char*)0;
    reply=line_menu(DELETEBLOCK,menu,0);
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
      case 0: menu[0].str=mystrmalloc(WHOLECOL); break;
      case 1: menu[0].str=mystrmalloc(WHOLELINE); break;
      case 2: menu[0].str=mystrmalloc(WHOLELAYER); break;
      default: assert(0);
    }
    menu[1].str=mystrmalloc(SINGLECELL); menu[1].c='\0';
    menu[2].str=(char*)0;
    r=line_menu(DELETEBLOCK,menu,0);
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
    x1=sheet->mark1x; x2=sheet->mark2x; posorder(&x1,&x2);
    y1=sheet->mark1y; y2=sheet->mark2y; posorder(&y1,&y2);
    z1=sheet->mark1z; z2=sheet->mark2z; posorder(&z1,&z2);
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
  sheet->marking=0;
  return -1;
}
/*}}}*/
/* do_move        -- copy or move a block */ /*{{{*/
static int do_move(Sheet *sheet, int copy, int force)
{
  int c;

  c=-1;
  if (sheet->mark1x==-1) line_msg(copy ? COPYBLOCK : MOVEBLOCK,NOMARK);
  else if (force || (c=line_ok(copy ? COPYBLOCK : MOVEBLOCK,0))==1)
  {
    int x1,y1,z1;
    int x2,y2,z2;
    
    x1=sheet->mark1x; x2=sheet->mark2x; posorder(&x1,&x2);
    y1=sheet->mark1y; y2=sheet->mark2y; posorder(&y1,&y2);
    z1=sheet->mark1z; z2=sheet->mark2z; posorder(&z1,&z2);
    moveblock(sheet,x1,y1,z1,x2,y2,z2,sheet->curx,sheet->cury,sheet->curz,copy);
    sheet->marking=0;
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
    
  if (sheet->mark1x==-1) line_msg(FILLBLOCK,NOMARK);
  else
  {
    x1=sheet->mark1x; x2=sheet->mark2x; posorder(&x1,&x2);
    y1=sheet->mark1y; y2=sheet->mark2y; posorder(&y1,&y2);
    z1=sheet->mark1z; z2=sheet->mark2z; posorder(&z1,&z2);
    cols=rows=layers=1;
    firstmenu:
    offx=0;
    edx=0;
    do if ((c=line_numedit(&cols,FILLCOLS,&edx,&offx))<0) return c; while (cols<=0);
    secondmenu:
    offx=0;
    edx=0;
    do
    {
      c=line_numedit(&rows,FILLROWS,&edx,&offx);
      if (c==-1) return -1;
      else if (c==-2) goto firstmenu;
    } while (rows<=0);
    offx=0;
    edx=0;
    do
    {
      c=line_numedit(&layers,FILLLAYERS,&edx,&offx);
      if (c==-1) return -1;
      else if (c==-2) goto secondmenu;
    } while (layers<=0);
    for (x=0; x<cols; ++x) for (y=0; y<rows; ++y) for (z=0; z<layers; ++z) moveblock(sheet,x1,y1,z1,x2,y2,z2,sheet->curx+x*(x2-x1+1),sheet->cury+y*(y2-y1+1),sheet->curz+z*(z2-z1+1),1);
    sheet->marking=0;
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
  
  /* cry if no block is selected */ /*{{{*/
  if (sheet->mark1x==-1)
  {
    line_msg(SORTBLOCK,NOREGION);
    return -1;
  }
  /*}}}*/
  /* note and order block coordinates */ /*{{{*/
  x1=sheet->mark1x; x2=sheet->mark2x; posorder(&x1,&x2);
  y1=sheet->mark1y; y2=sheet->mark2y; posorder(&y1,&y2);
  z1=sheet->mark1z; z2=sheet->mark2z; posorder(&z1,&z2);
  /*}}}*/
  /* build menues */ /*{{{*/
  menu1[0].str=mystrmalloc(INX);     menu1[0].c='\0';
  menu1[1].str=mystrmalloc(INY);     menu1[1].c='\0';
  menu1[2].str=mystrmalloc(INZ);     menu1[2].c='\0';
  menu1[3].str=(char*)0;
  menu2[0].str=mystrmalloc(SORTIT);  menu2[0].c='\0';
  menu2[1].str=mystrmalloc(ADDKEY);  menu2[1].c='\0';
  menu2[2].str=(char*)0;
  menu3[0].str=mystrmalloc(ASCEND);  menu3[0].c='\0';
  menu3[1].str=mystrmalloc(DESCEND); menu3[0].c='\0';
  menu3[2].str=(char*)0;
  /*}}}*/
  
  last=-1;
  /* ask for sort direction */ /*{{{*/
  zero: switch (c=line_menu(SORTBLOCK,menu1,0))
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
        c=line_numedit(&(sk[key].x),XPOS,&x,&offx);
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
        c=line_numedit(&(sk[key].y),YPOS,&x,&offx);
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
        c=line_numedit(&(sk[key].z),ZPOS,&x,&offx);
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
    switch (c=line_menu(SORTBLOCK,menu3,0))
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
      c=line_ok(SORTBLOCK,0);
      if (c==-1) goto greak;
      else if (c==-2) goto four;
      else if (c==0) doit=1;
    }
    /*}}}*/
    else /* ask for sort or adding another key */ /*{{{*/
    switch (line_menu(SORTBLOCK,menu2,0))
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
  if ((msg=sortblock(sheet,x1,y1,z1,x2,y2,z2,in_dir,sk,key))!=(const char*)0) line_msg(SORTBLOCK,msg);
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
    unsigned int key = 0;
    char* next;
    while( *arg != '\0' ) {
	while (isspace((int)*arg)) *arg++;
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
    sortblock(sheet,sheet->mark1x,sheet->mark1y,sheet->mark1z,sheet->mark2x,sheet->mark2y,sheet->mark2z,dir,sk,key);
}
/*}}}*/
/* do_mirror      -- mirror block */ /*{{{*/
static int do_mirror(Sheet *sheet)
{
  /* variables */ /*{{{*/
  int x1,y1,z1,x2,y2,z2,reply;
  /*}}}*/
  
  /* cry if no block is selected */ /*{{{*/
  if (sheet->mark1x==-1)
  {
    line_msg(MIRRORBLOCK,NOREGION);
    return -1;
  }
  /*}}}*/
  /* note and order block coordinates */ /*{{{*/
  x1=sheet->mark1x; x2=sheet->mark2x; posorder(&x1,&x2);
  y1=sheet->mark1y; y2=sheet->mark2y; posorder(&y1,&y2);
  z1=sheet->mark1z; z2=sheet->mark2z; posorder(&z1,&z2);
  /*}}}*/
  /* ask for direction of mirroring */ /*{{{*/
  {
    MenuChoice menu[4];

    menu[0].str=mystrmalloc(LEFTRIGHT); menu[0].c='\0';
    menu[1].str=mystrmalloc(UPSIDEDOWN); menu[1].c='\0';
    menu[2].str=mystrmalloc(FRONTBACK); menu[2].c='\0';
    menu[3].str=(char*)0;
    reply=line_menu(MIRRORBLOCK,menu,0);
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
/* do_shell       -- spawn a shell */ /*{{{*/
static int do_shell(void)
{
  pid_t pid;
  struct sigaction interrupt;

  refresh();
  interrupt.sa_flags=0;
  sigemptyset(&interrupt.sa_mask);
  interrupt.sa_handler=SIG_IGN;
  sigaction(SIGINT,&interrupt,(struct sigaction *)0);
  sigaction(SIGQUIT,&interrupt,(struct sigaction *)0);
  switch (pid=fork())
  {
    /*      -1 */ /*{{{*/
    case -1: line_msg(SHELL,strerror(errno)); break;
    /*}}}*/
    /*       0 */ /*{{{*/
    case 0:
    {
      const char *shell;

      if ((shell=getenv("SHELL"))==(const char*)0)
      {
        struct passwd *pwd;

        if ((pwd=getpwuid(getuid()))==(struct passwd*)0)
        {
          shell="/bin/sh";
        }
        else
        {
          shell=pwd->pw_shell;
        }
      }
      line_msg((const char*)0,SUBSHELL);
      move(LINES-1,0);
      curs_set(1);
      refresh();
      reset_shell_mode();
      write(1,"\n",1);
      interrupt.sa_handler=SIG_DFL;
      sigaction(SIGINT,&interrupt,(struct sigaction *)0);
      sigaction(SIGQUIT,&interrupt,(struct sigaction *)0);
      execl(shell,shell,(const char*)0);
      exit(127);
      break;
    }
    /*}}}*/
    /* default */ /*{{{*/
    default:
    {
      pid_t r;
      int status;

      while ((r=wait(&status))!=-1 && r!=pid);
      reset_prog_mode();
      interrupt.sa_handler=SIG_DFL;
      sigaction(SIGINT,&interrupt,(struct sigaction *)0);
      sigaction(SIGQUIT,&interrupt,(struct sigaction *)0);
      clear();
      refresh();
      curs_set(0);
    }
    /*}}}*/
  }
  return -1;
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
  else if ((c=line_edit(sheet,buf,sizeof(buf),GOTO,&x,&offx))<0) return c;
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
    {
      sheet->curx=value.u.location[0];
      sheet->cury=value.u.location[1];
      sheet->curz=value.u.location[2];
      if (sheet->marking)
      {
        sheet->mark2x=sheet->curx;
        sheet->mark2y=sheet->cury;
        sheet->mark2z=sheet->curz;
      }
    }
    else
    {
      line_msg(GOTO,LOCBOO);
    }
    tfree(&value);
  }
  return -1;
}
/*}}}*/
/* do_block       -- block menu */ /*{{{*/
static int do_block(Sheet *cursheet)
{
  MenuChoice block[9];
  int c;

  block[0].str=mystrmalloc(CLEAR);  block[0].c='\0';
  block[1].str=mystrmalloc(INSERT); block[1].c='\0';
  block[2].str=mystrmalloc(DELETE); block[2].c='\0';
  block[3].str=mystrmalloc(MOVE);   block[3].c='\0';
  block[4].str=mystrmalloc(COPY);   block[4].c='\0';
  block[5].str=mystrmalloc(FILL);   block[5].c='\0';
  block[6].str=mystrmalloc(SORT);   block[6].c='\0';
  block[7].str=mystrmalloc(MIRROR); block[7].c='\0';
  block[8].str=(char*)0;
  c=0;
  do
  {
    switch (c=line_menu(BLOCKMENU,block,c))
    {
      case -2:
      case -1: break;
      case 0: if (do_clear(cursheet)==-1) c=-1; break;
      case 1: if (do_insert(cursheet)==-1) c=-1; break;
      case 2: if (do_delete(cursheet)==-1) c=-1; break;
      case 3: if (do_move(cursheet,0,0)==-1) c=-1; break;
      case 4: if (do_move(cursheet,1,0)==-1) c=-1; break;
      case 5: if (do_fill(cursheet)==-1) c=-1; break;
      case 6: if (do_sort(cursheet)==-1) c=-1; break;    
      case 7: if (do_mirror(cursheet)==-1) c=-1; break;    
    }
    redraw_sheet(cursheet);
  } while (c>=0);
  free(block[0].str);
  free(block[1].str);
  free(block[2].str);
  free(block[3].str);
  free(block[4].str);
  free(block[5].str);
  free(block[6].str);
  free(block[7].str);
  return c;
}
/*}}}*/
/* do_bg          -- background teapot */ /*{{{*/
static void do_bg(void)
{
  struct termios t;

  if (tcgetattr(0,&t)==0 && t.c_cc[VSUSP]!=_POSIX_VDISABLE)
  {
  line_msg((const char*)0,BACKGROUND);
  move(LINES-1,0);
  curs_set(1);
  refresh();
  reset_shell_mode();
  write(1,"\n",1);
  kill(getpid(),SIGSTOP);
  clear();
  refresh();
  reset_prog_mode();
  curs_set(0);
  }
  else line_msg((const char*)0,NOTSUSPENDED);
}
/*}}}*/

/* do_sheetcmd    -- process one key press */ /*{{{*/
int do_sheetcmd(Sheet *cursheet, chtype c, int moveonly)
{
  switch (c)
  {
    /* UP         -- move up */ /*{{{*/
    case KEY_UP:
    {
      if (cursheet->cury>0) --cursheet->cury;
      if (cursheet->marking) cursheet->mark2y=cursheet->cury;
      break;
    }
    /*}}}*/
    /* DOWN       -- move down */ /*{{{*/
    case KEY_DOWN:
    {
      ++cursheet->cury;
      if (cursheet->marking) cursheet->mark2y=cursheet->cury;
      break;
    }
    /*}}}*/
    /* LEFT       -- move left */ /*{{{*/
    case KEY_LEFT:
    {
      if (cursheet->curx>0) --cursheet->curx;
      if (cursheet->marking) cursheet->mark2x=cursheet->curx;
      break;
    }
    /*}}}*/
    /* RIGHT      -- move right */ /*{{{*/
    case KEY_RIGHT:
    {
      ++cursheet->curx; while (shadowed(cursheet,cursheet->curx,cursheet->cury,cursheet->curz)) ++cursheet->curx;
      if (cursheet->marking) cursheet->mark2x=cursheet->curx;
      break;
    }
    /*}}}*/
    /* <          -- move to first line */ /*{{{*/
    case '<':
    {
      cursheet->cury=0;
      if (cursheet->marking) cursheet->mark2y=cursheet->cury;
      break;
    }
    /*}}}*/
    /* >          -- move to last line */ /*{{{*/
    case '>':
    {
      cursheet->cury=(cursheet->dimy ? cursheet->dimy-1 : 0);
      if (cursheet->marking) cursheet->mark2y=cursheet->cury;
      break;
    }
    /*}}}*/
    /* BEG        -- move to beginning of line */ /*{{{*/
    case KEY_BEG:
    {
      cursheet->curx=0;
      if (cursheet->marking) cursheet->mark2x=cursheet->curx;
      break;
    }
    /*}}}*/
    /* END        -- move to end of line */ /*{{{*/
    case KEY_END:
    {
      cursheet->curx=(cursheet->dimx ? cursheet->dimx-1 : 0);
      while (shadowed(cursheet,cursheet->curx,cursheet->cury,cursheet->curz)) ++cursheet->curx;
      if (cursheet->marking) cursheet->mark2x=cursheet->curx;
      break;
    }
    /*}}}*/
    /* +          -- move one sheet up */ /*{{{*/
    case '+':
    {
      ++cursheet->curz; 
      if (cursheet->marking) cursheet->mark2z=cursheet->curz;
      break;
    }
    /*}}}*/
    /* -          -- move one sheet down */ /*{{{*/
    case '-':
    {
      if (cursheet->curz>0) --cursheet->curz; 
      if (cursheet->marking) cursheet->mark2z=cursheet->curz;
      break;
    }
    /*}}}*/
    /* *          -- move to bottom sheet */ /*{{{*/
    case '*':
    {
      cursheet->curz=(cursheet->dimz ? cursheet->dimz-1 : 0);
      if (cursheet->marking) cursheet->mark2z=cursheet->curz;
      break;
    }
    /*}}}*/
    /* _          -- move to top sheet */ /*{{{*/
    case '_':
    {
      cursheet->curz=0;
      if (cursheet->marking) cursheet->mark2z=cursheet->curz;
      break;
    }
    /*}}}*/
    /* ENTER      -- edit current cell */ /*{{{*/
    case KEY_ENTER: if (moveonly) break; do_edit(cursheet,'\0',(const char*)0,0); break;
    /*}}}*/
    /* MENTER     -- edit current clocked cell */ /*{{{*/
    case KEY_MENTER: if (moveonly) break; do_edit(cursheet,'\0',(const char*)0,1); break;
    /*}}}*/
    /* ", @, digit -- edit current cell with character already in buffer */ /*{{{*/
    case KEY_BACKSPACE:
    case KEY_DC:
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
    /* .          -- toggle block marking */ /*{{{*/
    case KEY_MARK:
    case '.': if (moveonly) break; do_mark(cursheet,0); break;
    /*}}}*/
    /* F2         -- save menu */ /*{{{*/
    case KEY_F(2): if (moveonly) break; do_save(cursheet); break;
    /*}}}*/
    /* F3         -- load */ /*{{{*/
    case KEY_F(3): if (moveonly) break; do_load(cursheet); break;
    /*}}}*/
    /* F0, F10, / -- main menu */ /*{{{*/
    case KEY_F(0):
    case KEY_F(10):
    case '/':
    {
      /* variables */ /*{{{*/
      MenuChoice menu[9];
      int c;
      /*}}}*/

      if (moveonly) break;
      menu[0].str=mystrmalloc(ATTR);   menu[0].c='\0';
      menu[1].str=mystrmalloc(CWIDTH); menu[1].c='\0';
      menu[2].str=mystrmalloc(BLOCK);  menu[2].c='\0';
      menu[3].str=mystrmalloc(FILER);  menu[3].c='\0';
      menu[4].str=mystrmalloc(MGOTO);  menu[4].c='\0';
      menu[5].str=mystrmalloc(MSHELL); menu[5].c='\0';
      menu[6].str=mystrmalloc(ABOUT);  menu[6].c='\0';
      menu[7].str=mystrmalloc(QUIT);   menu[7].c='\0';
      menu[8].str=(char*)0;
      c=0;
      do
      {
        switch (c=line_menu(MAINMENU,menu,c))
        {
          case -2:
          case -1: break;
          case 0: if (do_attribute(cursheet)==-1) c=-1; break;
          case 1: if (do_columnwidth(cursheet)==-1) c=-1; break;
          case 2: if (do_block(cursheet)==-1) c=-1; break;
          case 3: if (do_file(cursheet)==-1) c=-1; break;
          case 4: if (do_goto(cursheet,(const char*)0)==-1) c=-1; break;
          case 5: if (do_shell()==-1) c=-1; break;
          case 6: do_about(); c=-1; break;
          case 7: return 1;
          default: assert(0);
        }
      } while (c>=0);
      free(menu[0].str);
      free(menu[1].str);
      free(menu[2].str);
      free(menu[3].str);
      free(menu[4].str);
      free(menu[5].str);
      free(menu[6].str);
      free(menu[7].str);
      break;
    }
    /*}}}*/
    /* SAVE       -- save in current native format */ /*{{{*/
    case KEY_SAVE: if (usexdr) do_savexdr(cursheet); else do_saveport(cursheet,".asc"); break;
    /*}}}*/
    /* LOAD       -- load in current native format */ /*{{{*/
    case KEY_LOAD: if (usexdr) do_loadxdr(cursheet); else do_loadport(cursheet); break;
    /*}}}*/
    /* C-y        -- copy block */ /*{{{*/
    case '\031': if (moveonly) break; do_move(cursheet,1,1); break;
    /*}}}*/
    /* C-z        -- suspend */ /*{{{*/
    case KEY_SUSPEND:
    case '\032': do_bg(); break;
    /*}}}*/
    /* C-r        -- recalculate */ /*{{{*/
    case '\022': if (moveonly) break; forceupdate(cursheet); break;
    /*}}}*/
    /* C-c        -- clock */ /*{{{*/
    case '\023':
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
    case KEY_NPAGE:
    {
      cursheet->cury+=(cursheet->maxy-3);
      cursheet->offy+=(cursheet->maxy-3);
      if (cursheet->marking) cursheet->mark2y=cursheet->cury;
      break;
    }
    /*}}}*/
    /* PPAGE      -- page up    */ /*{{{*/
    case KEY_PPAGE:
    {
      if (cursheet->cury>=(cursheet->maxy-3))
      {
        cursheet->cury-=(cursheet->maxy-3);
        cursheet->offy=(cursheet->offy>=(cursheet->maxy-3) ? cursheet->offy-(cursheet->maxy-3) : 0);
        if (cursheet->marking) cursheet->mark2y=cursheet->cury;
      }
      break;
    }
    /*}}}*/
    /* FPAGE      -- page right    */ /*{{{*/
    case KEY_FPAGE:
    {
      cursheet->curx+=cursheet->width;
      cursheet->offx+=cursheet->width;
      if (cursheet->marking) cursheet->mark2x=cursheet->curx;
      break;
    }
    /*}}}*/
    /* BPAGE      -- page left */ /*{{{*/
    case KEY_BPAGE:
    {
      if (cursheet->curx>=cursheet->width)
      {
        cursheet->curx-=cursheet->width;
        cursheet->offx=(cursheet->offx>=cursheet->width ? cursheet->offx-cursheet->width : 0);
        if (cursheet->marking) cursheet->mark2x=cursheet->curx;
      }
      break;
    }
    /*}}}*/
    /* SAVEQUIT   -- save and quit */ /*{{{*/
    case KEY_SAVEQUIT:
    {
      if (moveonly) break;
      if (usexdr) { if (do_savexdr(cursheet)!=-2) return 1; }
      else { if (do_saveport(cursheet,".asc")!=-2) return 1; }
      break;
    }
    /*}}}*/
    /* QUIT       -- quit */ /*{{{*/
    case KEY_QUIT: if (moveonly) break; return 1;
    /*}}}*/
  }
  return 0;
}
/*}}}*/
                  
/* main */ /*{{{*/
int main(int argc, char *argv[])
{
  /* variables */ /*{{{*/
  chtype c;
  Sheet sheet,*cursheet;
  int o;
  const char *loadfile;
  int redraw=0;
  char ln[1024];
  /*}}}*/

  /* set locale */ /*{{{*/
  #ifdef LC_MESSAGES
  (void)setlocale(LC_MESSAGES,"");
  #endif
  (void)setlocale(LC_CTYPE,"");
  (void)setlocale(LC_TIME,"");
  catd=catopen("teapot",0);
  /*}}}*/
  /* parse options */ /*{{{*/
  while ((o=getopt(argc,argv,"abhnrp:?"))!=EOF) switch (o)
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
    /* r       -- always redraw */ /*{{{*/
    case 'r':
    {
      redraw=1;
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
        fprintf(stderr,INVALPRECISION,DBL_DIG);
        exit(1);
      }
      def_precision=n;
      break;
    }
    /*}}}*/
    /* default -- includes ? and h */ /*{{{*/
    default:
    {
      fprintf(stderr,USAGE);
      exit(1);
    }
    /*}}}*/
  }
  loadfile=(optind<argc ? argv[optind] : (const char*)0);
  /*}}}*/
  /* start curses */ /*{{{*/
  if (!batch)
  {
    initscr();
    curs_set(0);
    noecho();
    raw();
    nonl();
    keypad(stdscr,TRUE);
    clear();
    refresh();
#ifdef HAVE_TYPEAHEAD
    if (redraw) typeahead(-1);
#endif
  }
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
  cursheet->maxx=COLS;
  cursheet->maxy=LINES-1;
  cursheet->name=(char*)0;
  cursheet->mark1x=-1;
  cursheet->marking=0;
  cursheet->changed=0;
  cursheet->moveonly=0;
  cursheet->clk=0;
  (void)memset(cursheet->labelcache,0,sizeof(cursheet->labelcache));
  /*}}}*/
  /* say hi */ /*{{{*/
  if (!batch) line_msg((const char*)0,TOMENU);
  /*}}}*/
  if (loadfile) /* load given sheet */ /*{{{*/
  {
    const char *msg;

    cursheet->name=mystrmalloc(loadfile);
    if (usexdr)
    {
      if ((msg=loadxdr(cursheet,cursheet->name))!=(const char*)0) line_msg(LOADXSHEET,msg);
    }
    else
    {
      if ((msg=loadport(cursheet,cursheet->name))!=(const char*)0) line_msg(LOADPSHEET,msg);
      if (strlen(cursheet->name)>(size_t)4 && strcmp(cursheet->name+strlen(cursheet->name)-4,".asc")==0) *(cursheet->name+strlen(cursheet->name)-4)='\0';
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
    else if (strcmp(cmd,"load-csv")==0) { loadcsv(cursheet,arg,0); forceupdate(cursheet); }
    /*}}}*/
    /* load-gcsv file  */ /*{{{*/
    else if (strcmp(cmd,"load-gcsv")==0) { loadcsv(cursheet,arg,1); forceupdate(cursheet); }
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
    else line_msg(WHAT,cmd);
    /*}}}*/
  }
  /*}}}*/
  else /* process interactive input */ /*{{{*/
  {
    do
    {
      redraw_sheet(cursheet);
      c=wgetc();
      wmove(stdscr,LINES-1,0); wclrtoeol(stdscr);
    } while (do_sheetcmd(cursheet,c,0)==0 || doanyway(cursheet,LEAVE)!=1);
    curs_set(1);
    echo();
    noraw();
    refresh();
    endwin();
  }
  /*}}}*/
  freesheet(cursheet,1);
  fclose(stdin);
  return 0;
}
/*}}}*/
