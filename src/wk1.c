/* #includes */ /*{{{C}}}*//*{{{*/
#ifndef NO_POSIX_SOURCE
#undef _POSIX_SOURCE
#define _POSIX_SOURCE   1
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 2
#endif

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef OLD_REALLOC
#define realloc(s,l) myrealloc(s,l)
#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif


#include "eval.h"
#include "main.h"
#include "misc.h"
#include "sheet.h"
#include "wk1.h"
/*}}}*/
/* #defines */ /*{{{*/
#define WK1DEBUG 0
#define LOTUS123 0x0404
#define SYMPHONY 0x0405
/* Applix uses that, too */
#define EXCEL    0x0406
/*}}}*/

static int unrpn(const char *s, const int *offset, int top, Token **t, int *tokens);

#if WK1DEBUG
static FILE *se;
#endif

/* it      -- convert string to int */ /*{{{*/
static int it(const char *s)
{
  return (((unsigned int)((const unsigned char)*s))|(*(s+1)<<8));
}
/*}}}*/
/* dbl     -- convert string to double */ /*{{{*/
static double dbl(const unsigned char *s)
{
  double x;
  int sg,e,i;

  x=0.0;
  for (i=1; i<256; i<<=1) x=x/2.0+!!(s[0]&i);
  for (i=1; i<256; i<<=1) x=x/2.0+!!(s[1]&i);
  for (i=1; i<256; i<<=1) x=x/2.0+!!(s[2]&i);
  for (i=1; i<256; i<<=1) x=x/2.0+!!(s[3]&i);
  for (i=1; i<256; i<<=1) x=x/2.0+!!(s[4]&i);
  for (i=1; i<256; i<<=1) x=x/2.0+!!(s[5]&i);
  x=x/2.0+!!(s[6]&0x01);
  x=x/2.0+!!(s[6]&0x02);
  x=x/2.0+!!(s[6]&0x04);
  x=x/2.0+!!(s[6]&0x08);
  x=x/2.0+1.0;
  if ((e=((s[6]>>4)+((s[7]&0x7f)<<4))-1023)==-1023)
  {
    x=0.0;
    e=0;
  }
  if (s[7]&0x80) sg=-1; else sg=1;
#if WK1DEBUG
  fprintf(se,"%02x %02x %02x %02x %02x %02x %02x %02x ",s[0],s[1],s[2],s[3],s[4],s[5],s[6],s[7]);
  fprintf(se,"%f (exp 2^%d)\r\n",sg*ldexp(x,e),e);
#endif
  return (sg*ldexp(x,e));
}
/*}}}*/
/* format  -- convert string into format */ /*{{{*/
static void format(unsigned char s, Cell *cell)
{
#if WK1DEBUG
  fprintf(se,", format 0x%02x",s);
  if (s&0x80) fprintf(se,", locked");
#endif
  switch (((unsigned int)(s&0x70))>>4)
  {
    case 0: /* fixed with given precision */ /*{{{*/
    {
      cell->precision=s&0x0f;
      cell->scientific=0;
      break;
    }
    /*}}}*/
    case 1: /* scientifix with given presision */ /*{{{*/
    {
      cell->precision=s&0x0f;
      cell->scientific=1;
      break;
    }
    /*}}}*/
    case 2: /* currency with given precision */ /*{{{*/
    {
      cell->precision=s&0x0f;
      break;
    }
    /*}}}*/
    case 3: /* percent with given precision */ /*{{{*/
    {
      cell->precision=s&0x0f;
      break;
    }
    /*}}}*/
    case 4: /* comma with given precision */ /*{{{*/
    {
      cell->precision=s&0x0f;
      break;
    }
    /*}}}*/
    case 5: /* unused */ break;
    case 6: /* unused */ break;
    case 7:
    {
      switch (s&0x0f)
      {
        case 0: /* +/- */; break;
        case 1: /* general */; break;
        case 2: /* day-month-year */; break;
        case 3: /* day-month */; break;
        case 4: /* month-year */; break;
        case 5: /* text */; break;
        case 6: /* hidden */; break;
        case 7: /* date;hour-min-sec */; break;
        case 8: /* date;hour-min */; break;
        case 9: /* date;intnt'l1 */; break;
        case 10: /* date;intnt'l2 */; break;
        case 11: /* time;intnt'l1 */; break;
        case 12: /* time;intnt'l2 */; break;
        case 13: /* unused13 */; break;
        case 14: /* unused14 */; break;
        case 15: /* default special format */; break;
      }
      break;
    }
  }
}
/*}}}*/
/* pair    -- convert coordinate pair */ /*{{{*/
static int pair(const char *s, Token **t, int *tokens)
{
  int x,y;

  x=it(s); y=it(s+2);
  if (t)
  {
    if ((t[*tokens]=malloc(sizeof(Token)))==(Token*)0) return -1;
    t[*tokens]->type=OPERATOR;
    t[*tokens]->u.op=OP;
#if WK1DEBUG
    fprintf(se,"[(");
#endif
  }
  if (tokens) ++(*tokens);
  switch (x&0xc000)
  {
    case 0x0000: /* MSB -> 0 0 */ /*{{{*/
    {
      x=x&0x2000 ? x|0xc000 : x&0x3fff;
      if (t)
      {
        if ((t[*tokens]=malloc(sizeof(Token)))==(Token*)0) return -1;
        t[*tokens]->type=INT;
        t[*tokens]->u.integer=x;
#if WK1DEBUG
        fprintf(se,"%d",x);
#endif
      }
      if (tokens) ++(*tokens);
      break;
    }
    /*}}}*/
    case 0x4000: assert(0); break;
    case 0x8000: /* MSB -> 1 0 */ /*{{{*/
    {
      x=x&0x2000 ? x|0xc000 : x&0x3fff;
      if (x!=0)
      {
        if (t)
        {
          if ((t[*tokens]=malloc(sizeof(Token)))==(Token*)0 || (t[*tokens+1]=malloc(sizeof(Token)))==(Token*)0 || (t[*tokens+2]=malloc(sizeof(Token)))==(Token*)0) return -1;
          t[*tokens]->type=FIDENT;
          t[*tokens]->u.fident=identcode("x",1);
          t[*tokens+1]->type=OPERATOR;
          t[*tokens+1]->u.op=OP;
          t[*tokens+2]->type=OPERATOR;
          t[*tokens+2]->u.op=CP;
#if WK1DEBUG
          fprintf(se,"x()");
#endif
        }
        if (tokens) *tokens+=3;
        if (t)
        {
          if ((t[*tokens]=malloc(sizeof(Token)))==(Token*)0 || (t[*tokens+1]=malloc(sizeof(Token)))==(Token*)0) return -1;
          t[*tokens]->type=OPERATOR;
          t[*tokens+1]->type=INT;
          if (x<0)
          {
            t[*tokens]->u.op=MINUS;
            t[*tokens+1]->u.integer=-x;
#if WK1DEBUG
            fprintf(se,"-%d",-x);
#endif
          }
          else
          {
            t[*tokens]->u.op=PLUS;
            t[*tokens+1]->u.integer=x;
#if WK1DEBUG
            fprintf(se,"+%d",x);
#endif
          }
        }
        if (tokens) *tokens+=2;
      }
      break;
    }
    /*}}}*/
    case 0xc000: assert(0); break;
  }
  if (t)
  {
    if ((t[*tokens]=malloc(sizeof(Token)))==(Token*)0) return -1;
    t[*tokens]->type=OPERATOR;
    t[*tokens]->u.op=COMMA;
#if WK1DEBUG
    fprintf(se,",");
#endif
  }
  if (tokens) ++(*tokens);
  switch (y&0xc000)
  {
    case 0x0000: /* MSB -> 0 0 */ /*{{{*/
    {
      y=y&0x2000 ? y|0xc000 : y&0x3fff;
      if (t)
      {
        if ((t[*tokens]=malloc(sizeof(Token)))==(Token*)0) return -1;
        t[*tokens]->type=INT;
        t[*tokens]->u.integer=y;
#if WK1DEBUG
        fprintf(se,"%d",y);
#endif
      }
      if (tokens) ++(*tokens);
      break;
    }
    /*}}}*/
    case 0x4000: assert(0); break;
    case 0x8000: /* MSB -> 1 0 */ /*{{{*/
    {
      y=y&0x2000 ? y|0xc000 : y&0x3fff;
      if (y)
      {
        if (t)
        {
          if ((t[*tokens]=malloc(sizeof(Token)))==(Token*)0 || (t[*tokens+1]=malloc(sizeof(Token)))==(Token*)0 || (t[*tokens+2]=malloc(sizeof(Token)))==(Token*)0) return -1;
          t[*tokens]->type=FIDENT;
          t[*tokens]->u.fident=identcode("y",1);
          t[*tokens+1]->type=OPERATOR;
          t[*tokens+1]->u.op=OP;
          t[*tokens+2]->type=OPERATOR;
          t[*tokens+2]->u.op=CP;
#if WK1DEBUG
          fprintf(se,"y()");
#endif
        }
        if (tokens) *tokens+=3;
        if (t)
        {
          if ((t[*tokens]=malloc(sizeof(Token)))==(Token*)0 || (t[*tokens+1]=malloc(sizeof(Token)))==(Token*)0) return -1;
          t[*tokens]->type=OPERATOR;
          t[*tokens+1]->type=INT;
          if (y<0)
          {
            if (t)
            {
              t[*tokens]->u.op=MINUS;
              t[*tokens+1]->u.integer=-y;
#if WK1DEBUG
              fprintf(se,"-%d",-y);
#endif
            }
          }
          else
          {
            if (t)
            {
              t[*tokens]->u.op=PLUS;
              t[*tokens+1]->u.integer=y;
#if WK1DEBUG
              fprintf(se,"+%d",y);
#endif
            }
          }
        }
        if (tokens) *tokens+=2;
      }
      break;
    }
    /*}}}*/
    case 0xc000: assert(0); break;
  }
  if (t)
  {
    if ((t[*tokens]=malloc(sizeof(Token)))==(Token*)0) return -1;
    t[*tokens]->type=OPERATOR;
    t[*tokens]->u.op=CP;
#if WK1DEBUG
    fprintf(se,")]");
#endif
  }
  if (tokens) ++(*tokens);
  return 0;
}
/*}}}*/
/* sumup   -- sum up arguments */ /*{{{*/
static int sumup(const char *s, const int *offset, int top, Token **t, int *tokens, int argc)
{
  int low;

  if (top<0) return -1;
  if (argc>1)
  {
    low=unrpn(s,offset,top,(Token**)0,(int*)0);
    if (low<0) return -1;
    sumup(s,offset,low-1,t,tokens,argc-1);
    if (t)
    {
      if ((t[*tokens]=malloc(sizeof(Token)))==(Token*)0) return -1;
      t[*tokens]->type=OPERATOR;
      t[*tokens]->u.op=PLUS;
#if WK1DEBUG
      fprintf(se,"[+]");
#endif
    }
    if (tokens) ++(*tokens);
  }
  if (s[offset[top]]==2)
  {
    if (t)
    {
      if ((t[*tokens]=malloc(sizeof(Token)))==(Token*)0 || (t[*tokens+1]=malloc(sizeof(Token)))==(Token*)0) return -1;
      t[*tokens]->type=FIDENT;
      t[*tokens]->u.fident=identcode("sum",3);
      t[*tokens+1]->type=OPERATOR;
      t[*tokens+1]->u.op=OP;
#if WK1DEBUG
      fprintf(se,"[sum(]");
#endif
    }
    if (tokens) *tokens+=2;
  }
  low=unrpn(s,offset,top,t,tokens);
  if (s[offset[top]]==2)
  {
    if (t)
    {
      if ((t[*tokens]=malloc(sizeof(Token)))==(Token*)0) return -1;
      t[*tokens]->type=OPERATOR;
      t[*tokens]->u.op=CP;
#if WK1DEBUG
      fprintf(se,"[)]");
#endif
    }
    if (tokens) ++(*tokens);
  }
  return low;
}
/*}}}*/
/* unrpn   -- convert RPN expression to infix */ /*{{{*/
static int unrpn(const char *s, const int *offset, int top, Token **t, int *tokens)
{
  int low;
  
  if (top<0) return -1;
  switch (s[offset[top]])
  {
    case  0: /* double constant                    */ /*{{{*/
    {
      if (t)
      {
        if ((t[*tokens]=malloc(sizeof(Token)))==(Token*)0) return -1;
        t[*tokens]->type=FLOAT;
        t[*tokens]->u.flt=dbl((const unsigned char*)s+offset[top]+1);
#if WK1DEBUG
        fprintf(se,"[constant %f]",dbl((const unsigned char*)s+offset[top]+1));
#endif
      }
      if (tokens) ++(*tokens);
      low=top;
      break;
    }
    /*}}}*/
    case  1: /* variable                           */ /*{{{*/
    {
      if (t)
      {
        if ((t[*tokens]=malloc(sizeof(Token)))==(Token*)0) return -1;
        t[*tokens]->type=FIDENT;
        t[*tokens]->u.fident=identcode("@",1);
      }
      if (tokens) ++(*tokens);
      if (pair(s+offset[top]+1,t,tokens)==-1) low=-1; else low=top;
      break;
    }
    /*}}}*/
    case  2: /* range                              */ /*{{{*/
    {
      if (t)
      {
        if ((t[*tokens]=malloc(sizeof(Token)))==(Token*)0) return -1;
        t[*tokens]->type=FIDENT;
        t[*tokens]->u.fident=identcode("&",1);
#if WK1DEBUG
        fprintf(se,"[&]");
#endif
      }
      if (tokens) ++(*tokens);
      pair(s+offset[top]+1,t,tokens);
      if (t)
      {
        if ((t[*tokens]=malloc(sizeof(Token)))==(Token*)0 || (t[*tokens+1]=malloc(sizeof(Token)))==(Token*)0) return -1;
        t[*tokens]->type=OPERATOR;
        t[*tokens]->u.fident=COMMA;
        t[*tokens+1]->type=FIDENT;
        t[*tokens+1]->u.op=identcode("&",1);
#if WK1DEBUG
        fprintf(se,"[,&]");
#endif
      }
      if (tokens) *tokens+=2;
      pair(s+offset[top]+5,t,tokens);
      low=top;
      break;
    }
    /*}}}*/
    case  3: /* return                             */ /*{{{*/
    {
      low=unrpn(s,offset,top-1,t,tokens);
      if (t)
      {
        t[*tokens]=(Token*)0;
#if WK1DEBUG
        fprintf(se,"[RETURN]");
#endif
      }
      if (tokens) ++(*tokens);
      break;
    }
    /*}}}*/
    case  4: /* paren                              */ /*{{{*/
    {
      if (t)
      {
        if ((t[*tokens]=malloc(sizeof(Token)))==(Token*)0) return -1;
        t[*tokens]->type=OPERATOR;
        t[*tokens]->u.op=OP;
#if WK1DEBUG
        fprintf(se,"[(]");
#endif
      }
      if (tokens) ++(*tokens);
      low=unrpn(s,offset,top-1,t,tokens);
      if (t)
      {
        if ((t[*tokens]=malloc(sizeof(Token)))==(Token*)0) return -1;
        t[*tokens]->type=OPERATOR;
        t[*tokens]->u.op=CP;
#if WK1DEBUG
        fprintf(se,"[)]");
#endif
      }
      if (tokens) ++(*tokens);
      break;
    }
    /*}}}*/
    case  5: /* int constant                       */ /*{{{*/
    {
      if (t)
      {
        if ((t[*tokens]=malloc(sizeof(Token)))==(Token*)0) return -1;
        t[*tokens]->type=INT;
        t[*tokens]->u.integer=it(s+offset[top]+1);
#if WK1DEBUG
        fprintf(se,"[constant %d]",it(s+offset[top]+1));
#endif
      }
      if (tokens) ++(*tokens);
      low=top;
      break;
    }
    /*}}}*/
    case  9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19: /* +, -, *, /, ^, -, !=, <=, >=, <, > */ /*{{{*/
    {
      if (t)
      {
        low=unrpn(s,offset,top-1,(Token**)0,(int*)0);
        low=unrpn(s,offset,low-1,t,tokens);
        if (low<0) return -1;
        if ((t[*tokens]=malloc(sizeof(Token)))==(Token*)0) return -1;
        t[*tokens]->type=OPERATOR;
        switch (s[offset[top]])
        {
          case  9: /* +  */ /*{{{*/
          {
            t[*tokens]->u.op=PLUS;
#if WK1DEBUG
            fprintf(se,"[+]");
#endif
            break;
          }
          /*}}}*/
          case 10: /* -  */ /*{{{*/
          {
            t[*tokens]->u.op=MINUS;
#if WK1DEBUG
            fprintf(se,"[-]");
#endif
            break;
          }
          /*}}}*/
          case 11: /* *  */ /*{{{*/
          {
            t[*tokens]->u.op=MUL;
#if WK1DEBUG
            fprintf(se,"[*]");
#endif
            break;
          }
          /*}}}*/
          case 12: /* /  */ /*{{{*/
          {
            t[*tokens]->u.op=DIV;
#if WK1DEBUG
            fprintf(se,"[/]");
#endif
            break;
          }
          /*}}}*/
          case 13: /* ^  */ /*{{{*/
          {
            t[*tokens]->u.op=POW;
#if WK1DEBUG
            fprintf(se,"[^]");
#endif
            break;
          }
          /*}}}*/
          case 14: /* == */ /*{{{*/
          {
            t[*tokens]->u.op=ISEQUAL;
#if WK1DEBUG
            fprintf(se,"[==]");
#endif
            break;
          }
          /*}}}*/
          case 15: /* != */ /*{{{*/
          {
            t[*tokens]->u.op=NE;
#if WK1DEBUG
            fprintf(se,"[!=]");
#endif
            break;
          }
          /*}}}*/
          case 16: /* <= */ /*{{{*/
          {
            t[*tokens]->u.op=LE;
#if WK1DEBUG
            fprintf(se,"[<=]");
#endif
            break;
          }
          /*}}}*/
          case 17: /* >= */ /*{{{*/
          {
            t[*tokens]->u.op=GE;
#if WK1DEBUG
            fprintf(se,"[>=]");
#endif
            break;
          }
          /*}}}*/
          case 18: /* <  */ /*{{{*/
          {
            t[*tokens]->u.op=LT;
#if WK1DEBUG
            fprintf(se,"[<]");
#endif
            break;
          }
          /*}}}*/
          case 19: /* >  */ /*{{{*/
          {
            t[*tokens]->u.op=GT;
#if WK1DEBUG
            fprintf(se,"[>]");
#endif
            break;
          }
          /*}}}*/
          default: assert(0);
        }
        if (tokens) ++(*tokens);
        unrpn(s,offset,top-1,t,tokens);
      }
      else
      {
        low=unrpn(s,offset,top-1,(Token**)0,tokens);
        if (tokens) ++(*tokens);
        low=unrpn(s,offset,low-1,(Token**)0,tokens);
      }
      break;
    }
    /*}}}*/
    case 23: /* unary +                            */ /*{{{*/
    {
      low=unrpn(s,offset,top-1,t,tokens);
      break;
    }
    /*}}}*/
    case 80: /* sum                                */ /*{{{*/
    {
      int argc;
      
      argc=s[offset[top]+1];
#if WK1DEBUG
      if (t) fprintf(se,"[sum argc=%d]",argc);
#endif
      if (t)
      {
        if ((t[*tokens]=malloc(sizeof(Token)))==(Token*)0) return -1;
        t[*tokens]->type=OPERATOR;
        t[*tokens]->u.op=OP;
#if WK1DEBUG
        fprintf(se,"[(]");
#endif
      }
      if (tokens) ++(*tokens);
      low=sumup(s,offset,top-1,t,tokens,argc);
      if (t)
      {
        if ((t[*tokens]=malloc(sizeof(Token)))==(Token*)0) return -1;
        t[*tokens]->type=OPERATOR;
        t[*tokens]->u.op=CP;
#if WK1DEBUG
        fprintf(se,"[)]");
#endif
      }
      if (tokens) ++(*tokens);
      break;
    }
    /*}}}*/
    default: assert(0); low=-1;
  }
  return (low<0 ? -1 : low);
}
/*}}}*/

