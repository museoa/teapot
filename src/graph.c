/* #includes */ /*{{{C}}}*//*{{{*/
#undef  _POSIX_SOURCE
#define _POSIX_SOURCE   1
#undef  _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 2
#define _GNU_SOURCE   1

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
double strtod(const char *nptr, char **endptr); /* SunOS 4 hack */
#ifdef OLD_REALLOC
#define realloc(s,l) myrealloc(s,l)
#endif
extern char *optarg;
extern int optind,opterr,optopt;
int getopt(int argc, char * const *argv, const char *optstring);

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include "csv.h"
/*}}}*/

int main(int argc, char *argv[]) /*{{{*/
{
  /* variables */ /*{{{*/
  struct Pair
  {
    const char *label;
    double value;
    int special;
  } *pair;
  int pairs;
  double height,width;
  int box;
  int nl;
  /*}}}*/

  /* parse options */ /*{{{*/
  {
    int c;
    height=width=2.0;
    box=0;
    nl=1;
    while ((c=getopt(argc,argv,"bnw:h:?"))!=EOF) switch (c)
    {
      /* w width */ /*{{{*/
      case 'w':
      {
        char *end;

        width=strtod(optarg,&end);
        if (*end!='\0')
        {
          fprintf(stderr,"graph: invalid width\n");
          exit(1);
        }
        break;
      }
      /*}}}*/
      /* h height */ /*{{{*/
      case 'h':
      {
        char *end;

        height=strtod(optarg,&end);
        if (*end!='\0')
        {
          fprintf(stderr,"graph: invalid height\n");
          exit(1);
        }
        break;
      }
      /*}}}*/
      /* n */
      case 'n':
      {
        nl=0;
        break;
      }
      /* b */
      case 'b':
      {
        box=1;
        break;
      }
      default:
      {
        fprintf(stderr,"Usage: graph [-b][-h height][-w width]\n");
        exit(1);
      }
    }
  }
  /*}}}*/
  /* get data */ /*{{{*/
  {
    char ln[256];
    int line,pairsz;

    pairs=pairsz=0;
    pair=(struct Pair*)0;
    for (line=1; fgets(ln,sizeof(ln),stdin); ++line)
    {
      const char *s;
      const char *end;

      if (pairs==pairsz)
      {
        if ((pair=realloc(pair,sizeof(struct Pair)*(pairsz+=128)))==(struct Pair*)0)
        {
          fprintf(stderr,"graph:%d:out of memory\n",line);
          exit(1);
        }
      }
      s=ln;
      pair[pairs].label=csv_string(s,&end);
      if (s==end) fprintf(stderr,"graph:%d:invalid string, ignoring record\n",line);
      else
      {
        s=end;
        pair[pairs].value=csv_double(s,&end);
        if (s==end) fprintf(stderr,"graph:%d:invalid value, ignoring record\n",line);
        else
        {
          s=end;
          pair[pairs].special=csv_double(s,&end);
          if (s==end && *s!='\n') fprintf(stderr,"graph:%d:invalid mark value, ignoringrecord\n",line);
          else
          {
            s=end;
            if (*s!='\n') fprintf(stderr,"graph:%d:trailing garbage\n",line);
          }
          ++pairs;
        }
      }
    }
  }
  /*}}}*/

  if (box) /* make box graph */ /*{{{*/
  {
    double height,boxwid,min,max,scale;
    int i;

    height=2;
    boxwid=width/pairs;
    for (min=max=0.0,i=0; i<pairs; ++i)
    {
      if (pair[i].value<min) min=pair[i].value;
      else if (pair[i].value>max) max=pair[i].value;
    }
    scale=height/(max-min);
    printf("[\nFRAME: box invis wid %f ht %f\n",width,height);
    for (i=0; i<pairs; ++i)
    {
      double v;

      v=fabs(pair[i].value);
      printf
      (
      "box wid %f ht %f with .%s at FRAME.sw + (%f,%f) \"%s\"\n",
      boxwid,
      v*scale,
      pair[i].value>0 ? "sw" : "nw",
      i*boxwid,
      -min*scale,
      pair[i].label
      );
    }
    printf("]"); if (nl) printf("\n");
  }
  /*}}}*/
  else /* make pie graph */ /*{{{*/
  {
    int anyspecial,i;
    double sum,scale,extra,arc;

    for (sum=0.0,i=0,anyspecial=0; i<pairs; ++i)
    {
      sum+=pair[i].value;
      if (pair[i].special) anyspecial=1;
    }
    scale=2.0*M_PI/sum;
    printf("[\n");
    printf("box invis wid %f ht %f\n",width,width);
    if (anyspecial) width-=width/10.0;
    for (sum=0,i=0; i<pairs; sum+=pair[i].value,++i)
    {
      extra=(pair[i].special!=0)*width/20.0;
      for (arc=0; (arc+M_PI_4)<(pair[i].value*scale); arc+=M_PI_4)
      {
        printf
        (
        "arc from last box.c + (%f,%f) to last box.c + (%f,%f) rad %f\n",
        cos((sum+arc)*scale)*width/2.0 + cos((sum+pair[i].value/2.0)*scale)*extra,
        sin((sum+arc)*scale)*width/2.0 + sin((sum+pair[i].value/2.0)*scale)*extra,
        cos((sum+arc+M_PI_4)*scale)*width/2.0 + cos((sum+pair[i].value/2.0)*scale)*extra,
        sin((sum+arc+M_PI_4)*scale)*width/2.0 + sin((sum+pair[i].value/2.0)*scale)*extra,
        width/2.0
        );
      }
      printf
      (
      "arc from last box.c + (%f,%f) to last box.c + (%f,%f) rad %f\n",
      cos((sum+arc)*scale)*width/2.0 + cos((sum+pair[i].value/2.0)*scale)*extra,
      sin((sum+arc)*scale)*width/2.0 + sin((sum+pair[i].value/2.0)*scale)*extra,
      cos((sum+pair[i].value)*scale)*width/2.0 + cos((sum+pair[i].value/2.0)*scale)*extra,
      sin((sum+pair[i].value)*scale)*width/2.0 + sin((sum+pair[i].value/2.0)*scale)*extra,
      width/2.0
      );

      printf
      (
      "line from last box.c + (%f,%f) to last box.c + (%f,%f)\n",
      cos((sum+pair[i].value/2.0)*scale)*extra,
      sin((sum+pair[i].value/2.0)*scale)*extra,
      cos(sum*scale)*width/2.0+cos((sum+pair[i].value/2.0)*scale)*extra,
      sin(sum*scale)*width/2.0+sin((sum+pair[i].value/2.0)*scale)*extra
      );

      printf
      (
      "line from last box.c + (%f,%f) to last box.c + (%f,%f)\n",
      cos((sum+pair[i].value/2.0)*scale)*extra,
      sin((sum+pair[i].value/2.0)*scale)*extra,
      cos((sum+pair[i].value)*scale)*width/2.0+cos((sum+pair[i].value/2.0)*scale)*extra,
      sin((sum+pair[i].value)*scale)*width/2.0+sin((sum+pair[i].value/2.0)*scale)*extra
      );

      printf
      (
      "\"%s\" at last box.c + (%f,%f)\n",
      pair[i].label,
      cos((sum+pair[i].value/2.0)*scale)*width*0.35,
      sin((sum+pair[i].value/2.0)*scale)*width*0.35
      );
    }
    printf("]"); if (nl) printf("\n");
  }
  /*}}}*/
  return 0;
}
/*}}}*/
