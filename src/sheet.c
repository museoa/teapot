/* #includes */ /*{{{C}}}*//*{{{*/
#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "csv.h"
#include "default.h"
#include "display.h"
#include "eval.h"
#include "main.h"
#include "misc.h"
#include "parser.h"
#include "scanner.h"
#include "sheet.h"
#include "utf8.h"
#include "xdr.h"

/*}}}*/
/* #defines */ /*{{{*/
#define SHEET(s,x,y,z) (*(s->sheet+(x)*s->dimz*s->dimy+(y)*s->dimz+(z)))

#define HASH(x,s) \
{ \
  const unsigned char *S=(const unsigned char*)s; \
  \
  x=0; \
  while (*S) { x=(x<<5)+((x>>27)^*S); ++S; } \
  x%=LABEL_CACHE; \
}

#define SHADOWED(sheet,x,y,z) (x<sheet->dimx && y<sheet->dimy && z<sheet->dimz && SHEET(sheet,x,y,z)!=(Cell*)0 && SHEET(sheet,x,y,z)->shadowed)
/*}}}*/

/* variables */ /*{{{*/
static int upd_clock; /* evaluate clocked expressions */
/* Used during evaluation of a cell to specify the currently updated cell */
Sheet *upd_sheet;
int upd_x;
int upd_y;
int upd_z;
int max_eval;
/*}}}*/

/* copycell      -- copy a cell */ /*{{{*/
static void copycell(Sheet *sheet1, int x1, int y1, int z1, Sheet *sheet2, int x2, int y2, int z2)
{
  /* variables */ /*{{{*/
  Token **run;
  int i,len;
  /*}}}*/

  assert(sheet1!=(Sheet*)0);
  assert(sheet2!=(Sheet*)0);
  if (x1<sheet1->dimx && y1<sheet1->dimy && z1<sheet1->dimz)
  {
    sheet2->changed=1;
    if (SHEET(sheet1,x1,y1,z1)==(Cell*)0) freecell(sheet2,x2,y2,z2);
    else
    /* copy first cell to second */ /*{{{*/
    {
      freecell(sheet2,x2,y2,z2);
      initcell(sheet2,x2,y2,z2);
      memcpy(SHEET(sheet2,x2,y2,z2),SHEET(sheet1,x1,y1,z1),sizeof(Cell));
      if (SHEET(sheet1,x1,y1,z1)->contents!=(Token**)0)
      {
        for (len=1,run=SHEET(sheet1,x1,y1,z1)->contents; *run!=(Token*)0; ++len,++run);
        SHEET(sheet2,x2,y2,z2)->contents=malloc(len*sizeof(Token*));
        for (i=0; i<len; ++i)
        {
          if (*(SHEET(sheet1,x1,y1,z1)->contents+i)==(Token*)0) *(SHEET(sheet2,x2,y2,z2)->contents+i)=(Token*)0;
          else
          {
            *(SHEET(sheet2,x2,y2,z2)->contents+i)=malloc(sizeof(Token));
            **(SHEET(sheet2,x2,y2,z2)->contents+i)=tcopy(**(SHEET(sheet1,x1,y1,z1)->contents+i));
          }
        }
      }
      if (SHEET(sheet1,x1,y1,z1)->ccontents!=(Token**)0)
      {
        for (len=1,run=SHEET(sheet1,x1,y1,z1)->ccontents; *run!=(Token*)0; ++len,++run);
        SHEET(sheet2,x2,y2,z2)->ccontents=malloc(len*sizeof(Token*));
        for (i=0; i<len; ++i)
        {
          if (*(SHEET(sheet1,x1,y1,z1)->ccontents+i)==(Token*)0) *(SHEET(sheet2,x2,y2,z2)->ccontents+i)=(Token*)0;
          else
          {
            *(SHEET(sheet2,x2,y2,z2)->ccontents+i)=malloc(sizeof(Token));
            **(SHEET(sheet2,x2,y2,z2)->ccontents+i)=tcopy(**(SHEET(sheet1,x1,y1,z1)->ccontents+i));
          }
        }
      }
      if (SHEET(sheet1,x1,y1,z1)->label!=(char*)0) SHEET(sheet2,x2,y2,z2)->label=strcpy(malloc(strlen(SHEET(sheet1,x1,y1,z1)->label)+1),SHEET(sheet1,x1,y1,z1)->label);
      SHEET(sheet2,x2,y2,z2)->value.type=EMPTY;
    }
    /*}}}*/
  }
  else freecell(sheet2,x2,y2,z2);
}
/*}}}*/
/* swapblock     -- swap two non-overlapping blocks of cells */ /*{{{*/
static void swapblock(Sheet *sheet1, int x1, int y1, int z1, Sheet *sheet2, int x2, int y2, int z2, int xdist, int ydist, int zdist)
{
  int xoff, yoff, zoff;

  assert(sheet1!=(Sheet*)0);
  assert(sheet2!=(Sheet*)0);
  resize(sheet1,x1+xdist-1,y1+ydist-1,z1+zdist-1);
  resize(sheet2,x2+xdist-1,y2+ydist-1,z2+zdist-1);
  for (xoff=0; xoff<xdist; ++xoff)
  for (yoff=0; yoff<ydist; ++yoff)
  for (zoff=0; zoff<zdist; ++zoff)
  {
    Cell *t;

    t=SHEET(sheet1,x1+xoff,y1+yoff,z1+zoff);
    SHEET(sheet1,x1+xoff,y1+yoff,z1+zoff)=SHEET(sheet2,x2+xoff,y2+yoff,z2+zoff);
    SHEET(sheet2,x2+xoff,y2+yoff,z2+zoff)=t;
  }
  sheet1->changed=1;
  sheet2->changed=1;
}
/*}}}*/
/* cmpcell       -- compare to cells with given order flags */ /*{{{*/
/* Notes */ /*{{{*/
/*
Compare the _values_ of two cells.  The result is -1 if first is smaller
than second, 0 if they are equal and 1 if the first is bigger than the
second.  A result of 2 means they are not comparable.
*/
/*}}}*/
static int cmpcell(Sheet *sheet1, int x1, int y1, int z1, Sheet *sheet2, int x2, int y2, int z2, int sortkey)
{
  assert(sheet1!=(Sheet*)0);
  assert(sheet2!=(Sheet*)0);
  /* empty cells are smaller than any non-empty cell */ /*{{{*/
  if (x1>=sheet1->dimx || y1>=sheet1->dimy || z1>=sheet1->dimz || SHEET(sheet1,x1,y1,z1)==(Cell*)0 || SHEET(sheet1,x1,y1,z1)->value.type==EMPTY)
  {
    if (x2>=sheet2->dimx || y2>=sheet2->dimy || z2>=sheet2->dimz || SHEET(sheet2,x2,y2,z2)==(Cell*)0 || SHEET(sheet2,x2,y2,z2)->value.type==EMPTY) return 0;
    else return (sortkey&ASCENDING ? -1 : 1);
  }
  if (x2>=sheet2->dimx || y2>=sheet2->dimy || z2>=sheet2->dimz || SHEET(sheet2,x2,y2,z2)==(Cell*)0 || SHEET(sheet2,x2,y2,z2)->value.type==EMPTY) return (sortkey&ASCENDING ? 1 : -1);
  /*}}}*/
  switch (SHEET(sheet1,x1,y1,z1)->value.type)
  {
    /* STRING */ /*{{{*/
    case STRING:
    {
      if (SHEET(sheet2,x2,y2,z2)->value.type==STRING)
      {
        int r;

        r=strcmp(SHEET(sheet1,x1,y1,z1)->value.u.string,SHEET(sheet2,x2,y2,z2)->value.u.string);
        if (r<0) return (sortkey&ASCENDING ? -1 : 1);
        else if (r==0) return 0;
        else return (sortkey&ASCENDING ? 1 : -1);
      }
      return 2;
    }
    /*}}}*/
    /* FLOAT */ /*{{{*/
    case FLOAT:
    {
      if (SHEET(sheet2,x2,y2,z2)->value.type==FLOAT)
      {
        if (SHEET(sheet1,x1,y1,z1)->value.u.flt<SHEET(sheet2,x2,y2,z2)->value.u.flt) return (sortkey&ASCENDING ? -1 : 1);
        else if (SHEET(sheet1,x1,y1,z1)->value.u.flt==SHEET(sheet2,x2,y2,z2)->value.u.flt) return 0;
        else return (sortkey&ASCENDING ? 1 : -1);
      }
      if (SHEET(sheet2,x2,y2,z2)->value.type==INT)
      {
        if (SHEET(sheet1,x1,y1,z1)->value.u.flt<SHEET(sheet2,x2,y2,z2)->value.u.integer) return (sortkey&ASCENDING ? -1 : 1);
        else if (SHEET(sheet1,x1,y1,z1)->value.u.flt==SHEET(sheet2,x2,y2,z2)->value.u.integer) return 0;
        else return (sortkey&ASCENDING ? 1 : -1);
      }
      return 2;
    }
    /*}}}*/
    /* INT */ /*{{{*/
    case INT:
    {
      if (SHEET(sheet2,x2,y2,z2)->value.type==INT)
      {
        if (SHEET(sheet1,x1,y1,z1)->value.u.integer<SHEET(sheet2,x2,y2,z2)->value.u.integer) return (sortkey&ASCENDING ? -1 : 1);
        else if (SHEET(sheet1,x1,y1,z1)->value.u.integer==SHEET(sheet2,x2,y2,z2)->value.u.integer) return 0;
        else return (sortkey&ASCENDING ? 1 : -1);
      }
      if (SHEET(sheet2,x2,y2,z2)->value.type==FLOAT)
      {
        if (SHEET(sheet1,x1,y1,z1)->value.u.integer<SHEET(sheet2,x2,y2,z2)->value.u.flt) return (sortkey&ASCENDING ? -1 : 1);
        else if (SHEET(sheet1,x1,y1,z1)->value.u.integer==SHEET(sheet2,x2,y2,z2)->value.u.flt) return 0;
        else return (sortkey&ASCENDING ? 1 : -1);
      }
      return 2;
    }
    /*}}}*/
    default: return 2;
  }
}
/*}}}*/

