/* #includes */ /*{{{C}}}*//*{{{*/
#ifndef NO_POSIX_SOURCE
#undef _POSIX_SOURCE
#define _POSIX_SOURCE   1
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 2
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <curses.h>
#include <errno.h>
#include <pwd.h>
#include <termios.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
#include "version.h"
/*}}}*/

/* redraw       -- redraw whole screen */ /*{{{*/
static void redraw(void)
{
  (void)touchwin(curscr);
  (void)wrefresh(curscr);
}
/*}}}*/

/* do_attribute   -- set cell attributes */ /*{{{*/
static int do_attribute(Sheet *cursheet)
{
  /* variables */ /*{{{*/
  MenuChoice mainmenu[5];
  MenuChoice adjmenu[9];
  int c;
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
  do
  {
    c = line_menu(cursheet->mark1x==-1 ? CELLATTR : BLOCKATTR,mainmenu,0);
    if (cursheet->mark1x==-1 && c!=2 && locked(cursheet,cursheet->curx,cursheet->cury,cursheet->curz)) line_msg(CELLATTR,ISLOCKED);
    else
    {
      switch (c)
      {
        case -2:
        case -1: c = KEY_CANCEL; break;
        case 0:
        {
          switch (c=line_menu(cursheet->mark1x==-1 ? CELLATTR : BLOCKATTR,adjmenu,0))
          {
            case -2:
            case -1: c = K_INVALID; break;
            case 0: c = ADJUST_LEFT; break;
            case 1: c = ADJUST_RIGHT; break;
            case 2: c = ADJUST_CENTER; break;
            case 3: c = ADJUST_SCIENTIFIC_ON; break;
            case 4: c = ADJUST_SCIENTIFIC_OFF; break;
            case 5: c = ADJUST_PRECISION; break;
            case 6: c = ADJUST_SHADOW; break;
            case 7: c = ADJUST_TRANSPARENT; break;
            default: assert(0);
          }
          break;
        }
        case 1: c = ADJUST_LABEL; break;
        case 2: c = ADJUST_LOCK; break;
        case 3: c = ADJUST_IGNORE; break;
        default: assert(0);
      }
    }
  } while (c == K_INVALID);
  if (c == KEY_CANCEL) c = K_INVALID;
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
    switch (c=line_menu(FILERMENU,menu,0))
    {
      case -2:
      case -1: c = KEY_CANCEL; break;
      case 0: c = K_LOADMENU; break;
      case 1: c = K_SAVEMENU; break;
      case 2: c = K_NAME; break;
      default: assert(0);
    }
  } while (c == K_INVALID);
  if (c == KEY_CANCEL) c = K_INVALID;
  free(menu[0].str);
  free(menu[1].str);
  free(menu[2].str);
  return c;
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
      redraw();
    }
    /*}}}*/
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
    switch (c=line_menu(BLOCKMENU,block,0))
    {
      case -2:
      case -1: c = KEY_CANCEL; break;
      case 0: c = BLOCK_CLEAR; break;
      case 1: c = BLOCK_INSERT; break;
      case 2: c = BLOCK_DELETE; break;
      case 3: c = BLOCK_MOVE; break;
      case 4: c = BLOCK_COPY; break;
      case 5: c = BLOCK_FILL; break;
      case 6: c = BLOCK_SORT; break;    
      case 7: c = BLOCK_MIRROR; break;    
    }
  } while (c == K_INVALID);
  if (c == KEY_CANCEL) c = K_INVALID;
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

int show_menu(Sheet *cursheet)
{
  /* variables */ /*{{{*/
  MenuChoice menu[9];
  int c = K_INVALID;
  /*}}}*/

  menu[0].str=mystrmalloc(ATTR);   menu[0].c='\0';
  menu[1].str=mystrmalloc(CWIDTH); menu[1].c='\0';
  menu[2].str=mystrmalloc(BLOCK);  menu[2].c='\0';
  menu[3].str=mystrmalloc(FILER);  menu[3].c='\0';
  menu[4].str=mystrmalloc(MGOTO);  menu[4].c='\0';
  menu[5].str=mystrmalloc(MSHELL); menu[5].c='\0';
  menu[6].str=mystrmalloc(ABOUT);  menu[6].c='\0';
  menu[7].str=mystrmalloc(QUIT);   menu[7].c='\0';
  menu[8].str=(char*)0;

  do
  {
    switch (c=line_menu(MAINMENU,menu,0))
    {
      case -2:
      case -1: c = KEY_CANCEL; break;
      case 0: c = do_attribute(cursheet); break;
      case 1: c = K_COLWIDTH; break;
      case 2: c = do_block(cursheet); break;
      case 3: c = do_file(cursheet); break;
      case 4: c = K_GOTO; break;
      case 5: do_shell(); c = KEY_CANCEL; break;
      case 6: c = K_ABOUT; break;
      case 7: c = K_QUIT; break;
      default: assert(0);
    }
  } while (c == K_INVALID);
  if (c == KEY_CANCEL) c = K_INVALID;
 
  free(menu[0].str);
  free(menu[1].str);
  free(menu[2].str);
  free(menu[3].str);
  free(menu[4].str);
  free(menu[5].str);
  free(menu[6].str);
  free(menu[7].str);

  return c;
}

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


void display_main(Sheet *cursheet)
{
  Key k;
  int quit = 0;

  cursheet->maxx=COLS;
  cursheet->maxy=LINES-1;

  do
  {
    quit = 0;
    redraw_sheet(cursheet);
    k=wgetc();
    wmove(stdscr,LINES-1,0);
    wclrtoeol(stdscr);
    switch ((int)k)
    {
      case KEY_SUSPEND:
      case '\032': do_bg(); k = K_INVALID; break;
      case '\014': redraw(); k = K_INVALID; break;
      case KEY_F(0):
      case KEY_F(10): k = show_menu(cursheet); break;
    }
  } while (k == K_INVALID || !do_sheetcmd(cursheet,k,0) || doanyway(cursheet,LEAVE)!=1);
}

