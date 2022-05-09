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


#include "context.h"
#include "main.h"
#include "misc.h"
/*}}}*/

/* savecontext     -- save as ConTeXt table */ /*{{{*/
const char *savecontext(Sheet *sheet, const char *name, int body, int x1, int y1, int z1, int x2, int y2, int z2, unsigned int *count)
{
  /* variables */ /*{{{*/
  FILE *fp=(FILE*)0; /* cause runtime error */
  int x,y,z;
  char buf[1024];
  char num[20];
  char fullname[PATH_MAX];
  /*}}}*/
  
  /* asserts */ /*{{{*/
  assert(sheet!=(Sheet*)0);
  assert(name!=(const char*)0);
  /*}}}*/
  *count=0;
  for (z=z1; z<=z2; ++z) for (y=y1; y<=y2; ++y) if (shadowed(sheet,x1,y,z)) return _("Shadowed cells in first column");
  if (!body && (fp=fopen(name,"w"))==(FILE*)0) return strerror(errno);
  for (z=z1; z<=z2; ++z)
  {
    if (body)
    /* open new file */ /*{{{*/
    {
      sprintf(num,".%d",z);

      fullname[sizeof(fullname)-strlen(num)-1]='\0';
      (void)strncpy(fullname,name,sizeof(fullname)-strlen(num)-1);
      fullname[sizeof(fullname)-1]='\0';
      (void)strncat(fullname,num,sizeof(fullname)-strlen(num)-1);
      fullname[sizeof(fullname)-1]='\0';  
      if ((fp=fopen(fullname,"w"))==(FILE*)0) return strerror(errno);
    }
    /*}}}*/
    else
    /* print header */ /*{{{*/
    if (z==z1)
    {
      if (fputs_close("\\starttext\n",fp)==EOF) return strerror(errno);
    }
    else
    {
      if (fputs_close("\\page\n",fp)==EOF) return strerror(errno);
    }
    /*}}}*/

    /* print bogus format */ /*{{{*/
    fprintf(fp,"\\starttable[");
    for (x=x1; x<=x2; ++x) if (fputs_close("|l",fp)==EOF) return strerror(errno);
    fprintf(fp,"|]\n");
    /*}}}*/
    for (y=y1; y<=y2; ++y)
    /* print contents */ /*{{{*/
    {
      for (x=x1; x<=x2; )
      {
        int multicols;
        char *bufp;
      
        if (x>x1 && fputs_close("\\NC",fp)==EOF) return strerror(errno);
        for (multicols=x+1; multicols<sheet->dimx && shadowed(sheet,multicols,y,z); ++multicols);
        multicols=multicols-x;
        if (multicols>1) fprintf(fp,"\\use{%d}",multicols);
        switch (getadjust(sheet,x,y,z))
        {
          case LEFT: if (fputs_close("\\JustLeft ",fp)==EOF) return strerror(errno); break;
          case RIGHT: if (fputs_close("\\JustRight ",fp)==EOF) return strerror(errno); break;
          case CENTER: if (fputs_close("\\JustCenter ",fp)==EOF) return strerror(errno); break;
          default: assert(0);
        }
        printvalue(buf,sizeof(buf),0,0,getscientific(sheet,x,y,z),getprecision(sheet,x,y,z),sheet,x,y,z);
/*        if (fputs_close("}{",fp)==EOF) return strerror(errno);*/
        if (transparent(sheet,x,y,z))
        {
          if (fputs_close(buf,fp)==EOF) return strerror(errno);
        }
        else for (bufp=buf; *bufp; ++bufp) switch (*bufp)
        {
          case '%':
          case '$':
          case '&':
          case '#':
          case '_':
          case '{':
          case '}':
          case '~':
          case '^': if (fputc_close('\\',fp)==EOF || fputc_close(*bufp,fp)==EOF) return strerror(errno); break;
          case '\\': if (fputs_close("\\backslash ",fp)==EOF) return strerror(errno); break;
          default: if (fputc_close(*bufp,fp)==EOF) return strerror(errno);
        }
/*        if (fputc_close('}',fp)==EOF) return strerror(errno);*/
        x+=multicols;
        ++*count;
      }
      if (fputs_close(y<y2 ? "\\MR\n" : "\n\\stoptable\n",fp)==EOF) return strerror(errno);
    }
    /*}}}*/
    if (body)
    {
      if (fclose(fp)==EOF) return strerror(errno);
    }
    else
    {
      if (fputs_close("\n",fp)==EOF) return strerror(errno);
    }
  }
  if (!body)
  {
    if (fputs_close("\\stoptext\n",fp)==EOF) return strerror(errno);
    if (fclose(fp)==EOF) return strerror(errno);
  }
  return (const char*)0;
}
/*}}}*/
