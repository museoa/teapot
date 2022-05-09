#ifndef SCANNER_H
#define SCANNER_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	EMPTY
#ifndef __cplusplus
	, STRING, FLOAT, INT, OPERATOR, LIDENT, FIDENT, LOCATION, EEK
#endif
} Type;

typedef enum { PLUS, MINUS, MUL, DIV, OP, CP, COMMA, LT, LE, GE, GT, ISEQUAL, ABOUTEQ, NE, POW, MOD } Operator;

typedef struct
{
  Type type;
  union
  {
    char *string;
    double flt;
    long integer;
    Operator op;
    char *lident;
    int fident;
    int location[3];
    char *err;
  } u;
} Token;

int identcode(const char *s, size_t len);
Token **scan(const char **s);
void print(char *s, size_t size, size_t chars, int quote, int scientific, int precision, Token **n);

#ifdef __cplusplus
}
#endif

#endif
