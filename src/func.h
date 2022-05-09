#ifndef FUNC_H
#define FUNC_H

#include "scanner.h"

typedef struct 
{
  const char name[20];
  Token (*func)(int, const Token[]);
} Tfunc;

extern Tfunc tfunc[];

#endif
