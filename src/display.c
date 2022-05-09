/* #includes */ /*{{{C}}}*//*{{{*/
#ifndef NO_POSIX_SOURCE
#undef _POSIX_SOURCE
#define _POSIX_SOURCE   1
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 2
#endif

#include <assert.h>
#include <ctype.h>
#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef NEED_BCOPY
#define memmove(dst,src,len) bcopy(src,dst,len)
#endif
#ifdef OLD_REALLOC
#define realloc(s,l) myrealloc(s,l)
#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include "cat.h"
#include "complete.h"
#include "default.h"
#include "display.h"
#include "eval.h"
#include "main.h"
#include "misc.h"
#include "sheet.h"
#include "wgetc.h"
/*}}}*/

/* redraw       -- redraw whole screen */ /*{{{*/
void redraw(void)
{
  (void)touchwin(curscr);
  (void)wrefresh(curscr);
}
/*}}}*/
/* redraw_sheet -- draw a sheet with cell cursor */ /*{{{*/
void redraw_sheet(Sheet *sheet)
{
  /* variables */ /*{{{*/
  int width,col,x,y,again;
  char pbuf[80];
  char *buf=malloc(128);
  size_t bufsz=128;
  const char *label;
  char *err;
  char moveonly;
  /*}}}*/
  
  /* asserts */ /*{{{*/
  assert(sheet!=(Sheet*)0);
  assert(sheet->curx>=0);
  assert(sheet->cury>=0);
  assert(sheet->curz>=0);
  assert(sheet->offx>=0);
  assert(sheet->offy>=0);
  /*}}}*/
  (void)wattron(stdscr,DEF_NUMBER);
  /* correct offsets to keep cursor visible */ /*{{{*/
  while (shadowed(sheet,sheet->curx,sheet->cury,sheet->curz))
  {
    --(sheet->curx);
    assert(sheet->curx>=0);
  }
  if (sheet->cury-sheet->offy>(sheet->maxy-3)) sheet->offy=sheet->cury-sheet->maxy+3;
  if (sheet->cury<sheet->offy) sheet->offy=sheet->cury;
  if (sheet->curx<sheet->offx) sheet->offx=sheet->curx;
  do
  {
    again=0;
    for (width=4,x=sheet->offx,col=0; width<=sheet->maxx; width+=columnwidth(sheet,x,sheet->curz),++x,++col);
    --col;
    sheet->width=col;
    if (sheet->curx!=sheet->offx)
    {
      if (col==0) { ++sheet->offx; again=1; }
      else if (sheet->curx-sheet->offx>=col) { ++sheet->offx; again=1; }
    }
  } while (again);
  /*}}}*/
  /* draw x numbers */ /*{{{*/
  for (width=4; width<sheet->maxx; ++width) mvwaddch(stdscr,0+sheet->oriy,sheet->orix+width,(chtype)(unsigned char)' ');
  for (width=4,x=sheet->offx; width<sheet->maxx; width+=col,++x)
  {
    col=columnwidth(sheet,x,sheet->curz);
    if (bufsz<(size_t)((col+1)<10 ? 10 : col+1)) buf=realloc(buf,bufsz=(size_t)((col+1)<10 ? 10 : col+1));
    sprintf(buf,"%d",x);
    if ((int)strlen(buf)>col)
    {
      buf[col-1]='$';
      buf[col]='\0';
    }
    adjust(CENTER,buf,(size_t)col+1);
    assert(sheet->maxx>=width);
    if ((sheet->maxx-width)<col) buf[sheet->maxx-width]='\0';
    mvwaddstr(stdscr,sheet->oriy,sheet->orix+width,buf);
  }
  /*}}}*/
  /* draw y numbers */ /*{{{*/
  for (y=1; y<(sheet->maxy-1); ++y) (void)mvwprintw(stdscr,sheet->oriy+y,sheet->orix,"%-4d",y-1+sheet->offy);
  /*}}}*/
  /* draw z number */ /*{{{*/
  (void)mvwprintw(stdscr,sheet->oriy,sheet->orix,"%-4d",sheet->curz);
  /*}}}*/
  (void)wattroff(stdscr,DEF_NUMBER);
  /* draw elements */ /*{{{*/
  for (y=1; y<sheet->maxy-1; ++y) for (width=4,x=sheet->offx; width<sheet->maxx; width+=columnwidth(sheet,x,sheet->curz),++x)
  {
    size_t size,realsize,fill,cutoff;
    int realx;

    realx=x;
    cutoff=0;
    if (x==sheet->offx) while (shadowed(sheet,realx,y-1+sheet->offy,sheet->curz))
    {
      --realx;
      cutoff+=columnwidth(sheet,realx,sheet->curz);
    }
    if ((size=cellwidth(sheet,realx,y-1+sheet->offy,sheet->curz)))
    {
      int invert;
      
      if (bufsz<(size+1)) buf=realloc(buf,bufsz=(size+1));
      printvalue(buf,size+1,1,quote,getscientific(sheet,realx,y-1+sheet->offy,sheet->curz),getprecision(sheet,realx,y-1+sheet->offy,sheet->curz),sheet,realx,y-1+sheet->offy,sheet->curz);
      adjust(getadjust(sheet,realx,y-1+sheet->offy,sheet->curz),buf,size+1);
      assert(size>=cutoff);
      if (width+((int)(size-cutoff))>=sheet->maxx)
      {
        *(buf+cutoff+sheet->maxx-width)='\0';
        realsize=sheet->maxx-width+cutoff;
      }
      else realsize=size;
      invert=
      (
      (sheet->mark1x!=-1) &&
      ((x>=sheet->mark1x && x<=sheet->mark2x) || (x>=sheet->mark2x && x<=sheet->mark1x)) &&
      ((y-1+sheet->offy>=sheet->mark1y && y-1+sheet->offy<=sheet->mark2y) || (y-1+sheet->offy>=sheet->mark2y && y-1+sheet->offy<=sheet->mark1y)) &&
      ((sheet->curz>=sheet->mark1z && sheet->curz<=sheet->mark2z) || (sheet->curz>=sheet->mark2z && sheet->curz<=sheet->mark1z))
      );
      if (x==sheet->curx && (y-1+sheet->offy)==sheet->cury) invert=(sheet->marking ? 1 : 1-invert);
      if (invert) (void)wattron(stdscr,DEF_CELLCURSOR);
      (void)mvwaddstr(stdscr,sheet->oriy+y,sheet->orix+width,buf+cutoff);
      for (fill=strlen(buf+cutoff); fill<realsize; ++fill) (void)waddch(stdscr,(chtype)(unsigned char)' ');
      if (invert) (void)wattroff(stdscr,DEF_CELLCURSOR);
    }
  }
  /*}}}*/
  /* draw contents of current element */ /*{{{*/
  if (bufsz<(unsigned int)(sheet->maxx+1)) buf=realloc(buf,bufsz=(sheet->maxx+1));
  label=getlabel(sheet,sheet->curx,sheet->cury,sheet->curz);
  assert(label!=(const char*)0);
  moveonly=sheet->moveonly ? *MOVEONLY : *MOVEEDIT;
  if (*label=='\0') sprintf(pbuf,"%c @(%d,%d,%d)=",moveonly,sheet->curx,sheet->cury,sheet->curz);
  else sprintf(pbuf,"%c @(%s)=",moveonly,label);
  (void)strncpy(buf,pbuf,sheet->maxx+1);
  buf[sheet->maxx]='\0';
  if ((err=geterror(sheet,sheet->curx,sheet->cury,sheet->curz))!=(const char*)0)
  {
    (void)strncpy(buf,err,width-strlen(buf));
    free(err);
    buf[sheet->maxx]='\0';
  }
  else
  {
    print(buf+strlen(buf),sheet->maxx+1-strlen(buf),0,1,getscientific(sheet,sheet->curx,sheet->cury,sheet->curz),-1,getcont(sheet,sheet->curx,sheet->cury,sheet->curz,0));
    if (getcont(sheet,sheet->curx,sheet->cury,sheet->curz,1) && strlen(buf)<(size_t)(sheet->maxx+1-4))
    {
      strcat(buf," -> ");
      print(buf+strlen(buf),sheet->maxx+1-strlen(buf),0,1,getscientific(sheet,sheet->curx,sheet->cury,sheet->curz),-1,getcont(sheet,sheet->curx,sheet->cury,sheet->curz,1));
    }
  }
  (void)mvwaddstr(stdscr,sheet->oriy+sheet->maxy-1,sheet->orix,buf);
  for (col=strlen(buf); col<sheet->maxx; ++col) (void)waddch(stdscr,' ');
  free(buf);
  /*}}}*/
}
/*}}}*/
/* line_edit    -- line editor function */ /*{{{*/
int line_edit(Sheet *sheet, char *buf, size_t size, const char *prompt, size_t *x, size_t *offx)
{
  /* variables */ /*{{{*/
  size_t promptlen;
  int i,mx,my,insert;
  chtype c;
  /*}}}*/

  /* asserts */ /*{{{*/
  assert(buf!=(char*)0);
  assert(prompt!=(char*)0);  
  assert(x!=(size_t*)0);  
  assert(offx!=(size_t*)0);  
  /*}}}*/
  (void)curs_set(1);
  mx=COLS;
  my=LINES;
  promptlen=strlen(prompt)+1;
  (void)mvwaddstr(stdscr,LINES-1,0,prompt); (void)waddch(stdscr,(chtype)(unsigned char)' ');
  insert=1;
  do
  {
    /* correct offx to cursor stays visible */ /*{{{*/
    if (*x<*offx) *offx=*x;
    if ((*x-*offx)>(mx-promptlen-1)) *offx=*x-mx+promptlen+1;
    /*}}}*/
    /* display buffer */ /*{{{*/
    (void)wmove(stdscr,LINES-1,(int)promptlen);
    for (i=promptlen; buf[i-promptlen+*offx]!='\0' && i<COLS; ++i) (void)waddch(stdscr,(chtype)(unsigned char)(buf[i-promptlen+*offx]));
    if (i!=mx) (void)wclrtoeol(stdscr);
    /*}}}*/
    /* show cursor */ /*{{{*/
    (void)wmove(stdscr,LINES-1,(int)(*x-*offx+promptlen));
    /*}}}*/
    c=wgetc();
    if (sheet!=(Sheet*)0 && sheet->moveonly) /* move around in sheet */ /*{{{*/
    switch (c)
    {
      /*      ^o -- switch back to line editor */ /*{{{*/
      case '\017': sheet->moveonly=0; break;
      /*}}}*/
      /*       v -- insert value of current cell */ /*{{{*/
      case 'v':
      {
        /* variables */ /*{{{*/
        char valbuf[1024];
        /*}}}*/

        printvalue(valbuf,sizeof(valbuf),0,1,getscientific(sheet,sheet->curx,sheet->cury,sheet->curz),-1,sheet,sheet->curx,sheet->cury,sheet->curz);
        if (strlen(buf)+strlen(valbuf)>=(size-1)) break;
        (void)memmove(buf+*x+strlen(valbuf),buf+*x,strlen(buf)-*x+strlen(valbuf));
        (void)memcpy(buf+*x,valbuf,strlen(valbuf));
        (*x)+=strlen(valbuf);
        break;
      }
      /*}}}*/
      /*       p -- insert position of current cell */ /*{{{*/
      case 'p':
      {
        /* variables */ /*{{{*/
        char valbuf[1024];
        /*}}}*/

        sprintf(valbuf,"(%d,%d,%d)",sheet->curx,sheet->cury,sheet->curz);
        if (strlen(buf)+strlen(valbuf)>=(size-1)) break;
        (void)memmove(buf+*x+strlen(valbuf),buf+*x,strlen(buf)-*x+strlen(valbuf));
        (void)memcpy(buf+*x,valbuf,strlen(valbuf));
        (*x)+=strlen(valbuf);
        break;
      }
      /*}}}*/
      /* default -- move around in sheet */ /*{{{*/
      default: (void)do_sheetcmd(sheet,c,1); redraw_sheet(sheet); break;
      /*}}}*/
    }
    /*}}}*/
    else /* edit line */ /*{{{*/
    switch (c)
    {
      /* UP */ /*{{{*/
      case KEY_UP: break;
      /*}}}*/
      /* LEFT */ /*{{{*/
      case KEY_LEFT: if (*x>0) --(*x); break;
      /*}}}*/
      /* RIGHT */ /*{{{*/
      case KEY_RIGHT: if (*x<strlen(buf)) ++(*x); break;
      /*}}}*/
      /* BACKSPACE      */ /*{{{*/
      case KEY_BACKSPACE:
      {
        if (*x>0)
        {
          memmove(buf+*x-1,buf+*x,strlen(buf+*x)+1);
          --(*x);
        }
        break;
      }
      /*}}}*/
      /* C-i -- file name completion */ /*{{{*/
      case '\t':
      {
        char *s,*r,ch;
        
        ch=buf[*x];
        buf[*x]='\0';
        if ((r=s=completefile(buf)))
        {
          buf[*x]=ch;
          for (; *r; ++r)
          {
            if (strlen(buf)==(size-1)) break;
            memmove(buf+*x+1,buf+*x,strlen(buf)-*x+1);
            *(buf+*x)=*r;
            ++(*x);
          }  
          free(s);
        }
        else buf[*x]=ch;
        break;
      }
      /*}}}*/
      /* DC */ /*{{{*/
      case KEY_DC:
      {
        if (*x<strlen(buf)) memmove(buf+*x,buf+*x+1,strlen(buf+*x));
        break;
      }
      /*}}}*/
      /* BEG */ /*{{{*/
      case KEY_BEG:
      case KEY_HOME:
      {
        *x=0;
        break;
      }  
      /*}}}*/
      /* END */ /*{{{*/
      case KEY_END:
      {
        *x=strlen(buf);
        break;
      }  
      /*}}}*/
      /* IC */ /*{{{*/
      case KEY_IC:
      {
        insert=1-insert;
        break;
      }  
      /*}}}*/
      /* EIC */ /*{{{*/
      case KEY_EIC:
      {
        insert=0;
        break;
      }
      /*}}}*/
      /* control t */ /*{{{*/
      case '\024':
      {
        if (*x>0 && (strlen(buf)<(size-1) || *x!=strlen(buf)))
        {
          char c;

          c=*(buf+*x);
          *(buf+*x)=*(buf+*x-1);
          if (c=='\0')
          {
            c=' ';
            *(buf+*x+1)='\0';
          }
          *(buf+*x-1)=c;
          ++(*x);
        }
        break;
      }
      /*}}}*/
      /* control backslash */ /*{{{*/
      case '\034':
      {
        int curx,level;

        curx=*x;
        level=1;
        switch (buf[curx])
        {
          /* (    */ /*{{{*/
          case '(':
          {
            if (buf[curx]) ++curx;
            while (buf[curx]!='\0' && level>0) 
            {
              if (buf[curx]==')') --level;
              else if (buf[curx]=='(') ++level;
              if (level) ++curx;
            }
            if (level==0) *x=curx;
            break;
          }
          /*}}}*/
          /* ) */ /*{{{*/
          case ')':
          {
            if (curx>0) --curx;
            while (curx>0 && level>0) 
            {
              if (buf[curx]==')') ++level;
              else if (buf[curx]=='(') --level;
              if (level) --curx;
            }
            if (level==0) *x=curx;
            break;
          }
          /*}}}*/
        }   
        break;
      }
      /*}}}*/
      /* DL */ /*{{{*/
      case KEY_DL:
      {
        *(buf+*x)='\0';
        break;
      }
      /*}}}*/
      /* control o */ /*{{{*/
      case '\017': if (sheet!=(Sheet*)0) sheet->moveonly=1; break;
      /*}}}*/
      /* default */ /*{{{*/
      default:
      {
        if (((unsigned)c)<' ' || ((unsigned)c)>=256 || strlen(buf)==(size-1)) break;
        if (insert) memmove(buf+*x+1,buf+*x,strlen(buf)-*x+1);
        else if (*x==strlen(buf)) *(buf+*x+1)='\0';
        *(buf+*x)=(char)c;
        ++(*x);
        break;
      }
      /*}}}*/
    }
    /*}}}*/
  } while (c!=KEY_ENTER && c!=KEY_CANCEL && (c!=KEY_UP || (sheet!=(Sheet*)0 && sheet->moveonly)));
  if (sheet) sheet->moveonly=0;
  (void)curs_set(0);
  (void)wmove(stdscr,LINES-1,0);
  (void)wclrtoeol(stdscr);
  switch (c)
  {
    case KEY_CANCEL: return -1;
    case KEY_UP: return -2;
    default: return 0;
  }
}
/*}}}*/
/* line_menu    -- one line menu */ /*{{{*/
/* Notes */ /*{{{*/
/*

The choices are terminated by the last element having a (const char*)str
field.  Each item can be chosen by tolower(*str) or by the key stored in
the c field.  Use a space as first character of str, if you only want the
function key to work.

*/
/*}}}*/
int line_menu(const char *prompt, const MenuChoice *choice, int curx)
{
  /* variables */ /*{{{*/
  int maxx,x,width,offx;
  chtype c;
  size_t promptlen;
  /*}}}*/

  /* asserts */ /*{{{*/
  assert(prompt!=(const char*)0);
  assert(choice!=(const MenuChoice*)0);
  assert(curx>=0);
  /*}}}*/
  mvwaddstr(stdscr,LINES-1,0,prompt);
  promptlen=strlen(prompt);
  for (maxx=0; (choice+maxx)->str!=(const char*)0; ++maxx);
  offx=0;
  do
  {
    (void)wmove(stdscr,LINES-1,(int)promptlen);
    /* correct offset so choice is visible */ /*{{{*/
    if (curx<=offx) offx=curx;
    else do
    {
      for (width=promptlen,x=offx; x<maxx && width+((int)strlen((choice+x)->str+1))+1<=COLS; width+=((int)(strlen((choice+x)->str)))+1,++x);
      --x;
      if (x<curx) ++offx;
    } while (x<curx);
    /*}}}*/
    /* show visible choices */ /*{{{*/
    for (width=promptlen,x=offx; x<maxx && width+((int)strlen((choice+x)->str+1))+1<=COLS; width+=strlen((choice+x)->str+1)+1,++x)
    {
      (void)waddch(stdscr,(chtype)(unsigned char)' ');
      if (x==curx) (void)wattron(stdscr,DEF_MENU);
      (void)waddstr(stdscr,(char*)(choice+x)->str+1);
      if (x==curx) (void)wattroff(stdscr,DEF_MENU);
    }
    /*}}}*/
    if (width<COLS) (void)wclrtoeol(stdscr);
    switch (c=wgetc())
    {
      /* KEY_LEFT              -- move to previous item */ /*{{{*/
      case KEY_BACKSPACE:
      case KEY_LEFT: if (curx>0) --curx; else curx=maxx-1; break;
      /*}}}*/
      /* Space, Tab, KEY_RIGHT -- move to next item */ /*{{{*/
      case ' ':
      case '\t':
      case KEY_RIGHT: if (curx<(maxx-1)) ++curx; else curx=0; break;
      /*}}}*/
      /* default               -- search choice keys */ /*{{{*/
      default:
      {
        int i;

        for (i=0; (choice+i)->str!=(const char*)0; ++i)
        {
          if ((c<256 && tolower(c)==*((choice+i)->str)) || (choice+i)->c==c)
          {
            c=KEY_ENTER;
            curx=i;
          }
        }
      }
      /*}}}*/
    }
  }
  while (c!=KEY_ENTER && c!=KEY_DOWN && c!=KEY_CANCEL && c!=KEY_UP);
  (void)wmove(stdscr,LINES-1,0);
  (void)wclrtoeol(stdscr);
  switch (c)
  {
    case KEY_CANCEL: return -1;
    case KEY_UP: return -2;
    default: return curx;
  }
}
/*}}}*/
/* line_msg     -- one line message which will be cleared by someone else */ /*{{{*/
void line_msg(const char *prompt, const char *msg)
{
  /* variables */ /*{{{*/
  int width;
  /*}}}*/

  /* asserts */ /*{{{*/
  assert(msg!=(const char*)0);
  /*}}}*/
  if (!batch)
  {
    width=1;
    mvwaddch(stdscr,LINES-1,0,(chtype)(unsigned char)'[');
    if (prompt!=(const char*)0)
    {
      for (; width<COLS && *prompt!='\0'; ++width,++prompt) (void)waddch(stdscr,(chtype)(unsigned char)(*prompt));
      if (width<COLS) { (void)waddch(stdscr,(chtype)(unsigned char)' '); ++width; }
    }
    for (; width<COLS && *msg!='\0'; ++width,++msg) (void)waddch(stdscr,(chtype)(unsigned char)(*msg));
    if (width<COLS) (void)waddch(stdscr,(chtype)(unsigned char)']');
    if (width+1<COLS) (void)wclrtoeol(stdscr);
  }
  else
  {
    if (prompt) fprintf(stderr,"line %u: %s %s\n",batchln,prompt,msg);
    else fprintf(stderr,"line %u: %s\n",batchln,msg);
    exit(1);
  }
}
/*}}}*/
/* keypressed   -- get keypress, if there is one */ /*{{{*/
int keypressed(void)
{
  (void)nodelay(stdscr,TRUE);
  if (getch()==ERR)
  {
    (void)nodelay(stdscr,FALSE);
    return 0;
  }
  else
  {
    (void)nodelay(stdscr,FALSE);
    return 1;
  }
}
/*}}}*/



