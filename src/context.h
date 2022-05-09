#ifndef CONTEXT_H
#define CONTEXT_H

#include "sheet.h"

const char *savecontext(Sheet *sheet, const char *name, int body, int x1, int y1, int z1, int x2, int y2, int z2, unsigned int *count);

#endif
