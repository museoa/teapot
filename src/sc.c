/* #includes */ /*{{{C}}}*//*{{{*/
#ifndef NO_POSIX_SOURCE
#undef _POSIX_SOURCE
#define _POSIX_SOURCE   1
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 2
#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "eval.h"
#include "main.h"
#include "sheet.h"
#include "sc.h"
/*}}}*/

static const char *s2t_s;
static char *s2t_t;

static int s2t_term(Sheet *sheet);

/* s2t_loc */ /*{{{*/
static int s2t_loc(Sheet *sheet, const char **s, char **t)
{
  int x,y;
  char label[10],*l;

  l=label;
  if (**s>='A' && **s<='Z')
  {
    *l++=**s;
    *(*t)++=**s;
    x=*(*s)++-'A';
  }
  else return 0;
  if (**s>='A' && **s<='Z')
  {
    *l++=**s;
    *(*t)++=**s;
    x=x*26+(*(*s)++-'A');
  }
  if (**s>='0' && **s<='9')
  {
    *l++=**s;
    y=**s-'0';
    *(*t)++=*(*s)++;
  }
  else return 0;
  while (**s>='0' && **s<='9')
  {
    *l++=**s;
    y=y*10+(**s-'0');
    *(*t)++=*(*s)++;
  }
  *l='\0';
  if (*getlabel(sheet,x,y,0)=='\0') setlabel(sheet,x,y,0,label,0);
  return 1;
}
/*}}}*/
/* s2t_range */ /*{{{*/
static int s2t_range(Sheet *sheet)
{
  if (s2t_loc(sheet,&s2t_s,&s2t_t)==0) return 0;
  if (*s2t_s==':')
  {
    s2t_s++; *s2t_t++=',';
    if (s2t_loc(sheet,&s2t_s,&s2t_t)==0) return 0;
    return 1;
  }
  else return 0;
}
/*}}}*/
/* s2t_primary */ /*{{{*/
static int s2t_primary(Sheet *sheet)
{
  if (*s2t_s=='@')
  /* @function */ /*{{{*/
  {
    ++s2t_s;
    if (strncmp(s2t_s,"sum(",4)==0)
    /* @sum(range) -> sum(range) */ /*{{{*/
    {
      s2t_s+=4;
      *s2t_t++='s'; *s2t_t++='u'; *s2t_t++='m'; *s2t_t++='(';
      if (s2t_range(sheet)==0) return 0;
      *s2t_t++=')';
      return 1;
    }
    /*}}}*/
    else if (strncmp(s2t_s,"rnd(",4)==0)
    /* @rnd(e) -> int(e,-1,1) */ /*{{{*/
    {
      s2t_s+=4;
      *s2t_t++='i'; *s2t_t++='n'; *s2t_t++='t'; *s2t_t++='(';
      if (s2t_term(sheet)==0) return 0;
      *s2t_t++=','; *s2t_t++='-'; *s2t_t++='1'; *s2t_t++=','; *s2t_t++='1'; *s2t_t++=')';
      return 1;
    }
    /*}}}*/
    else if (strncmp(s2t_s,"floor(",6)==0)
    /* @floor(e) -> int(e,-2,-2) */ /*{{{*/
    {
      s2t_s+=6;
      *s2t_t++='i'; *s2t_t++='n'; *s2t_t++='t'; *s2t_t++='(';
      if (s2t_term(sheet)==0) return 0;
      *s2t_t++=','; *s2t_t++='-'; *s2t_t++='2'; *s2t_t++=','; *s2t_t++='-'; *s2t_t++='2'; *s2t_t++=')';
      return 1;
    }
    /*}}}*/
    else if (strncmp(s2t_s,"ceil(",5)==0)
    /* @ceil(e) -> int(e,2,2) */ /*{{{*/
    {
      s2t_s+=5;
      *s2t_t++='i'; *s2t_t++='n'; *s2t_t++='t'; *s2t_t++='(';
      if (s2t_term(sheet)==0) return 0;
      *s2t_t++=','; *s2t_t++='2'; *s2t_t++=','; *s2t_t++='2'; *s2t_t++=')';
      return 1;
    }
    /*}}}*/
    else return 0;
  }
  /*}}}*/
  else if ((*s2t_s>='0' && *s2t_s<='9') || *s2t_s=='.')
  /* number */ /*{{{*/
  {
    if (*s2t_s=='.') *s2t_t++='0'; else *s2t_t++=*s2t_s++;
    while (*s2t_s>='0' && *s2t_s<='9') *s2t_t++=*s2t_s++;
    if (*s2t_s=='.')
    {
      *s2t_t++=*s2t_s++;
      while (*s2t_s>='0' && *s2t_s<='9') *s2t_t++=*s2t_s++;
    }
    else
    {
      *s2t_t++='.'; *s2t_t++='0';
    }
    return 1;
  }
  /*}}}*/
  else if (*s2t_s>='A' && *s2t_s<='Z')
  /* cell value */ /*{{{*/
  {
    *s2t_t++='@'; *s2t_t++='(';
    if (s2t_loc(sheet,&s2t_s,&s2t_t)==0) return 0;
    *s2t_t++=')';
    return 1;
  }
  /*}}}*/
  else if (*s2t_s) return 0;
  else return 1;
}
/*}}}*/
/* s2t_powterm */ /*{{{*/
static int s2t_powterm(Sheet *sheet)
{
  if (s2t_primary(sheet)==0) return 0;
  while (*s2t_s=='^')
  {
    *s2t_t++=*s2t_s++;
    if (s2t_primary(sheet)==0) return 0;
  }
  return 1;
}
/*}}}*/
/* s2t_piterm */ /*{{{*/
static int s2t_piterm(Sheet *sheet)
{
  if (s2t_powterm(sheet)==0) return 0;
  while (*s2t_s=='*' || *s2t_s=='/')
  {
    *s2t_t++=*s2t_s++;
    if (s2t_powterm(sheet)==0) return 0;
  }
  return 1;
}
/*}}}*/
/* s2t_term */ /*{{{*/
static int s2t_term(Sheet *sheet)
{
  if (s2t_piterm(sheet)==0) return 0;
  while (*s2t_s=='+' || *s2t_s=='-')
  {
    *s2t_t++=*s2t_s++;
    if (s2t_piterm(sheet)==0) return 0;
  }
  return 1;
}
/*}}}*/

