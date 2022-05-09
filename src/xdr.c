/* Notes */ /*{{{C}}}*//*{{{*/
/*

xdr_enum() is unusable, because enum_t may have a different size than
an enum.  The construction

  int_value=*enum_value;
  result=xdr_int(xdrs,&int_value);
  *enum_value=int_value;
  return result;

solves the problem and works for both encoding and decoding.
Unfortunately, I could not yet find such a solution for a variable sized
array terminated by a special element.

*/
/*}}}*/
/* #includes */ /*{{{*/
#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "sheet.h"
#include "xdr.h"
/*}}}*/

/* xdr_token */ /*{{{*/
static bool_t xdr_token(XDR *xdrs, Token *t)
{
  int x,result;

  if (xdrs->x_op==XDR_DECODE) (void)memset(t,0,sizeof(Token));
  x=t->type;
  if (t->type==OPERATOR) x|=t->u.op<<8;
  result=xdr_int(xdrs,&x);
  if ((x&0xff)==OPERATOR) t->u.op=(x>>8)&0xff;
  t->type=x&0xff;
  if (result==0) return result;
  switch (t->type)
  {
    /* EMPTY */ /*{{{*/
    case EMPTY:
    {
      return 1;
    }
    /*}}}*/
    /* STRING */ /*{{{*/
    case STRING:
    {
      return xdr_wrapstring(xdrs,&(t->u.string));
    }
    /*}}}*/
    /* FLOAT */ /*{{{*/
    case FLOAT:
    {
      return xdr_double(xdrs,&(t->u.flt));
    }
    /*}}}*/
    /* INT */ /*{{{*/
    case INT:
    {
      return xdr_long(xdrs,&(t->u.integer));
    }
    /*}}}*/
    /* OPERATOR */ /*{{{*/
    case OPERATOR:
    {
      return 1; /* since op is encoded in type */
    }
    /*}}}*/
    /* LIDENT */ /*{{{*/
    case LIDENT:
    {
      return xdr_wrapstring(xdrs,&(t->u.lident));
    }
    /*}}}*/
    /* FIDENT */ /*{{{*/
    case FIDENT:
    {
      return xdr_int(xdrs,&(t->u.fident));
    }
    /*}}}*/
    /* LOCATION */ /*{{{*/
    case LOCATION:
    {
      return (xdr_int(xdrs,&(t->u.location[0])) && xdr_int(xdrs,&(t->u.location[1])) && xdr_int(xdrs,&(t->u.location[2])));
    }
    /*}}}*/
    /* EEK */ /*{{{*/
    case EEK:
    {
      return xdr_wrapstring(xdrs,&(t->u.err));
    }
    /*}}}*/
    /* default -- should not happen */ /*{{{*/
    default: assert(0);
    /*}}}*/
  }
  return 0;
}
/*}}}*/
/* xdr_tokenptr */ /*{{{*/
static bool_t xdr_tokenptr(XDR *xdrs, Token **t)
{
  return xdr_pointer(xdrs,(char**)t,sizeof(Token),(xdrproc_t)xdr_token);
}
/*}}}*/
/* xdr_tokenptrvec */ /*{{{*/
static bool_t xdr_tokenptrvec(XDR *xdrs, Token ***t)
{
  unsigned int len;
  int result;

  assert(t!=(Token***)0);
  if (xdrs->x_op!=XDR_DECODE)
  {
    Token **run;

    if (*t==(Token**)0) len=0;
    else for (len=1,run=*t; *run!=(Token*)0; ++len,++run);
  }
  result=xdr_array(xdrs,(char**)t,&len,65536,sizeof(Token*),(xdrproc_t)xdr_tokenptr);
  if (len==0) *t=(Token**)0;
  return result;
}
/*}}}*/
/* xdr_mystring */ /*{{{*/
static bool_t xdr_mystring(XDR *xdrs, char **str)
{
  static struct xdr_discrim arms[3]=
  {
    { 0, (xdrproc_t)xdr_void },
    { 1, (xdrproc_t)xdr_wrapstring },
    { -1, (xdrproc_t)0 }
  };
  enum_t x;
  int res;

  x=(*str!=(char*)0);
  res=xdr_union(xdrs, &x, (char*)str, arms, (xdrproc_t)0);
  if (!x) *str=(char*)0;
  return res;
}
/*}}}*/

/* Notes */ /*{{{*/
/*

The saved sheet consists of three xdr_int()s which specify x, y and z
position of the cell saved with xdr_cell().  Perhaps xdr_cell could be
given those as parameters, which would be more correct concerning the
purpose of the xdr_functions.  Then again, reading the position may
fail (eof), whereas after the position has been read, xdr_cell() must
not fail when loading a sheet.

*/
/*}}}*/
/* xdr_cell */ /*{{{*/
bool_t xdr_cell(XDR *xdrs, Cell *cell)
{
  int result,x;

  assert(cell!=(Cell*)0);
  if (!(xdr_tokenptrvec(xdrs, &(cell->contents)) && xdr_tokenptrvec(xdrs, &(cell->ccontents)) /* && xdr_token(xdrs, &(cell->value)) */ )) return 0;
  if (xdr_mystring(xdrs, &(cell->label))==0) return 0;
  x=cell->adjust;
  result=xdr_int(xdrs, &x);
  cell->adjust=x;
  if (result==0) return 0;
  if (xdr_int(xdrs, &(cell->precision))==0) return 0;
  x=(cell->updated&1)|((cell->shadowed&1)<<1)|((cell->scientific&1)<<2)|((cell->locked&1)<<3)|((cell->transparent&1)<<4)|((cell->ignored&1)<<5)|((cell->bold&1)<<6)|((cell->underline&1)<<7);
  result=xdr_int(xdrs, &x);
  cell->updated=((x&(1))!=0);
  cell->shadowed=((x&(1<<1))!=0);
  cell->scientific=((x&(1<<2))!=0);
  cell->locked=((x&(1<<3))!=0);
  cell->transparent=((x&(1<<4))!=0);
  cell->ignored=((x&(1<<5))!=0);
  cell->bold=((x&(1<<6))!=0);
  cell->underline=((x&(1<<7))!=0);
  return result;
}
/*}}}*/
/* xdr_column */ /*{{{*/
bool_t xdr_column(XDR *xdrs, int *x, int *z, int *width)
{
  return (xdr_int(xdrs, x) && xdr_int(xdrs, z) && xdr_int(xdrs, width));
}
/*}}}*/
/* xdr_magic */ /*{{{*/
#define MAGIC0 (('#'<<24)|('!'<<16)|('t'<<8)|'e')
#define MAGIC1 (('a'<<24)|('p'<<16)|('o'<<8)|'t')
#define MAGIC2 (('\n'<<24)|('x'<<16)|('d'<<8)|'r')

bool_t xdr_magic(XDR *xdrs)
{
  long m0,m1,m2;

  m0=MAGIC0;
  m1=MAGIC1;
  m2=MAGIC2;
  return (xdr_long(xdrs,&m0) && m0==MAGIC0 && xdr_long(xdrs,&m1) && m1==MAGIC1 && xdr_long(xdrs,&m2) && m2==MAGIC2);
}
/*}}}*/