/* loadwk1 -- load WK1 file */ /*{{{*/
const char *loadwk1(Sheet *sheet, const char *name)
{
  /* variables */ /*{{{*/
  FILE *fp;
  const char *err;
  int head[4];
  char *body=(char*)0,*newbody;
  size_t bodymaxlen=0;
  size_t bodylen;
  int found_bof=0,found_eof=0;
  /*}}}*/

#if WK1DEBUG
  se=fopen("/dev/tty","w"); assert(se!=(FILE*)0); fprintf(se,"\r\n");
#endif
  if ((fp=fopen(name,"r"))==(FILE*)0) return strerror(errno);
  err=(const char*)0;
  while (1)
  {
    /* read header */ /*{{{*/
    if ((head[0]=getc(fp))==EOF) break;
    if ((head[1]=getc(fp))==EOF) { err=_("The record header appears to be truncated"); goto ouch; }
    if ((head[2]=getc(fp))==EOF) { err=_("The record header appears to be truncated"); goto ouch; }
    if ((head[3]=getc(fp))==EOF) { err=_("The record header appears to be truncated"); goto ouch; }
    bodylen=head[2]|(head[3]<<8);
    /*}}}*/
    /* read body */ /*{{{*/
    if (bodylen>bodymaxlen)
    {
      newbody=realloc(body,bodymaxlen=bodylen);
      if (newbody==(char*)0) { err=_("Out of memory"); goto ouch; }
      else body=newbody;
    }
    if (bodylen) if (fread(body,bodylen,1,fp)!=1) { err=_("The record body appears to be truncated"); goto ouch; }
    /*}}}*/
    /* process record */ /*{{{*/
#if WK1DEBUG
    fprintf(se,"bodylen %d, type %04x\r\n",bodylen,head[0]|(head[1]<<8));
#endif
    switch (head[0]|(head[1]<<8))
    {
      /* BOF           -- Beginning of file                */ /*{{{*/
      case 0x0:
      {
        if (bodylen!=2) { err=_("Invalid record body length"); goto ouch; }
        if (!found_bof)
        {
          freesheet(sheet,0);
          found_bof=it(body);
        }
        break;
      }
      /*}}}*/
      /* EOF           -- End of file                      */ /*{{{*/
      case 0x1:
      {
        if (bodylen!=0) { err=_("Invalid record body length"); goto ouch; }
        found_eof=1;
        break;
      }
      /*}}}*/
      /* CALCMODE      -- Calculation mode                 */ /*{{{*/
      case 0x2:
      {
        if (bodylen!=1) { err=_("Invalid record body length"); goto ouch; }
        /* (unsigned char)body[0] means: */
        /* 0 -- manual                   */
        /* 0xff -- automatic             */
        break;
      }
      /*}}}*/
      /* CALCORDER     -- Calculation order                */ /*{{{*/
      case 0x3:
      {
        if (bodylen!=1) { err=_("Invalid record body length"); goto ouch; }
        /* (unsigned char)body[0] means: */
        /*    0 -- natural               */
        /*    1 -- by column             */
        /* 0xff -- by row                */
        break;
      }
      /*}}}*/
      /* SPLIT         -- Split window type                */ /*{{{*/
      case 0x4:
      {
        if (bodylen!=1) { err=_("Invalid record body length"); goto ouch; }
        /* (unsigned)body[0] means: */
        /*    0: not split          */
        /*    1: vertical split     */
        /* 0xff: horizontal split   */
        break;
      }
      /*}}}*/
      /* SYNC          -- Split window sync                */ /*{{{*/
      case 0x5:
      {
        if (bodylen!=1) { err=_("Invalid record body length"); goto ouch; }
        /* (unsigned)body[0] means: */
        /*    0: not synchronized   */
        /* 0xff: synchronized       */
        break;
      }
      /*}}}*/
      /* RANGE         -- Active worksheet range           */ /*{{{*/
      case 0x6:
      {
        if (bodylen!=8) { err=_("Invalid record body length"); goto ouch; }
        resize(sheet,it(body+4),it(body+6),0);
        /* range is from &(it(body),it(body+2)) to &(it(body+4),it(body+6)) */
        break;
      }
      /*}}}*/
      /* WINDOW1       -- Window 1 record                  */ /*{{{*/
      case 0x7:
      {
        /* 31 is the specification, but Applix generates 32 while claiming to be Excel */
        if (bodylen!=31 && (found_bof!=EXCEL && bodylen!=32)) { err=_("Invalid record body length"); goto ouch; }
        break;
      }
      /*}}}*/
      /* COLW1         -- Column width, window 1           */ /*{{{*/
      case 0x8:
      {
        if (bodylen!=3) { err=_("Invalid record body length"); goto ouch; }
        break;
      }
      /*}}}*/
      /* WINTWO        -- Window 2 record                  */ /*{{{*/
      case 0x9:
      {
        if (bodylen!=31) { err=_("Invalid record body length"); goto ouch; }
        break;
      }
      /*}}}*/
      /* COLW2         -- Column width, window 2           */ /*{{{*/
      case 0xA:
      {
        if (bodylen!=3) { err=_("Invalid record body length"); goto ouch; }
        break;
      }
      /*}}}*/
      /* _("nN)ame")          -- Named range                      */ /*{{{*/
      case 0xB:
      {
        if (bodylen!=24) { err=_("Invalid record body length"); goto ouch; }
        break;
      }
      /*}}}*/
      /* BLANK         -- Blank cell                       */ /*{{{*/
      case 0xC:
      {
        if (bodylen!=5) { err=_("Invalid record body length"); goto ouch; }
        initcell(sheet,it(body+1),it(body+3),0);
        format((unsigned char)body[0],SHEET(sheet,it(body+1),it(body+3),0));
        break;
      }
      /*}}}*/
      /* INTEGER       -- Integer number cell              */ /*{{{*/
      case 0xD:
      {
        Token **t;

        assert(bodylen==7);
        initcell(sheet,it(body+1),it(body+3),0);
        t=malloc(2*sizeof(Token*));
        t[0]=malloc(sizeof(Token));
        t[1]=(Token*)0;
        t[0]->type=INT;
        t[0]->u.integer=it(body+5);
        putcont(sheet,it(body+1),it(body+3),0,t,0);
        format((unsigned char)body[0],SHEET(sheet,it(body+1),it(body+3),0));
        break;
      }
      /*}}}*/
      /* NUMBER        -- Floating point number            */ /*{{{*/
      case 0xE:
      {
        Token **t;

        assert(bodylen==13);
        initcell(sheet,it(body+1),it(body+3),0);
        t=malloc(2*sizeof(Token*));
        t[0]=malloc(sizeof(Token));
        t[1]=(Token*)0;
        t[0]->type=FLOAT;
        t[0]->u.flt=dbl((unsigned char*)body+5);
        putcont(sheet,it(body+1),it(body+3),0,t,0);
        format((unsigned char)body[0],SHEET(sheet,it(body+1),it(body+3),0));
        break;
      }
      /*}}}*/
      /* _("lL)abel")         -- Label cell                       */ /*{{{*/
      case 0xF:
      {
        Token **t;

        assert(bodylen>=6 && bodylen<=245);
        initcell(sheet,it(body+1),it(body+3),0);
        t=malloc(2*sizeof(Token*));
        t[0]=malloc(sizeof(Token));
        t[1]=(Token*)0;
        t[0]->type=STRING;
        t[0]->u.string=mystrmalloc(body+6);
        putcont(sheet,it(body+1),it(body+3),0,t,0);
        format((unsigned char)body[0],SHEET(sheet,it(body+1),it(body+3),0));
        break;
      }
      /*}}}*/
      /* FORMULA       -- Formula cell                     */ /*{{{*/
      case 0x10:
      {
        int i,j,size;
        int *offset;
        int tokens;
        Token **t;

        assert(bodylen>15);
        if ((offset=malloc(it(body+13)*sizeof(int)))==0) { err=_("Out of memory"); goto ouch; }
#if WK1DEBUG
        fprintf(se,"FORMULA: &(%d,%d)=",it(body+1),it(body+3));
#endif
        for (i=15,size=it(body+13)+15,j=0; i<size; ++i,++j)
        {
          offset[j]=i;
          switch(body[i])
          {
            case  0: /* double constant */ /*{{{*/
            {
              i+=8;
              break;
            }
            /*}}}*/
            case  1: /* variable */ /*{{{*/
            {
              i+=4;
              break;
            }
            /*}}}*/
            case  2: /* range */ /*{{{*/
            {
              i+=8;
              break;
            }
            /*}}}*/
            case  5: /* int constant */ /*{{{*/
            {
              i+=2;
              break;
            }
            /*}}}*/
            case 80: /* sum */ /*{{{*/
            {
              ++i;
              break;
            }
            /*}}}*/
          }
        }
#if WK1DEBUG
        fprintf(se,", value %f",dbl((unsigned char*)body+5));
#endif
        format((unsigned char)body[0],SHEET(sheet,it(body+1),it(body+3),0));
#if WK1DEBUG
        fprintf(se,"\r\n");
#endif
        tokens=0; unrpn(body,offset,j-1,(Token**)0,&tokens);
        if ((t=malloc(tokens*sizeof(Token*)))==(Token**)0)
        {
          free(offset);
          err=_("Out of memory");
          goto ouch;
        }
        for (; tokens; --tokens) t[tokens-1]=(Token*)0;
        if (unrpn(body,offset,j-1,t,&tokens)==-1)
        {
          err=_("Out of memory");
          tvecfree(t);
          goto ouch;
        }
        else
        {
#if WK1DEBUG
        fprintf(se,"[<-- %d tokens]\r\n",tokens);
#endif
        free(offset);
        initcell(sheet,it(body+1),it(body+3),0);
        SHEET(sheet,it(body+1),it(body+3),0)->value.type=FLOAT;
        SHEET(sheet,it(body+1),it(body+3),0)->value.u.flt=dbl((unsigned char*)body+5);
        putcont(sheet,it(body+1),it(body+3),0,t,0);
        }
        break;
      }
      /*}}}*/
      /* TABLE         -- Data table range                 */ /*{{{*/
      case 0x18: assert(bodylen==25); break;
      /*}}}*/
      /* ORANGE/QRANGE -- Query range                      */ /*{{{*/
      case 0x19: assert(bodylen==25); break;
      /*}}}*/
      /* PRANGE        -- Print range                      */ /*{{{*/
      case 0x1A: assert(bodylen==8); break;
      /*}}}*/
      /* SRANGE        -- Sort range                       */ /*{{{*/
      case 0x1B: assert(bodylen==8); break;
      /*}}}*/
      /* FRANGE        -- Fill range                       */ /*{{{*/
      case 0x1C: assert(bodylen==8); break;
      /*}}}*/
      /* KRANGE1       -- Primary sort key range           */ /*{{{*/
      case 0x1D: assert(bodylen==9); break;
      /*}}}*/
      /* HRANGE        -- Distribution range               */ /*{{{*/
      case 0x20: assert(bodylen==16); break;
      /*}}}*/
      /* KRANGE2       -- Secondary sort key range         */ /*{{{*/
      case 0x23: assert(bodylen==9); break;
      /*}}}*/
      /* PROTEC        -- Global protection                */ /*{{{*/
      case 0x24:
      {
        if (bodylen!=1) { err=_("Invalid record body length"); goto ouch; }
        /* (unsigned)body[0] means: */
        /*    0: off                */
        /* 0xff: on                 */
        break;
      }
      /*}}}*/
      /* FOOTER        -- Print footer                     */ /*{{{*/
      case 0x25:
      {
        if (body[bodylen-1]!='\0' || bodylen<1 || bodylen>243) { err=_("Invalid record body length"); goto ouch; }
        break;
      }
      /*}}}*/
      /* HEADER        -- Print header                     */ /*{{{*/
      case 0x26:
      {
        if (body[bodylen-1]!='\0' || bodylen<1 || bodylen>243) { err=_("Invalid record body length"); goto ouch; }
        break;
      }
      /*}}}*/
      /* SETUP         -- Print setup                      */ /*{{{*/
      case 0x27: assert(bodylen==40); break;
      /*}}}*/
      /* MARGINS       -- Print margins code               */ /*{{{*/
      case 0x28: assert(bodylen==10); break;
      /*}}}*/
      /* LABELFMT      -- Label alignment                  */ /*{{{*/
      case 0x29:
      {
        if (bodylen!=1) { err=_("Invalid record body length"); goto ouch; }
        /* (unsigned char)body[0] means: */
        /* 0x22: right aligned labels    */
        /* 0x27: left aligned labels     */
        /* 0x5e: centered labels         */
        break;
      }
      /*}}}*/
      /* TITLES        -- Print borders                    */ /*{{{*/
      case 0x2A: assert(bodylen==16); break;
      /*}}}*/
      /* GRAPH         -- Current graph settings           */ /*{{{*/
      case 0x2D:
      {
        /* The specification says bodylen is 437, Excel 5 says it are */
        /* 443 bytes.  We better silently ignore this.                */
        break;
      }
      /*}}}*/
      /* NGRAPH        -- Named graph settings             */ /*{{{*/
      case 0x2E: assert(bodylen==453); break;
      /*}}}*/
      /* CALCCOUNT     -- Iteration count                  */ /*{{{*/
      case 0x2F:
      {
        if (bodylen!=1) { err=_("Invalid record body length"); goto ouch; }
        /* Do up to %d Iterations */
        break;
      }
      /*}}}*/
      /* UNFORMATTED   -- Formatted/unformatted print      */ /*{{{*/
      case 0x30: assert(bodylen==1); break;
      /*}}}*/
      /* CURSORW12     -- Cursor location                  */ /*{{{*/
      case 0x31:
      {
        if (bodylen!=1) { err=_("Invalid record body length"); goto ouch; }
        /* (unsigned)body[0] means cursor in window: */
        /* 1: 1                                      */
        /* 2: 2                                      */
        break;
      }
      /*}}}*/
      /* WINDOW        -- Symphony window settings         */ /*{{{*/
      case 0x32: assert(bodylen==144); break;
      /*}}}*/
      /* STRING        -- Value of string formula          */ /*{{{*/
      case 0x33:
      {
        Token **t;

        assert(bodylen>=6 && bodylen<=245);
        initcell(sheet,it(body+1),it(body+3),0);
        t=malloc(2*sizeof(Token*));
        t[0]=malloc(sizeof(Token));
        t[1]=(Token*)0;
        t[0]->type=STRING;
        t[0]->u.string=mystrmalloc(body+5);
        putcont(sheet,it(body+1),it(body+3),0,t,0);
        format((unsigned char)body[0],SHEET(sheet,it(body+1),it(body+3),0));
        break;
      }
      /*}}}*/
      /* PASSWORD      -- File lockout (CHKSUM)            */ /*{{{*/
      case 0x37: assert(bodylen==4); break;
      /*}}}*/
      /* LOCKED        -- Lock flag                        */ /*{{{*/
      case 0x38: assert(bodylen==1); break;
      /*}}}*/
      /* QUERY         -- Symphony query settings          */ /*{{{*/
      case 0x3C: assert(bodylen==127); break;
      /*}}}*/
      /* QUERYNAME     -- Query name                       */ /*{{{*/
      case 0x3D: assert(bodylen==16); break;
      /*}}}*/
      /* PRINT         -- Symphony print record            */ /*{{{*/
      case 0x3E: assert(bodylen==679); break;
      /*}}}*/
      /* PRINTNAME     -- Print record name                */ /*{{{*/
      case 0x3F: assert(bodylen==16); break;
      /*}}}*/
      /* GRAPH2        -- Symphony graph record            */ /*{{{*/
      case 0x40: assert(bodylen==499); break;
      /*}}}*/
      /* GRAPHNAME     -- Graph record name                */ /*{{{*/
      case 0x41: assert(bodylen==16); break;
      /*}}}*/
      /* ZOOM          -- Orig coordinates expanded window */ /*{{{*/
      case 0x42: assert(bodylen==9); break;
      /*}}}*/
      /* SYMSPLIT      -- No. of split windows             */ /*{{{*/
      case 0x43: assert(bodylen==2); break;
      /*}}}*/
      /* NSROWS        -- No. of screen rows               */ /*{{{*/
      case 0x44: assert(bodylen==2); break;
      /*}}}*/
      /* NSCOLS        -- No. of screen columns            */ /*{{{*/
      case 0x45: assert(bodylen==2); break;
      /*}}}*/
      /* RULER         -- Named ruler range                */ /*{{{*/
      case 0x46: assert(bodylen==25); break;
      /*}}}*/
      /* NNAME         -- Named sheet range                */ /*{{{*/
      case 0x47: assert(bodylen==25); break;
      /*}}}*/
      /* ACOMM         -- Autoload.comm code               */ /*{{{*/
      case 0x48: assert(bodylen==65); break;
      /*}}}*/
      /* AMACRO        -- Autoexecute macro address        */ /*{{{*/
      case 0x49: assert(bodylen==8); break;
      /*}}}*/
      /* PARSE         -- Query parse information          */ /*{{{*/
      case 0x4A: assert(bodylen==16); break;
      /*}}}*/
    }
    /*}}}*/
    if (!found_bof) { err=_("This is not a WK1 file"); goto ouch; }
  }
  if (!found_eof) err=_("File truncated");
  ouch:
  if (body) free(body);
  if (fclose(fp)==EOF && err==(const char*)0) err=strerror(errno);
  sheet->changed=0;
  cachelabels(sheet);
  forceupdate(sheet);
  return err;
}
/*}}}*/
