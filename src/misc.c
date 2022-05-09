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
#include <math.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#ifdef NEED_BCOPY
#define memmove(dst,src,len) bcopy(src,dst,len)
#endif


#include "default.h"
#include "main.h"
#include "misc.h"
#include "utf8.h"
/*}}}*/

/* posnumber   -- match positive integer */ /*{{{*/
long int posnumber(const char *s, const char **endptr)
{
  unsigned int base=10;
  unsigned char c;
  register const char *nptr = s;
  long int result = 0L;
  int saw_a_digit = 0;

  if (*nptr == '0')
  {
    if ((c = *++nptr) == 'x' || c == 'X')
    {
      ++nptr;
      base = 16;
    }
    else
    {
      saw_a_digit = 1;
      base = 8;
    }
  }

  --nptr;
  while ((c=*++nptr)!='\0')
  {
    if (isdigit(c)) c -= '0';
    else if (isupper(c)) c -= ('A'-10);
    else if (islower(c)) c -= ('a'-10);
    else break;
    if (c>=base) break;
    saw_a_digit = 1;
    result = result*base+c;
  }

  *endptr=(saw_a_digit ? nptr : s);
  return result;
}
/*}}}*/
/* posorder    -- sort two integers */ /*{{{*/
void posorder(int *x, int *y)
{
  /* variables */ /*{{{*/
  int t;
  /*}}}*/

  assert(x!=(int*)0);
  assert(*x>=0);
  assert(y!=(int*)0);
  assert(*y>=0);
  if (*x>*y)
  {
    t=*x;
    *x=*y;
    *y=t;
  }
}
/*}}}*/
/* mystrmalloc -- return malloced copy of string */ /*{{{*/
char *mystrmalloc(const char *str)
{
  return (strcpy(malloc(strlen(str)+1),str));
}
/*}}}*/
/* finite      -- return error message about number or null */ /*{{{*/
static volatile int caughtfpe;

static void catchfpe(int n)
{
  caughtfpe=1;
}

const char *dblfinite(double x)
{
  /*struct sigaction act;

  caughtfpe=0;
  act.sa_handler=catchfpe;
  act.sa_flags=0;
  (void)sigemptyset(&act.sa_mask);
  (void)sigaction(SIGFPE,&act,(struct sigaction*)0);*/
  signal(SIGFPE, catchfpe);
  if (x==0.0)
  {
    if (caughtfpe) return _("Not a (finite) floating point number"); /* who knows */
    else return (const char*)0;
  }
  else
  {
    if (caughtfpe) return _("Not a (finite) floating point number");
    /* If one comparison was allowed, more won't hurt either. */
    if (x<0.0)
    {
      if (x<-DBL_MAX) return _("Not a (finite) floating point number"); /* -infinite */
      else return (const char*)0;
    }
    else if (x>0.0)
    {
      if (x>DBL_MAX) return _("Not a (finite) floating point number"); /* +infinite */
      else return (const char*)0;
    }
    else return _("Not a (finite) floating point number"); /* NaN */
  }
}
/*}}}*/
/* fputc_close -- error checking fputc which closes stream on error */ /*{{{*/
int fputc_close(char c, FILE *fp)
{
  int e;

  if ((e=fputc(c,fp))==EOF)
  {
    int oerrno;

    oerrno=errno;
    (void)fclose(fp);
    errno=oerrno;
  }
  return e;
}

/* fputs_close -- error checking fputs which closes stream on error */ /*{{{*/
int fputs_close(const char *s, FILE *fp)
{
  int e;

  if ((e=fputs(s,fp))==EOF)
  {
    int oerrno;

    oerrno=errno;
    (void)fclose(fp);
    errno=oerrno;
  }
  return e;
}

/* adjust      -- readjust a left adjusted string in a buffer */ /*{{{*/
void adjust(Adjust a, char *s, size_t n)
{
  assert(s!=(char*)0);
  assert(mbslen(s)<=n);
  switch (a)
  {
    /* LEFT */ /*{{{*/
    case LEFT: break;
    /*}}}*/
    /* RIGHT */ /*{{{*/
    case RIGHT:
    {
      size_t len;

      len=mbslen(s);
      if (len < n)
      {
        memmove(s+n-len, s, strlen(s)+1);
        memset(s, ' ', n-len);
      }
      break;
    }
    /*}}}*/
    /* CENTER */ /*{{{*/
    case CENTER:
    {
      size_t len,pad;

      len=mbslen(s);
      pad=(n-len)/2;
      assert((pad+len)<=n);
      memmove(s+pad, s, strlen(s)+1);
      memset(s, ' ', pad);
      //*(s+strlen(s)+n-pad-len)='\0';
      //(void)memset(s+strlen(s),' ',n-pad-len-1);
      break;
    }
    /*}}}*/
    /* default */ /*{{{*/
    default: assert(0);
    /*}}}*/
  }
}
/*}}}*/
/* strerror    -- strerror(3) */ /*{{{*/
#ifdef NEED_STRERROR
extern int sys_nerr;
extern const char *sys_errlist[];

const char *strerror(int errno)
{
  return (errno>=0 && errno<sys_nerr ? sys_errlist[errno] : "unknown error");
}
#endif
/*}}}*/
/* myrealloc   -- ANSI conforming realloc() */ /*{{{*/
#ifdef OLD_REALLOC
#undef realloc
void *myrealloc(void *p, size_t n)
{
  return (p==(void*)0 ? malloc(n) : realloc(p,n));
}
#endif
/*}}}*/

char *striphtml(const char *in)
{
    char *end, *stripped = malloc(strlen(in)), *out = stripped;
    in--;

    while (in && (end = strchr(++in, '<'))) {
        memcpy(out, in, end-in);
        out += end-in;
        in = strchr(end+1, '>');
    }
    if (in) strcpy(out, in);
    else *out = 0;
    return stripped;
}