void display_init(Sheet *cursheet, int always_redraw)
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
  if (always_redraw) typeahead(-1);
#endif
}

void display_end(void)
{
  curs_set(1);
  echo();
  noraw();
  refresh();
  endwin();
}

void redraw_cell(Sheet *sheet, int x, int y, int z)
{
	redraw_sheet(sheet);
}

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

/* line_file    -- line editor function for file name entry */ /*{{{*/
const char *line_file(const char *file, const char *pattern, const char *title, int create)
{
	static char buf[_POSIX_PATH_MAX];
	int rc;
	size_t dummy = 0;

	strncpy(buf, file, sizeof(buf));
	buf[sizeof(buf)-1] = 0;
	rc = line_edit((Sheet*)0, buf, sizeof(buf), title, &dummy, &dummy);
	if (rc < 0) return NULL;
	return buf;
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
      case K_UP: break;
      /*}}}*/
      /* LEFT */ /*{{{*/
      case K_LEFT: if (*x>0) --(*x); break;
      /*}}}*/
      /* RIGHT */ /*{{{*/
      case K_RIGHT: if (*x<strlen(buf)) ++(*x); break;
      /*}}}*/
      /* BACKSPACE      */ /*{{{*/
      case K_BACKSPACE:
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
      case K_DC:
      {
        if (*x<strlen(buf)) memmove(buf+*x,buf+*x+1,strlen(buf+*x));
        break;
      }
      /*}}}*/
      /* HOME */ /*{{{*/
      case K_HOME:
      {
        *x=0;
        break;
      }  
      /*}}}*/
      /* END */ /*{{{*/
      case K_END:
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
  } while (c!=K_ENTER && c!=KEY_CANCEL && (c!=K_UP || (sheet!=(Sheet*)0 && sheet->moveonly)));
  if (sheet) sheet->moveonly=0;
  (void)curs_set(0);
  (void)wmove(stdscr,LINES-1,0);
  (void)wclrtoeol(stdscr);
  switch (c)
  {
    case KEY_CANCEL: return -1;
    case K_UP: return -2;
    default: return 0;
  }
}
/*}}}*/
/* line_ok        -- one line yes/no menu */ /*{{{*/
int line_ok(const char *prompt, int curx)
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
      case K_BACKSPACE:
      case K_LEFT: if (curx>0) --curx; else curx=maxx-1; break;
      /*}}}*/
      /* Space, Tab, KEY_RIGHT -- move to next item */ /*{{{*/
      case ' ':
      case '\t':
      case K_RIGHT: if (curx<(maxx-1)) ++curx; else curx=0; break;
      /*}}}*/
      /* default               -- search choice keys */ /*{{{*/
      default:
      {
        int i;

        for (i=0; (choice+i)->str!=(const char*)0; ++i)
        {
          if ((c<256 && tolower(c)==*((choice+i)->str)) || (choice+i)->c==c)
          {
            c=K_ENTER;
            curx=i;
          }
        }
      }
      /*}}}*/
    }
  }
  while (c!=K_ENTER && c!=K_DOWN && c!=KEY_CANCEL && c!=K_UP);
  (void)wmove(stdscr,LINES-1,0);
  (void)wclrtoeol(stdscr);
  switch (c)
  {
    case KEY_CANCEL: return -1;
    case K_UP: return -2;
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

void show_text(const char *text)
{
  int offset; /* TODO: actually display text. */

  offset=(COLS-41)/2;
  (void)clear();
  (void)move(0,offset+13); (void)addstr("` ',`    '  '");
  (void)move(1,offset+14); (void)addstr("`   '  ` ' '");
  (void)move(2,offset+15); (void)addstr("`' '   '`'");
  (void)move(3,offset+15); (void)addstr("' `   ' '`");
  (void)move(4,offset+2); (void)addstr("'           '` ' ` '`` `");
  (void)move(5,offset+2); (void)addstr("`.   Table Editor And Planner, or:");
  (void)move(6,offset+4); (void)addstr(",         . ,   ,  . .");
  (void)move(7,offset+14); (void)addstr("` '   `  ' '");
  (void)move(8,offset+3); (void)addstr("`::\\    /:::::::::::::::::\\   ___");
  (void)move(9,offset+4); (void)addstr("`::\\  /:::::::::::::::::::\\,'::::\\");
  (void)move(10,offset+5); (void)addstr(":::\\/:::::::::::::::::::::\\/   \\:\\");
  (void)move(11,offset+5); (void)addstr(":::::::::::::::::::::::::::\\    :::");
  (void)move(12,offset+5); (void)addstr("::::::::::::::::::::::::::::;  /:;'");
  (void)move(13,offset+5); (void)addstr("`::::::::::::::::::::::::::::_/:;'");
  (void)move(14,offset+7); (void)addstr("`::::::::::::::::::::::::::::'");
  (void)move(15,offset+8); (void)addstr("`////////////////////////'");
  (void)move(16,offset+9); (void)addstr("`:::::::::::::::::::::'");
  (void)move(17,offset+2); (void)addstr("");
  (void)move(18,offset+17); (void)addstr("Teapot");
  (void)move(20,offset+0); (void)addstr("Version " VERSION ", Copyright by M. Haardt, J. Walter");
  (void)move(22,offset+8); (void)addstr("Press any key to continue");
  (void)refresh();
  (void)getch();
}

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

#include "wgetc.c"
