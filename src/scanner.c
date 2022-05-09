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
#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
extern double strtod(const char *nptr, char **endptr); /* SunOS 4 hack */
#include <string.h>


#include "default.h"
#include "func.h"
#include "main.h"
#include "misc.h"
#include "scanner.h"
/*}}}*/

/* identcode  -- return number of identifier */ /*{{{*/
int identcode(const char *s, size_t len)
{
  Tfunc *p;
  int fident;
  
  for (p=tfunc,fident=0; p->name[0]!='\0' && (len!=strlen(p->name) || strncmp(s,p->name,len)); ++p,++fident);
  if (p->name[0]=='\0') return -1;
  else return fident;
}
/*}}}*/

/* charstring -- match quoted string and return token */ /*{{{*/
static Token *charstring(const char **s)
{
  const char *r;

  r=*s;
  if (**s=='"')
  {
    ++(*s);
    while (**s!='\0' && **s!='"') if (**s=='\\' && *((*s)+1)!='\0') (*s)+=2; else ++(*s);
    if (**s=='\0') { *s=r; return 0; }
    else 
    {
      Token *n;
      char *t;

      ++(*s);
      n=malloc(sizeof(Token));
      n->type=STRING;
      t=n->u.string=malloc((size_t)(*s-r));
      /* Clean string of quotes.  This may waste a few bytes, so? */
      ++r;
      while (r<(*s-1)) if (*r=='\\') { *t++=*(r+1); r+=2; } else *t++=*r++;
      *t='\0';
      return n;
    }
  }
  else return (Token*)0;
}
/*}}}*/
/* integer    -- match an unsigned integer and return token */ /*{{{*/
static Token *integer(const char **s)
{
  const char *r;
  long i;

  r=*s;
  i=posnumber(r,s);
  if (*s!=r && **s!='.' && **s!='e')
  {
    Token *n;

    n=malloc(sizeof(Token));
    n->type=INT;
    n->u.integer=i;
    return n;
  }
  else { *s=r; return (Token*)0; }
}
/*}}}*/
/* flt        -- match a floating point number */ /*{{{*/
static Token *flt(const char **s)
{
  /* variables */ /*{{{*/
  const char *t;
  char *end;
  Token *n;
  double x;
  /*}}}*/

  t=*s;
  x=strtod(t,&end);
  *s = end;
  if (t!=*s && dblfinite(x)==(const char*)0)
  {
    n=malloc(sizeof(Token));
    n->type=FLOAT;
    n->u.flt=x;
    return n;
  }
  else
  {
    *s=t;
    return (Token*)0;
  }
}
/*}}}*/
/* op   -- match an op and return token */ /*{{{*/
static Token *op(const char **s)
{
  Token *n;
  Operator op;

  switch (**s)
  {
    case '+': op=PLUS; break;
    case '-': op=MINUS; break;
    case '*': op=MUL; break;
    case '/': op=DIV; break;
    case '%': op=MOD; break;
    case '(': op=OP; break;
    case ')': op=CP; break;
    case ',': op=COMMA; break;
    case '^': op=POW; break;
    case '<': if (*(*s+1)=='=') { ++(*s); op=LE; } else op=LT; break;
    case '>': if (*(*s+1)=='=') { ++(*s); op=GE; } else op=GT; break;
    case '=': if (*(*s+1)=='=') { ++(*s); op=ISEQUAL; } else return (Token*)0; break;
    case '~': if (*(*s+1)=='=') { ++(*s); op=ABOUTEQ; } else return (Token*)0; break;
    case '!': if (*(*s+1)=='=') { ++(*s); op=NE; } else return (Token*)0; break;
    default: return (Token*)0;
  }
  n=malloc(sizeof(Token));
  n->type=OPERATOR;
  n->u.op=op;
  ++(*s);
  return n;
}
/*}}}*/
/* ident      -- match an identifier and return token */ /*{{{*/
static Token *ident(const char **s)
{
  const char *begin;
  Token *result;
  
  if (isalpha((int)**s) || **s=='_' || **s=='@' || **s=='&' || **s=='.' || **s=='$')
  {
    int fident;

    begin=*s; ++(*s);
    while (isalpha((int)**s) || **s=='_' || **s=='@' || **s=='&' || **s=='.' || **s=='$' || isdigit((int)**s)) ++(*s);
    result=malloc(sizeof(Token));
    if ((fident=identcode(begin,(size_t)(*s-begin)))==-1)
    {
      result->type=LIDENT;
      result->u.lident=malloc((size_t)(*s-begin+1));
      (void)strncpy(result->u.lident,begin,(size_t)(*s-begin));
      result->u.lident[*s-begin]='\0';
    }
    else
    {
      result->type=FIDENT;
      result->u.fident=fident;
    }
    return result;
  }
  return (Token*)0;
}
/*}}}*/

