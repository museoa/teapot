/* #includes */ /*{{{C}}}*//*{{{*/
#ifndef NO_POSIX_SOURCE
#undef _POSIX_SOURCE
#define _POSIX_SOURCE   1
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 2
#endif

#define _XOPEN_SOURCE /* glibc2 needs this */

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
extern double strtod(const char *nptr, char **endptr); /* SunOS 4 hack */
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "cat.h"
#include "default.h"
#include "eval.h"
#include "func.h"
#include "main.h"
#include "misc.h"
#include "parser.h"
#include "scanner.h"
#include "sheet.h"
/*}}}*/
/* #defines */ /*{{{*/
/* There is a BSD extensions, but they are possibly more exact. */
#ifdef M_E
#define CONST_E M_E
#else
#define CONST_E ((double)2.7182818284590452354)
#endif
#ifdef M_PI
#define CONST_PI M_PI
#else
#define CONST_PI ((double)3.14159265358979323846)
#endif
/*}}}*/

/* sci_func -- map a double to a double */ /*{{{*/
static Token sci_func(int argc, const Token argv[], double (*func)(double), const char *func_name)
{
  Token result;

  if (argc==1 && argv[0].type==FLOAT)
  {
    result.type=FLOAT;
    errno=0;
    result.u.flt=(func)(argv[0].u.flt);
    if (errno)
    {
      result.type=EEK;
      result.u.err=malloc(strlen(func_name)+2+strlen(strerror(errno))+1);
      sprintf(result.u.err,"%s: %s",func_name,strerror(errno));
    }
  }
  else
  {
    if (argc==1 && argv[0].type==INT)
    {
      result.type=FLOAT;
      errno=0;
      result.u.flt=(func)((double)argv[0].u.integer);
      if (errno)
      {
        result.type=EEK;
        result.u.err=malloc(strlen(func_name)+2+strlen(strerror(errno))+1);
        sprintf(result.u.err,"%s: %s",func_name,strerror(errno));
      }
    }
    else	
    {
      result.type=EEK;
      /* This is actually too much, but always enough for %s formats. */
      result.u.err=malloc(strlen(WRONGTYPE)+strlen(func_name)+1);
      sprintf(result.u.err,WRONGTYPE,func_name);
    }
  }
  return result;
}
/*}}}*/
/* arsinh */ /*{{{*/
static double arsinh(double x)
{
  return log(x+sqrt(x*x+1.0));
}
/*}}}*/
/* arcosh */ /*{{{*/
static double arcosh(double x)
{
  if (x>=1.0) return log(x+sqrt(x*x-1.0));
  else { errno=EDOM; return 0.0; }
}
/*}}}*/
/* artanh */ /*{{{*/
static double artanh(double x)
{
  if (x>-1.0 && x<1.0) return 0.5*log((1.0+x)/(1.0-x));
  else { errno=EDOM; return 0.0; }
}
/*}}}*/
/* rad2deg */ /*{{{*/
static double rad2deg(double x)
{
  return (360.0/(2.0*CONST_PI))*x;
}
/*}}}*/
/* deg2rad */ /*{{{*/
static double deg2rad(double x)
{
  return (2.0*CONST_PI/360.0)*x;
}
/*}}}*/