/* resize        -- check if sheet needs to be resized in any dimension */ /*{{{*/
void resize(Sheet *sheet, int x, int y, int z)
{
  assert(x>=0);
  assert(y>=0);
  assert(z>=0);
  assert(sheet!=(Sheet*)0);
  if (x>=sheet->dimx || y>=sheet->dimy || z>=sheet->dimz)
  {
    /* variables */ /*{{{*/
    Cell **newsheet;
    int *newcolumn;
    unsigned int ndimx,ndimy,ndimz;
    /*}}}*/

    sheet->changed=1;
    ndimx=(x>=sheet->dimx ? x+1 : sheet->dimx);
    ndimy=(y>=sheet->dimy ? y+1 : sheet->dimy);
    ndimz=(z>=sheet->dimz ? z+1 : sheet->dimz);
    /* allocate new sheet */ /*{{{*/
    newsheet=malloc(ndimx*ndimy*ndimz*sizeof(Cell*));
    for (x=0; x<ndimx; ++x) for (y=0; y<ndimy; ++y) for (z=0; z<ndimz; ++z)
    {
      if (x<sheet->dimx && y<sheet->dimy && z<sheet->dimz) *(newsheet+x*ndimz*ndimy+y*ndimz+z)=SHEET(sheet,x,y,z);
      else *(newsheet+x*ndimz*ndimy+y*ndimz+z)=(Cell*)0;
    }
    if (sheet->sheet!=(Cell**)0) free(sheet->sheet);
    sheet->sheet=newsheet;
    /*}}}*/
    /* allocate new columns */ /*{{{*/
    if (x>sheet->dimx || z>=sheet->dimz)
    {
      newcolumn=malloc(ndimx*ndimz*sizeof(int));
      for (x=0; x<ndimx; ++x) for (z=0; z<ndimz; ++z)
      {
        if (x<sheet->dimx && z<sheet->dimz) *(newcolumn+x*ndimz+z)=*(sheet->column+x*sheet->dimz+z);
        else *(newcolumn+x*ndimz+z)=DEF_COLUMNWIDTH;
      }
      if (sheet->column!=(int*)0) free(sheet->column);
      sheet->column=newcolumn;
    }
    /*}}}*/
    sheet->dimx=ndimx;
    sheet->dimy=ndimy;
    sheet->dimz=ndimz;
  }
}
/*}}}*/
/* initcell      -- initialise new cell, if it does not exist yet */ /*{{{*/
void initcell(Sheet *sheet, int x, int y, int z)
{
  assert(x>=0);
  assert(y>=0);
  assert(z>=0);
  resize(sheet,x,y,z);
  if (SHEET(sheet,x,y,z)==(Cell*)0)
  {
    sheet->changed=1;
    SHEET(sheet,x,y,z)=malloc(sizeof(Cell));
    SHEET(sheet,x,y,z)->contents=(Token**)0;
    SHEET(sheet,x,y,z)->ccontents=(Token**)0;
    SHEET(sheet,x,y,z)->label=(char*)0;
    SHEET(sheet,x,y,z)->adjust=AUTOADJUST;
    SHEET(sheet,x,y,z)->precision=-1;
    SHEET(sheet,x,y,z)->shadowed=0;
    SHEET(sheet,x,y,z)->bold=0;
    SHEET(sheet,x,y,z)->underline=0;
    SHEET(sheet,x,y,z)->scientific=DEF_SCIENTIFIC;
    SHEET(sheet,x,y,z)->value.type=EMPTY;
    SHEET(sheet,x,y,z)->resvalue.type=EMPTY;
    SHEET(sheet,x,y,z)->locked=0;
    SHEET(sheet,x,y,z)->ignored=0;
    SHEET(sheet,x,y,z)->clock_t0=0;
    SHEET(sheet,x,y,z)->clock_t1=0;
    SHEET(sheet,x,y,z)->clock_t2=0;
  }
}
/*}}}*/
/* cachelabels   -- create new label cache */ /*{{{*/
void cachelabels(Sheet *sheet)
{
  int i,x,y,z;

  if (sheet==(Sheet*)0) return;
  for (i=0; i<LABEL_CACHE; ++i) /* free bucket */ /*{{{*/
  {
    struct Label *run;

    for (run=sheet->labelcache[i]; run!=(struct Label*)0;)
    {
      struct Label *runnext;

      runnext=run->next;
      free(run);
      run=runnext;
    }
    sheet->labelcache[i]=(struct Label*)0;
  }
  /*}}}*/
  for (x=0; x<sheet->dimx; ++x) for (y=0; y<sheet->dimy; ++y) for (z=0; z<sheet->dimz; ++z)
  /* cache all labels */ /*{{{*/
  {
    const char *l;

    l=getlabel(sheet,x,y,z);
    if (*l)
    {
      unsigned long hx;
      struct Label **run;

      HASH(hx,l);
      for (run=&sheet->labelcache[(unsigned int)hx]; *run!=(struct Label*)0 && strcmp(l,(*run)->label); run=&((*run)->next));
      if (*run==(struct Label*)0)
      {
        *run=malloc(sizeof(struct Label));
        (*run)->next=(struct Label*)0;
        (*run)->label=l;
        (*run)->x=x;
        (*run)->y=y;
        (*run)->z=z;
      }
      /* else we have a duplicate label, which _can_ happen under */
      /* unfortunate conditions.  Don't tell anybody. */
    }
  }
  /*}}}*/
}
/*}}}*/
/* freesheet     -- free all cells of an entire spread sheet */ /*{{{*/
void freesheet(Sheet *sheet, int all)
{
  /* variables */ /*{{{*/
  int x,y,z;
  /*}}}*/

  assert(sheet!=(Sheet*)0);
  sheet->changed=0;
  for (x=0; x<sheet->dimx; ++x) for (y=0; y<sheet->dimy; ++y) for (z=0; z<sheet->dimz; ++z)
  {
    freecell(sheet,x,y,z);
  }
  if (all)
  {
    int i;

    for (i=0; i<LABEL_CACHE; ++i) /* free all buckets */ /*{{{*/
    {
      struct Label *run;

      for (run=sheet->labelcache[i]; run!=(struct Label*)0;)
      {
        struct Label *runnext;

        runnext=run->next;
        free(run);
        run=runnext;
      }
    }
    /*}}}*/
    if (sheet->sheet) free(sheet->sheet);
    if (sheet->column) free(sheet->column);
    if (sheet->name) free(sheet->name);
  }
  else
  {
    for (x=0; x<sheet->dimx; ++x) for (z=0; z<sheet->dimz; ++z)
    {
      *(sheet->column+x*sheet->dimz+z)=DEF_COLUMNWIDTH;
    }
    cachelabels(sheet);
    forceupdate(sheet);
  }
}
/*}}}*/
/* forceupdate   -- clear all clock and update flags */ /*{{{*/
void forceupdate(Sheet *sheet)
{
  int i;

  assert(sheet!=(Sheet*)0);
  for (i=0; i<sheet->dimx*sheet->dimy*sheet->dimz; ++i) if (*(sheet->sheet+i)!=(Cell*)0)
  {
    (*(sheet->sheet+i))->updated=0;
    (*(sheet->sheet+i))->clock_t0=0;
    (*(sheet->sheet+i))->clock_t1=0;
    (*(sheet->sheet+i))->clock_t2=0;
  }
  update(sheet);
}
/*}}}*/
/* freecell      -- free one cell */ /*{{{*/
void freecell(Sheet *sheet, int x, int y, int z)
{
  assert(sheet!=(Sheet*)0);
  if (sheet->sheet!=(Cell**)0 && x<sheet->dimx && y<sheet->dimy && z<sheet->dimz && SHEET(sheet,x,y,z)!=(Cell*)0)
  {
    tvecfree(SHEET(sheet,x,y,z)->contents);
    tvecfree(SHEET(sheet,x,y,z)->ccontents);
    tfree(&(SHEET(sheet,x,y,z)->value));
    free(SHEET(sheet,x,y,z));
    SHEET(sheet,x,y,z)=(Cell*)0;
    sheet->changed=1;
  }
}
/*}}}*/
/* columnwidth   -- get width of column */ /*{{{*/
int columnwidth(Sheet *sheet, int x, int z)
{
  assert(sheet!=(Sheet*)0);
  if (x<sheet->dimx && z<sheet->dimz) return (*(sheet->column+x*sheet->dimz+z));
  else return DEF_COLUMNWIDTH;
}
/*}}}*/
/* setwidth      -- set width of column */ /*{{{*/
void setwidth(Sheet *sheet, int x, int z, int width)
{
  assert(sheet!=(Sheet*)0);
  resize(sheet,x,1,z);
  sheet->changed=1;
  *(sheet->column+x*sheet->dimz+z)=width;
}
/*}}}*/
/* cellwidth     -- get width of a cell */ /*{{{*/
int cellwidth(Sheet *sheet, int x, int y, int z)
{
  int width;

  if (SHADOWED(sheet,x,y,z)) return 0;
  width=columnwidth(sheet,x,z);
  for (++x; SHADOWED(sheet,x,y,z); width+=columnwidth(sheet,x,z),++x);
  return width;
}
/*}}}*/
/* putcont       -- assign new contents */ /*{{{*/
void putcont(Sheet *sheet, int x, int y, int z, Token **t, int c)
{
  assert(sheet!=(Sheet*)0);
  sheet->changed=1;
  resize(sheet,x,y,z);
  initcell(sheet,x,y,z);
  if (c)
  {
    tvecfree(SHEET(sheet,x,y,z)->ccontents);
    SHEET(sheet,x,y,z)->ccontents=t;
  }
  else
  {
    tvecfree(SHEET(sheet,x,y,z)->contents);
    SHEET(sheet,x,y,z)->contents=t;
  }
  redraw_cell(sheet, x, y, z);
}
/*}}}*/
/* getcont       -- get contents */ /*{{{*/
Token **getcont(Sheet *sheet, int x, int y, int z, int c)
{
  if (x>=sheet->dimx || y>=sheet->dimy || z>=sheet->dimz) return (Token**)0;
  else if (SHEET(sheet,x,y,z)==(Cell*)0) return (Token**)0;
  else if (c==2) return (SHEET(sheet,x,y,z)->clock_t0 && SHEET(sheet,x,y,z)->ccontents ? SHEET(sheet,x,y,z)->ccontents : SHEET(sheet,x,y,z)->contents);
  else return (c ? SHEET(sheet,x,y,z)->ccontents : SHEET(sheet,x,y,z)->contents);
}
/*}}}*/
/* getvalue      -- get tcopy()ed value */ /*{{{*/
Token getvalue(Sheet *sheet, int x, int y, int z)
{
  /* variables */ /*{{{*/
  Token result;
  int orig_upd_clock;
  /*}}}*/

  assert(sheet!=(Sheet*)0);
  if (x<0 || y<0 || z<0)
  /* return error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=mystrmalloc(_("Negative index"));
  }
  /*}}}*/
  else if (getcont(sheet,x,y,z,2)==(Token**)0)
  /* return empty value */ /*{{{*/
  result.type=EMPTY;
  /*}}}*/
  else
  /* update value of this cell if needed and return it */ /*{{{*/
  {
    orig_upd_clock=upd_clock;
    if (SHEET(sheet,x,y,z)->ignored)
    {
      /* variables */ /*{{{*/
      Token oldvalue;
      /*}}}*/

      oldvalue=SHEET(sheet,x,y,z)->value;
      SHEET(sheet,x,y,z)->updated=1;
      SHEET(sheet,x,y,z)->value.type=EMPTY;
      tfree(&oldvalue);
    }
    else if (SHEET(sheet,x,y,z)->updated==0)
    {
      /* variables */ /*{{{*/
      Sheet *old_sheet;
      int old_x,old_y,old_z,old_max_eval;
      Token oldvalue;
      /*}}}*/

      old_sheet=upd_sheet;
      old_x=upd_x;
      old_y=upd_y;
      old_z=upd_z;
      old_max_eval=max_eval;
      upd_sheet=sheet;
      upd_x=x;
      upd_y=y;
      upd_z=z;
      max_eval=MAX_EVALNEST;
      if (SHEET(sheet,x,y,z)->clock_t1==0)
      {
        SHEET(sheet,x,y,z)->updated=1;
        oldvalue=SHEET(sheet,x,y,z)->value;
        upd_clock=0;
        SHEET(sheet,x,y,z)->value=eval(getcont(sheet,x,y,z,2));
        tfree(&oldvalue);
      }
      else if (upd_clock)
      {
        SHEET(sheet,x,y,z)->updated=1;
        upd_clock=0;
        SHEET(sheet,x,y,z)->resvalue=eval(getcont(sheet,x,y,z,2));
      }
      upd_sheet=old_sheet;
      upd_x=old_x;
      upd_y=old_y;
      upd_z=old_z;
      max_eval=old_max_eval;
    }
    if (!orig_upd_clock) result=tcopy(SHEET(sheet,x,y,z)->value);
  }
  /*}}}*/
  return result;
}
/*}}}*/
/* update        -- update all cells that need it */ /*{{{*/
void update(Sheet *sheet)
{
  int x,y,z,kp,iterating;

  assert(sheet!=(Sheet*)0);
  kp=0;
  iterating=0;
  do
  {
    sheet->clk=0;
    if (iterating==1)
    {
      line_msg((const char*)0,_("Calculating running, press Escape to abort it"));
      ++iterating;
    }
    else if (iterating==0) ++iterating;
    for (x=0; x<sheet->dimx; ++x) for (y=0; y<sheet->dimy; ++y) for (z=0; z<sheet->dimz; ++z)
    {
      if (SHEET(sheet,x,y,z) && SHEET(sheet,x,y,z)->clock_t2)
      {
        SHEET(sheet,x,y,z)->updated=0;
        SHEET(sheet,x,y,z)->clock_t0=1;
        SHEET(sheet,x,y,z)->clock_t1=1;
        SHEET(sheet,x,y,z)->clock_t2=0;
      }
    }
    for (x=0; x<sheet->dimx; ++x) for (y=0; y<sheet->dimy; ++y) for (z=0; z<sheet->dimz; ++z)
    {
      upd_clock=1;
      getvalue(sheet,x,y,z);
    }
    for (x=0; x<sheet->dimx; ++x) for (y=0; y<sheet->dimy; ++y) for (z=0; z<sheet->dimz; ++z)
    {
      if (SHEET(sheet,x,y,z) && SHEET(sheet,x,y,z)->clock_t1)
      {
        tfree(&(SHEET(sheet,x,y,z)->value));
        SHEET(sheet,x,y,z)->value=SHEET(sheet,x,y,z)->resvalue;
        SHEET(sheet,x,y,z)->clock_t1=0;
      }
    }
    upd_clock=0;
  } while (sheet->clk && !(kp=keypressed()));
  if (iterating==2) line_msg((const char*)0,kp ? _("Calculation aborted") : _("Calculation finished"));
  sheet->clk=0;
  redraw_sheet(sheet);
}
/*}}}*/
/* geterror      -- get malloc()ed error string */ /*{{{*/
char *geterror(Sheet *sheet, int x, int y, int z)
{
  Token v;

  assert(sheet!=(Sheet*)0);
  if ((v=getvalue(sheet,x,y,z)).type!=EEK)
  {
    tfree(&v);
    return (char*)0;
  }
  else
  {
    return (v.u.err);
  }
}
/*}}}*/
/* printvalue    -- get ASCII representation of value */ /*{{{*/
void printvalue(char *s, size_t size, size_t chars, int quote, int scientific, int precision, Sheet *sheet, int x, int y, int z)
{
  Token *tv[2],t;

  assert(sheet!=(Sheet*)0);
  t=getvalue(sheet,x,y,z); tv[0]=&t;
  tv[1]=(Token*)0;
  print(s,size,chars,quote,scientific,precision,tv);
  tfree(&t);
}
/*}}}*/
/* getadjust     -- get cell adjustment */ /*{{{*/
Adjust getadjust(Sheet *sheet, int x, int y, int z)
{
  assert(sheet!=(Sheet*)0);
  if (x>=sheet->dimx || y>=sheet->dimy || z>=sheet->dimz || SHEET(sheet,x,y,z)==(Cell*)0)
  {
    return LEFT;
  }
  else if (SHEET(sheet,x,y,z)->adjust==AUTOADJUST) return (SHEET(sheet,x,y,z)->value.type==INT || SHEET(sheet,x,y,z)->value.type==FLOAT ? RIGHT : LEFT);
  else return (SHEET(sheet,x,y,z)->adjust);
}
/*}}}*/
/* setadjust     -- set cell adjustment */ /*{{{*/
void setadjust(Sheet *sheet, int x, int y, int z, Adjust adjust)
{
  assert(sheet!=(Sheet*)0);
  sheet->changed=1;
  resize(sheet,x,y,z);
  initcell(sheet,x,y,z);
  SHEET(sheet,x,y,z)->adjust=adjust;
}
/*}}}*/

