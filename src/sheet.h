#ifndef SHEET_H
#define SHEET_H

#include "scanner.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SHEET(s,x,y,z) (*(s->sheet+(x)*s->dimz*s->dimy+(y)*s->dimz+(z)))

typedef enum { LEFT=0, RIGHT=1, CENTER=2, AUTOADJUST=3 } Adjust;

typedef enum { IN_X, IN_Y, IN_Z } Direction;

/* must be a prime */
#define LABEL_CACHE 29

#define ASCENDING  001

typedef struct
{
  int x;
  int y;
  int z;
  int sortkey; /* OR-ed value of the above constants */
} Sortkey;

typedef struct
{
  Token **contents;
  Token **ccontents;
  char *label;
  Token value;
  Token resvalue;
  Adjust adjust;
  int precision;
  unsigned int updated:1;
  unsigned int shadowed:1;
  unsigned int scientific:1;
  unsigned int locked:1;
  unsigned int transparent:1;
  unsigned int ignored:1;
  unsigned int clock_t0:1;
  unsigned int clock_t1:1;
  unsigned int clock_t2:1;
  unsigned int bold:1;
  unsigned int underline:1;
} Cell;

struct Label
{
  const char *label;
  int x,y,z;
  struct Label *next;
};

typedef struct
{
  struct Label *labelcache[LABEL_CACHE];
  int curx, cury, curz;
  int mark1x, mark1y, mark1z;
  int mark2x, mark2y, mark2z;
  int marking;
  int offx, offy;
  Cell **sheet;
  int *column;
  int dimx, dimy, dimz;
  int orix, oriy, maxx, maxy;
  int width;
  char *name;
  unsigned int changed:1;
  unsigned int moveonly:1;
  unsigned int clk:1;
  void *display;
} Sheet;

extern Sheet *upd_sheet;
extern int upd_x;
extern int upd_y;
extern int upd_z;
extern int max_eval;

void resize(Sheet *sheet, int x, int y, int z);
void initcell(Sheet *sheet, int x, int y, int z);
void cachelabels(Sheet *sheet);
void freesheet(Sheet *sheet, int all);
void forceupdate(Sheet *sheet);
void freecell(Sheet *sheet, int x, int y, int z);
int columnwidth(Sheet *sheet, int x, int z);
void setwidth(Sheet *sheet, int x, int z, int width);
int cellwidth(Sheet *sheet, int x, int y, int z);
void putcont(Sheet *sheet, int x, int y, int z, Token **t, int c);
Token **getcont(Sheet *sheet, int x, int y, int z, int c);
Token getvalue(Sheet *sheet, int x, int y, int z);
void update(Sheet *sheet);
char *geterror(Sheet *sheet, int x, int y, int z);
void printvalue(char *s, size_t size, size_t chars, int quote, int scientific, int precision, Sheet *sheet, int x, int y, int z);
Adjust getadjust(Sheet *sheet, int x, int y, int z);
void setadjust(Sheet *sheet, int x, int y, int z, Adjust adjust);
void shadow(Sheet *sheet, int x, int y, int z, int yep);
int shadowed(Sheet *sheet, int x, int y, int z);
void bold(Sheet *sheet, int x, int y, int z, int yep);
int isbold(Sheet *sheet, int x, int y, int z);
void underline(Sheet *sheet, int x, int y, int z, int yep);
int underlined(Sheet *sheet, int x, int y, int z);
void lockcell(Sheet *sheet, int x, int y, int z, int yep);
int locked(Sheet *sheet, int x, int y, int z);
int transparent(Sheet *sheet, int x, int y, int z);
void maketrans(Sheet *sheet, int x, int y, int z, int yep);
void igncell(Sheet *sheet, int x, int y, int z, int yep);
int ignored(Sheet *sheet, int x, int y, int z);
void clk(Sheet *sheet, int x, int y, int z);
void setscientific(Sheet *sheet, int x, int y, int z, int yep);
int getscientific(Sheet *sheet, int x, int y, int z);
void setprecision(Sheet *sheet, int x, int y, int z, int precision);
int getprecision(Sheet *sheet, int x, int y, int z);
const char *getlabel(Sheet *sheet, int x, int y, int z);
void setlabel(Sheet *sheet, int x, int y, int z, const char *buf, int update);
Token findlabel(Sheet *sheet, const char *label);
void relabel(Sheet *sheet, const char *oldlabel, const char *newlabel, int x, int y, int z);
const char *savexdr(Sheet *sheet, const char *name, unsigned int *count);
const char *savetbl(Sheet *sheet, const char *name, int body, int x1, int y1, int z1, int x2, int y2, int z2, unsigned int *count);
const char *savetext(Sheet *sheet, const char *name, int x1, int y1, int z1, int x2, int y2, int z2, unsigned int *count);
const char *savecsv(Sheet *sheet, const char *name, char sep, int x1, int y1, int z1, int x2, int y2, int z2, unsigned int *count);
const char *saveport(Sheet *sheet, const char *name, unsigned int *count);
const char *loadxdr(Sheet *sheet, const char *name);
const char *loadport(Sheet *sheet, const char *name);
const char *loadcsv(Sheet *sheet, const char *name);
void insertcube(Sheet *sheet, int x1, int y1, int z1, int x2, int y2, int z2, Direction ins);
void deletecube(Sheet *sheet, int x1, int y1, int z1, int x2, int y2, int z2, Direction ins);
void moveblock(Sheet *sheet, int x1, int y1, int z1, int x2, int y2, int z2, int x3, int y3, int z3, int copy);
const char *sortblock(Sheet *sheet, int x1, int y1, int z1, int x2, int y2, int z2, Direction dir, Sortkey *sk, size_t sklen);
void mirrorblock(Sheet *sheet, int x1, int y1, int z1, int x2, int y2, int z2, Direction dir);

#ifdef __cplusplus
}
#endif

#endif
