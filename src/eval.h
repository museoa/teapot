#ifndef THE_EVAL_H
#define THE_EVAL_H

#include "scanner.h"

Token tcopy(Token n);
void tfree(Token *n);
void tvecfree(Token **tvec);
Token tpow(Token l, Token r);
Token tdiv(Token l, Token r);
Token tmod(Token l, Token r);
Token tmul(Token l, Token r);
Token tadd(Token l, Token r);
Token tsub(Token l, Token r);
Token tneg(Token x);
Token tfuncall(Token *ident, int argc, Token argv[]);
Token tlt(Token l, Token r);
Token tle(Token l, Token r);
Token tge(Token l, Token r);
Token tgt(Token l, Token r);
Token teq(Token l, Token r);
Token tabouteq(Token l, Token r);
Token tne(Token l, Token r);

#endif
