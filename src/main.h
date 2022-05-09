#ifndef MAIN_H
#define MAIN_H

#include "config.h"

#define _(x) (x)

#include "sheet.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int batch;
extern int def_precision;
extern int quote;
extern int header;
extern unsigned int batchln;

/* A variable of type Key is either a special symbol from this enum, representing
   actions the user has triggered (by whatever means available), or a Unicode
   character that was entered
*/
typedef enum {
  K_NONE = 0,
  K_INVALID = -1,
  K_BACKSPACE = -2,
  K_BPAGE = -3,
  K_CLOCK = -4,
  K_COPY = -5,
  K_DC = -6,
  K_DOWN = -7,
  K_END = -8,
  K_ENTER = -9,
  K_FIRSTL = -10,
  K_FPAGE = -11,
  K_FSHEET = -12,
  K_HOME = -13,
  K_LASTL = -14,
  K_LEFT = -15,
  K_LOAD = -16,
  K_LOADMENU = -17,
  K_LSHEET = -18,
  K_MARK = -19,
  K_MENTER = -20,
  K_NPAGE = -21,
  K_NSHEET = -22,
  K_PPAGE = -23,
  K_PSHEET = -24,
  K_QUIT = -25,
  K_RECALC = -26,
  K_RIGHT = -27,
  K_SAVE = -28,
  K_SAVEMENU = -29,
  K_SAVEQUIT = -30,
  K_UP = -31,
  K_GOTO = -32,
  K_NAME = -33,
  ADJUST_LEFT = -34,
  ADJUST_RIGHT = -35,
  ADJUST_CENTER = -36,
  ADJUST_SCIENTIFIC = -37,
  ADJUST_BOLD = -38,
  ADJUST_PRECISION = -39,
  ADJUST_SHADOW = -40,
  ADJUST_TRANSPARENT = -41,
  ADJUST_LABEL = -42,
  ADJUST_LOCK = -43,
  ADJUST_IGNORE = -44,
  K_COLWIDTH = -45,
  BLOCK_CLEAR = -46,
  BLOCK_INSERT = -47,
  BLOCK_DELETE = -48,
  BLOCK_MOVE = -49,
  BLOCK_COPY = -50,
  BLOCK_FILL = -51,
  BLOCK_SORT = -52,
  BLOCK_MIRROR = -53,
  K_ABOUT = -54,
  K_HELP = -55,
  ADJUST_UNDERLINE = -56
} Key;

extern int do_sheetcmd(Sheet *cursheet, Key c, int moveonly);
extern int doanyway(Sheet *sheet, const char *msg);
extern void moveto(Sheet *sheet, int x, int y, int z);
extern void relmoveto(Sheet *sheet, int x, int y, int z);
extern void do_mark(Sheet *cursheet, int force);

#ifdef __cplusplus
}
#endif

#endif
