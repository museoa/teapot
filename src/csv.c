#ifndef NO_POSIX_SOURCE
#undef  _POSIX_SOURCE
#define _POSIX_SOURCE   1
#undef  _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 2
#endif

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#ifdef OLD_REALLOC
#define realloc(s,l) myrealloc(s,l)
#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include "csv.h"

static int semicol=0;

/* csv_setopt    -- set language */ /*{{{C}}}*//*{{{*/
void csv_setopt(int sem)
{
  semicol=sem;
}
/*}}}*/
#if 0
/* csv_date      -- convert string "year<sep>month<sep>day<sep>" to date */ /*{{{*/
static struct CSV_Date csv_date(const char *s, const char **end) 
{
  struct CSV_Date date;

  assert(s!=(const char*)0);
  assert(end!=(const char**)0);
  *end=s;
  if (*s=='"')
  {
    ++s;
    if (isdigit(*s))
    {
      for (date.year=0; isdigit(*s); date.year=10*date.year+(*s-'0'),++s);
      if (*s)
      {
        ++s;
        if (isdigit(*s))
        {
          for (date.month=0; isdigit(*s); date.month=10*date.month+(*s-'0'),++s);
          if (*s)
          {
            ++s;
            if (isdigit(*s))
            {
              for (date.day=0; isdigit(*s); date.day=10*date.day+(*s-'0'),++s);
              if (*s=='"')
              {
                ++s;
                *end=s;
              }
            }
          }
        }
      }
    }
  }
  return date;
}
/*}}}*/
#endif
/* csv_separator -- convert string \t to nothing */ /*{{{*/
void csv_separator(const char *s, const char **end)
{
  assert(s!=(const char*)0);
  assert(end!=(const char**)0);
  *end=s+(*s=='\t' || (semicol && *s==';') || (!semicol && *s==','));
}
/*}}}*/
/* csv_long    -- convert string [0[x]]12345 to long */ /*{{{*/
long csv_long(const char *s, const char **end)
{
  long value;
  const char *t;

  assert(s!=(const char*)0);
  assert(end!=(const char**)0);
  if (*s=='\t') { *end=s; return 0L; };
  value=strtol(s,(char**)end,0);
  if (s!=*end)
  {
    t=*end; csv_separator(t,end);
    if (t!=*end || *t=='\0' || *t=='\n') return value;
  }
  *end=s; return 0L;
}
/*}}}*/
/* csv_double    -- convert string 123.4e5 to double */ /*{{{*/
double csv_double(const char *s, const char **end)
{
  double value;
  const char *t;

  assert(s!=(const char*)0);
  assert(end!=(const char**)0);
  if (*s=='\t') { *end=s; return 0.0; };
  value=strtod(s,(char**)end);
  if (s!=*end)
  {
    t=*end; csv_separator(t,end);
    if (t!=*end || *t=='\0' || *t=='\n') return value;
  }
  *end=s; return 0.0;
}
/*}}}*/
/* csv_string    -- convert almost any string to string */ /*{{{*/
char *csv_string(const char *s, const char **end)
{
  static char *string;
  static int strings,stringsz;

  assert(s!=(const char*)0);
  assert(end!=(const char**)0);
  strings=0;
  stringsz=0;
  string=(char*)0;
  if (!isprint((int)*s) || (!semicol && *s==',') || (semicol && *s==';')) return (char*)0;
  if (*s=='"')
  {
    ++s;
    while (*s!='\0' && *s!='\n' && *s!='"')
    {
      if ((strings+2)>=stringsz)
      {
        string=realloc(string,stringsz+=32);
      }
      string[strings]=*s;
      ++strings;
      ++s;
    }
    if (*s=='"') ++s;
  }
  else
  {
    while (*s!='\0' && *s!='\n' && *s!='\t' && ((!semicol && *s!=',') || (semicol && *s!=';')))
    {
      if ((strings+2)>=stringsz)
      {
        string=realloc(string,stringsz+=32);
      }
      string[strings]=*s;
      ++strings;
      ++s;
    }
  }
  string[strings]='\0';
  *end=s;
  csv_separator(s,end);
  return string;
}
/*}}}*/
