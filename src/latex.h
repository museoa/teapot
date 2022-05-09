#ifndef LATEX_H
#define LATEX_H

#include "sheet.h"

const char *savelatex(Sheet *sheet, const char *name, int body, int x1, int y1, int z1, int x2, int y2, int z2, unsigned int *count);

#endif
