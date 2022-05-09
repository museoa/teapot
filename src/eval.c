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
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "default.h"
#include "eval.h"
#include "func.h"
#include "main.h"
#include "misc.h"
#include "scanner.h"
#include "sheet.h"
/*}}}*/

/* tcopy     -- return copy of token */ /*{{{*/
Token tcopy(Token n)
{
  /* result */ /*{{{*/
  Token result;
  /*}}}*/

  result=n;
  if (result.type==STRING) result.u.string=strcpy(malloc(strlen(n.u.string)+1),n.u.string);
  else if (result.type==EEK) result.u.err=strcpy(malloc(strlen(n.u.err)+1),n.u.err);  
  else if (result.type==LIDENT) result.u.lident=strcpy(malloc(strlen(n.u.lident)+1),n.u.lident);
  return result;
}
/*}}}*/
/* tfree     -- free dynamic data of token */ /*{{{*/
void tfree(Token *n)
{
  if (n->type==STRING)
  {
    free(n->u.string);
    n->u.string=(char*)0;
  }
  else if (n->type==EEK)
  {
    free(n->u.err);
    n->u.err=(char*)0;
  }
  else if (n->type==LIDENT)
  {
    free(n->u.lident);
    n->u.lident=(char*)0;
  }
}
/*}}}*/
/* tvecfree  -- free a vector of pointer to tokens entirely */ /*{{{*/
void tvecfree(Token **tvec)
{
  if (tvec!=(Token**)0)
  {
    /* variables */ /*{{{*/
    Token **t;
    /*}}}*/
      
    for (t=tvec; *t!=(Token*)0; ++t)
    {
      tfree(*t);
      free(*t);
    }
    free(tvec);
  }
}      
/*}}}*/
/* tadd      -- + operator */ /*{{{*/
Token tadd(Token l, Token r)
{
  /* variables */ /*{{{*/
  Token result;
  const char *msg;
  /*}}}*/

  if (l.type==EEK)
  /* return left error */ /*{{{*/
  return tcopy(l);
  /*}}}*/
  else if (r.type==EEK)
  /* return right error */ /*{{{*/
  return tcopy(r);
  /*}}}*/
  else if (l.type==INT && r.type==INT)
  /* result is int sum of two ints */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=l.u.integer+r.u.integer;
  }
  /*}}}*/
  else if (l.type==STRING && r.type==STRING)
  /* result is concatenated strings */ /*{{{*/
  {
    result.type=STRING;
    result.u.string=malloc(strlen(l.u.string)+strlen(r.u.string)+1);
    (void)strcpy(result.u.string,l.u.string);
    (void)strcat(result.u.string,r.u.string);
  }
  /*}}}*/
  else if (l.type==EMPTY && (r.type==INT || r.type==STRING || r.type==FLOAT || r.type==EMPTY)) 
  /* return right argument */ /*{{{*/
  return tcopy(r);
  /*}}}*/
  else if ((l.type==INT || l.type==STRING || l.type==FLOAT) && r.type==EMPTY) 
  /* return left argument */ /*{{{*/
  return tcopy(l);
  /*}}}*/
  else if (l.type==INT && r.type==FLOAT)
  /* result is float sum of int and float */ /*{{{*/
  {
    result.type=FLOAT;
    result.u.flt=((double)l.u.integer)+r.u.flt;
  }
  /*}}}*/
  else if (l.type==FLOAT && r.type==INT)
  /* result is float sum of float and int */ /*{{{*/
  {
    result.type=FLOAT;
    result.u.flt=l.u.flt+((double)r.u.integer);
  }
  /*}}}*/
  else if (l.type==FLOAT && r.type==FLOAT)
  /* result is float sum of float and float */ /*{{{*/
  {
    result.type=FLOAT;
    result.u.flt=l.u.flt+r.u.flt;
  }
  /*}}}*/
  else if (l.type==EMPTY && r.type==EMPTY)
  /* result is emty */ /*{{{*/
  {
    result.type=EMPTY;
  }
  /*}}}*/
  else
  /* result is type error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=strcpy(malloc(strlen(_("wrong types for + operator"))+1),_("wrong types for + operator"));
  }
  /*}}}*/
  if (result.type==FLOAT && (msg=dblfinite(result.u.flt))!=(const char*)0)
  /* result is error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=malloc(strlen(msg)+4);
    (void)strcpy(result.u.err,"+: ");
    (void)strcat(result.u.err,msg);
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* tsub      -- binary - operator */ /*{{{*/
Token tsub(Token l, Token r)
{
  /* variables */ /*{{{*/
  Token result;
  const char *msg;
  /*}}}*/

  if (l.type==EEK)
  /* return left error */ /*{{{*/
  return tcopy(l);
  /*}}}*/
  else if (r.type==EEK)
  /* return right error */ /*{{{*/
  return tcopy(r);
  /*}}}*/
  else if (l.type==INT && r.type==INT)
  /* result is int difference between left int and right int */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=l.u.integer-r.u.integer;
  }
  /*}}}*/
  else if (l.type==FLOAT && r.type==FLOAT)
  /* result is float difference between left float and right float */ /*{{{*/
  {
    result.type=FLOAT;
    result.u.flt=l.u.flt-r.u.flt;
  }
  /*}}}*/
  else if (l.type==EMPTY)
  /* return negated right argument */ /*{{{*/
  return tneg(r);
  /*}}}*/
  else if ((l.type==INT || l.type==FLOAT) && r.type==EMPTY)
  /* return left argument */ /*{{{*/
  return tcopy(l);
  /*}}}*/
  else if (l.type==INT && r.type==FLOAT)
  /* result is float difference of left integer and right float */ /*{{{*/
  {
    result.type=FLOAT;
    result.u.flt=((double)l.u.integer)-r.u.flt;
  }
  /*}}}*/
  else if (l.type==FLOAT && r.type==INT)
  /* result is float difference between left float and right integer */ /*{{{*/
  {
    result.type=FLOAT;
    result.u.flt=l.u.flt-((double)r.u.integer);
  }
  /*}}}*/
  else
  /* result is difference type error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=strcpy(malloc(strlen(_("wrong types for - operator"))+1),_("wrong types for - operator"));
  }
  /*}}}*/
  if (result.type==FLOAT && (msg=dblfinite(result.u.flt))!=(const char*)0)
  /* result is error  */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=malloc(strlen(msg)+4);
    (void)strcpy(result.u.err,"-: ");
    (void)strcat(result.u.err,msg);
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* tdiv      -- / operator */ /*{{{*/
Token tdiv(Token l, Token r)
{
  /* variables */ /*{{{*/
  Token result;
  const char *msg;
  /*}}}*/

  if (l.type==EEK)
  /* return left error */ /*{{{*/
  return tcopy(l);
  /*}}}*/
  else if (r.type==EEK)
  /* return right error */ /*{{{*/
  return tcopy(r);
  /*}}}*/
  else if ((r.type==INT && r.u.integer==0) || (r.type==FLOAT && r.u.flt==0.0) || (r.type==EMPTY))
  /* result is division by 0 error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=strcpy(malloc(strlen(_("division by 0"))+1),_("division by 0"));
  }
  /*}}}*/
  else if (l.type==INT && r.type==INT)
  /* result is quotient of left int and right int */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=l.u.integer/r.u.integer;
  }
  /*}}}*/
  else if (l.type==FLOAT && r.type==FLOAT)
  /* result is quotient of left float and right float */ /*{{{*/
  {
    result.type=FLOAT;
    result.u.flt=l.u.flt/r.u.flt;
  }
  /*}}}*/
  else if (l.type==EMPTY && r.type==INT)
  /* result is 0 */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=0;
  }
  /*}}}*/
  else if (l.type==EMPTY && r.type==FLOAT)
  /* result is 0.0 */ /*{{{*/
  {
    result.type=FLOAT;
    result.u.flt=0.0;
  }  
  /*}}}*/
  else if (l.type==INT && r.type==FLOAT)
  /* result is float quotient of left int and right float */ /*{{{*/
  {
    result.type=FLOAT;
    result.u.flt=((double)l.u.integer)/r.u.flt;
  }
  /*}}}*/
  else if (l.type==FLOAT && r.type==INT)
  /* result is float quotient of left float and right int */ /*{{{*/
  {
    result.type=FLOAT;
    result.u.flt=l.u.flt/((double)r.u.integer);
  }
  /*}}}*/
  else
  /* result is quotient type error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=strcpy(malloc(strlen(_("wrong types for / operator"))+1),_("wrong types for / operator"));
  }
  /*}}}*/
  if (result.type==FLOAT && (msg=dblfinite(result.u.flt))!=(const char*)0)
  /* result is error  */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=malloc(strlen(msg)+4);
    (void)strcpy(result.u.err,"/: ");
    (void)strcat(result.u.err,msg);
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* tmod      -- % operator */ /*{{{*/
Token tmod(Token l, Token r)
{
  /* variables */ /*{{{*/
  Token result;
  const char *msg;
  /*}}}*/

  if (l.type==EEK) /* return left error */ /*{{{*/
  return tcopy(l);
  /*}}}*/
  else if (r.type==EEK) /* return right error */ /*{{{*/
  return tcopy(r);
  /*}}}*/
  else if ((r.type==INT && r.u.integer==0) || (r.type==FLOAT && r.u.flt==0.0) || (r.type==EMPTY)) /* result is modulo 0 error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=strcpy(malloc(strlen(_("modulo 0"))+1),_("modulo 0"));
  }
  /*}}}*/
  else if (l.type==INT && r.type==INT) /* result is remainder of left int and right int */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=l.u.integer%r.u.integer;
  }
  /*}}}*/
  else if (l.type==FLOAT && r.type==FLOAT) /* result is remainder of left float and right float */ /*{{{*/
  {
    result.type=FLOAT;
    result.u.flt=fmod(l.u.flt,r.u.flt);
  }
  /*}}}*/
  else if (l.type==EMPTY && r.type==INT) /* result is 0 */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=0;
  }
  /*}}}*/
  else if (l.type==EMPTY && r.type==FLOAT) /* result is 0.0 */ /*{{{*/
  {
    result.type=FLOAT;
    result.u.flt=0.0;
  }  
  /*}}}*/
  else if (l.type==INT && r.type==FLOAT) /* result is float remainder of left int and right float */ /*{{{*/
  {
    result.type=FLOAT;
    result.u.flt=fmod((double)l.u.integer,r.u.flt);
  }
  /*}}}*/
  else if (l.type==FLOAT && r.type==INT) /* result is float remainder of left float and right int */ /*{{{*/
  {
    result.type=FLOAT;
    result.u.flt=fmod(l.u.flt,(double)r.u.integer);
  }
  /*}}}*/
  else /* result is remainder type error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=strcpy(malloc(strlen(_("wrong types for % operator"))+1),_("wrong types for % operator"));
  }
  /*}}}*/
  if (result.type==FLOAT && (msg=dblfinite(result.u.flt))!=(const char*)0) /* result is error  */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=malloc(strlen(msg)+4);
    (void)strcpy(result.u.err,"%: ");
    (void)strcat(result.u.err,msg);
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* tmul      -- * operator */ /*{{{*/
Token tmul(Token l, Token r)
{
  /* variables */ /*{{{*/
  Token result;
  const char *msg;
  /*}}}*/

  if (l.type==EEK)
  /* return left error */ /*{{{*/
  result=tcopy(l);
  /*}}}*/
  else if (r.type==EEK)
  /* return right error */ /*{{{*/
  result=tcopy(r);
  /*}}}*/
  else if (l.type==INT && r.type==INT)
  /* result is int product of left int and right int */ /*{{{*/
  {
    result=l;
    result.u.integer=l.u.integer*r.u.integer;
  }
  /*}}}*/
  else if (l.type==FLOAT && r.type==FLOAT)
  /* result is float product of left float and right float */ /*{{{*/
  {
    result.type=FLOAT;
    result.u.flt=l.u.flt*r.u.flt;
  }
  /*}}}*/
  else if ((l.type==EMPTY && r.type==INT) || (l.type==INT && r.type==EMPTY))
  /* result is 0 */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=0;
  }
  /*}}}*/
  else if ((l.type==EMPTY && r.type==FLOAT) || (l.type==FLOAT && r.type==EMPTY))
  /* result is 0.0 */ /*{{{*/
  {
    result.type=FLOAT;
    result.u.flt=0.0;
  }
  /*}}}*/
  else if (l.type==INT && r.type==FLOAT)
  /* result is float product of left int and right float */ /*{{{*/
  {
    result.type=FLOAT;
    result.u.flt=((double)l.u.integer)*r.u.flt;
  }
  /*}}}*/
  else if (l.type==FLOAT && r.type==INT)
  /* result is float product of left float and right int */ /*{{{*/
  {
    result.type=FLOAT;
    result.u.flt=l.u.flt*((double)r.u.integer);
  }
  /*}}}*/
  else if (l.type==EMPTY && r.type==EMPTY)
  /* result is empty */ /*{{{*/
  {
    result.type=EMPTY;
  }
  /*}}}*/
  else
  /* result is product type error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=strcpy(malloc(strlen(_("wrong types for * operator"))+1),_("wrong types for * operator"));
  }
  /*}}}*/
  if (result.type==FLOAT && (msg=dblfinite(result.u.flt))!=(const char*)0)
  /* result is error  */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=malloc(strlen(msg)+4);
    (void)strcpy(result.u.err,"*: ");
    (void)strcat(result.u.err,msg);
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* tneg      -- monadic - operator */ /*{{{*/
Token tneg(Token x)
{
  /* variables */ /*{{{*/
  Token result;
  /*}}}*/

  if (x.type==EEK)
  /* return error */ /*{{{*/
  return tcopy(x);
  /*}}}*/
  else if (x.type==INT)
  /* result is negated int argument */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=-x.u.integer;
  }
  /*}}}*/
  else if (x.type==FLOAT)
  /* result is negated float argument */ /*{{{*/
  {
    result.type=FLOAT;
    result.u.flt=-x.u.flt;
  }
  /*}}}*/
  else if (x.type==EMPTY)
  /* result is argument itself */ /*{{{*/
  {
    result=tcopy(x);
  }
  /*}}}*/
  else 
  /* result is negation error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=strcpy(malloc(strlen(_("wrong type for - operator"))+1),_("wrong type for - operator"));
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* tpow      -- ^ operator */ /*{{{*/
Token tpow(Token l, Token r)
{
  /* variables */ /*{{{*/
  Token result;
  const char *msg;
  /*}}}*/

  if (l.type==EEK) /* return left error */ /*{{{*/
  return tcopy(l);
  /*}}}*/
  else if (r.type==EEK) /* return right error */ /*{{{*/
  return tcopy(r);
  /*}}}*/
  else if ((l.type==INT || l.type==FLOAT || l.type==EMPTY) && (r.type==INT || r.type==FLOAT || l.type==EMPTY)) /* do the real work */ /*{{{*/
  {
    if ((l.type==INT || l.type==EMPTY) && ((r.type==INT && r.u.integer>=0) || r.type==EMPTY))
    /* int^int, return int or error if 0^0 */ /*{{{*/
    {
      long x,y;

      if (l.type==EMPTY) x=0;
      else x=l.u.integer;
      if (r.type==EMPTY) y=0;
      else y=r.u.integer;
      if (x==0 && y==0)
      {
        result.type=EEK;
        result.u.err=strcpy(malloc(strlen(_("0^0 is not defined"))+1),_("0^0 is not defined"));
      }
      else
      {
        long i;

        result.type=INT;
        if (x==0) result.u.integer=0;
        else if (y==0) result.u.integer=1;
        else for (result.u.integer=x,i=1; i<y; ++i) result.u.integer*=x;
      }
    }
    /*}}}*/
    else
    /* float^float */ /*{{{*/
    {
      double x=0.0,y=0.0;

      switch (l.type)
      {
        case INT: x=(double)l.u.integer; break;
        case FLOAT: x=l.u.flt; break;
        case EMPTY: x=0.0; break;
        default: assert(0);
      }
      switch (r.type)
      {
        case INT: y=(double)r.u.integer; break;
        case FLOAT: y=r.u.flt; break;
        case EMPTY: y=0.0; break;
        default: assert(0);
      }
      result.type=FLOAT;
      errno=0; /* there is no portable EOK :( */
      result.u.flt=pow(x,y);
      switch (errno)
      {
        case 0: result.type=FLOAT; break;
        case ERANGE:
        case EDOM: result.type=EEK; result.u.err=strcpy(malloc(strlen(_("^ caused a domain error"))+1),_("^ caused a domain error")); break;
        default: assert(0);
      }
    }
    /*}}}*/
  }
  /*}}}*/
  else /* result is type error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=strcpy(malloc(strlen(_("wrong types for ^ operator"))+1),_("wrong types for ^ operator"));
  }
  /*}}}*/
  if (result.type==FLOAT && (msg=dblfinite(result.u.flt))!=(const char*)0) /* result is error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=malloc(strlen(msg)+4);
    (void)strcpy(result.u.err,"^: ");
    (void)strcat(result.u.err,msg);
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* tfuncall  -- function operator */ /*{{{*/
Token tfuncall(Token *ident, int argc, Token argv[])
{
  return tfunc[ident->u.fident].func(argc, argv);
}
/*}}}*/
/* tlt       -- < operator */ /*{{{*/
Token tlt(Token l, Token r)
{
  /* variables */ /*{{{*/
  Token result;
  static char empty[]="";
  /*}}}*/

  if (l.type==EEK) /* return left error argument */ /*{{{*/
  return tcopy(l);
  /*}}}*/
  if (r.type==EEK) /* return right error argument */ /*{{{*/
  return tcopy(r);
  /*}}}*/
  if (l.type==EMPTY) /* try to assign 0 element of r.type */ /*{{{*/
  {
    l.type=r.type;
    switch (r.type)
    {
      case INT: l.u.integer=0; break;
      case FLOAT: l.u.flt=0.0; break;
      case STRING: l.u.string=empty; break;
      default: ;
    }
  }
  /*}}}*/
  if (r.type==EMPTY) /* try to assign 0 element of l.type */ /*{{{*/
  {
    r.type=l.type;
    switch (l.type)
    {
      case INT: r.u.integer=0; break;
      case FLOAT: r.u.flt=0.0; break;
      case STRING: r.u.string=empty; break;
      default: ;
    }  
  }
  /*}}}*/
  if (l.type==INT && r.type==INT) /* return left int < right int */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=l.u.integer<r.u.integer;
  }
  /*}}}*/
  else if (l.type==STRING && r.type==STRING) /* return left string < right string */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=(strcmp(l.u.string,r.u.string)<0);
  }
  /*}}}*/
  else if (l.type==FLOAT && r.type==FLOAT) /* return left float < right float */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=l.u.flt<r.u.flt;
  }
  /*}}}*/
  else if (l.type==FLOAT && r.type==INT) /* return left float < right float */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=l.u.flt<((double)r.u.integer);
  }  
  /*}}}*/
  else if (l.type==INT && r.type==FLOAT) /* return left int < right float */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=((double)l.u.integer)<r.u.flt;
  }    
  /*}}}*/
  else /* return < type error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=strcpy(malloc(strlen(_("type mismatch for relational operator"))+1),_("type mismatch for relational operator"));
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* tle       -- <= operator */ /*{{{*/
Token tle(Token l, Token r)
{
  /* variables */ /*{{{*/
  Token result;
  static char empty[]="";
  /*}}}*/

  if (l.type==EEK) /* return left error argument */ /*{{{*/
  return tcopy(l);
  /*}}}*/
  if (r.type==EEK) /* return right error argument */ /*{{{*/
  return tcopy(r);
  /*}}}*/
  if (l.type==EMPTY) /* try to assign 0 element of r.type */ /*{{{*/
  {
    l.type=r.type;
    switch (r.type)
    {
      case INT: l.u.integer=0; break;
      case FLOAT: l.u.flt=0.0; break;
      case STRING: l.u.string=empty; break;
      default: ;
    }
  }
  /*}}}*/
  if (r.type==EMPTY) /* try to assign 0 element of l.type */ /*{{{*/
  {
    r.type=l.type;
    switch (l.type)
    {
      case INT: r.u.integer=0; break;
      case FLOAT: r.u.flt=0.0; break;
      case STRING: r.u.string=empty; break;
      default: ;
    }  
  }
  /*}}}*/
  if (l.type==INT && r.type==INT) /* result is left int <= right int */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=l.u.integer<=r.u.integer;
  }
  /*}}}*/
  else if (l.type==STRING && r.type==STRING) /* result is left string <= right string */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=(strcmp(l.u.string,r.u.string)<=0);
  }
  /*}}}*/
  else if (l.type==FLOAT && r.type==FLOAT) /* result is left float <= right float */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=l.u.flt<=r.u.flt;
  }
  /*}}}*/
  else if (l.type==FLOAT && r.type==INT) /* result is left float <= (double)right int */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=l.u.flt<=((double)r.u.integer);
  }  
  /*}}}*/
  else if (l.type==INT && r.type==FLOAT) /* result is (double)left int <= right float */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=((double)l.u.integer)<=r.u.flt;
  }    
  /*}}}*/
  else if (l.type==EMPTY && r.type==EMPTY) /* result is 1 */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=1;
  }
  /*}}}*/
  else /* result is <= type error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=strcpy(malloc(strlen(_("type mismatch for relational operator"))+1),_("type mismatch for relational operator"));
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* tge       -- >= operator */ /*{{{*/
Token tge(Token l, Token r)
{
  /* variables */ /*{{{*/
  Token result;
  static char empty[]="";
  /*}}}*/

  if (l.type==EEK) /* return left error argument */ /*{{{*/
  return tcopy(l);
  /*}}}*/
  if (r.type==EEK) /* return right error argument */ /*{{{*/
  return tcopy(r);
  /*}}}*/
  if (l.type==EMPTY) /* try to assign 0 element of r.type */ /*{{{*/
  {
    l.type=r.type;
    switch (r.type)
    {
      case INT: l.u.integer=0; break;
      case FLOAT: l.u.flt=0.0; break;
      case STRING: l.u.string=empty; break;
      default: ;
    }
  }
  /*}}}*/
  if (r.type==EMPTY) /* try to assign 0 element of l.type */ /*{{{*/
  {
    r.type=l.type;
    switch (l.type)
    {
      case INT: r.u.integer=0; break;
      case FLOAT: r.u.flt=0.0; break;
      case STRING: r.u.string=empty; break;
      default: ;
    }  
  }
  /*}}}*/
  if (l.type==INT && r.type==INT) /* result is left int >= right int */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=l.u.integer>=r.u.integer;
  }
  /*}}}*/
  else if (l.type==STRING && r.type==STRING) /* return left string >= right string */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=(strcmp(l.u.string,r.u.string)>=0);
  }
  /*}}}*/
  else if (l.type==FLOAT && r.type==FLOAT) /* return left float >= right float */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=l.u.flt>=r.u.flt;
  }
  /*}}}*/
  else if (l.type==FLOAT && r.type==INT) /* return left float >= (double) right int */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=l.u.flt>=((double)r.u.integer);
  }  
  /*}}}*/
  else if (l.type==INT && r.type==FLOAT) /* return (double) left int >= right float */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=((double)l.u.integer)>=r.u.flt;
  }    
  /*}}}*/
  else /* return >= type error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=strcpy(malloc(strlen(_("type mismatch for relational operator"))+1),_("type mismatch for relational operator"));
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* tgt       -- <= operator */ /*{{{*/
Token tgt(Token l, Token r)
{
  /* variables */ /*{{{*/
  Token result;
  static char empty[]="";
  /*}}}*/
  
  if (l.type==EEK) /* return left error argument */ /*{{{*/
  return tcopy(l);
  /*}}}*/
  if (r.type==EEK) /* return right error argument */ /*{{{*/
  return tcopy(r);
  /*}}}*/
  if (l.type==EMPTY) /* try to assign 0 element of r.type */ /*{{{*/
  {
    l.type=r.type;
    switch (r.type)
    {
      case INT: l.u.integer=0; break;
      case FLOAT: l.u.flt=0.0; break;
      case STRING: l.u.string=empty; break;
      default: ;
    }
  }
  /*}}}*/
  if (r.type==EMPTY) /* try to assign 0 element of l.type */ /*{{{*/
  {
    r.type=l.type;
    switch (l.type)
    {
      case INT: r.u.integer=0; break;
      case FLOAT: r.u.flt=0.0; break;
      case STRING: r.u.string=empty; break;
      default: ;
    }  
  }
  /*}}}*/
  if (l.type==INT && r.type==INT) /* result is left int > right int */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=l.u.integer>r.u.integer;
  }
  /*}}}*/
  else if (l.type==STRING && r.type==STRING) /* result is left string > right string */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=(strcmp(l.u.string,r.u.string)>0);
  }
  /*}}}*/
  else if (l.type==FLOAT && r.type==FLOAT) /* result is left float > right float */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=l.u.flt>r.u.flt;
  }
  /*}}}*/
  else if (l.type==FLOAT && r.type==INT) /* result is left float > (double) right int */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=l.u.flt>((double)r.u.integer);
  }  
  /*}}}*/
  else if (l.type==INT && r.type==FLOAT) /* result is left int > right float */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=((double)l.u.integer)>r.u.flt;
  }    
  /*}}}*/
  else /* result is relation op type error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=mystrmalloc(_("type mismatch for relational operator"));
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* teq       -- == operator */ /*{{{*/
Token teq(Token l, Token r)
{
  /* variables */ /*{{{*/
  Token result;
  static char empty[]="";
  /*}}}*/
  
  if (l.type==EEK) return tcopy(l);
  else if (r.type==EEK) return tcopy(r);
  if (l.type==EMPTY)
  /* try to assign 0 element of r.type */ /*{{{*/
  {
    l.type=r.type;
    switch (r.type)
    {
      case INT: l.u.integer=0; break;
      case FLOAT: l.u.flt=0.0; break;
      case STRING: l.u.string=empty; break;
      default: ;
    }
  }
  /*}}}*/
  if (r.type==EMPTY)
  /* try to assign 0 element of l.type */ /*{{{*/
  {
    r.type=l.type;
    switch (l.type)
    {
      case INT: r.u.integer=0; break;
      case FLOAT: r.u.flt=0.0; break;
      case STRING: r.u.string=empty; break;
      default: ;
    }  
  }
  /*}}}*/
  if (l.type==FLOAT && r.type==INT)
  {
    result.type=INT;
    result.u.integer=l.u.flt==((double)r.u.integer);
  }  
  else if (l.type==INT && r.type==FLOAT)
  {
    result.type=INT;
    result.u.integer=((double)l.u.integer)==r.u.flt;
  }    
  else if (l.type!=r.type)
  {
    result.type=INT;
    result.u.integer=0;
  }
  else if (l.type==INT)
  {
    result.type=INT;
    result.u.integer=l.u.integer==r.u.integer;
  }
  else if (l.type==STRING)
  {
    result.type=INT;
    result.u.integer=(strcmp(l.u.string,r.u.string)==0);
  }
  else if (l.type==FLOAT)
  {
    result.type=INT;
    result.u.integer=l.u.flt==r.u.flt;
  }
  else if (l.type==EMPTY)
  {
    result.type=INT;
    result.u.integer=1;
  }
  else
  {
    result.type=EEK;
    result.u.err=strcpy(malloc(strlen(_("type mismatch for relational operator"))+1),_("type mismatch for relational operator"));
  }
  return result;
}
/*}}}*/
/* tabouteq  -- ~= operator */ /*{{{*/
Token tabouteq(Token l, Token r)
{
  /* variables */ /*{{{*/
  Token result;
  /*}}}*/
  
  if (l.type==EEK) return tcopy(l);
  else if (r.type==EEK) return tcopy(r);
  if (l.type==EMPTY)
  /* try to assign 0 element of r.type */ /*{{{*/
  {
    l.type=r.type;
    switch (r.type)
    {
      case FLOAT: l.u.flt=0.0; break;
      default: ;
    }
  }
  /*}}}*/
  if (r.type==EMPTY)
  /* try to assign 0 element of l.type */ /*{{{*/
  {
    r.type=l.type;
    switch (l.type)
    {
      case FLOAT: r.u.flt=0.0; break;
      default: ;
    }  
  }
  /*}}}*/
  if (l.type==FLOAT && r.type==FLOAT)
  {
    result.type=INT;
    result.u.integer=(fabs(l.u.flt-r.u.flt)<=DBL_EPSILON);
  }
  else
  {
    result.type=EEK;
    result.u.err=strcpy(malloc(strlen(_("type mismatch for relational operator"))+1),_("type mismatch for relational operator"));
  }
  return result;
}
/*}}}*/
/* tne       -- != operator */ /*{{{*/
Token tne(Token l, Token r)
{
  /* variables */ /*{{{*/
  Token result;
  static char empty[]="";
  /*}}}*/
  
  if (l.type==EEK) return tcopy(l);
  else if (r.type==EEK) return tcopy(r);
  if (l.type==EMPTY)
  /* try to assign 0 element of r.type */ /*{{{*/
  {
    l.type=r.type;
    switch (r.type)
    {
      case INT: l.u.integer=0; break;
      case FLOAT: l.u.flt=0.0; break;
      case STRING: l.u.string=empty; break;
      default: ;
    }
  }
  /*}}}*/
  if (r.type==EMPTY)
  /* try to assign 0 element of l.type */ /*{{{*/
  {
    r.type=l.type;
    switch (l.type)
    {
      case INT: r.u.integer=0; break;
      case FLOAT: r.u.flt=0.0; break;
      case STRING: r.u.string=empty; break;
      default: ;
    }  
  }
  /*}}}*/
  if (l.type==FLOAT && r.type==INT)
  {
    result.type=INT;
    result.u.integer=l.u.flt!=((double)r.u.integer);
  }  
  else if (l.type==INT && r.type==FLOAT)
  {
    result.type=INT;
    result.u.integer=((double)l.u.integer)!=r.u.flt;
  }    
  else if (l.type!=r.type)
  {
    result.type=INT;
    result.u.integer=1;
  }
  else if (l.type==INT)
  {
    result.type=INT;
    result.u.integer=l.u.integer!=r.u.integer;
  }
  else if (l.type==STRING)
  {
    result.type=INT;
    result.u.integer=(strcmp(l.u.string,r.u.string)!=0);
  }
  else if (l.type==FLOAT)
  {
    result.type=INT;
    result.u.integer=l.u.flt!=r.u.flt;
  }
  else if (l.type==EMPTY)
  {
    result.type=INT;
    result.u.integer=0;
  }
  else
  {
    result.type=EEK;
    result.u.err=strcpy(malloc(strlen(_("type mismatch for relational operator"))+1),_("type mismatch for relational operator"));
  }
  return result;
}
/*}}}*/