/* scan       -- scan string into tokens */ /*{{{*/
Token **scan(const char **s)
{
  /* variables */ /*{{{*/
  Token **na,*n;
  const char *r;
  int i,j;
  /*}}}*/

  /* compute number of tokens */ /*{{{*/
  r=*s;
  while (*r==' ') ++r;
  for (i=0; *r!='\0'; ++i)
  {
    const char *or;

    or=r;
    while (*r==' ') ++r;  
    n=charstring(&r);
    if (n==(Token*)0) n=op(&r);
    if (n==(Token*)0) n=integer(&r);
    if (n==(Token*)0) n=flt(&r);
    if (n==(Token*)0) n=ident(&r);
    if (or==r) { *s=r; return (Token**)0; }
  }
  /*}}}*/
  /* allocate token space */ /*{{{*/
  na=malloc(sizeof(Token*)*(i+1));
  /*}}}*/
  /* store tokens */ /*{{{*/
  r=*s;
  while (*r==' ') ++r;
  for (j=0; j<i; ++j) 
  {
    while (*r==' ') ++r;    
    n=charstring(&r);
    if (n==(Token*)0) n=op(&r);  
    if (n==(Token*)0) n=integer(&r);
    if (n==(Token*)0) n=flt(&r);
    if (n==(Token*)0) n=ident(&r);
    na[j]=n;
  }
  na[j]=(Token*)0;
  /*}}}*/
  return na;
}
/*}}}*/
/* print      -- print token sequence */ /*{{{*/
void print(char *s, size_t size, size_t chars, int quote, int scientific, int precision, Token **n)
{
  size_t cur;
  
  cur=0;
  if (n!=(Token**)0) for (; cur<size-1 && (*n)!=(Token*)0; ++n) switch ((*n)->type)
  {
    /* EMPTY */ /*{{{*/
    case EMPTY: assert(cur==0); *(s+cur)='\0'; ++cur; break;
    /*}}}*/
    /* STRING */ /*{{{*/
    case STRING:
    {
      char *str=(*n)->u.string;

      if (quote)
      {
        *(s+cur)='"';
        ++cur;
      }
      while (cur<size && *str!='\0') 
      {
        if (quote) if (*str=='"' || *str=='\\') *(s+cur++)='\\';
        if (cur<size) *(s+cur++)=*str;
        ++str;
      }
      if (quote)
      {
        if (cur<size) *(s+cur)='"';
        ++cur;
      }
      break;
    }
    /*}}}*/
    /* INT */ /*{{{*/
    case INT:
    {
      char buf[20];
      size_t buflen;

      buflen=sprintf(buf,"%ld",(*n)->u.integer);
      assert(buflen<sizeof(buf));
      (void)strncpy(s+cur,buf,size-cur-1);
      cur+=buflen;
      break;
    }  
    /*}}}*/
    /* FLOAT */ /*{{{*/
    case FLOAT:
    {
      /* variables */ /*{{{*/
      char buf[1024],*p;
      size_t len;
      /*}}}*/

      len=sprintf(buf,scientific ? "%.*e" : "%.*f",precision==-1 ? DBL_DIG-2 : precision, (*n)->u.flt);
      assert(len<sizeof(buf));
      if (!scientific && precision==-1)
      {
        p=&buf[len-1];
        while (p>buf && *p=='0' && *(p-1)!='.') { *p='\0'; --p; --len; }
      }
      p=buf+len;
      while (*--p==' ') { *p='\0'; --len; }
      (void)strncpy(s+cur,buf,size-cur-1);
      cur+=len;
      break;
    }
    /*}}}*/
    /* OPERATOR */ /*{{{*/
    case OPERATOR:
    {
      static const char *ops[]={ "+", "-", "*", "/", "(", ")", ",", "<", "<=", ">=", ">", "==", "~=", "!=", "^", "%" };
      
      if ((size-cur)>1) *(s+cur++)=*ops[(*n)->u.op];
      if (*(ops[(*n)->u.op]+1) && (size-cur)>1) *(s+cur++)=*(ops[(*n)->u.op]+1);
      break;
    }
    /*}}}*/
    /* LIDENT */ /*{{{*/
    case LIDENT:
    {
      size_t identlen;

      identlen=strlen((*n)->u.lident);
      if ((cur+identlen+1)<=size) strcpy(s+cur,(*n)->u.lident);
      else (void)strncpy(s+cur,(*n)->u.lident,size-cur-1);  
      cur+=identlen;
      break;
    }
    /*}}}*/
    /* FIDENT */ /*{{{*/
    case FIDENT:
    {
      size_t identlen;

      identlen=strlen(tfunc[(*n)->u.fident].name);
      if ((cur+identlen+1)<=size) strcpy(s+cur,tfunc[(*n)->u.fident].name);
      else (void)strncpy(s+cur,tfunc[(*n)->u.fident].name,size-cur-1);  
      cur+=identlen;
      break;
    }
    /*}}}*/
    /* LOCATION */ /*{{{*/
    case LOCATION:
    {
      char buf[60];

      sprintf(buf,"&(%d,%d,%d)",(*n)->u.location[0],(*n)->u.location[1],(*n)->u.location[2]);
      (void)strncpy(s+cur,buf,size-cur-1);
      cur+=strlen(buf);
      break;
    }
    /*}}}*/
    /* EEK */ /*{{{*/
    case EEK:
    {
      (void)strncpy(s+cur,_("ERROR"),size-cur-1);
      cur+=5;
      break;
    }
    /*}}}*/
    /* default */ /*{{{*/
    default: assert(0);
    /*}}}*/
  }
  if (cur<size) s[cur] = 0;
  else s[size-1] = 0;
  if (chars && mbslen(s) > chars) {
    for (cur=0; cur < chars; ++cur) s[cur] = '#';
    s[cur] = 0;
  }
}
/*}}}*/