/* shadow        -- shadow cell by left neighbour */ /*{{{*/
void shadow(Sheet *sheet, int x, int y, int z, int yep)
{
  sheet->changed=1;
  initcell(sheet,x,y,z);
  SHEET(sheet,x,y,z)->shadowed=yep;
}
/*}}}*/
/* shadowed      -- is cell shadowed? */ /*{{{*/
int shadowed(Sheet *sheet, int x, int y, int z)
{
  return (SHADOWED(sheet,x,y,z));
}
/*}}}*/
/* bold        -- bold font */ /*{{{*/
void bold(Sheet *sheet, int x, int y, int z, int yep)
{
  sheet->changed=1;
  initcell(sheet,x,y,z);
  SHEET(sheet,x,y,z)->bold=yep;
}
/*}}}*/
/* isbold      -- is cell bold? */ /*{{{*/
int isbold(Sheet *sheet, int x, int y, int z)
{
  return (x<sheet->dimx && y<sheet->dimy && z<sheet->dimz && SHEET(sheet,x,y,z)!=(Cell*)0 && SHEET(sheet,x,y,z)->bold);
}
/*}}}*/
/* underline        -- underline */ /*{{{*/
void underline(Sheet *sheet, int x, int y, int z, int yep)
{
  sheet->changed=1;
  initcell(sheet,x,y,z);
  SHEET(sheet,x,y,z)->underline=yep;
}
/*}}}*/
/* isunderline      -- is cell underlined? */ /*{{{*/
int underlined(Sheet *sheet, int x, int y, int z)
{
  return (x<sheet->dimx && y<sheet->dimy && z<sheet->dimz && SHEET(sheet,x,y,z)!=(Cell*)0 && SHEET(sheet,x,y,z)->underline);
}
/*}}}*/
/* lockcell      -- lock cell */ /*{{{*/
void lockcell(Sheet *sheet, int x, int y, int z, int yep)
{
  sheet->changed=1;
  initcell(sheet,x,y,z);
  SHEET(sheet,x,y,z)->locked=yep;
}
/*}}}*/
/* locked        -- is cell locked? */ /*{{{*/
int locked(Sheet *sheet, int x, int y, int z)
{
  return (x<sheet->dimx && y<sheet->dimy && z<sheet->dimz && SHEET(sheet,x,y,z)!=(Cell*)0 && SHEET(sheet,x,y,z)->locked);
}
/*}}}*/
/* transparent   -- is cell transparent? */ /*{{{*/
int transparent(Sheet *sheet, int x, int y, int z)
{
  return (x<sheet->dimx && y<sheet->dimy && z<sheet->dimz && SHEET(sheet,x,y,z)!=(Cell*)0 && SHEET(sheet,x,y,z)->transparent);
}
/*}}}*/
/* maketrans     -- make cell transparent */ /*{{{*/
void maketrans(Sheet *sheet, int x, int y, int z, int yep)
{
  sheet->changed=1;
  initcell(sheet,x,y,z);
  SHEET(sheet,x,y,z)->transparent=yep;
}
/*}}}*/
/* igncell       -- ignore cell */ /*{{{*/
void igncell(Sheet *sheet, int x, int y, int z, int yep)
{
  sheet->changed=1;
  initcell(sheet,x,y,z);
  SHEET(sheet,x,y,z)->ignored=yep;
}
/*}}}*/
/* ignored       -- is cell ignored? */ /*{{{*/
int ignored(Sheet *sheet, int x, int y, int z)
{
  return (x<sheet->dimx && y<sheet->dimy && z<sheet->dimz && SHEET(sheet,x,y,z)!=(Cell*)0 && SHEET(sheet,x,y,z)->ignored);
}
/*}}}*/
/* clk           -- clock cell */ /*{{{*/
void clk(Sheet *sheet, int x, int y, int z)
{
  assert(sheet!=(Sheet*)0);
  assert(x>=0 && x<sheet->dimx);
  assert(y>=0 && y<sheet->dimy);
  assert(z>=0 && z<sheet->dimz);
  if (SHEET(sheet,x,y,z))
  {
    SHEET(sheet,x,y,z)->clock_t2=1;
    sheet->clk=1;
  }
}
/*}}}*/
/* setscientific -- cell value should be displayed in scientific notation */ /*{{{*/
void setscientific(Sheet *sheet, int x, int y, int z, int yep)
{
  sheet->changed=1;
  resize(sheet,x,y,z);
  initcell(sheet,x,y,z);
  SHEET(sheet,x,y,z)->scientific=yep;
}
/*}}}*/
/* getscientific -- should value be displayed in scientific notation? */ /*{{{*/
int getscientific(Sheet *sheet, int x, int y, int z)
{
  if (x<sheet->dimx && y<sheet->dimy && z<sheet->dimz && SHEET(sheet,x,y,z)!=(Cell*)0) return SHEET(sheet,x,y,z)->scientific;
  else return DEF_SCIENTIFIC;
}
/*}}}*/
/* setprecision  -- set cell precision */ /*{{{*/
void setprecision(Sheet *sheet, int x, int y, int z, int precision)
{
  sheet->changed=1;
  resize(sheet,x,y,z);
  initcell(sheet,x,y,z);
  SHEET(sheet,x,y,z)->precision=precision;
}
/*}}}*/
/* getprecision  -- get cell precision */ /*{{{*/
int getprecision(Sheet *sheet, int x, int y, int z)
{
  if (x<sheet->dimx && y<sheet->dimy && z<sheet->dimz && SHEET(sheet,x,y,z)!=(Cell*)0) return (SHEET(sheet,x,y,z)->precision==-1 ? def_precision : SHEET(sheet,x,y,z)->precision);
  else return def_precision;
}
/*}}}*/
/* getlabel      -- get cell label */ /*{{{*/
const char *getlabel(Sheet *sheet, int x, int y, int z)
{
  if (x>=sheet->dimx || y>=sheet->dimy || z>=sheet->dimz || SHEET(sheet,x,y,z)==(Cell*)0 || SHEET(sheet,x,y,z)->label==(char*)0) return "";
  else return (SHEET(sheet,x,y,z)->label);
}
/*}}}*/
/* setlabel      -- set cell label */ /*{{{*/
void setlabel(Sheet *sheet, int x, int y, int z, const char *buf, int update)
{
  sheet->changed=1;
  resize(sheet,x,y,z);
  initcell(sheet,x,y,z);
  if (SHEET(sheet,x,y,z)->label!=(char*)0) free(SHEET(sheet,x,y,z)->label);
  if (*buf!='\0') SHEET(sheet,x,y,z)->label=strcpy(malloc(strlen(buf)+1),buf);
  else SHEET(sheet,x,y,z)->label=(char*)0;
  if (update)
  {
    cachelabels(sheet);
    forceupdate(sheet);
  }
}
/*}}}*/
/* findlabel     -- return cell location for a given label */ /*{{{*/
Token findlabel(Sheet *sheet, const char *label)
{
  /* variables */ /*{{{*/
  Token result;
  unsigned long hx;
  struct Label *run;
  /*}}}*/

  assert(sheet!=(Sheet*)0);
/*
  if (sheet==(Sheet*)0) run=(struct Label*)0;
  else
*/
  {
    HASH(hx,label);
    for (run=sheet->labelcache[(unsigned int)hx]; run!=(struct Label*)0 && strcmp(label,run->label); run=run->next);
  }
  if (run)
  {
    result.type=LOCATION;
    result.u.location[0]=run->x;
    result.u.location[1]=run->y;
    result.u.location[2]=run->z;
  }
  else
  {
    result.type=EEK;
    result.u.err=mystrmalloc(_("No such label"));
  }
  return result;
}
/*}}}*/
/* relabel       -- search and replace for labels */ /*{{{*/
void relabel(Sheet *sheet, const char *oldlabel, const char *newlabel, int x, int y, int z)
{
  /* variables */ /*{{{*/
  Token **run;
  /*}}}*/

  /* asserts */ /*{{{*/
  assert(sheet!=(Sheet*)0);
  assert(oldlabel!=(const char*)0);
  assert(newlabel!=(const char*)0);
  assert(x>=0);
  assert(y>=0);
  assert(z>=0);
  /*}}}*/
  if (!(x>=sheet->dimx || y>=sheet->dimy || z>=sheet->dimz || SHEET(sheet,x,y,z)==(Cell*)0 || SHEET(sheet,x,y,z)->contents==(Token**)0))
  {
    for (run=SHEET(sheet,x,y,z)->contents; *run!=(Token*)0; ++run)
    {
      if ((*run)->type==LIDENT && strcmp((*run)->u.lident,oldlabel)==0)
      {
        free((*run)->u.lident);
        (*run)->u.lident=mystrmalloc(newlabel);
      }
    }
  }
  if (!(x>=sheet->dimx || y>=sheet->dimy || z>=sheet->dimz || SHEET(sheet,x,y,z)==(Cell*)0 || SHEET(sheet,x,y,z)->ccontents==(Token**)0))
  {
    for (run=SHEET(sheet,x,y,z)->ccontents; *run!=(Token*)0; ++run)
    {
      if ((*run)->type==LIDENT && strcmp((*run)->u.lident,oldlabel)==0)
      {
        free((*run)->u.lident);
        (*run)->u.lident=mystrmalloc(newlabel);
      }
    }
  }
  cachelabels(sheet);
  forceupdate(sheet);
}
/*}}}*/
/* savexdr       -- save a spread sheet in XDR */ /*{{{*/
const char *savexdr(Sheet *sheet, const char *name, unsigned int *count)
{
  /* variables */ /*{{{*/
  FILE *fp;
  XDR xdrs;
  int x,y,z;
  /*}}}*/

  *count=0;
  if ((fp=fopen(name,"w"))==(FILE*)0) return strerror(errno);
  xdrstdio_create(&xdrs,fp,XDR_ENCODE);
  if (!xdr_magic(&xdrs))
  {
    xdr_destroy(&xdrs);
    (void)fclose(fp);
    return strerror(errno);
  }
  for (x=sheet->dimx-1; x>=0; --x) for (z=sheet->dimz-1; z>=0; --z)
  {
    int width;
    int u;

    width=columnwidth(sheet,x,z);
    if (width!=DEF_COLUMNWIDTH)
    {
      u=0;
      if (xdr_int(&xdrs,&u)==0 || xdr_column(&xdrs,&x,&z,&width)==0)
      {
        xdr_destroy(&xdrs);
        (void)fclose(fp);
        return strerror(errno);
      }
    }
    for (y=sheet->dimy-1; y>=0; --y)
    {
      if (SHEET(sheet,x,y,z)!=(Cell*)0)
      {
        u=1;
        if (xdr_int(&xdrs,&u)==0 || xdr_int(&xdrs,&x)==0 || xdr_int(&xdrs,&y)==0 || xdr_int(&xdrs,&z)==0 || xdr_cell(&xdrs,SHEET(sheet,x,y,z))==0)
        {
          xdr_destroy(&xdrs);
          (void)fclose(fp);
          return strerror(errno);
        }
        ++*count;
      }
    }
  }
  xdr_destroy(&xdrs);
  if (fclose(fp)==EOF) return strerror(errno);
  sheet->changed=0;
  return (const char*)0;
}
/*}}}*/
/* savetbl       -- save as tbl tyble */ /*{{{*/
const char *savetbl(Sheet *sheet, const char *name, int body, int x1, int y1, int z1, int x2, int y2, int z2, unsigned int *count)
{
  /* variables */ /*{{{*/
  FILE *fp=(FILE*)0; /* cause run time error */
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
    else if (fputs_close(".TS\n",fp)==EOF) return strerror(errno);
    for (y=y1; y<=y2; ++y)
    {
      /* print format */ /*{{{*/
      if (y>y1 && fputs_close(".T&\n",fp)==EOF) return strerror(errno);
      for (x=x1; x<=x2; ++x)
      {
        if (x>x1 && fputc_close(' ',fp)==EOF) return strerror(errno);
        if (shadowed(sheet,x,y,z))
        {
          if (fputc_close('s',fp)==EOF) return strerror(errno);
        }
        if (isbold(sheet,x,y,z))
        {
          if (fputc_close('b',fp)==EOF) return strerror(errno);
        }
        if (underlined(sheet,x,y,z))
        {
          if (fputc_close('u',fp)==EOF) return strerror(errno);
        }
        else switch (getadjust(sheet,x,y,z))
        {
          case LEFT: if (fputc_close('l',fp)==EOF) return strerror(errno); break;
          case RIGHT: if (fputc_close('r',fp)==EOF) return strerror(errno); break;
          case CENTER: if (fputc_close('c',fp)==EOF) return strerror(errno); break;
          default: assert(0);
        }
      }
      if (fputs_close(".\n",fp)==EOF) return strerror(errno);
      /*}}}*/
      /* print contents */ /*{{{*/
      for (x=x1; x<=x2; ++x)
      {
        if (!shadowed(sheet,x,y,z))
        {
          if (x>x1 && fputc_close('\t',fp)==EOF) return strerror(errno);
          if (SHEET(sheet,x,y,z)!=(Cell*)0)
          {
            char *bufp;

            printvalue(buf,sizeof(buf),0,0,getscientific(sheet,x,y,z),getprecision(sheet,x,y,z),sheet,x,y,z);
            if (transparent(sheet,x,y,z))
            {
              if (fputs_close(buf,fp)==EOF) return strerror(errno);
            }
            else for (bufp=buf; *bufp; ++bufp) switch (*bufp)
            {
              case '\\':
              {
                if (fputc_close('\\',fp)==EOF || fputc_close('e',fp)==EOF) return strerror(errno);
                break;
              }
              case '_':
              {
                if (fputc_close('\\',fp)==EOF || fputc_close('&',fp)==EOF || fputc_close('_',fp)==EOF) return strerror(errno);
                break;
              }
              case '.':
              {
                if (x==x1 && bufp==buf && (fputc_close('\\',fp)==EOF || fputc_close('&',fp)==EOF)) return strerror(errno);
                if (fputc_close('.',fp)==EOF) return strerror(errno);
                break;
              }
              case '\'':
              {
                if (x==x1 && bufp==buf && (fputc_close('\\',fp)==EOF || fputc_close('&',fp)==EOF)) return strerror(errno);
                if (fputc_close('\'',fp)==EOF) return strerror(errno);
                break;
              }
              case '-':
              {
                if (*(bufp+1)=='-')
                {
                  if (fputc_close('-',fp)==EOF) return strerror(errno);
                  else ++bufp;
                }
                else if (fputs_close("\\-",fp)==EOF) return strerror(errno);
                break;
              }
              default: if (fputc_close(*bufp,fp)==EOF) return strerror(errno);
            }
          }
        }
      }
      if (fputc_close('\n',fp)==EOF) return strerror(errno);
      /*}}}*/
      ++*count;
    }
    if (!body)
    {
      if (fputs_close(".TE\n",fp)==EOF) return strerror(errno);
      if (z<z2 && fputs_close(".bp\n",fp)==EOF) return strerror(errno);
    }
    if (body && fclose(fp)==EOF) return strerror(errno);
  }
  if (!body && fclose(fp)==EOF) return strerror(errno);
  return (const char*)0;
}
/*}}}*/
/* savetext      -- save as text */ /*{{{*/
const char *savetext(Sheet *sheet, const char *name, int x1, int y1, int z1, int x2, int y2, int z2, unsigned int *count)
{
  /* variables */ /*{{{*/
  FILE *fp;
  int x,y,z;
  /*}}}*/

  /* asserts */ /*{{{*/
  assert(sheet!=(Sheet*)0);
  assert(name!=(const char*)0);
  /*}}}*/
  *count=0;
  for (z=z1; z<=z2; ++z) for (y=y1; y<=y2; ++y) if (shadowed(sheet,x1,y,z)) return _("Shadowed cells in first column");
  if ((fp=fopen(name,"w"))==(FILE*)0) return strerror(errno);
  for (z=z1; z<=z2; ++z)
  {
    for (y=y1; y<=y2; ++y)
    {
      size_t size,fill;

      for (x=x1; x<=x2; ++x)
      {
        size=cellwidth(sheet,x,y,z);
        if (SHEET(sheet,x,y,z)!=(Cell*)0)
        {
          char *buf;

          buf=malloc(size*UTF8SZ+1);
          printvalue(buf,size*UTF8SZ+1,size,0,getscientific(sheet,x,y,z),getprecision(sheet,x,y,z),sheet,x,y,z);
          adjust(getadjust(sheet,x,y,z),buf,size);
          if (fputs_close(buf,fp)==EOF)
          {
            free(buf);
            return strerror(errno);
          }
          for (fill=strlen(buf); fill<size; ++fill) if (fputc_close(' ',fp)==EOF)
          {
            free(buf);
            return strerror(errno);
          }
          free(buf);
        }
        else
        {
          for (fill=0; fill<size; ++fill) if (fputc_close(' ',fp)==EOF) return strerror(errno);
        }
        ++*count;
      }
      if (fputc_close('\n',fp)==EOF) return strerror(errno);
    }
    if (z<z2 && fputs_close("\f",fp)==EOF) return strerror(errno);
  }
  if (fclose(fp)==EOF) return strerror(errno);
  return (const char*)0;
}
/*}}}*/
/* savecsv       -- save as CSV */ /*{{{*/
const char *savecsv(Sheet *sheet, const char *name, char sep, int x1, int y1, int z1, int x2, int y2, int z2, unsigned int *count)
{
  /* variables */ /*{{{*/
  FILE *fp;
  int x,y,z;
  /*}}}*/

  /* asserts */ /*{{{*/
  assert(sheet!=(Sheet*)0);
  assert(name!=(const char*)0);
  /*}}}*/
  *count=0;
  for (z=z1; z<=z2; ++z) for (y=y1; y<=y2; ++y) if (shadowed(sheet,x1,y,z)) return _("Shadowed cells in first column");
  if ((fp=fopen(name,"w"))==(FILE*)0) return strerror(errno);
  for (z=z1; z<=z2; ++z)
  {
    for (y=y1; y<=y2; ++y)
    {
      for (x=x1; x<=x2; ++x)
      {
        if (x>x1) if (fputc_close(sep,fp)==EOF) return strerror(errno);
        if (SHEET(sheet,x,y,z)!=(Cell*)0)
        {
          char *buf,*s;

          buf=malloc(255*UTF8SZ+1);
          printvalue(buf,255*UTF8SZ+1,255,0,getscientific(sheet,x,y,z),getprecision(sheet,x,y,z),sheet,x,y,z);
          if (SHEET(sheet,x,y,z)->value.type==STRING && fputc_close('"',fp)==EOF)
          {
            free(buf);
            return strerror(errno);
          }
          for (s=buf; *s; ++s)
          {
            if (fputc_close(*s,fp)==EOF || (*s=='"' && fputc_close(*s,fp)==EOF))
            {
              free(buf);
              return strerror(errno);
            }
          }
          free(buf);
          if (SHEET(sheet,x,y,z)->value.type==STRING && fputc_close('"',fp)==EOF) return strerror(errno);
        }
        ++*count;
      }
      if (fputc_close('\n',fp)==EOF) return strerror(errno);
    }
    if (z<z2 && fputs_close("\f",fp)==EOF) return strerror(errno);
  }
  if (fclose(fp)==EOF) return strerror(errno);
  return (const char*)0;
}
/*}}}*/
/* saveport      -- save as portable text */ /*{{{*/
const char *saveport(Sheet *sheet, const char *name, unsigned int *count)
{
  /* variables */ /*{{{*/
  FILE *fp;
  int x,y,z;
  /*}}}*/

  /* asserts */ /*{{{*/
  assert(sheet!=(Sheet*)0);
  assert(name!=(const char*)0);
  /*}}}*/
  *count=0;
  if ((fp=fopen(name,"w"))==(FILE*)0) return strerror(errno);
  fprintf(fp,"# This is a work sheet generated with teapot %s.\n",VERSION);
  for (z=sheet->dimz-1; z>=0; --z)
  {
    for (y=sheet->dimy-1; y>=0; --y)
    {
      for (x=sheet->dimx-1; x>=0; --x)
      {
        if (y==0) if (columnwidth(sheet,x,z)!=DEF_COLUMNWIDTH) fprintf(fp,"W%d %d %d\n",x,z,columnwidth(sheet,x,z));
        if (SHEET(sheet,x,y,z)!=(Cell*)0)
        {
          fprintf(fp,"C%d %d %d ",x,y,z);
          if (SHEET(sheet,x,y,z)->adjust!=AUTOADJUST) fprintf(fp,"A%c ","lrc"[SHEET(sheet,x,y,z)->adjust]);
          if (SHEET(sheet,x,y,z)->label) fprintf(fp,"L%s ",SHEET(sheet,x,y,z)->label);
          if (SHEET(sheet,x,y,z)->precision!=-1) fprintf(fp,"P%d ",SHEET(sheet,x,y,z)->precision);
          if (SHEET(sheet,x,y,z)->shadowed) fprintf(fp,"S ");
          if (SHEET(sheet,x,y,z)->bold) fprintf(fp,"B ");
          if (SHEET(sheet,x,y,z)->underline) fprintf(fp,"U ");
          if (SHEET(sheet,x,y,z)->scientific!=DEF_SCIENTIFIC) fprintf(fp,"E ");
          if (SHEET(sheet,x,y,z)->locked) fprintf(fp,"C ");
          if (SHEET(sheet,x,y,z)->transparent) fprintf(fp,"T ");
          if (SHEET(sheet,x,y,z)->contents)
          {
            char buf[4096];

            if (fputc_close(':',fp)==EOF) return strerror(errno);
            print(buf,sizeof(buf),0,1,SHEET(sheet,x,y,z)->scientific,SHEET(sheet,x,y,z)->precision,SHEET(sheet,x,y,z)->contents);
            if (fputs_close(buf,fp)==EOF) return strerror(errno);
          }
          if (SHEET(sheet,x,y,z)->ccontents)
          {
            char buf[4096];

            if (fputs_close("\\\n",fp)==EOF) return strerror(errno);
            print(buf,sizeof(buf),0,1,SHEET(sheet,x,y,z)->scientific,SHEET(sheet,x,y,z)->precision,SHEET(sheet,x,y,z)->ccontents);
            if (fputs_close(buf,fp)==EOF) return strerror(errno);
          }
          if (fputc_close('\n',fp)==EOF) return strerror(errno);
          ++*count;
        }
      }
    }
  }
  if (fclose(fp)==EOF) return strerror(errno);
  return (const char*)0;
}
/*}}}*/
/* loadport      -- load from portable text */ /*{{{*/
const char *loadport(Sheet *sheet, const char *name)
{
  /* variables */ /*{{{*/
  static char errbuf[80];
  FILE *fp;
  int x,y,z;
  char buf[4096];
  int line;
  const char *ns,*os;
  const char *err;
  int precision;
  char *label;
  Adjust adjust;
  int shadowed;
  int bold;
  int underline;
  int scientific;
  int locked;
  int transparent;
  int ignored;
  Token **contents,**ccontents;
  int width;
  /*}}}*/

  if ((fp=fopen(name,"r"))==(FILE*)0) return strerror(errno);
  freesheet(sheet,0);
  err=(const char*)0;
  line=1;
  while (fgets(buf,sizeof(buf),fp)!=(char*)0)
  {
    /* remove nl */ /*{{{*/
    width=strlen(buf);
    if (width>0 && buf[width-1]=='\n') buf[--width]='\0';
    /*}}}*/
    switch (buf[0])
    {
      /* C       -- parse cell */ /*{{{*/
      case 'C':
      {
        int cc=0;

        if (width>0 && buf[width-1]=='\\') { buf[--width]='\0'; cc=1; }
        adjust=AUTOADJUST;
        precision=-1;
        label=(char*)0;
        contents=(Token**)0;
        ccontents=(Token**)0;
        shadowed=0;
        bold=0;
        underline=0;
        scientific=DEF_SCIENTIFIC;
        locked=0;
        transparent=0;
        ignored=0;
        /* parse x y and z */ /*{{{*/
        os=ns=buf+1;
        x=posnumber(os,&ns);
        if (os==ns)
        {
          sprintf(errbuf,_("Parse error for x position in line %d"),line);
          err=errbuf;
          goto eek;
        }
        while (*ns==' ') ++ns;
        os=ns;
        y=posnumber(os,&ns);
        if (os==ns)
        {
          sprintf(errbuf,_("Parse error for y position in line %d"),line);
          err=errbuf;
          goto eek;
        }
        while (*ns==' ') ++ns;
        os=ns;
        z=posnumber(os,&ns);
        if (os==ns)
        {
          sprintf(errbuf,_("Parse error for z position in line %d"),line);
          err=errbuf;
          goto eek;
        }
        /*}}}*/
        /* parse optional attributes */ /*{{{*/
        do
        {
          while (*ns==' ') ++ns;
          switch (*ns)
          {
            /* A       -- adjustment */ /*{{{*/
            case 'A':
            {
              ++ns;
              switch (*ns)
              {
                case 'l': adjust=LEFT; ++ns; break;
                case 'r': adjust=RIGHT; ++ns; break;
                case 'c': adjust=CENTER; ++ns; break;
                default:  sprintf(errbuf,_("Parse error for adjustment in line %d"),line); err=errbuf; goto eek;
              }
              break;
            }
            /*}}}*/
            /* L       -- label */ /*{{{*/
            case 'L':
            {
              char buf[1024],*p;

              p=buf;
              ++ns;
              while (*ns && *ns!=' ') { *p=*ns; ++p; ++ns; }
              *p='\0';
              label=mystrmalloc(buf);
              break;
            }
            /*}}}*/
            /* P       -- precision */ /*{{{*/
            case 'P':
            {
              os=++ns;
              precision=posnumber(os,&ns);
              if (os==ns)
              {
                sprintf(errbuf,_("Parse error for precision in line %d"),line);
                err=errbuf;
                goto eek;
              }
              break;
            }
            /*}}}*/
            /* S       -- shadowed */ /*{{{*/
            case 'S':
            {
              if (x==0)
              {
                sprintf(errbuf,_("Trying to shadow cell (%d,%d,%d) in line %d"),x,y,z,line);
                err=errbuf;
                goto eek;
              }
              ++ns;
              shadowed=1;
              break;
            }
            /*}}}*/
            /* U       -- underline */ /*{{{*/
            case 'U':
            {
              if (x==0)
              {
                sprintf(errbuf,_("Trying to underline cell (%d,%d,%d) in line %d"),x,y,z,line);
                err=errbuf;
                goto eek;
              }
              ++ns;
              underline=1;
              break;
            }
            /*}}}*/
            /* B       -- bold */ /*{{{*/
            case 'B':
            {
              if (x==0)
              {
                sprintf(errbuf,_("Trying to bold cell (%d,%d,%d) in line %d"),x,y,z,line);
                err=errbuf;
                goto eek;
              }
              ++ns;
              bold=1;
              break;
            }
            /*}}}*/
            /* E       -- scientific */ /*{{{*/
            case 'E':
            {
              ++ns;
              scientific=1;
              break;
            }
            /*}}}*/
            /* O       -- locked */ /*{{{*/
            case 'O':
            {
              ++ns;
              locked=1;
              break;
            }
            /*}}}*/
            /* T       -- transparent */ /*{{{*/
            case 'T':
            {
              ++ns;
              transparent=1;
              break;
            }
            /*}}}*/
            /* I       -- ignored */ /*{{{*/
            case 'I':
            {
              ++ns;
              ignored=1;
              break;
            }
            /*}}}*/
            /* : \0    -- do nothing */ /*{{{*/
            case ':':
            case '\0': break;
            /*}}}*/
            /* default -- error */ /*{{{*/
            default: sprintf(errbuf,_("Invalid option %c in line %d"),*ns,line); err=errbuf; goto eek;
            /*}}}*/
          }
        } while (*ns!=':' && *ns!='\0');
        /*}}}*/
        /* convert remaining string into token sequence  */ /*{{{*/
        if (*ns)
        {
          ++ns;
          contents=scan(&ns);
          if (contents==(Token**)0)
          {
            tvecfree(contents);
            sprintf(errbuf,_("Expression syntax error in line %d"),line);
            err=errbuf;
            goto eek;
          }
        }
        /*}}}*/
        /* convert remaining string into token sequence */ /*{{{*/
        if (cc && fgets(buf,sizeof(buf),fp)!=(char*)0)
        {
          ++line;
          /* remove nl */ /*{{{*/
          width=strlen(buf);
          if (width>0 && buf[width-1]=='\n') buf[width-1]='\0';
          /*}}}*/
          ns=buf;
          ccontents=scan(&ns);
          if (ccontents==(Token**)0)
          {
            tvecfree(ccontents);
            sprintf(errbuf,_("Expression syntax error in line %d"),line);
            err=errbuf;
            goto eek;
          }
        }
        /*}}}*/
        initcell(sheet,x,y,z);
        SHEET(sheet,x,y,z)->adjust=adjust;
        SHEET(sheet,x,y,z)->label=label;
        SHEET(sheet,x,y,z)->precision=precision;
        SHEET(sheet,x,y,z)->shadowed=shadowed;
        SHEET(sheet,x,y,z)->bold=bold;
        SHEET(sheet,x,y,z)->underline=underline;
        SHEET(sheet,x,y,z)->scientific=scientific;
        SHEET(sheet,x,y,z)->locked=locked;
        SHEET(sheet,x,y,z)->transparent=transparent;
        SHEET(sheet,x,y,z)->ignored=ignored;
        SHEET(sheet,x,y,z)->contents=contents;
        SHEET(sheet,x,y,z)->ccontents=ccontents;
        break;
      }
      /*}}}*/
      /* W       -- column width */ /*{{{*/
      case 'W':
      {
        /* parse x and z */ /*{{{*/
        os=ns=buf+1;
        x=posnumber(os,&ns);
        if (os==ns)
        {
          sprintf(errbuf,_("Parse error for x position in line %d"),line);
          err=errbuf;
          goto eek;
        }
        while (*ns==' ') ++ns;
        os=ns;
        z=posnumber(os,&ns);
        if (os==ns)
        {
          sprintf(errbuf,_("Parse error for z position in line %d"),line);
          err=errbuf;
          goto eek;
        }
        /*}}}*/
        /* parse width */ /*{{{*/
        while (*ns==' ') ++ns;
        os=ns;
        width=posnumber(os,&ns);
        if (os==ns)
        {
          sprintf(errbuf,_("Parse error for width in line %d"),line);
          err=errbuf;
          goto eek;
        }
        /*}}}*/
        setwidth(sheet,x,z,width);
        break;
      }
      /*}}}*/
      /* #       -- comment */ /*{{{*/
      case '#': break;
      /*}}}*/
      /* default -- error */ /*{{{*/
      default:
      {
        sprintf(errbuf,_("Unknown tag %c in line %d"),buf[0],line);
        err=errbuf;
        goto eek;
      }
      /*}}}*/
    }
    ++line;
  }
  eek:
  if (fclose(fp)==EOF && err==(const char*)0) err=strerror(errno);
  sheet->changed=0;
  cachelabels(sheet);
  forceupdate(sheet);
  return err;
}
/*}}}*/
/* loadxdr       -- load a spread sheet in XDR */ /*{{{*/
const char *loadxdr(Sheet *sheet, const char *name)
{
  /* variables */ /*{{{*/
  FILE *fp;
  XDR xdrs;
  int x,y,z;
  int width;
  int u;
  int olderror;
  /*}}}*/

  if ((fp=fopen(name,"r"))==(FILE*)0) return strerror(errno);
  xdrstdio_create(&xdrs,fp,XDR_DECODE);
  if (!xdr_magic(&xdrs))
  {
#if 0
    xdr_destroy(&xdrs);
    fclose(fp);
    return _("This is not a teapot worksheet in XDR format");
#else
    xdr_destroy(&xdrs);
    rewind(fp);
    xdrstdio_create(&xdrs,fp,XDR_DECODE);
#endif
  }
  freesheet(sheet,0);
  while (xdr_int(&xdrs,&u)) switch (u)
  {
    /* 0       -- column width element */ /*{{{*/
    case 0:
    {
      if (xdr_column(&xdrs,&x,&z,&width)==0)
      {
        olderror=errno;
        xdr_destroy(&xdrs);
        (void)fclose(fp);
        return strerror(olderror);
      }
      setwidth(sheet,x,z,width);
      break;
    }
    /*}}}*/
    /* 1       -- cell element */ /*{{{*/
    case 1:
    {
      if (xdr_int(&xdrs,&x)==0 || xdr_int(&xdrs,&y)==0 || xdr_int(&xdrs,&z)==0)
      {
        olderror=errno;
        xdr_destroy(&xdrs);
        (void)fclose(fp);
        return strerror(olderror);
      }
      initcell(sheet,x,y,z);
      if (xdr_cell(&xdrs,SHEET(sheet,x,y,z))==0)
      {
        freecell(sheet,x,y,z);
        olderror=errno;
        xdr_destroy(&xdrs);
        (void)fclose(fp);
        return strerror(olderror);
      }
      break;
    }
    /*}}}*/
    /* default -- should not happen */ /*{{{*/
    default:
    {
      xdr_destroy(&xdrs);
      fclose(fp);
      sheet->changed=0;
      cachelabels(sheet);
      forceupdate(sheet);
      return _("Invalid record, loading aborted");
    }
    /*}}}*/
  }
  xdr_destroy(&xdrs);
  if (fclose(fp)==EOF) return strerror(errno);
  sheet->changed=0;
  cachelabels(sheet);
  forceupdate(sheet);
  redraw_sheet(sheet);
  return (const char*)0;
}
/*}}}*/
/* loadcsv       -- load/merge CSVs */ /*{{{*/
const char *loadcsv(Sheet *sheet, const char *name)
{
  /* variables */ /*{{{*/
  FILE *fp;
  Token **t;
  const char *err;
  int line,x;
  char ln[4096];
  const char *str;
  double value;
  long lvalue;
  int separator = 0;
  /*}}}*/

  if ((fp=fopen(name,"r"))==(FILE*)0) return strerror(errno);
  err=(const char*)0;
  for (x=0,line=1; fgets(ln,sizeof(ln),fp); ++line)
  {
    const char *s;
    const char *cend;

    if (!separator) { /* FIXME: find a better way to autodetect */
        int ccnt = 0, scnt = 0;
        char *pos = ln;
        while ((pos = strchr(pos, ','))) pos++, ccnt++;
        pos = ln;
        while ((pos = strchr(pos, ';'))) pos++, scnt++;
        if (ccnt || scnt) separator = 1;
        csv_setopt(scnt > ccnt);
    }

    s=cend=ln;
    x=0;
    do
    {
      t=malloc(2*sizeof(Token*));
      t[0]=malloc(sizeof(Token));
      t[1]=(Token*)0;
      lvalue=csv_long(s,&cend);
      if (s!=cend) /* ok, it is a integer */ /*{{{*/
      {
        t[0]->type=INT;
        t[0]->u.integer=lvalue;
        putcont(sheet, sheet->curx+x, sheet->cury+line-1, sheet->curz, t, 0);
      }
      /*}}}*/
      else
      {
        value=csv_double(s,&cend);
        if (s!=cend) /* ok, it is a double */ /*{{{*/
        {
          t[0]->type=FLOAT;
          t[0]->u.flt=value;
          putcont(sheet, sheet->curx+x, sheet->cury+line-1, sheet->curz, t, 0);
        }
        /*}}}*/
        else
        {
          str=csv_string(s,&cend);
          if (s!=cend) /* ok, it is a string */ /*{{{*/
          {
            t[0]->type=STRING;
            t[0]->u.string=mystrmalloc(str);
            putcont(sheet, sheet->curx+x, sheet->cury+line-1, sheet->curz, t, 0);
          }
          /*}}}*/
          else
          {
            tvecfree(t);
            csv_separator(s,&cend);
            while (s==cend && *s && *s!='\n')
            {
              err=_("unknown values ignored");
              csv_separator(++s,&cend);
            }
            /* else it is nothing, which does not need to be stored :) */
          }
        }
      }
    } while (s!=cend ? s=cend,++x,1 : 0);
  }
  fclose(fp);
  return err;
}
/*}}}*/
/* insertcube    -- insert a block */ /*{{{*/
void insertcube(Sheet *sheet, int x1, int y1, int z1, int x2, int y2, int z2, Direction ins)
{
  /* variables */ /*{{{*/
  int x,y,z;
  /*}}}*/

  switch (ins)
  {
    /* IN_X    */ /*{{{*/
    case IN_X:
    {
      int right;

      right=sheet->dimx+x2-x1;
      for (z=z1; z<=z2; ++z) for (y=y1; y<=y2; ++y) for (x=right; x>x2; --x)
      {
        resize(sheet,x,y,z);
        SHEET(sheet,x,y,z)=SHEET(sheet,x-(x2-x1+1),y,z);
        SHEET(sheet,x-(x2-x1+1),y,z)=(Cell*)0;
      }
      break;
    }
    /*}}}*/
    /* IN_Y    */ /*{{{*/
    case IN_Y:
    {
      int down;

      down=sheet->dimy+y2-y1;
      for (z=z1; z<=z2; ++z) for (x=x1; x<=x2; ++x) for (y=down; y>y2; --y)
      {
        resize(sheet,x,y,z);
        SHEET(sheet,x,y,z)=SHEET(sheet,x,y-(y2-y1+1),z);
        SHEET(sheet,x,y-(y2-y1+1),z)=(Cell*)0;
      }
      break;
    }
    /*}}}*/
    /* IN_Z */ /*{{{*/
    case IN_Z:
    {
      int bottom;

      bottom=sheet->dimz+z2-z1;
      for (y=y1; y<=y2; ++y) for (x=x1; x<=x2; ++x) for (z=bottom; z>z2; --z)
      {
        resize(sheet,x,y,z);
        SHEET(sheet,x,y,z)=SHEET(sheet,x,y,z-(z2-z1+1));
        SHEET(sheet,x,y,z-(z2-z1+1))=(Cell*)0;
      }
      break;
    }
    /*}}}*/
    /* default */ /*{{{*/
    default: assert(0);
    /*}}}*/
  }
  sheet->changed=1;
  cachelabels(sheet);
  forceupdate(sheet);
}
/*}}}*/
/* deletecube    -- delete a block */ /*{{{*/
void deletecube(Sheet *sheet, int x1, int y1, int z1, int x2, int y2, int z2, Direction del)
{
  /* variables */ /*{{{*/
  int x,y,z;
  /*}}}*/

  /* free cells in marked block */ /*{{{*/
  for (x=x1; x<=x2; ++x)
  for (y=y1; y<=y2; ++y)
  for (z=z1; z<=z2; ++z)
  freecell(sheet,x,y,z);
  /*}}}*/
  switch (del)
  {
    /* IN_X */ /*{{{*/
    case IN_X:
    {
      for (z=z1; z<=z2; ++z) for (y=y1; y<=y2; ++y) for (x=x1; x<=sheet->dimx-(x2-x1+1); ++x)
      {
        if (x+(x2-x1+1)<sheet->dimx && y<sheet->dimy && z<sheet->dimz)
        {
          SHEET(sheet,x,y,z)=SHEET(sheet,x+(x2-x1+1),y,z);
          SHEET(sheet,x+(x2-x1+1),y,z)=(Cell*)0;
        }
      }
      break;
    }
    /*}}}*/
    /* IN_Y */ /*{{{*/
    case IN_Y:
    {
      for (z=z1; z<=z2; ++z) for (x=x1; x<=x2; ++x) for (y=y1; y<=sheet->dimy-(y2-y1+1); ++y)
      {
        if (x<sheet->dimx && y+(y2-y1+1)<sheet->dimy && z<sheet->dimz)
        {
          SHEET(sheet,x,y,z)=SHEET(sheet,x,y+(y2-y1+1),z);
          SHEET(sheet,x,y+(y2-y1+1),z)=(Cell*)0;
        }
      }
      break;
    }
    /*}}}*/
    /* IN_Z */ /*{{{*/
    case IN_Z:
    {
      for (y=y1; y<=y2; ++y) for (x=x1; x<=x2; ++x) for (z=z1; z<=sheet->dimz-(z2-z1+1); ++z)
      {
        if (x<sheet->dimx && y<sheet->dimy && z+(z2-z1+1)<sheet->dimz)
        {
          SHEET(sheet,x,y,z)=SHEET(sheet,x,y,z+(z2-z1+1));
          SHEET(sheet,x,y,z+(z2-z1+1))=(Cell*)0;
        }
      }
      break;
    }
    /*}}}*/
    /* default */ /*{{{*/
    default: assert(0);
    /*}}}*/
  }
  sheet->changed=1;
  cachelabels(sheet);
  forceupdate(sheet);
}
/*}}}*/
/* moveblock     -- move a block */ /*{{{*/
void moveblock(Sheet *sheet, int x1, int y1, int z1, int x2, int y2, int z2, int x3, int y3, int z3, int copy)
{
  /* variables */ /*{{{*/
  int dirx, diry, dirz;
  int widx, widy, widz;
  int x, y, z;
  int xf, xt, yf, yt, zf, zt;
  /*}}}*/

  if (x1==x3 && y1==y3 && z1==z3) return;
  widx=(x2-x1);
  widy=(y2-y1);
  widz=(z2-z1);
  if (x3>x1) { dirx=-1; xf=widx; xt=-1; } else { dirx=1; xf=0; xt=widx+1; }
  if (y3>y1) { diry=-1; yf=widy; yt=-1; } else { diry=1; yf=0; yt=widy+1; }
  if (z3>z1) { dirz=-1; zf=widz; zt=-1; } else { dirz=1; zf=0; zt=widz+1; }
  for (x=xf; x!=xt; x+=dirx)
  for (y=yf; y!=yt; y+=diry)
  for (z=zf; z!=zt; z+=dirz)
  {
    if (copy)
    {
      copycell(sheet,x1+x,y1+y,z1+z,sheet,x3+x,y3+y,z3+z);
    }
    else
    {
      if (x1+x<sheet->dimx && y1+y<sheet->dimy && z1+z<sheet->dimz)
      {
        resize(sheet,x3+x,y3+y,z3+z);
        SHEET(sheet,x3+x,y3+y,z3+z)=SHEET(sheet,x1+x,y1+y,z1+z);
        SHEET(sheet,x1+x,y1+y,z1+z)=(Cell*)0;
      }
      else
      {
        freecell(sheet,x3+x,y3+y,z3+z);
      }
    }
  }
  sheet->changed=1;
  cachelabels(sheet);
  forceupdate(sheet);
}
/*}}}*/
/* sortblock     -- sort a block */ /*{{{*/
/* Notes */ /*{{{*/
/*
The idea is to sort a block of cells in one direction by swapping the
planes which are canonical to the sort key vectors.  An example is to
sort a two dimensional block line-wise with one column as sort key.
You can have multiple sort keys which all have the same direction and
you can sort a cube plane-wise.
*/
/*}}}*/
const char *sortblock(Sheet *sheet, int x1, int y1, int z1, int x2, int y2, int z2, Direction dir, Sortkey *sk, size_t sklen)
{
  /* variables */ /*{{{*/
  int x,y,z;
  int incx=0,incy=0,incz=0;
  int distx,disty,distz;
  int i,r=-3,norel,work;
  /*}}}*/

  /* asserts */ /*{{{*/
  assert(sklen>0);
  assert(x1>=0);
  assert(x2>=0);
  assert(y1>=0);
  assert(y2>=0);
  assert(z1>=0);
  assert(z2>=0);
  /*}}}*/
  norel=0;
  posorder(&x1,&x2);
  posorder(&y1,&y2);
  posorder(&z1,&z2);
  distx=(x2-x1+1);
  disty=(y2-y1+1);
  distz=(z2-z1+1);
  switch (dir)
  {
    case IN_X: incx=1; --x2; incy=0; incz=0; distx=1; break;
    case IN_Y: incx=0; incy=1; --y2; incz=0; disty=1; break;
    case IN_Z: incx=0; incy=0; incz=1; --z2; distz=1; break;
    default: assert(0);
  }
  assert(incx || incy || incz);
  do
  {
    work=0;
    for (x=x1,y=y1,z=z1; x<=x2&&y<=y2&&z<=z2; x+=incx,y+=incy,z+=incz)
    {
      for (i=0; i<sklen; ++i)
      {
        r=cmpcell(sheet,x+sk[i].x,y+sk[i].y,z+sk[i].z,sheet,x+incx+sk[i].x,y+incy+sk[i].y,z+incz+sk[i].z,sk[i].sortkey);
        if (r==2) norel=1;
        else if (r==-1 || r==1) break;
        else assert(r==0);
      }
      if (r==1)
      {
        swapblock(sheet,dir==IN_X ? x : x1,dir==IN_Y ? y : y1,dir==IN_Z ? z : z1,sheet,dir==IN_X ? x+incx : x1,dir==IN_Y ? y+incy : y1,dir==IN_Z ? z+incz : z1,distx,disty,distz);
        work=1;
      }
    }
    x2-=incx;
    y2-=incy;
    z2-=incz;
  } while (work);
  cachelabels(sheet);
  forceupdate(sheet);
  sheet->changed=1;
  if (norel) return _("uncomparable elements");
  else return (const char*)0;
}
/*}}}*/
/* mirrorblock   -- mirror a block */ /*{{{*/
void mirrorblock(Sheet *sheet, int x1, int y1, int z1, int x2, int y2, int z2, Direction dir)
{
  switch (dir)
  {
    case IN_X: /* left-right */ /*{{{*/
    {
      int x,middle=(x2-x1+1)/2;
      for (x=0; x<middle; ++x)
      {
        swapblock(sheet,x1+x,y1,z1,sheet,x2-x,y1,z1, 1,y2-y1+1,z2-z1+1);
      }
      break;
    }
    /*}}}*/
    case IN_Y: /* upside-down */ /*{{{*/
    {
      int y,middle=(y2-y1+1)/2;
      for (y=0; y<middle; ++y)
      {
        swapblock(sheet,x1,y1+y,z1,sheet,x1,y2-y,z1, x2-x1+1,1,z2-z1+1);
      }
      break;
    }
    /*}}}*/
    case IN_Z: /* front-back */ /*{{{*/
    {
      int z,middle=(z2-z1+1)/2;
      for (z=0; z<middle; ++z)
      {
        swapblock(sheet,x1,y1,z1+z,sheet,x1,y1,z2-z, x2-x1+1,y2-y1+1,1);
      }
      break;
    }
    /*}}}*/
    default: assert(0);
  }
  sheet->changed=1;
  cachelabels(sheet);
  forceupdate(sheet);
}
/*}}}*/