/* loadsc */ /*{{{*/
const char *loadsc(Sheet *sheet, const char *name)
{
  /* variables */ /*{{{*/
  static char errbuf[80];
  FILE *fp;
  char buf[256];
  int line;
  size_t width;
  const char *err;
  int x,y;
  /*}}}*/

  if ((fp=fopen(name,"r"))==(FILE*)0) return strerror(errno);
  freesheet(sheet,0);
  err=(const char*)0;
  line=1;
  while (fgets(buf,sizeof(buf),fp)!=(char*)0)
  {
    /* remove nl */ /*{{{*/
    width=strlen(buf);
    if (width>0 && buf[width-1]=='\n') buf[width-1]='\0';
    /*}}}*/
    if (buf[0] && buf[0]!='#')
    {
      if (strncmp(buf,"format ",7)==0) /* format col width precision whoknows */ /*{{{*/
      {
        char colstr[3];
        int col,colwidth,precision,whoknows;

        sscanf(buf+7,"%s %d %d %d",colstr,&colwidth,&precision,&whoknows);
        col=(colstr[0]-'A'); if (colstr[1]) col=col*26+(colstr[1]-'A');
        initcell(sheet,col,0,0);
        SHEET(sheet,col,0,0)->adjust=RIGHT;
        SHEET(sheet,col,0,0)->precision=precision;
        setwidth(sheet,col,0,colwidth);
      }
      /*}}}*/
      else if (strncmp(buf,"leftstring ",11)==0 || strncmp(buf,"rightstring ",12)==0) /* rightstring/leftstring cell = "string" */ /*{{{*/
      {
        int x,y;
        const char *s;
        Token **contents;

        if (strncmp(buf,"leftstring ",11)==0) s=buf+11; else s=buf+12;
        x=*s++-'A'; if (*s>='A' && *s<='Z') x=x*26+(*s++-'A');
        y=*s++-'0'; while (*s>='0' && *s<='9') y=10*y+(*s++-'0');
        s+=3;
        contents=scan(&s);
        if (contents==(Token**)0)
        {
          tvecfree(contents);
          sprintf(errbuf,_("Expression syntax error in line %d"),line);
          err=errbuf;
          goto eek;
        }
        initcell(sheet,x,y,0);
        SHEET(sheet,x,y,0)->adjust=strncmp(buf,"leftstring ",11) ? RIGHT : LEFT;
        SHEET(sheet,x,y,0)->contents=contents;
      }
      /*}}}*/
      else if (strncmp(buf,"let ",4)==0)  /* let cell = expression */ /*{{{*/
      {
        /* variables */ /*{{{*/
        const char *s;
        Token **contents;
        char newbuf[512];
        /*}}}*/

        s=buf+4;
        x=*s++-'A'; if (*s>='A' && *s<='Z') x=x*26+(*s++-'A');
        y=*s++-'0'; while (*s>='0' && *s<='9') y=10*y+(*s++-'0');
        if (getcont(sheet,x,y,0,0)==(Token**)0)
        {
          s+=3;
          s2t_s=s; s2t_t=newbuf;
          if (s2t_term(sheet)==0)
          {
            *s2t_t='\0';
            if (err==(const char*)0)
            {
              sprintf(errbuf,_("Unimplemented SC feature in line %d"),line);
              err=errbuf;
            }
          }
          *s2t_t='\0';
          s=newbuf;
          contents=scan(&s);
          if (contents==(Token**)0)
          {
            tvecfree(contents);
            sprintf(errbuf,_("Expression syntax error in line %d"),line);
            err=errbuf;
            goto eek;
          }
          initcell(sheet,x,y,0);
          SHEET(sheet,x,y,0)->adjust=RIGHT;
          SHEET(sheet,x,y,0)->contents=contents;
        }
      }
      /*}}}*/
    }
    ++line;
  }
  /* set precisions for each column */ /*{{{*/
  for (x=0; x<sheet->dimx; ++x)
  {
    int prec;

    prec=getprecision(sheet,x,0,0)==def_precision ? 2 : getprecision(sheet,x,0,0);
    for (y=1; y<sheet->dimy; ++y) if (SHEET(sheet,x,y,0)) SHEET(sheet,x,y,0)->precision=prec;
  }
  /*}}}*/
  eek:
  if (fclose(fp)==EOF && err==(const char*)0) err=strerror(errno);
  sheet->changed=0;
  cachelabels(sheet);
  forceupdate(sheet);
  return err;
}
/*}}}*/
