/* #includes */ /*{{{C}}}*//*{{{*/
#ifndef NO_POSIX_SOURCE
#undef _POSIX_SOURCE
#define _POSIX_SOURCE   1
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 2
#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cat.h"
#include "eval.h"
#include "main.h"
#include "misc.h"
#include "parser.h"
#include "scanner.h"
#include "sheet.h"
/*}}}*/
/* #defines */ /*{{{*/
#define MAXARGC 16
/*}}}*/

/* prototypes */ /*{{{*/
static Token term(Token *n[], int *i);
/*}}}*/

/* primary   -- parse and evaluate a primary term */ /*{{{*/
static Token primary(Token *n[], int *i)
{
  /* variables */ /*{{{*/
  int argc,j;
  Token *ident,argv[MAXARGC],result;
  /*}}}*/

  if (n[*i]==(Token*)0)
  /* error */ /*{{{*/
  {
    result.type=EEK;
    result.u.err=strcpy(malloc(strlen(MISSOPR)+1),MISSOPR);
    return result;
  }
  /*}}}*/
  else switch (n[*i]->type)
  {
    /* STRING, FLOAT, INT */ /*{{{*/
    case STRING:
    case FLOAT:
    case INT:
    {
      return tcopy(*n[(*i)++]);
    }
    /*}}}*/
    /* OPERATOR */ /*{{{*/
    case OPERATOR:
    {
      if (n[*i]->u.operator==OP)
      /* return paren term */ /*{{{*/
      {
        ++(*i);
        result=term(n,i);
        if (result.type==EEK) return result;
        if (n[*i]!=(Token*)0 && n[*i]->type==OPERATOR && n[*i]->u.operator==CP) 
        {
          ++(*i);
          return result;
        }
        tfree(&result);
        result.type=EEK;
        result.u.err=strcpy(malloc(strlen(CPEXPCT)+1),CPEXPCT);
        return result;
      }
      /*}}}*/
      else if (n[*i]->u.operator==MINUS)
      /* return negated term */ /*{{{*/
      {
        ++(*i);
        return(tneg(primary(n,i)));
      }
      /*}}}*/
      else
      /* return error, value expected */ /*{{{*/
      {
        result.type=EEK;
        result.u.err=mystrmalloc(VLEXPCT);
        return result;
      }
      /*}}}*/
    }  
    /*}}}*/
    /* LIDENT */ /*{{{*/
    case LIDENT:
    {
      ident=n[*i];
      ++(*i);
      return findlabel(upd_sheet,ident->u.lident);
    }
    /*}}}*/
    /* FIDENT */ /*{{{*/
    case FIDENT:
    {
      ident=n[*i];
      ++(*i);
      if (n[*i]!=(Token*)0 && n[*i]->type==OPERATOR && n[*i]->u.operator==OP)
      /* parse arguments and closing paren of function call, return its value */ /*{{{*/
      {
        ++(*i);
        argc=0;
        if (!(n[*i]!=(Token*)0 && n[*i]->type==OPERATOR && n[*i]->u.operator==CP))
        /* parse at least one argument */ /*{{{*/
        {
          if (n[*i]!=(Token*)0 && n[*i]->type==OPERATOR && n[*i]->u.operator==COMMA)
          /* empty argument */ /*{{{*/
          {
            argv[argc].type=EMPTY;
          }
          /*}}}*/
          else argv[argc]=term(n,i);
          if (argv[argc].type==EEK) return argv[argc];
          ++argc;
          while (n[*i]!=(Token*)0 && n[*i]->type==OPERATOR && n[*i]->u.operator==COMMA)
          /* parse the following argument */ /*{{{*/
          {
            ++(*i);
            if (argc<=MAXARGC)
            {
              if (n[*i]!=(Token*)0 && n[*i]->type==OPERATOR && (n[*i]->u.operator==COMMA || n[*i]->u.operator==CP))
              {
                argv[argc].type=EMPTY;
              }    
              else argv[argc]=term(n,i);
            }
            else
            {
              result.type=EEK;
              result.u.err=strcpy(malloc(strlen(TOOMANY)+1),TOOMANY);
              for (j=0; j<=argc; ++j) tfree(&argv[j]);
              return result;
            }
            ++argc;
          }
          /*}}}*/
        }
        /*}}}*/
        if (n[*i]!=(Token*)0 && n[*i]->type==OPERATOR && n[*i]->u.operator==CP) 
        /* eval function */ /*{{{*/
        {
          ++(*i);
          result=tfuncall(ident,argc,argv);  
          for (j=0; j<argc; ++j) tfree(&argv[j]);
        }
        /*}}}*/
        else
        /* ) expected */ /*{{{*/
        {
          for (j=0; j<argc; ++j) tfree(&argv[j]);
          result.type=EEK;
          result.u.err=strcpy(malloc(strlen(CPEXPCT)+1),CPEXPCT);
        }
        /*}}}*/
        return result;  
      }
      /*}}}*/
      else
      { 
        result.type=EEK;
        result.u.err=mystrmalloc(OPEXPECT);
        return result;
      }
    }
    /*}}}*/
    default: ; /* fall through */
  }
  result.type=EEK;
  result.u.err=mystrmalloc(VLEXPCT);
  return result;
}
/*}}}*/
/* powterm   -- parse and evaluate a x^y term */ /*{{{*/
static Token powterm(Token *n[], int *i)
{
  Token l;

  l=primary(n,i);
  if (l.type==EEK) return l;
  while (n[*i]!=(Token*)0 && n[*i]->type==OPERATOR && n[*i]->u.operator==POW)
  {
    Token result,r;

    ++(*i);
    r=primary(n,i);
    result=tpow(l,r);
    tfree(&l);
    tfree(&r);
    if (result.type==EEK) return result;
    l=result;
  }
  return l;
}
/*}}}*/
/* piterm    -- parse and evaluate a product/division/modulo term */ /*{{{*/
static Token piterm(Token *n[], int *i)
{
  Token l;

  l=powterm(n,i);
  if (l.type==EEK) return l;
  while (n[*i]!=(Token*)0 && n[*i]->type==OPERATOR && (n[*i]->u.operator==DIV || n[*i]->u.operator==MUL || n[*i]->u.operator==MOD))
  {
    Operator op;
    Token result,r;

    op=n[*i]->u.operator;    
    ++(*i);
    r=powterm(n,i);
    switch (op)
    {
      case MUL: result=tmul(l,r); break;
      case DIV: result=tdiv(l,r); break;
      case MOD: result=tmod(l,r); break;
      default: assert(0);
    }
    tfree(&l);
    tfree(&r);
    if (result.type==EEK) return result;
    l=result;
  }
  return l;
}
/*}}}*/
/* factor    -- parse and evaluate a factor of sums/differences */ /*{{{*/
static Token factor(Token *n[], int *i)
{
  Token l;

  l=piterm(n,i);
  if (l.type==EEK) return l;
  while (n[*i]!=(Token*)0 && n[*i]->type==OPERATOR && (n[*i]->u.operator==PLUS || n[*i]->u.operator==MINUS))
  {
    Operator op;
    Token result,r;

    op=n[*i]->u.operator;    
    ++(*i);
    r=piterm(n,i);
    result=(op==PLUS ? tadd(l,r) : tsub(l,r));
    tfree(&l);
    tfree(&r);
    if (result.type==EEK) return result;
    l=result;
  }
  return l;
}
/*}}}*/
/* term      -- parse and evaluate a relational term */ /*{{{*/
static Token term(Token *n[], int *i)
{
  Token l;

  l=factor(n,i);
  if (l.type==EEK) return l;
  while (n[*i]!=(Token*)0 && n[*i]->type==OPERATOR && n[*i]->u.operator>=LT && n[*i]->u.operator<=NE)
  {
    Operator op;
    Token result,r;

    op=n[*i]->u.operator;    
    ++(*i);
    r=factor(n,i);
    switch (op)
    {
      case LT: result=tlt(l,r); break;
      case LE: result=tle(l,r); break;
      case GE: result=tge(l,r); break;
      case GT: result=tgt(l,r); break;
      case ISEQUAL: result=teq(l,r); break;
      case ABOUTEQ: result=tabouteq(l,r); break;
      case NE: result=tne(l,r); break;
      default: assert(0);
    }
    tfree(&l);
    tfree(&r);
    if (result.type==EEK) return result;
    l=result;
  }
  return l;
}
/*}}}*/

/* eval      -- parse and evaluate token sequence */ /*{{{*/
Token eval(Token **n)
{
  Token result;
  int i;

  assert(upd_sheet!=(Sheet*)0);
  i=0;
  result=term(n,&i);
  if (result.type==EEK) return result;
  if (n[i]!=(Token*)0)
  {
    tfree(&result);
    result.type=EEK;
    result.u.err=strcpy(malloc(strlen(PARSERR)+1),PARSERR);
    return result;
  }
  return result;
}
/*}}}*/
