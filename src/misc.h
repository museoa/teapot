#ifndef MISC_H
#define MISC_H

#include <stdio.h>

#include "sheet.h"

#ifdef __cplusplus
extern "C" {
#endif

void posorder(int *x, int *y);
long int posnumber(const char *s, const char **endptr);
char *mystrmalloc(const char *str);
const char *dblfinite(double x);
int fputc_close(char c, FILE *fp);
int fputs_close(const char *s, FILE *fp);
void adjust(Adjust a, char *s, size_t n);
char *striphtml(const char *in);

#ifdef __cplusplus
}
#endif

#endif
