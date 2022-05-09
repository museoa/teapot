/* #includes */ /*{{{C}}}*//*{{{*/
#ifndef NO_POSIX_SOURCE
#undef  _POSIX_SOURCE
#define _POSIX_SOURCE 1
#undef  _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 2
#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

#include "complete.h"
/*}}}*/

char *completefile(const char *file)
{
  char *dir,*slash,*complete;
  size_t required,atmost;
  int first;
  DIR *dirp;

  if ((slash=strrchr(file,'/'))==(char*)0)
  {
    dir=strcpy(malloc(1),"");
    complete=strcpy(malloc((required=strlen(file))+1),file);
  }
  else
  {
    if (slash==file)
    {
      dir=strcpy(malloc(2),"/");
    }
    else
    {
      dir=strncpy(malloc(slash-file+1),file,slash-file);
      dir[slash-file]='\0';
    }
    complete=strcpy(malloc((required=strlen(slash+1))+1),slash+1);
  }
  atmost=-1;
  first=1;
  if ((dirp=opendir(*dir ? dir : "."))==(DIR*)0)
  {
    free(dir);
    free(complete);
    return (char*)0;
  }
  else
  {
    struct dirent *direntp;
    char *final;

    while ((direntp=readdir(dirp)))
    {
      if (strncmp(complete,direntp->d_name,required)==0)
      {
        if (first)
        {
          free(complete);
          complete=strcpy(malloc((atmost=strlen(direntp->d_name))+1),direntp->d_name);
          first=0;
        }
        else
        {
          int i;
          char *c,*n;
          
          c=complete;
          n=direntp->d_name;
          for (c=complete,n=direntp->d_name,i=required; i<atmost && c[i]==n[i]; ++i);
          if (i<atmost) complete[atmost=i]='\0';
        }
      }
    }
    closedir(dirp);
    if (atmost==0) return (char*)0;
    final=malloc(strlen(complete)-required+1);
    strcpy(final,complete+required);
    free(dir);
    free(complete);
    return final;
  }
}


