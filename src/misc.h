#ifndef MISC_H
#define MISC_H

#include <stdio.h>

#include "sheet.h"

void posorder(int *x, int *y);
long int posnumber(const char *s, const char **endptr);
char *mystrmalloc(const char *str);
const char *dblfinite(double x);
int fputc_close(char c, FILE *fp);
int fputs_close(const char *s, FILE *fp);
void adjust(Adjust a, char *s, size_t n);

#endif