/* @ */ /*{{{*/
static Token at_func(int argc, const Token argv[])
{
  /* variables */ /*{{{*/
  Token result;
  /*}}}*/

  /* asserts */ /*{{{*/
  assert(argv!=(Token*)0);
  /*}}}*/
  if (argc==0)
  /* return value at current location */ /*{{{*/
  return (getvalue(upd_sheet,upd_x,upd_y,upd_z));
  /*}}}*/
  if (argc==1 && argv[0].type==LOCATION)
  /* return value at location pointed to by argument */ /*{{{*/
  return (getvalue(upd_sheet,argv[0].u.location[0],argv[0].u.location[1],argv[0].u.location[2]));
  /*}}}*/
  else if (argc==1 && (argv[0].type==INT || argv[0].type==EMPTY))
  /* return value at x on current y,z */ /*{{{*/
  return (getvalue(upd_sheet,argv[0].type==INT ? argv[0].u.integer : upd_x,upd_y,upd_z));
  /*}}}*/
  else if (argc==2 && (argv[0].type==INT || argv[0].type==EMPTY) && (argv[1].type==INT || argv[1].type==EMPTY))
  /* return value at x,y on current z */ /*{{{*/
  return (getvalue(upd_sheet,argv[0].type==INT ? argv[0].u.integer : upd_x,argv[1].type==INT ? argv[1].u.integer : upd_y,upd_z));
  /*}}}*/
  else if (argc==3 && (argv[0].type==INT || argv[0].type==EMPTY) && (argv[1].type==INT || argv[1].type==EMPTY) && (argv[2].type==INT || argv[2].type==EMPTY))
  /* return value at x,y,z */ /*{{{*/
  return (getvalue(upd_sheet,argv[0].type==INT ? argv[0].u.integer : upd_x,argv[1].type==INT ? argv[1].u.integer : upd_y,argv[2].type==INT ? argv[2].u.integer : upd_z));  
  /*}}}*/
  else
  /* return error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=strcpy(malloc(strlen(USAGE_AT)+1),USAGE_AT);
    return result;
  }
  /*}}}*/
}
/*}}}*/
/* & */ /*{{{*/
static Token adr_func(int argc, const Token argv[])
{
  /* variables */ /*{{{*/
  Token result;
  /*}}}*/

  /* asserts */ /*{{{*/
  assert(argv!=(Token*)0);
  /*}}}*/
  if (argc==3 && (argv[0].type==INT || argv[0].type==EMPTY) && (argv[1].type==INT || argv[1].type==EMPTY) && (argv[2].type==INT || argv[2].type==EMPTY))
  /* result is location of the given position */ /*{{{*/
  {
    result.type=LOCATION;
    result.u.location[0]=(argv[0].type==INT ? argv[0].u.integer : upd_x);
    result.u.location[1]=(argv[1].type==INT ? argv[1].u.integer : upd_y);
    result.u.location[2]=(argv[2].type==INT ? argv[2].u.integer : upd_z);
  }
  /*}}}*/
  else if (argc==2 && (argv[0].type==INT || argv[0].type==EMPTY) && (argv[1].type==INT || argv[1].type==EMPTY))
  /* result is location of the given position in the current z layer */ /*{{{*/
  {
    result.type=LOCATION;
    result.u.location[0]=(argv[0].type==INT ? argv[0].u.integer : upd_x);
    result.u.location[1]=(argv[1].type==INT ? argv[1].u.integer : upd_y);
    result.u.location[2]=upd_z;
  }
  /*}}}*/
  else if (argc==1 && (argv[0].type==INT || argv[0].type==EMPTY))
  /* result is location of the given position in the current y,z layer */ /*{{{*/
  {
    result.type=LOCATION;
    result.u.location[0]=(argv[0].type==INT ? argv[0].u.integer : upd_x);
    result.u.location[1]=upd_y;
    result.u.location[2]=upd_z;
  }
  /*}}}*/
  else if (argc==0)
  /* result is location of the current position */ /*{{{*/
  {
    result.type=LOCATION;
    result.u.location[0]=upd_x;
    result.u.location[1]=upd_y;
    result.u.location[2]=upd_z;
  }
  /*}}}*/
  else
  /* result is type error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=strcpy(malloc(strlen(USAGE_ADR)+1),USAGE_ADR);
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* x */ /*{{{*/
static Token x_func(int argc, const Token argv[])
{
  /* variables */ /*{{{*/
  Token result;
  /*}}}*/

  if (argc==0)
  /* result is currently updated x position */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=upd_x;
  }
  /*}}}*/
  else if (argc==1 && argv[0].type==LOCATION)
  /* return x component of location */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=argv[0].u.location[0];
  }
  /*}}}*/
  else
  /* x type error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=strcpy(malloc(strlen(XTYPE)+1),XTYPE);
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* y */ /*{{{*/
static Token y_func(int argc, const Token argv[])
{
  /* variables */ /*{{{*/
  Token result;
  /*}}}*/

  if (argc==0)
  /* result is currently updated y position */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=upd_y;
  }
  /*}}}*/
  else if (argc==1 && argv[0].type==LOCATION)
  /* return y component of location */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=argv[0].u.location[1];
  }
  /*}}}*/
  else
  /* y type error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=strcpy(malloc(strlen(YTYPE)+1),YTYPE);
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* z */ /*{{{*/
static Token z_func(int argc, const Token argv[])
{
  /* variables */ /*{{{*/
  Token result;
  /*}}}*/

  if (argc==0)
  /* result is currently updated z position */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=upd_z;
  }
  /*}}}*/
  else if (argc==1 && argv[0].type==LOCATION)
  /* return z component of location */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=argv[0].u.location[2];
  }
  /*}}}*/
  else
  /* result is z type error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=mystrmalloc(ZTYPE);
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* e */ /*{{{*/
static Token e_func(int argc, const Token argv[])
{
  /* variables */ /*{{{*/
  Token result;
  /*}}}*/

  if (argc==0) /* result is constant e */ /*{{{*/
  {
    result.type=FLOAT;
    result.u.flt=CONST_E;
  }
  /*}}}*/
  else /* result is e type error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=mystrmalloc(ETYPE);
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* eval */ /*{{{*/
static Token eval_func(int argc, const Token argv[])
{
  /* variables */ /*{{{*/
  Token result;
  /*}}}*/

  --max_eval;
  if (max_eval<0)
  /* nesting error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=mystrmalloc(NESTEVAL);
  }
  /*}}}*/
  else if (argc==1 && argv[0].type==LOCATION)
  /* evaluate expression in cell at given position */ /*{{{*/
  {
    Token **contents;

    contents=getcont(upd_sheet,argv[0].u.location[0],argv[0].u.location[1],argv[0].u.location[2],2);
    if (contents==(Token**)0) result.type=EMPTY;
    else result=eval(contents);
  }
  /*}}}*/
  else
  /* eval type error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=strcpy(malloc(strlen(USAGE_EVAL)+1),USAGE_EVAL);
  }
  /*}}}*/
  ++max_eval;
  return result;
}
/*}}}*/
/* error */ /*{{{*/
static Token error_func(int argc, const Token argv[])
{
  /* variables */ /*{{{*/
  Token result;
  /*}}}*/

  /* asserts */ /*{{{*/
  assert(argv!=(Token*)0);
  /*}}}*/
  result.type=EEK;
  if (argc!=1 || argv[0].type!=STRING)
  /* result is type error */ /*{{{*/
  result.u.err=strcpy(malloc(strlen(USAGE_ERROR)+1),USAGE_ERROR);
  /*}}}*/
  else
  /* result is user defined error */ /*{{{*/
  result.u.err=strcpy(malloc(strlen(argv[0].u.string)+1),argv[0].u.string);
  /*}}}*/
  return result;
}
/*}}}*/
/* string */ /*{{{*/
static Token string_func(int argc, const Token argv[])
{
  /* variables */ /*{{{*/
  Token result;
  char *buf;
  size_t size;
  /*}}}*/

  if (argc==1 && argv[0].type==LOCATION) /* cell to string */ /*{{{*/
  {
    buf=(char*)0;
    size=0;
    do
    {
      if (buf!=(char*)0) free(buf);
      size+=16;
      buf=malloc(size);
      printvalue(buf,size,0,0,getscientific(upd_sheet,argv[0].u.location[0],argv[0].u.location[1],argv[0].u.location[2]),getprecision(upd_sheet,argv[0].u.location[0],argv[0].u.location[1],argv[0].u.location[2]),upd_sheet,argv[0].u.location[0],argv[0].u.location[1],argv[0].u.location[2]);
    } while (strlen(buf)==size-1);
    result.type=STRING;
    result.u.string=buf;
  }
  /*}}}*/
  else if (argc==1 && argv[0].type==FLOAT) /* float to string */ /*{{{*/
  {
    result.u.string=malloc(def_precision+10);
    sprintf(result.u.string,DEF_SCIENTIFIC ? "%.*e" : "%.*f", def_precision,argv[0].u.flt);
    result.type=STRING;
  }
  /*}}}*/
  else if (argc==1 && argv[0].type==INT) /* int to string */ /*{{{*/
  {
    int length=2;
    int n=argv[0].u.integer;

    while (n!=0) n/=10;
    result.u.string=malloc(length);
    sprintf(result.u.string,"%ld",argv[0].u.integer);
    result.type=STRING;
  }
  /*}}}*/
  else if (argc==2 && argv[0].type==FLOAT && argv[1].type==INT) /* float to string */ /*{{{*/
  {
    result.u.string=malloc(argv[1].u.integer==-1 ? def_precision+10 : argv[1].u.integer+10);
    sprintf(result.u.string,DEF_SCIENTIFIC ? "%.*e" : "%.*f",argv[1].u.integer==-1 ? def_precision : (int)(argv[1].u.integer), argv[0].u.flt);
    result.type=STRING;
  }
  /*}}}*/
  else if (argc==3 && argv[0].type==FLOAT && (argv[1].type==INT || argv[1].type==EMPTY) && argv[2].type==INT) /* float to string */ /*{{{*/
  {
    result.u.string=malloc((argv[1].type==INT ? argv[1].u.integer : def_precision)+10);
    sprintf(result.u.string,argv[2].u.integer ? "%.*e" : "%.*f",argv[1].type==INT && argv[1].u.integer>=0 ? (int)argv[1].u.integer : def_precision, argv[0].u.flt);
    result.type=STRING;
  }
  /*}}}*/
  else /* return string type error  */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=strcpy(malloc(strlen(STRINGTYPE)+1),STRINGTYPE);
    return result;
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* sum */ /*{{{*/
static Token sum_func(int argc, const Token argv[])
{
  /* variables */ /*{{{*/
  Token result;
  /*}}}*/

  if (argc==2 && argv[0].type==LOCATION && argv[1].type==LOCATION) /* result is sum */ /*{{{*/
  {
    /* variables */ /*{{{*/
    int x,y,z;
    int x1,y1,z1;
    int x2,y2,z2;
    Token tmp;
    /*}}}*/

    x1=argv[0].u.location[0]; x2=argv[1].u.location[0]; posorder(&x1,&x2);
    y1=argv[0].u.location[1]; y2=argv[1].u.location[1]; posorder(&y1,&y2);
    z1=argv[0].u.location[2]; z2=argv[1].u.location[2]; posorder(&z1,&z2);
    result.type=EMPTY;
    for (x=x1; x<=x2; ++x)
    for (y=y1; y<=y2; ++y)
    for (z=z1; z<=z2; ++z)
    {
      Token t;

      tmp=tadd(result,t=getvalue(upd_sheet,x,y,z));
      tfree(&t);
      tfree(&result);
      result=tmp;
      if (result.type==EEK) return result;
    }
  }
  /*}}}*/
  else /* result is sum type error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=strcpy(malloc(strlen(SUMTYPE)+1),SUMTYPE);
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* n */ /*{{{*/
static Token n_func(int argc, const Token argv[])
{
  /* variables */ /*{{{*/
  Token result;
  /*}}}*/

  if (argc==2 && argv[0].type==LOCATION && argv[1].type==LOCATION)
  /* result is number of elements */ /*{{{*/
  {
    /* variables */ /*{{{*/
    int x,y,z;
    int x1,y1,z1;
    int x2,y2,z2;
    Token tmp;
    int n;
    /*}}}*/

    x1=argv[0].u.location[0]; x2=argv[1].u.location[0]; posorder(&x1,&x2);
    y1=argv[0].u.location[1]; y2=argv[1].u.location[1]; posorder(&y1,&y2);
    z1=argv[0].u.location[2]; z2=argv[1].u.location[2]; posorder(&z1,&z2);
    n=0;
    for (x=x1; x<=x2; ++x)
    for (y=y1; y<=y2; ++y)
    for (z=z1; z<=z2; ++z)
    {
      tmp=getvalue(upd_sheet,x,y,z);
      if (tmp.type!=EMPTY) ++n;
      tfree(&tmp);
    }
    result.type=INT;
    result.u.integer=n;
  }
  /*}}}*/
  else
  /* result is n type error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=strcpy(malloc(strlen(NTYPE)+1),NTYPE);
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* int */ /*{{{*/
static Token int_func(int argc, const Token argv[])
{
  /* variables */ /*{{{*/
  Token result;
  /*}}}*/

  if (argc==1 && argv[0].type==FLOAT)
  /* result is integer with cutoff fractional part */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=(long)(argv[0].u.flt);
  }
  /*}}}*/
  else if (argc==3 && argv[0].type==FLOAT && argv[1].type==INT && argv[2].type==INT)
  /* result is integer with given conversion */ /*{{{*/
  {
    result.type=INT;
    if (argv[0].u.flt<0)
    {
      if (argv[1].u.integer<-1) result.u.integer=(long)floor(argv[0].u.flt);
      else if (argv[1].u.integer==-1) result.u.integer=(long)(argv[0].u.flt-0.5);
      else if (argv[1].u.integer==0) result.u.integer=(long)(argv[0].u.flt);
      else if (argv[1].u.integer==1) result.u.integer=(long)(argv[0].u.flt+0.5);
      else result.u.integer=(long)ceil(argv[0].u.flt);
    }
    else
    {
      if (argv[2].u.integer<-1) result.u.integer=(long)floor(argv[0].u.flt);
      else if (argv[2].u.integer==-1) result.u.integer=(long)(argv[0].u.flt-0.5);
      else if (argv[2].u.integer==0) result.u.integer=(long)(argv[0].u.flt);
      else if (argv[2].u.integer==1) result.u.integer=(long)(argv[0].u.flt+0.5);
      else result.u.integer=(long)ceil(argv[0].u.flt);
    }
  }
  /*}}}*/
  else if (argc==1 && argv[0].type==STRING)
  /* result is integer */ /*{{{*/
  {
    char *s;
    
    errno=0;
    result.u.integer=strtol(argv[0].u.string,&s,10);
    if (s==(char*)0 || *s)
    {
      result.type=EEK;
      result.u.err=mystrmalloc(NOINT);
    }
    else if (errno==ERANGE && (result.u.integer==LONG_MAX || result.u.integer==LONG_MIN))
    {
      result.type=EEK;
      result.u.err=mystrmalloc(INFINT);
    }
    else result.type=INT;
  }
  /*}}}*/
  else
  /* result is int type error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=strcpy(malloc(strlen(INTTYPE)+1),INTTYPE);
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* frac */ /*{{{*/
static Token frac_func(int argc, const Token argv[])
{
  /* variables */ /*{{{*/
  Token result;
  double foo;
  /*}}}*/

  if (argc==1 && argv[0].type==FLOAT)
  /* result is fractional part */ /*{{{*/
  {
    result.type=FLOAT;
    result.u.flt=modf(argv[0].u.flt,&foo);
  }
  /*}}}*/
  else
  /* result is frac type error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=strcpy(malloc(strlen(FRACTYPE)+1),FRACTYPE);
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* len */ /*{{{*/
static Token len_func(int argc, const Token argv[])
{
  /* variables */ /*{{{*/
  Token result;
  /*}}}*/

  if (argc==1 && argv[0].type==STRING)
  /* result is length */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=strlen(argv[0].u.string);
  }
  /*}}}*/
  else
  /* result is frac type error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=mystrmalloc(LENTYPE);
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* log */ /*{{{*/
static Token log_func(int argc, const Token argv[])
{
  /* variables */ /*{{{*/
  double x=-1.0,y=-1.0;
  Token result;
  /*}}}*/

  /* set x and y to first two arguments */ /*{{{*/
  if (argc>=1)
  {
    if (argv[0].type==FLOAT) x=argv[0].u.flt;
    else if (argv[0].type==INT) x=(double)argv[0].u.integer;
  }
  if (argc==2)
  {
    if (argv[1].type==FLOAT) y=argv[1].u.flt;
    else if (argv[1].type==INT) y=(double)argv[1].u.integer;
  }
  /*}}}*/
  if (argc==1 && (argv[0].type==FLOAT || argv[0].type==INT)) /* result is ln(x) */ /*{{{*/
  {
    result.type=FLOAT;
    result.u.flt=log(x);
  }
  /*}}}*/
  else if (argc==2 && (argv[0].type==FLOAT || argv[0].type==INT) && (argv[1].type==FLOAT || argv[1].type==INT)) /* result is ln(x)/ln(y) */ /*{{{*/
  {
    result.type=FLOAT;
    if (y==CONST_E) result.u.flt=log(x);
    else if (y==10.0) result.u.flt=log10(x);
    else result.u.flt=log(x)/log(y);
  }
  /*}}}*/
  else /* result is log type error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=mystrmalloc(LOGTYPE);
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* minmax */ /*{{{*/
static Token minmax_func(int argc, const Token argv[], int min)
{
  /* variables */ /*{{{*/
  Token result;
  int resultx,resulty,resultz;
  /*}}}*/

  if (argc==2 && argv[0].type==LOCATION && argv[1].type==LOCATION)
  /* result is min/max */ /*{{{*/
  {
    /* variables */ /*{{{*/
    int x,y,z;
    int x1,y1,z1;
    int x2,y2,z2;
    Token tmp;
    /*}}}*/

    x1=argv[0].u.location[0]; x2=argv[1].u.location[0]; posorder(&x1,&x2);
    y1=argv[0].u.location[1]; y2=argv[1].u.location[1]; posorder(&y1,&y2);
    z1=argv[0].u.location[2]; z2=argv[1].u.location[2]; posorder(&z1,&z2);
    result=getvalue(upd_sheet,x1,y1,z1);
    resultx=x1;
    resulty=y1;
    resultz=z1;
    for (x=x1; x<=x2; ++x)
    for (y=y1; y<=y2; ++y)
    for (z=z1; z<=z2; ++z)
    {
      Token t;

      tmp=(min ? tle(result,t=getvalue(upd_sheet,x,y,z)) : tge(result,t=getvalue(upd_sheet,x,y,z)));
      if (tmp.type==INT)
      /* successful comparison */ /*{{{*/
      {
        tfree(&tmp);
        if (tmp.u.integer==0)
        {
          tfree(&result);
          result=t;
          resultx=x;
          resulty=y;
          resultz=z;
        }
        else tfree(&t);
      }
      /*}}}*/
      else
      /* successless comparison, return with error */ /*{{{*/
      {
        tfree(&result);
        tfree(&t);
        return tmp;
      }
      /*}}}*/
    }
    tfree(&result);
    result.type=LOCATION;
    result.u.location[0]=resultx;
    result.u.location[1]=resulty;
    result.u.location[2]=resultz;
    return result;
  }
  /*}}}*/
  else
  /* result is min/max type error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=mystrmalloc(min ? MINTYPE : MAXTYPE);
    return result;
  }
  /*}}}*/
}
/*}}}*/
/* min */ /*{{{*/
static Token min_func(int argc, const Token argv[])
{
  return minmax_func(argc,argv,1);
}
/*}}}*/
/* max */ /*{{{*/
static Token max_func(int argc, const Token argv[])
{
  return minmax_func(argc,argv,0);
}
/*}}}*/
/* abs */ /*{{{*/
static Token abs_func(int argc, const Token argv[])
{
  /* variables */ /*{{{*/
  Token result;
  /*}}}*/

  if (argc==1 && argv[0].type==FLOAT)
  /* result is absolute floating point number */ /*{{{*/
  {
    result.type=FLOAT;
    result.u.flt=fabs(argv[0].u.flt);
  }
  /*}}}*/
  else if (argc==1 && argv[0].type==INT)
  /* result is absolute integer number */ /*{{{*/
  {
    result.type=INT;
    result.u.integer=(argv[0].u.integer<0 ? -argv[0].u.integer : argv[0].u.integer);
  }
  /*}}}*/
  else
  /* result is abs type error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=mystrmalloc(ABSTYPE);
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* $ */ /*{{{*/
static Token env_func(int argc, const Token argv[])
{
  /* variables */ /*{{{*/
  Token result;
  /*}}}*/

  if (argc==1 && argv[0].type==STRING)
  {
    const char *e;

    if ((e=getenv(argv[0].u.string))==(char*)0) e="";
    result.type=STRING;
    result.u.string=mystrmalloc(e);
  }
  else
  {
    result.type=EEK;
    result.u.err=mystrmalloc(USAGE_ENV);
  }
  return result;
}
/*}}}*/
/* float */ /*{{{*/
static Token float_func(int argc, const Token argv[])
{
  Token result;
  
  if (argc==1 && argv[0].type==STRING)
  /* convert string to float */ /*{{{*/
  {
    char *p;

    result.u.flt=strtod(argv[0].u.string,&p);
    if (p!=argv[0].u.string && *p=='\0' && dblfinite(result.u.flt)==(const char*)0)
    {
      result.type=FLOAT;
    }
    else
    {
      result.type=EEK;
      result.u.err=mystrmalloc(NOTFINITE);
    }
  }
  /*}}}*/
  else
  /* float type error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=mystrmalloc(FLOATTYPE);
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* strftime */ /*{{{*/
static Token strftime_func(int argc, const Token argv[])
{
  /* variables */ /*{{{*/
  Token result;
  /*}}}*/

  if (argc==1 && argv[0].type==STRING) /* format and return string */ /*{{{*/
  {
    time_t t;
    struct tm *tm;
    char s[1024];
    
    t=time((time_t*)0);
    tm=localtime(&t);
    strftime(s,sizeof(s),argv[0].u.string,tm);
    s[sizeof(s)-1]='\0';
    result.u.string=mystrmalloc(s);
    result.type=STRING;
  }
  /*}}}*/
  else if (argc==2 && argv[0].type==STRING && argv[1].type==INT) /* format and return string */ /*{{{*/
  {
    time_t t;
    struct tm *tm;
    char s[1024];
    
    t=argv[1].u.integer;
    tm=localtime(&t);
    strftime(s,sizeof(s),argv[0].u.string,tm);
    s[sizeof(s)-1]='\0';
    result.u.string=mystrmalloc(s);
    result.type=STRING;
  }
  /*}}}*/
  else /* strftime type error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=mystrmalloc(STRFTIMETYPE);
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* clock */ /*{{{*/
static Token clock_func(int argc, const Token argv[])
{
  /* variables */ /*{{{*/
  Token result;
  /*}}}*/

  if (argc==2 && argv[0].type==INT && argv[1].type==LOCATION) /* clock(condition,location) */ /*{{{*/
  {
    if (argv[0].u.integer) clk(upd_sheet,argv[1].u.location[0],argv[1].u.location[1],argv[1].u.location[2]);
    result.type=EMPTY;
  }
  /*}}}*/
  else if (argc==3 && argv[0].type==INT && argv[1].type==LOCATION && argv[2].type==LOCATION) /* clock(condition,location,location) */ /*{{{*/
  {
    if (argv[0].u.integer)
    {
      /* variables */ /*{{{*/
      int x,y,z;
      int x1,y1,z1;
      int x2,y2,z2;
      /*}}}*/

      x1=argv[1].u.location[0]; x2=argv[2].u.location[0]; posorder(&x1,&x2);
      y1=argv[1].u.location[1]; y2=argv[2].u.location[1]; posorder(&y1,&y2);
      z1=argv[1].u.location[2]; z2=argv[2].u.location[2]; posorder(&z1,&z2);
      for (x=x1; x<=x2; ++x)
      for (y=y1; y<=y2; ++y)
      for (z=z1; z<=z2; ++z) clk(upd_sheet,x,y,z);
    }
    result.type=EMPTY;
  }
  /*}}}*/
  else /* wrong usage */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=mystrmalloc(CLOCK);
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* poly */ /*{{{*/
static Token poly_func(int argc, const Token argv[])
{
  /* variables */ /*{{{*/
  Token result;
  int i;
  /*}}}*/

  for (i=0; i<argc; ++i) if (argc<2 || (argv[i].type!=INT && argv[i].type!=FLOAT)) /* type error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=strcpy(malloc(strlen(POLYTYPE)+1),POLYTYPE);
  }
  /*}}}*/
  else
  {
    Token tmp;

    result=tcopy(argv[1]);
    for (i=2; i<argc; ++i)
    {
      tmp=tmul(result,argv[0]);
      tfree(&result);
      result=tmp;
      tmp=tadd(result,argv[i]);
      tfree(&result);
      result=tmp;
      if (result.type==EEK) return result;
    }
  }
  return result;
}
/*}}}*/
/* sin */ /*{{{*/
static Token sin_func(int argc, const Token argv[])
{
  return sci_func(argc,argv,sin,"sin");
}
/*}}}*/
/* cos */ /*{{{*/
static Token cos_func(int argc, const Token argv[])
{
  return sci_func(argc,argv,cos,"cos");
}
/*}}}*/
/* tan */ /*{{{*/
static Token tan_func(int argc, const Token argv[])
{
  return sci_func(argc,argv,tan,"tan");
}
/*}}}*/
/* sinh */ /*{{{*/
static Token sinh_func(int argc, const Token argv[])
{
  return sci_func(argc,argv,sinh,"sinh");
}
/*}}}*/
/* cosh */ /*{{{*/
static Token cosh_func(int argc, const Token argv[])
{
  return sci_func(argc,argv,cosh,"cosh");
}
/*}}}*/
/* tanh */ /*{{{*/
static Token tanh_func(int argc, const Token argv[])
{
  return sci_func(argc,argv,tanh,"tanh");
}
/*}}}*/
/* asin */ /*{{{*/
static Token asin_func(int argc, const Token argv[])
{
  return sci_func(argc,argv,asin,"asin");
}
/*}}}*/
/* acos */ /*{{{*/
static Token acos_func(int argc, const Token argv[])
{
  return sci_func(argc,argv,acos,"acos");
}
/*}}}*/
/* atan */ /*{{{*/
static Token atan_func(int argc, const Token argv[])
{
  return sci_func(argc,argv,atan,"atan");
}
/*}}}*/
/* arsinh */ /*{{{*/
static Token arsinh_func(int argc, const Token argv[])
{
  return sci_func(argc,argv,arsinh,"arsinh");
}
/*}}}*/
/* arcosh */ /*{{{*/
static Token arcosh_func(int argc, const Token argv[])
{
  return sci_func(argc,argv,arcosh,"arcosh");
}
/*}}}*/
/* artanh */ /*{{{*/
static Token artanh_func(int argc, const Token argv[])
{
  return sci_func(argc,argv,artanh,"artanh");
}
/*}}}*/
/* rad2deg */ /*{{{*/
static Token rad2deg_func(int argc, const Token argv[])
{
  return sci_func(argc,argv,rad2deg,"rad2deg");
}
/*}}}*/
/* deg2rad */ /*{{{*/
static Token deg2rad_func(int argc, const Token argv[])
{
  return sci_func(argc,argv,deg2rad,"deg2rad");
}
/*}}}*/
/* rnd */ /*{{{*/
static Token rnd_func(int argc, const Token argv[])
{
  /* variables */ /*{{{*/
  Token result;
  /*}}}*/

  if (argc==0)
  {
    result.type=FLOAT;
    result.u.flt=rand()/((double)RAND_MAX);
  }
  else
  {
    result.type=EEK;
    result.u.err=mystrmalloc(USAGE_RND);
  }
  return result;
}
/*}}}*/
/* substr */ /*{{{*/
static Token substr_func(int argc, const Token argv[])
{
  /* variables */ /*{{{*/
  Token result;
  /*}}}*/

  if (argc==3 && argv[0].type==STRING && argv[1].type==INT && argv[2].type==INT)
  {
    char ss[1024];
    int n, l, b, e;

    b = argv[1].u.integer;
    e = argv[2].u.integer;
    l = strlen(argv[0].u.string);
    if( b < 0 ) b = 0;
    if( b > l ) b = l;
    if( e > l ) e = l;
    n = e - b + 1;
    if( n >= 1024 ) n = 1024 - 1;
    if(n > 0) {
	ss[n] = '\0';
	strncpy(ss, argv[0].u.string + b, n);
	result.type=STRING;
	result.u.string=mystrmalloc(ss);
    }
    else {
	result.type=EMPTY;
    }
  }
  else
  {
    result.type=EEK;
    result.u.err=mystrmalloc(USAGE_SUBSTR);
  }
  return result;
}
/*}}}*/
/* strptime */ /*{{{*/
static Token strptime_func(int argc, const Token argv[])
{
  /* variables */ /*{{{*/
  Token result;
  /*}}}*/

  if (argc==2 && argv[0].type==STRING && argv[1].type==STRING) /* format and return string */ /*{{{*/
  {
    time_t t;
    struct tm tm;
    
    t=time((time_t*)0);
    tm=*localtime(&t);
    strptime(argv[1].u.string,argv[0].u.string,&tm);
    result.u.integer=mktime(&tm);
    result.type=INT;
  }
  /*}}}*/
  else /* strftime type error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=mystrmalloc(STRPTIMETYPE);
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* time */ /*{{{*/
static Token time_func(int argc, const Token argv[])
{
  Token result;

  if (argc==0)
  {
    result.type=INT;
    result.u.integer=time((time_t*)0);
  }
  else	
  {
    result.type=EEK;
    result.u.err=mystrmalloc(USAGE_TIME);
  }
  return result;
}
/*}}}*/
  
/* table of functions */ /*{{{*/
/* The order of these entries has no influence on performance, but to stay
   compatible, new entries should be appended. */
Tfunc tfunc[]=
{
  { "@", at_func },
  { "&", adr_func },
  { "x", x_func },
  { "y", y_func },
  { "z", z_func },
  { "eval", eval_func },
  { "error", error_func },
  { "string", string_func },
  { "sum", sum_func },
  { "n", n_func },
  { "int", int_func },
  { "frac", frac_func },
  { "len", len_func },
  { "min", min_func },
  { "max", max_func },
  { "abs", abs_func },
  { "$", env_func },
  { "float", float_func },
  { "strftime", strftime_func },
  { "clock", clock_func },
  { "poly", poly_func },
  { "e", e_func },
  { "log", log_func },
  { "sin", sin_func },
  { "cos", cos_func },
  { "tan", tan_func },
  { "sinh", sinh_func },
  { "cosh", cosh_func },
  { "tanh", tanh_func },
  { "asin", asin_func },
  { "acos", acos_func },
  { "atan", atan_func },
  { "arsinh", arsinh_func },
  { "arcosh", arcosh_func },
  { "artanh", artanh_func },
  { "deg2rad", deg2rad_func },
  { "rad2deg", rad2deg_func },
  { "rnd", rnd_func },
  { "substr", substr_func },
  { "strptime", strptime_func },
  { "time", time_func },
  { "", (Token (*)(int, const Token[]))0 }
};
/*}}}*/
