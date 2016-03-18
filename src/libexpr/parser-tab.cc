/* A Bison parser, made by GNU Bison 2.7.  */

/* Skeleton implementation for Bison GLR parsers in C
   
      Copyright (C) 2002-2012 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C GLR parser skeleton written by Paul Hilfinger.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.7"

/* Skeleton name.  */
#define YYSKELETON_NAME "glr.c"

/* Pure parsers.  */
#define YYPURE 1






/* Copy the first part of user declarations.  */
/* Line 207 of glr.c  */
#line 53 "src/libexpr/parser.y"


#include "parser-tab.hh"
#include "lexer-tab.hh"

YY_DECL;

using namespace nix;


namespace nix {


static void dupAttr(const AttrPath & attrPath, const Pos & pos, const Pos & prevPos)
{
    throw ParseError(format("attribute ‘%1%’ at %2% already defined at %3%")
        % showAttrPath(attrPath) % pos % prevPos);
}


static void dupAttr(Symbol attr, const Pos & pos, const Pos & prevPos)
{
    throw ParseError(format("attribute ‘%1%’ at %2% already defined at %3%")
        % attr % pos % prevPos);
}


static void addAttr(ExprAttrs * attrs, AttrPath & attrPath,
    Expr * e, const Pos & pos)
{
    AttrPath::iterator i;
    // All attrpaths have at least one attr
    assert(!attrPath.empty());
    for (i = attrPath.begin(); i + 1 < attrPath.end(); i++) {
        if (i->symbol.set()) {
            ExprAttrs::AttrDefs::iterator j = attrs->attrs.find(i->symbol);
            if (j != attrs->attrs.end()) {
                if (!j->second.inherited) {
                    ExprAttrs * attrs2 = dynamic_cast<ExprAttrs *>(j->second.e);
                    if (!attrs2) dupAttr(attrPath, pos, j->second.pos);
                    attrs = attrs2;
                } else
                    dupAttr(attrPath, pos, j->second.pos);
            } else {
                ExprAttrs * nested = new ExprAttrs;
                attrs->attrs[i->symbol] = ExprAttrs::AttrDef(nested, pos);
                attrs = nested;
            }
        } else {
            ExprAttrs *nested = new ExprAttrs;
            attrs->dynamicAttrs.push_back(ExprAttrs::DynamicAttrDef(i->expr, nested, pos));
            attrs = nested;
        }
    }
    if (i->symbol.set()) {
        ExprAttrs::AttrDefs::iterator j = attrs->attrs.find(i->symbol);
        if (j != attrs->attrs.end()) {
            dupAttr(attrPath, pos, j->second.pos);
        } else {
            attrs->attrs[i->symbol] = ExprAttrs::AttrDef(e, pos);
            e->setName(i->symbol);
        }
    } else {
        attrs->dynamicAttrs.push_back(ExprAttrs::DynamicAttrDef(i->expr, e, pos));
    }
}


static void addFormal(const Pos & pos, Formals * formals, const Formal & formal)
{
    if (formals->argNames.find(formal.name) != formals->argNames.end())
        throw ParseError(format("duplicate formal function argument ‘%1%’ at %2%")
            % formal.name % pos);
    formals->formals.push_front(formal);
    formals->argNames.insert(formal.name);
}


static Expr * stripIndentation(const Pos & pos, SymbolTable & symbols, vector<Expr *> & es)
{
    if (es.empty()) return new ExprString(symbols.create(""));

    /* Figure out the minimum indentation.  Note that by design
       whitespace-only final lines are not taken into account.  (So
       the " " in "\n ''" is ignored, but the " " in "\n foo''" is.) */
    bool atStartOfLine = true; /* = seen only whitespace in the current line */
    unsigned int minIndent = 1000000;
    unsigned int curIndent = 0;
    for (auto & i : es) {
        ExprIndStr * e = dynamic_cast<ExprIndStr *>(i);
        if (!e) {
            /* Anti-quotations end the current start-of-line whitespace. */
            if (atStartOfLine) {
                atStartOfLine = false;
                if (curIndent < minIndent) minIndent = curIndent;
            }
            continue;
        }
        for (unsigned int j = 0; j < e->s.size(); ++j) {
            if (atStartOfLine) {
                if (e->s[j] == ' ')
                    curIndent++;
                else if (e->s[j] == '\n') {
                    /* Empty line, doesn't influence minimum
                       indentation. */
                    curIndent = 0;
                } else {
                    atStartOfLine = false;
                    if (curIndent < minIndent) minIndent = curIndent;
                }
            } else if (e->s[j] == '\n') {
                atStartOfLine = true;
                curIndent = 0;
            }
        }
    }

    /* Strip spaces from each line. */
    vector<Expr *> * es2 = new vector<Expr *>;
    atStartOfLine = true;
    unsigned int curDropped = 0;
    unsigned int n = es.size();
    for (vector<Expr *>::iterator i = es.begin(); i != es.end(); ++i, --n) {
        ExprIndStr * e = dynamic_cast<ExprIndStr *>(*i);
        if (!e) {
            atStartOfLine = false;
            curDropped = 0;
            es2->push_back(*i);
            continue;
        }

        string s2;
        for (unsigned int j = 0; j < e->s.size(); ++j) {
            if (atStartOfLine) {
                if (e->s[j] == ' ') {
                    if (curDropped++ >= minIndent)
                        s2 += e->s[j];
                }
                else if (e->s[j] == '\n') {
                    curDropped = 0;
                    s2 += e->s[j];
                } else {
                    atStartOfLine = false;
                    curDropped = 0;
                    s2 += e->s[j];
                }
            } else {
                s2 += e->s[j];
                if (e->s[j] == '\n') atStartOfLine = true;
            }
        }

        /* Remove the last line if it is empty and consists only of
           spaces. */
        if (n == 1) {
            string::size_type p = s2.find_last_of('\n');
            if (p != string::npos && s2.find_first_not_of(' ', p + 1) == string::npos)
                s2 = string(s2, 0, p + 1);
        }

        es2->push_back(new ExprString(symbols.create(s2)));
    }

    /* If this is a single string, then don't do a concatenation. */
    return es2->size() == 1 && dynamic_cast<ExprString *>((*es2)[0]) ? (*es2)[0] : new ExprConcatStrings(pos, true, es2);
}


static inline Pos makeCurPos(const YYLTYPE & loc, ParseData * data)
{
    return Pos(data->path, loc.first_line, loc.first_column);
}

#define CUR_POS makeCurPos(*yylocp, data)


}


void yyerror(YYLTYPE * loc, yyscan_t scanner, ParseData * data, const char * error)
{
    data->error = (format("%1%, at %2%")
        % error % makeCurPos(*loc, data)).str();
}



/* Line 207 of glr.c  */
#line 244 "src/libexpr/parser-tab.cc"

# ifndef YY_NULL
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULL nullptr
#  else
#   define YY_NULL 0
#  endif
# endif

#include "parser-tab.hh"

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* Default (constant) value used for initialization for null
   right-hand sides.  Unlike the standard yacc.c template, here we set
   the default value of $$ to a zeroed-out value.  Since the default
   value is undefined, this behavior is technically correct.  */
static YYSTYPE yyval_default;
static YYLTYPE yyloc_default
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;

/* Copy the second part of user declarations.  */

/* Line 230 of glr.c  */
#line 278 "src/libexpr/parser-tab.cc"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(N) (N)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#ifndef YYFREE
# define YYFREE free
#endif
#ifndef YYMALLOC
# define YYMALLOC malloc
#endif
#ifndef YYREALLOC
# define YYREALLOC realloc
#endif

#define YYSIZEMAX ((size_t) -1)

#ifdef __cplusplus
   typedef bool yybool;
#else
   typedef unsigned char yybool;
#endif
#define yytrue 1
#define yyfalse 0

#ifndef YYSETJMP
# include <setjmp.h>
# define YYJMP_BUF jmp_buf
# define YYSETJMP(Env) setjmp (Env)
/* Pacify clang.  */
# define YYLONGJMP(Env, Val) (longjmp (Env, Val), YYASSERT (0))
#endif

/*-----------------.
| GCC extensions.  |
`-----------------*/

#ifndef __attribute__
/* This feature is available in gcc versions 2.5 and later.  */
# if (! defined __GNUC__ || __GNUC__ < 2 \
      || (__GNUC__ == 2 && __GNUC_MINOR__ < 5))
#  define __attribute__(Spec) /* empty */
# endif
#endif

#ifndef YYASSERT
# define YYASSERT(Condition) ((void) ((Condition) || (abort (), 0)))
#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  53
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   344

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  59
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  20
/* YYNRULES -- Number of rules.  */
#define YYNRULES  85
/* YYNRULES -- Number of states.  */
#define YYNSTATES  169
/* YYMAXRHS -- Maximum number of symbols on right-hand side of rule.  */
#define YYMAXRHS 7
/* YYMAXLEFT -- Maximum number of symbols to the left of a handle
   accessed by $0, $-1, etc., in any rule.  */
#define YYMAXLEFT 0

/* YYTRANSLATE(X) -- Bison symbol number corresponding to X.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   292

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    50,    52,     2,     2,     2,     2,     2,
      53,    54,    40,    38,    58,    39,    51,    41,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    45,    49,
      32,    57,    33,    43,    48,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    55,     2,    56,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    46,     2,    47,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    34,    35,    36,
      37,    42,    44
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short int yyprhs[] =
{
       0,     0,     3,     5,     7,    11,    17,    25,    33,    38,
      43,    48,    50,    57,    59,    62,    65,    69,    73,    77,
      81,    85,    89,    93,    97,   101,   105,   109,   113,   117,
     121,   125,   129,   131,   134,   136,   140,   146,   149,   151,
     153,   155,   157,   161,   165,   167,   169,   171,   173,   177,
     182,   187,   191,   195,   197,   199,   200,   203,   208,   212,
     217,   220,   225,   226,   232,   237,   245,   246,   249,   252,
     253,   257,   261,   263,   265,   267,   269,   273,   277,   280,
     281,   285,   287,   288,   290,   292
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const signed char yyrhs[] =
{
      60,     0,    -1,    61,    -1,    62,    -1,     3,    45,    62,
      -1,    46,    77,    47,    45,    62,    -1,    46,    77,    47,
      48,     3,    45,    62,    -1,     3,    48,    46,    77,    47,
      45,    62,    -1,    16,    61,    49,    62,    -1,    17,    61,
      49,    62,    -1,    18,    71,    19,    62,    -1,    63,    -1,
      13,    61,    14,    61,    15,    61,    -1,    64,    -1,    50,
      64,    -1,    39,    64,    -1,    64,    22,    64,    -1,    64,
      23,    64,    -1,    64,    32,    64,    -1,    64,    35,    64,
      -1,    64,    33,    64,    -1,    64,    34,    64,    -1,    64,
      24,    64,    -1,    64,    25,    64,    -1,    64,    26,    64,
      -1,    64,    36,    64,    -1,    64,    43,    73,    -1,    64,
      38,    64,    -1,    64,    39,    64,    -1,    64,    40,    64,
      -1,    64,    41,    64,    -1,    64,    42,    64,    -1,    65,
      -1,    65,    66,    -1,    66,    -1,    67,    51,    73,    -1,
      67,    51,    73,    27,    66,    -1,    67,    27,    -1,    67,
      -1,     3,    -1,     7,    -1,     8,    -1,    52,    68,    52,
      -1,    29,    70,    30,    -1,     9,    -1,    10,    -1,    11,
      -1,    12,    -1,    53,    61,    54,    -1,    18,    46,    71,
      47,    -1,    20,    46,    71,    47,    -1,    46,    71,    47,
      -1,    55,    76,    56,    -1,     5,    -1,    69,    -1,    -1,
      69,     5,    -1,    69,    28,    61,    47,    -1,    28,    61,
      47,    -1,     5,    28,    61,    47,    -1,    70,     6,    -1,
      70,    28,    61,    47,    -1,    -1,    71,    73,    57,    61,
      49,    -1,    71,    21,    72,    49,    -1,    71,    21,    53,
      61,    54,    72,    49,    -1,    -1,    72,    74,    -1,    72,
      75,    -1,    -1,    73,    51,    74,    -1,    73,    51,    75,
      -1,    74,    -1,    75,    -1,     3,    -1,    27,    -1,    52,
      68,    52,    -1,    28,    61,    47,    -1,    76,    66,    -1,
      -1,    78,    58,    77,    -1,    78,    -1,    -1,    31,    -1,
       3,    -1,     3,    43,    61,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   293,   293,   295,   298,   300,   302,   304,   306,   308,
     310,   316,   320,   321,   325,   326,   327,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   337,   338,   340,   341,
     342,   343,   344,   348,   350,   354,   356,   360,   362,   366,
     372,   373,   374,   375,   378,   379,   380,   387,   388,   391,
     393,   395,   397,   401,   402,   403,   407,   408,   409,   410,
     418,   419,   420,   424,   425,   434,   443,   447,   448,   458,
     462,   463,   472,   473,   485,   486,   490,   491,   495,   496,
     500,   502,   505,   506,   511,   512
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "ID", "ATTRPATH", "STR", "IND_STR",
  "INT", "FLOAT", "PATH", "HPATH", "SPATH", "URI", "IF", "THEN", "ELSE",
  "ASSERT", "WITH", "LET", "IN", "REC", "INHERIT", "EQ", "NEQ", "AND",
  "OR", "IMPL", "OR_KW", "DOLLAR_CURLY", "IND_STRING_OPEN",
  "IND_STRING_CLOSE", "ELLIPSIS", "'<'", "'>'", "GEQ", "LEQ", "UPDATE",
  "NOT", "'+'", "'-'", "'*'", "'/'", "CONCAT", "'?'", "NEGATE", "':'",
  "'{'", "'}'", "'@'", "';'", "'!'", "'.'", "'\"'", "'('", "')'", "'['",
  "']'", "'='", "','", "$accept", "start", "expr", "expr_function",
  "expr_if", "expr_op", "expr_app", "expr_select", "expr_simple",
  "string_parts", "string_parts_interpolated", "ind_string_parts", "binds",
  "attrs", "attrpath", "attr", "string_attr", "expr_list", "formals",
  "formal", YY_NULL
};
#endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    59,    60,    61,    62,    62,    62,    62,    62,    62,
      62,    62,    63,    63,    64,    64,    64,    64,    64,    64,
      64,    64,    64,    64,    64,    64,    64,    64,    64,    64,
      64,    64,    64,    65,    65,    66,    66,    66,    66,    67,
      67,    67,    67,    67,    67,    67,    67,    67,    67,    67,
      67,    67,    67,    68,    68,    68,    69,    69,    69,    69,
      70,    70,    70,    71,    71,    71,    71,    72,    72,    72,
      73,    73,    73,    73,    74,    74,    75,    75,    76,    76,
      77,    77,    77,    77,    78,    78
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     1,     3,     5,     7,     7,     4,     4,
       4,     1,     6,     1,     2,     2,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     1,     2,     1,     3,     5,     2,     1,     1,
       1,     1,     3,     3,     1,     1,     1,     1,     3,     4,
       4,     3,     3,     1,     1,     0,     2,     4,     3,     4,
       2,     4,     0,     5,     4,     7,     0,     2,     2,     0,
       3,     3,     1,     1,     1,     1,     3,     3,     2,     0,
       3,     1,     0,     1,     1,     3
};

/* YYDPREC[RULE-NUM] -- Dynamic precedence of rule #RULE-NUM (0 if none).  */
static const unsigned char yydprec[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0
};

/* YYMERGER[RULE-NUM] -- Index of merging function for rule #RULE-NUM.  */
static const unsigned char yymerger[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0
};

/* YYDEFACT[S] -- default reduction number in state S.  Performed when
   YYTABLE doesn't specify something else to do.  Zero means the default
   is an error.  */
static const unsigned char yydefact[] =
{
       0,    39,    40,    41,    44,    45,    46,    47,     0,     0,
       0,    66,     0,    62,     0,    66,     0,    55,     0,    79,
       0,     2,     3,    11,    13,    32,    34,    38,     0,     0,
       0,     0,     0,    66,     0,    66,     0,    39,     0,    66,
      15,    84,    83,     0,     0,    81,    14,    53,     0,     0,
      54,     0,     0,     1,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      33,    37,     0,     4,    82,     0,     0,     0,     0,    74,
       0,    69,    75,     0,    55,     0,    72,    73,     0,    60,
       0,    43,     0,    51,     0,    82,     0,     0,    42,    56,
       0,    48,    52,    78,    16,    17,    22,    23,    24,    18,
      20,    21,    19,    25,    27,    28,    29,    30,    31,    26,
      35,     0,     0,     8,     9,    49,    10,     0,     0,     0,
       0,     0,     0,    50,     0,    85,     0,     0,    80,     0,
      58,     0,     0,     0,     0,     0,    64,    67,    68,    77,
      76,    70,    71,     0,    61,     5,     0,    59,    57,    36,
       0,    12,    69,    63,     0,     7,     0,     6,    65
};

/* YYPDEFGOTO[NTERM-NUM].  */
static const short int yydefgoto[] =
{
      -1,    20,    21,    22,    23,    24,    25,    26,    27,    49,
      50,    36,    43,   128,    85,    86,    87,    52,    44,    45
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -123
static const short int yypact[] =
{
     134,    72,  -123,  -123,  -123,  -123,  -123,  -123,   134,   134,
     134,   -30,   -24,  -123,   159,     2,   159,    42,   134,  -123,
       7,  -123,  -123,  -123,   211,   210,  -123,   -14,   134,   -17,
      34,     9,    15,  -123,    11,  -123,    51,  -123,   -30,  -123,
    -123,    17,  -123,    14,    26,    19,   152,    50,   134,    33,
      48,    32,    16,  -123,   159,   159,   159,   159,   159,   159,
     159,   159,   159,   159,   159,   159,   159,   159,   159,    88,
    -123,  -123,    88,  -123,    12,   134,   134,   134,    62,  -123,
     134,    59,  -123,   134,    42,   -39,  -123,  -123,   154,  -123,
     134,  -123,   134,  -123,   108,    12,   134,    46,  -123,  -123,
     134,  -123,  -123,  -123,   279,   279,   301,   257,   235,    87,
      87,    87,    87,    87,   118,   118,   -22,   -22,   -22,    36,
      23,    66,   103,  -123,  -123,  -123,  -123,   134,   175,    75,
      79,    88,   134,  -123,    86,  -123,   134,   135,  -123,   101,
    -123,   102,   210,   110,   134,   111,  -123,  -123,  -123,  -123,
    -123,  -123,  -123,   115,  -123,  -123,   129,  -123,  -123,  -123,
     134,  -123,  -123,  -123,   134,  -123,   180,  -123,  -123
};

/* YYPGOTO[NTERM-NUM].  */
static const signed char yypgoto[] =
{
    -123,  -123,    -8,   -25,  -123,    40,  -123,   -21,  -123,    92,
    -123,  -123,    99,    10,   127,  -122,  -120,  -123,   -15,  -123
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -67
static const short int yytable[] =
{
      30,    31,    32,    73,    70,    41,   147,    53,   148,   151,
      51,   152,   131,    71,    79,    41,    33,    79,   132,    37,
      68,    69,    35,     2,     3,     4,     5,     6,     7,    74,
      80,   103,    81,    42,    38,    81,    12,    72,    82,    83,
      97,    82,    83,    42,   147,    13,   148,    47,    75,   -66,
     142,   123,   124,    99,    40,   126,    46,    89,    76,   121,
      92,    93,    39,    84,    77,    79,    84,   122,    17,    18,
      48,    19,   102,    94,   131,   129,   100,    95,    96,    90,
     138,    91,   134,    81,   135,    98,   101,   131,   139,    82,
      83,    79,   141,   140,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   125,
      34,   155,   127,   143,    84,    82,    83,    28,   144,   145,
      29,   159,   149,    63,   153,    64,    65,    66,    67,    68,
      69,   150,    78,   154,    88,   165,   161,     1,   156,   167,
      84,     2,     3,     4,     5,     6,     7,     8,   157,   158,
       9,    10,    11,   136,    12,   160,   137,    79,    66,    67,
      68,    69,    37,    13,   163,   162,     2,     3,     4,     5,
       6,     7,   166,    14,   164,    81,   130,    38,    79,    12,
      15,    82,    83,    79,    16,     0,    17,    18,    13,    19,
      64,    65,    66,    67,    68,    69,   119,     0,    14,   120,
       0,   133,    82,    83,     0,    39,    84,    82,    83,    16,
       0,    17,    18,    37,    19,     0,     0,     2,     3,     4,
       5,     6,     7,     0,   146,     0,     0,    84,    38,   168,
      12,     0,    84,    54,    55,    56,    57,    58,     0,    13,
       0,     0,     0,    59,    60,    61,    62,    63,     0,    64,
      65,    66,    67,    68,    69,     0,    39,    54,    55,    56,
      57,   -67,    17,    18,     0,    19,     0,    59,    60,    61,
      62,    63,     0,    64,    65,    66,    67,    68,    69,    54,
      55,    56,     0,     0,     0,     0,     0,     0,     0,    59,
      60,    61,    62,    63,     0,    64,    65,    66,    67,    68,
      69,   -67,   -67,     0,     0,     0,     0,     0,     0,     0,
       0,    59,    60,    61,    62,    63,     0,    64,    65,    66,
      67,    68,    69,    54,    55,     0,     0,     0,     0,     0,
       0,     0,     0,    59,    60,    61,    62,    63,     0,    64,
      65,    66,    67,    68,    69
};

/* YYCONFLP[YYPACT[STATE-NUM]] -- Pointer into YYCONFL of start of
   list of conflicting reductions corresponding to action entry for
   state STATE-NUM in yytable.  0 means no conflicts.  The list in
   yyconfl is terminated by a rule number of 0.  */
static const unsigned char yyconflp[] =
{
       0,     0,     0,     0,     0,     1,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     3,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0
};

/* YYCONFL[I] -- lists of conflicting rule numbers, each terminated by
   0, pointed into by YYCONFLP.  */
static const short int yyconfl[] =
{
       0,    66,     0,    82,     0
};

static const short int yycheck[] =
{
       8,     9,    10,    28,    25,     3,   128,     0,   128,   131,
      18,   131,    51,    27,     3,     3,    46,     3,    57,     3,
      42,    43,    46,     7,     8,     9,    10,    11,    12,    46,
      19,    52,    21,    31,    18,    21,    20,    51,    27,    28,
      48,    27,    28,    31,   166,    29,   166,     5,    14,    47,
      27,    76,    77,     5,    14,    80,    16,     6,    49,    74,
      43,    47,    46,    52,    49,     3,    52,    75,    52,    53,
      28,    55,    56,    47,    51,    83,    28,    58,    28,    28,
      95,    30,    90,    21,    92,    52,    54,    51,    96,    27,
      28,     3,   100,    47,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    47,
      11,   136,    53,    47,    52,    27,    28,    45,    15,   127,
      48,   142,    47,    36,   132,    38,    39,    40,    41,    42,
      43,    52,    33,    47,    35,   160,   144,     3,     3,   164,
      52,     7,     8,     9,    10,    11,    12,    13,    47,    47,
      16,    17,    18,    45,    20,    45,    48,     3,    40,    41,
      42,    43,     3,    29,    49,    54,     7,     8,     9,    10,
      11,    12,   162,    39,    45,    21,    84,    18,     3,    20,
      46,    27,    28,     3,    50,    -1,    52,    53,    29,    55,
      38,    39,    40,    41,    42,    43,    69,    -1,    39,    72,
      -1,    47,    27,    28,    -1,    46,    52,    27,    28,    50,
      -1,    52,    53,     3,    55,    -1,    -1,     7,     8,     9,
      10,    11,    12,    -1,    49,    -1,    -1,    52,    18,    49,
      20,    -1,    52,    22,    23,    24,    25,    26,    -1,    29,
      -1,    -1,    -1,    32,    33,    34,    35,    36,    -1,    38,
      39,    40,    41,    42,    43,    -1,    46,    22,    23,    24,
      25,    26,    52,    53,    -1,    55,    -1,    32,    33,    34,
      35,    36,    -1,    38,    39,    40,    41,    42,    43,    22,
      23,    24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    32,
      33,    34,    35,    36,    -1,    38,    39,    40,    41,    42,
      43,    22,    23,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    32,    33,    34,    35,    36,    -1,    38,    39,    40,
      41,    42,    43,    22,    23,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    32,    33,    34,    35,    36,    -1,    38,
      39,    40,    41,    42,    43
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     3,     7,     8,     9,    10,    11,    12,    13,    16,
      17,    18,    20,    29,    39,    46,    50,    52,    53,    55,
      60,    61,    62,    63,    64,    65,    66,    67,    45,    48,
      61,    61,    61,    46,    71,    46,    70,     3,    18,    46,
      64,     3,    31,    71,    77,    78,    64,     5,    28,    68,
      69,    61,    76,     0,    22,    23,    24,    25,    26,    32,
      33,    34,    35,    36,    38,    39,    40,    41,    42,    43,
      66,    27,    51,    62,    46,    14,    49,    49,    71,     3,
      19,    21,    27,    28,    52,    73,    74,    75,    71,     6,
      28,    30,    43,    47,    47,    58,    28,    61,    52,     5,
      28,    54,    56,    66,    64,    64,    64,    64,    64,    64,
      64,    64,    64,    64,    64,    64,    64,    64,    64,    73,
      73,    77,    61,    62,    62,    47,    62,    53,    72,    61,
      68,    51,    57,    47,    61,    61,    45,    48,    77,    61,
      47,    61,    27,    47,    15,    61,    49,    74,    75,    47,
      52,    74,    75,    61,    47,    62,     3,    47,    47,    66,
      45,    61,    54,    49,    45,    62,    72,    62,    49
};

/* Error token number */
#define YYTERROR 1


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (YYID (N))                                                     \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (YYID (0))
#endif

# define YYRHSLOC(Rhs, K) ((Rhs)[K].yystate.yyloc)


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

__attribute__((__unused__))
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static unsigned
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
#else
static unsigned
yy_location_print_ (yyo, yylocp)
    FILE *yyo;
    YYLTYPE const * const yylocp;
#endif
{
  unsigned res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += fprintf (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += fprintf (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += fprintf (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += fprintf (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += fprintf (yyo, "-%d", end_col);
    }
  return res;
 }

#  define YY_LOCATION_PRINT(File, Loc)          \
  yy_location_print_ (File, &(Loc))

# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */
#define YYLEX yylex (&yylval, &yylloc, scanner, data)


#undef yynerrs
#define yynerrs (yystackp->yyerrcnt)
#undef yychar
#define yychar (yystackp->yyrawchar)
#undef yylval
#define yylval (yystackp->yyval)
#undef yylloc
#define yylloc (yystackp->yyloc)


static const int YYEOF = 0;
static const int YYEMPTY = -2;

typedef enum { yyok, yyaccept, yyabort, yyerr } YYRESULTTAG;

#define YYCHK(YYE)                                                           \
   do { YYRESULTTAG yyflag = YYE; if (yyflag != yyok) return yyflag; }       \
   while (YYID (0))

#if YYDEBUG

# ifndef YYFPRINTF
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, void * scanner, nix::ParseData * data)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
  YYUSE (yylocationp);
  YYUSE (scanner);
  YYUSE (data);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
        break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, void * scanner, nix::ParseData * data)
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, scanner, data);
  YYFPRINTF (yyoutput, ")");
}

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)          \
do {                                                            \
  if (yydebug)                                                  \
    {                                                           \
      YYFPRINTF (stderr, "%s ", Title);                         \
      yy_symbol_print (stderr, Type, Value, Location, scanner, data);        \
      YYFPRINTF (stderr, "\n");                                 \
    }                                                           \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;

#else /* !YYDEBUG */

# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)

#endif /* !YYDEBUG */

/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYMAXDEPTH * sizeof (GLRStackItem)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif

/* Minimum number of free items on the stack allowed after an
   allocation.  This is to allow allocation and initialization
   to be completed by functions that call yyexpandGLRStack before the
   stack is expanded, thus insuring that all necessary pointers get
   properly redirected to new data.  */
#define YYHEADROOM 2

#ifndef YYSTACKEXPANDABLE
# if (! defined __cplusplus \
      || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
          && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL))
#  define YYSTACKEXPANDABLE 1
# else
#  define YYSTACKEXPANDABLE 0
# endif
#endif

#if YYSTACKEXPANDABLE
# define YY_RESERVE_GLRSTACK(Yystack)                   \
  do {                                                  \
    if (Yystack->yyspaceLeft < YYHEADROOM)              \
      yyexpandGLRStack (Yystack);                       \
  } while (YYID (0))
#else
# define YY_RESERVE_GLRSTACK(Yystack)                   \
  do {                                                  \
    if (Yystack->yyspaceLeft < YYHEADROOM)              \
      yyMemoryExhausted (Yystack);                      \
  } while (YYID (0))
#endif


#if YYERROR_VERBOSE

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static size_t
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      size_t yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return strlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

#endif /* !YYERROR_VERBOSE */

/** State numbers, as in LALR(1) machine */
typedef int yyStateNum;

/** Rule numbers, as in LALR(1) machine */
typedef int yyRuleNum;

/** Grammar symbol */
typedef short int yySymbol;

/** Item references, as in LALR(1) machine */
typedef short int yyItemNum;

typedef struct yyGLRState yyGLRState;
typedef struct yyGLRStateSet yyGLRStateSet;
typedef struct yySemanticOption yySemanticOption;
typedef union yyGLRStackItem yyGLRStackItem;
typedef struct yyGLRStack yyGLRStack;

struct yyGLRState {
  /** Type tag: always true.  */
  yybool yyisState;
  /** Type tag for yysemantics.  If true, yysval applies, otherwise
   *  yyfirstVal applies.  */
  yybool yyresolved;
  /** Number of corresponding LALR(1) machine state.  */
  yyStateNum yylrState;
  /** Preceding state in this stack */
  yyGLRState* yypred;
  /** Source position of the first token produced by my symbol */
  size_t yyposn;
  union {
    /** First in a chain of alternative reductions producing the
     *  non-terminal corresponding to this state, threaded through
     *  yynext.  */
    yySemanticOption* yyfirstVal;
    /** Semantic value for this state.  */
    YYSTYPE yysval;
  } yysemantics;
  /** Source location for this state.  */
  YYLTYPE yyloc;
};

struct yyGLRStateSet {
  yyGLRState** yystates;
  /** During nondeterministic operation, yylookaheadNeeds tracks which
   *  stacks have actually needed the current lookahead.  During deterministic
   *  operation, yylookaheadNeeds[0] is not maintained since it would merely
   *  duplicate yychar != YYEMPTY.  */
  yybool* yylookaheadNeeds;
  size_t yysize, yycapacity;
};

struct yySemanticOption {
  /** Type tag: always false.  */
  yybool yyisState;
  /** Rule number for this reduction */
  yyRuleNum yyrule;
  /** The last RHS state in the list of states to be reduced.  */
  yyGLRState* yystate;
  /** The lookahead for this reduction.  */
  int yyrawchar;
  YYSTYPE yyval;
  YYLTYPE yyloc;
  /** Next sibling in chain of options.  To facilitate merging,
   *  options are chained in decreasing order by address.  */
  yySemanticOption* yynext;
};

/** Type of the items in the GLR stack.  The yyisState field
 *  indicates which item of the union is valid.  */
union yyGLRStackItem {
  yyGLRState yystate;
  yySemanticOption yyoption;
};

struct yyGLRStack {
  int yyerrState;
  /* To compute the location of the error token.  */
  yyGLRStackItem yyerror_range[3];

  int yyerrcnt;
  int yyrawchar;
  YYSTYPE yyval;
  YYLTYPE yyloc;

  YYJMP_BUF yyexception_buffer;
  yyGLRStackItem* yyitems;
  yyGLRStackItem* yynextFree;
  size_t yyspaceLeft;
  yyGLRState* yysplitPoint;
  yyGLRState* yylastDeleted;
  yyGLRStateSet yytops;
};

#if YYSTACKEXPANDABLE
static void yyexpandGLRStack (yyGLRStack* yystackp);
#endif

static void yyFail (yyGLRStack* yystackp, YYLTYPE *yylocp, void * scanner, nix::ParseData * data, const char* yymsg)
  __attribute__ ((__noreturn__));
static void
yyFail (yyGLRStack* yystackp, YYLTYPE *yylocp, void * scanner, nix::ParseData * data, const char* yymsg)
{
  if (yymsg != YY_NULL)
    yyerror (yylocp, scanner, data, yymsg);
  YYLONGJMP (yystackp->yyexception_buffer, 1);
}

static void yyMemoryExhausted (yyGLRStack* yystackp)
  __attribute__ ((__noreturn__));
static void
yyMemoryExhausted (yyGLRStack* yystackp)
{
  YYLONGJMP (yystackp->yyexception_buffer, 2);
}

#if YYDEBUG || YYERROR_VERBOSE
/** A printable representation of TOKEN.  */
static inline const char*
yytokenName (yySymbol yytoken)
{
  if (yytoken == YYEMPTY)
    return "";

  return yytname[yytoken];
}
#endif

/** Fill in YYVSP[YYLOW1 .. YYLOW0-1] from the chain of states starting
 *  at YYVSP[YYLOW0].yystate.yypred.  Leaves YYVSP[YYLOW1].yystate.yypred
 *  containing the pointer to the next state in the chain.  */
static void yyfillin (yyGLRStackItem *, int, int) __attribute__ ((__unused__));
static void
yyfillin (yyGLRStackItem *yyvsp, int yylow0, int yylow1)
{
  int i;
  yyGLRState *s = yyvsp[yylow0].yystate.yypred;
  for (i = yylow0-1; i >= yylow1; i -= 1)
    {
      YYASSERT (s->yyresolved);
      yyvsp[i].yystate.yyresolved = yytrue;
      yyvsp[i].yystate.yysemantics.yysval = s->yysemantics.yysval;
      yyvsp[i].yystate.yyloc = s->yyloc;
      s = yyvsp[i].yystate.yypred = s->yypred;
    }
}

/* Do nothing if YYNORMAL or if *YYLOW <= YYLOW1.  Otherwise, fill in
 * YYVSP[YYLOW1 .. *YYLOW-1] as in yyfillin and set *YYLOW = YYLOW1.
 * For convenience, always return YYLOW1.  */
static inline int yyfill (yyGLRStackItem *, int *, int, yybool)
     __attribute__ ((__unused__));
static inline int
yyfill (yyGLRStackItem *yyvsp, int *yylow, int yylow1, yybool yynormal)
{
  if (!yynormal && yylow1 < *yylow)
    {
      yyfillin (yyvsp, *yylow, yylow1);
      *yylow = yylow1;
    }
  return yylow1;
}

/** Perform user action for rule number YYN, with RHS length YYRHSLEN,
 *  and top stack item YYVSP.  YYLVALP points to place to put semantic
 *  value ($$), and yylocp points to place for location information
 *  (@$).  Returns yyok for normal return, yyaccept for YYACCEPT,
 *  yyerr for YYERROR, yyabort for YYABORT.  */
/*ARGSUSED*/ static YYRESULTTAG
yyuserAction (yyRuleNum yyn, int yyrhslen, yyGLRStackItem* yyvsp,
              yyGLRStack* yystackp,
              YYSTYPE* yyvalp, YYLTYPE *yylocp, void * scanner, nix::ParseData * data)
{
  yybool yynormal __attribute__ ((__unused__)) =
    (yystackp->yysplitPoint == YY_NULL);
  int yylow;
  YYUSE (scanner);
  YYUSE (data);
# undef yyerrok
# define yyerrok (yystackp->yyerrState = 0)
# undef YYACCEPT
# define YYACCEPT return yyaccept
# undef YYABORT
# define YYABORT return yyabort
# undef YYERROR
# define YYERROR return yyerrok, yyerr
# undef YYRECOVERING
# define YYRECOVERING() (yystackp->yyerrState != 0)
# undef yyclearin
# define yyclearin (yychar = YYEMPTY)
# undef YYFILL
# define YYFILL(N) yyfill (yyvsp, &yylow, N, yynormal)
# undef YYBACKUP
# define YYBACKUP(Token, Value)                                              \
  return yyerror (yylocp, scanner, data, YY_("syntax error: cannot back up")),     \
         yyerrok, yyerr

  yylow = 1;
  if (yyrhslen == 0)
    *yyvalp = yyval_default;
  else
    *yyvalp = yyvsp[YYFILL (1-yyrhslen)].yystate.yysemantics.yysval;
  YYLLOC_DEFAULT ((*yylocp), (yyvsp - yyrhslen), yyrhslen);
  yystackp->yyerror_range[1].yystate.yyloc = *yylocp;

  switch (yyn)
    {
        case 2:
/* Line 868 of glr.c  */
#line 293 "src/libexpr/parser.y"
    { data->result = (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yysemantics.yysval.e); }
    break;

  case 4:
/* Line 868 of glr.c  */
#line 299 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprLambda(CUR_POS, data->symbols.create((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval.id)), false, 0, (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval.e)); }
    break;

  case 5:
/* Line 868 of glr.c  */
#line 301 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprLambda(CUR_POS, data->symbols.create(""), true, (((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (5))].yystate.yysemantics.yysval.formals), (((yyGLRStackItem const *)yyvsp)[YYFILL ((5) - (5))].yystate.yysemantics.yysval.e)); }
    break;

  case 6:
/* Line 868 of glr.c  */
#line 303 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprLambda(CUR_POS, data->symbols.create((((yyGLRStackItem const *)yyvsp)[YYFILL ((5) - (7))].yystate.yysemantics.yysval.id)), true, (((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (7))].yystate.yysemantics.yysval.formals), (((yyGLRStackItem const *)yyvsp)[YYFILL ((7) - (7))].yystate.yysemantics.yysval.e)); }
    break;

  case 7:
/* Line 868 of glr.c  */
#line 305 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprLambda(CUR_POS, data->symbols.create((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (7))].yystate.yysemantics.yysval.id)), true, (((yyGLRStackItem const *)yyvsp)[YYFILL ((4) - (7))].yystate.yysemantics.yysval.formals), (((yyGLRStackItem const *)yyvsp)[YYFILL ((7) - (7))].yystate.yysemantics.yysval.e)); }
    break;

  case 8:
/* Line 868 of glr.c  */
#line 307 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprAssert(CUR_POS, (((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (4))].yystate.yysemantics.yysval.e), (((yyGLRStackItem const *)yyvsp)[YYFILL ((4) - (4))].yystate.yysemantics.yysval.e)); }
    break;

  case 9:
/* Line 868 of glr.c  */
#line 309 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprWith(CUR_POS, (((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (4))].yystate.yysemantics.yysval.e), (((yyGLRStackItem const *)yyvsp)[YYFILL ((4) - (4))].yystate.yysemantics.yysval.e)); }
    break;

  case 10:
/* Line 868 of glr.c  */
#line 311 "src/libexpr/parser.y"
    { if (!(((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (4))].yystate.yysemantics.yysval.attrs)->dynamicAttrs.empty())
        throw ParseError(format("dynamic attributes not allowed in let at %1%")
            % CUR_POS);
      ((*yyvalp).e) = new ExprLet((((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (4))].yystate.yysemantics.yysval.attrs), (((yyGLRStackItem const *)yyvsp)[YYFILL ((4) - (4))].yystate.yysemantics.yysval.e));
    }
    break;

  case 12:
/* Line 868 of glr.c  */
#line 320 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprIf((((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (6))].yystate.yysemantics.yysval.e), (((yyGLRStackItem const *)yyvsp)[YYFILL ((4) - (6))].yystate.yysemantics.yysval.e), (((yyGLRStackItem const *)yyvsp)[YYFILL ((6) - (6))].yystate.yysemantics.yysval.e)); }
    break;

  case 14:
/* Line 868 of glr.c  */
#line 325 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprOpNot((((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yysemantics.yysval.e)); }
    break;

  case 15:
/* Line 868 of glr.c  */
#line 326 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprApp(CUR_POS, new ExprApp(new ExprVar(data->symbols.create("__sub")), new ExprInt(0)), (((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yysemantics.yysval.e)); }
    break;

  case 16:
/* Line 868 of glr.c  */
#line 327 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprOpEq((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval.e), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval.e)); }
    break;

  case 17:
/* Line 868 of glr.c  */
#line 328 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprOpNEq((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval.e), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval.e)); }
    break;

  case 18:
/* Line 868 of glr.c  */
#line 329 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprApp(CUR_POS, new ExprApp(new ExprVar(data->symbols.create("__lessThan")), (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval.e)), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval.e)); }
    break;

  case 19:
/* Line 868 of glr.c  */
#line 330 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprOpNot(new ExprApp(CUR_POS, new ExprApp(new ExprVar(data->symbols.create("__lessThan")), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval.e)), (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval.e))); }
    break;

  case 20:
/* Line 868 of glr.c  */
#line 331 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprApp(CUR_POS, new ExprApp(new ExprVar(data->symbols.create("__lessThan")), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval.e)), (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval.e)); }
    break;

  case 21:
/* Line 868 of glr.c  */
#line 332 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprOpNot(new ExprApp(CUR_POS, new ExprApp(new ExprVar(data->symbols.create("__lessThan")), (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval.e)), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval.e))); }
    break;

  case 22:
/* Line 868 of glr.c  */
#line 333 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprOpAnd(CUR_POS, (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval.e), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval.e)); }
    break;

  case 23:
/* Line 868 of glr.c  */
#line 334 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprOpOr(CUR_POS, (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval.e), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval.e)); }
    break;

  case 24:
/* Line 868 of glr.c  */
#line 335 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprOpImpl(CUR_POS, (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval.e), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval.e)); }
    break;

  case 25:
/* Line 868 of glr.c  */
#line 336 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprOpUpdate(CUR_POS, (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval.e), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval.e)); }
    break;

  case 26:
/* Line 868 of glr.c  */
#line 337 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprOpHasAttr((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval.e), *(((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval.attrNames)); }
    break;

  case 27:
/* Line 868 of glr.c  */
#line 339 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprConcatStrings(CUR_POS, false, new vector<Expr *>({(((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval.e), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval.e)})); }
    break;

  case 28:
/* Line 868 of glr.c  */
#line 340 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprApp(CUR_POS, new ExprApp(new ExprVar(data->symbols.create("__sub")), (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval.e)), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval.e)); }
    break;

  case 29:
/* Line 868 of glr.c  */
#line 341 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprApp(CUR_POS, new ExprApp(new ExprVar(data->symbols.create("__mul")), (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval.e)), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval.e)); }
    break;

  case 30:
/* Line 868 of glr.c  */
#line 342 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprApp(CUR_POS, new ExprApp(new ExprVar(data->symbols.create("__div")), (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval.e)), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval.e)); }
    break;

  case 31:
/* Line 868 of glr.c  */
#line 343 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprOpConcatLists(CUR_POS, (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval.e), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval.e)); }
    break;

  case 33:
/* Line 868 of glr.c  */
#line 349 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprApp(CUR_POS, (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yysemantics.yysval.e), (((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yysemantics.yysval.e)); }
    break;

  case 34:
/* Line 868 of glr.c  */
#line 350 "src/libexpr/parser.y"
    { ((*yyvalp).e) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yysemantics.yysval.e); }
    break;

  case 35:
/* Line 868 of glr.c  */
#line 355 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprSelect(CUR_POS, (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval.e), *(((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval.attrNames), 0); }
    break;

  case 36:
/* Line 868 of glr.c  */
#line 357 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprSelect(CUR_POS, (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (5))].yystate.yysemantics.yysval.e), *(((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (5))].yystate.yysemantics.yysval.attrNames), (((yyGLRStackItem const *)yyvsp)[YYFILL ((5) - (5))].yystate.yysemantics.yysval.e)); }
    break;

  case 37:
/* Line 868 of glr.c  */
#line 361 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprApp(CUR_POS, (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yysemantics.yysval.e), new ExprVar(CUR_POS, data->symbols.create("or"))); }
    break;

  case 38:
/* Line 868 of glr.c  */
#line 362 "src/libexpr/parser.y"
    { ((*yyvalp).e) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yysemantics.yysval.e); }
    break;

  case 39:
/* Line 868 of glr.c  */
#line 366 "src/libexpr/parser.y"
    {
      if (strcmp((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yysemantics.yysval.id), "__curPos") == 0)
          ((*yyvalp).e) = new ExprPos(CUR_POS);
      else
          ((*yyvalp).e) = new ExprVar(CUR_POS, data->symbols.create((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yysemantics.yysval.id)));
  }
    break;

  case 40:
/* Line 868 of glr.c  */
#line 372 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprInt((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yysemantics.yysval.n)); }
    break;

  case 41:
/* Line 868 of glr.c  */
#line 373 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprFloat((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yysemantics.yysval.nf)); }
    break;

  case 42:
/* Line 868 of glr.c  */
#line 374 "src/libexpr/parser.y"
    { ((*yyvalp).e) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (3))].yystate.yysemantics.yysval.e); }
    break;

  case 43:
/* Line 868 of glr.c  */
#line 375 "src/libexpr/parser.y"
    {
      ((*yyvalp).e) = stripIndentation(CUR_POS, data->symbols, *(((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (3))].yystate.yysemantics.yysval.string_parts));
  }
    break;

  case 44:
/* Line 868 of glr.c  */
#line 378 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprPath(absPath((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yysemantics.yysval.path), data->basePath)); }
    break;

  case 45:
/* Line 868 of glr.c  */
#line 379 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprPath(getEnv("HOME", "") + string{(((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yysemantics.yysval.path) + 1}); }
    break;

  case 46:
/* Line 868 of glr.c  */
#line 380 "src/libexpr/parser.y"
    {
      string path((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yysemantics.yysval.path) + 1, strlen((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yysemantics.yysval.path)) - 2);
      ((*yyvalp).e) = new ExprApp(CUR_POS,
          new ExprApp(new ExprVar(data->symbols.create("__findFile")),
              new ExprVar(data->symbols.create("__nixPath"))),
          new ExprString(data->symbols.create(path)));
  }
    break;

  case 47:
/* Line 868 of glr.c  */
#line 387 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprString(data->symbols.create((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yysemantics.yysval.uri))); }
    break;

  case 48:
/* Line 868 of glr.c  */
#line 388 "src/libexpr/parser.y"
    { ((*yyvalp).e) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (3))].yystate.yysemantics.yysval.e); }
    break;

  case 49:
/* Line 868 of glr.c  */
#line 392 "src/libexpr/parser.y"
    { (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (4))].yystate.yysemantics.yysval.attrs)->recursive = true; ((*yyvalp).e) = new ExprSelect(noPos, (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (4))].yystate.yysemantics.yysval.attrs), data->symbols.create("body")); }
    break;

  case 50:
/* Line 868 of glr.c  */
#line 394 "src/libexpr/parser.y"
    { (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (4))].yystate.yysemantics.yysval.attrs)->recursive = true; ((*yyvalp).e) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (4))].yystate.yysemantics.yysval.attrs); }
    break;

  case 51:
/* Line 868 of glr.c  */
#line 396 "src/libexpr/parser.y"
    { ((*yyvalp).e) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (3))].yystate.yysemantics.yysval.attrs); }
    break;

  case 52:
/* Line 868 of glr.c  */
#line 397 "src/libexpr/parser.y"
    { ((*yyvalp).e) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (3))].yystate.yysemantics.yysval.list); }
    break;

  case 54:
/* Line 868 of glr.c  */
#line 402 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprConcatStrings(CUR_POS, true, (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yysemantics.yysval.string_parts)); }
    break;

  case 55:
/* Line 868 of glr.c  */
#line 403 "src/libexpr/parser.y"
    { ((*yyvalp).e) = new ExprString(data->symbols.create("")); }
    break;

  case 56:
/* Line 868 of glr.c  */
#line 407 "src/libexpr/parser.y"
    { ((*yyvalp).string_parts) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yysemantics.yysval.string_parts); (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yysemantics.yysval.string_parts)->push_back((((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yysemantics.yysval.e)); }
    break;

  case 57:
/* Line 868 of glr.c  */
#line 408 "src/libexpr/parser.y"
    { ((*yyvalp).string_parts) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (4))].yystate.yysemantics.yysval.string_parts); (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (4))].yystate.yysemantics.yysval.string_parts)->push_back((((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (4))].yystate.yysemantics.yysval.e)); }
    break;

  case 58:
/* Line 868 of glr.c  */
#line 409 "src/libexpr/parser.y"
    { ((*yyvalp).string_parts) = new vector<Expr *>; ((*yyvalp).string_parts)->push_back((((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (3))].yystate.yysemantics.yysval.e)); }
    break;

  case 59:
/* Line 868 of glr.c  */
#line 410 "src/libexpr/parser.y"
    {
      ((*yyvalp).string_parts) = new vector<Expr *>;
      ((*yyvalp).string_parts)->push_back((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (4))].yystate.yysemantics.yysval.e));
      ((*yyvalp).string_parts)->push_back((((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (4))].yystate.yysemantics.yysval.e));
    }
    break;

  case 60:
/* Line 868 of glr.c  */
#line 418 "src/libexpr/parser.y"
    { ((*yyvalp).string_parts) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yysemantics.yysval.string_parts); (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yysemantics.yysval.string_parts)->push_back((((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yysemantics.yysval.e)); }
    break;

  case 61:
/* Line 868 of glr.c  */
#line 419 "src/libexpr/parser.y"
    { ((*yyvalp).string_parts) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (4))].yystate.yysemantics.yysval.string_parts); (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (4))].yystate.yysemantics.yysval.string_parts)->push_back((((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (4))].yystate.yysemantics.yysval.e)); }
    break;

  case 62:
/* Line 868 of glr.c  */
#line 420 "src/libexpr/parser.y"
    { ((*yyvalp).string_parts) = new vector<Expr *>; }
    break;

  case 63:
/* Line 868 of glr.c  */
#line 424 "src/libexpr/parser.y"
    { ((*yyvalp).attrs) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (5))].yystate.yysemantics.yysval.attrs); addAttr(((*yyvalp).attrs), *(((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (5))].yystate.yysemantics.yysval.attrNames), (((yyGLRStackItem const *)yyvsp)[YYFILL ((4) - (5))].yystate.yysemantics.yysval.e), makeCurPos((((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (5))].yystate.yyloc), data)); }
    break;

  case 64:
/* Line 868 of glr.c  */
#line 426 "src/libexpr/parser.y"
    { ((*yyvalp).attrs) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (4))].yystate.yysemantics.yysval.attrs);
      for (auto & i : *(((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (4))].yystate.yysemantics.yysval.attrNames)) {
          if (((*yyvalp).attrs)->attrs.find(i.symbol) != ((*yyvalp).attrs)->attrs.end())
              dupAttr(i.symbol, makeCurPos((((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (4))].yystate.yyloc), data), ((*yyvalp).attrs)->attrs[i.symbol].pos);
          Pos pos = makeCurPos((((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (4))].yystate.yyloc), data);
          ((*yyvalp).attrs)->attrs[i.symbol] = ExprAttrs::AttrDef(new ExprVar(CUR_POS, i.symbol), pos, true);
      }
    }
    break;

  case 65:
/* Line 868 of glr.c  */
#line 435 "src/libexpr/parser.y"
    { ((*yyvalp).attrs) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (7))].yystate.yysemantics.yysval.attrs);
      /* !!! Should ensure sharing of the expression in $4. */
      for (auto & i : *(((yyGLRStackItem const *)yyvsp)[YYFILL ((6) - (7))].yystate.yysemantics.yysval.attrNames)) {
          if (((*yyvalp).attrs)->attrs.find(i.symbol) != ((*yyvalp).attrs)->attrs.end())
              dupAttr(i.symbol, makeCurPos((((yyGLRStackItem const *)yyvsp)[YYFILL ((6) - (7))].yystate.yyloc), data), ((*yyvalp).attrs)->attrs[i.symbol].pos);
          ((*yyvalp).attrs)->attrs[i.symbol] = ExprAttrs::AttrDef(new ExprSelect(CUR_POS, (((yyGLRStackItem const *)yyvsp)[YYFILL ((4) - (7))].yystate.yysemantics.yysval.e), i.symbol), makeCurPos((((yyGLRStackItem const *)yyvsp)[YYFILL ((6) - (7))].yystate.yyloc), data));
      }
    }
    break;

  case 66:
/* Line 868 of glr.c  */
#line 443 "src/libexpr/parser.y"
    { ((*yyvalp).attrs) = new ExprAttrs; }
    break;

  case 67:
/* Line 868 of glr.c  */
#line 447 "src/libexpr/parser.y"
    { ((*yyvalp).attrNames) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yysemantics.yysval.attrNames); (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yysemantics.yysval.attrNames)->push_back(AttrName(data->symbols.create((((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yysemantics.yysval.id)))); }
    break;

  case 68:
/* Line 868 of glr.c  */
#line 449 "src/libexpr/parser.y"
    { ((*yyvalp).attrNames) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yysemantics.yysval.attrNames);
      ExprString * str = dynamic_cast<ExprString *>((((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yysemantics.yysval.e));
      if (str) {
          ((*yyvalp).attrNames)->push_back(AttrName(str->s));
          delete str;
      } else
          throw ParseError(format("dynamic attributes not allowed in inherit at %1%")
              % makeCurPos((((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yyloc), data));
    }
    break;

  case 69:
/* Line 868 of glr.c  */
#line 458 "src/libexpr/parser.y"
    { ((*yyvalp).attrNames) = new AttrPath; }
    break;

  case 70:
/* Line 868 of glr.c  */
#line 462 "src/libexpr/parser.y"
    { ((*yyvalp).attrNames) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval.attrNames); (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval.attrNames)->push_back(AttrName(data->symbols.create((((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval.id)))); }
    break;

  case 71:
/* Line 868 of glr.c  */
#line 464 "src/libexpr/parser.y"
    { ((*yyvalp).attrNames) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval.attrNames);
      ExprString * str = dynamic_cast<ExprString *>((((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval.e));
      if (str) {
          ((*yyvalp).attrNames)->push_back(AttrName(str->s));
          delete str;
      } else
          ((*yyvalp).attrNames)->push_back(AttrName((((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval.e)));
    }
    break;

  case 72:
/* Line 868 of glr.c  */
#line 472 "src/libexpr/parser.y"
    { ((*yyvalp).attrNames) = new vector<AttrName>; ((*yyvalp).attrNames)->push_back(AttrName(data->symbols.create((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yysemantics.yysval.id)))); }
    break;

  case 73:
/* Line 868 of glr.c  */
#line 474 "src/libexpr/parser.y"
    { ((*yyvalp).attrNames) = new vector<AttrName>;
      ExprString *str = dynamic_cast<ExprString *>((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yysemantics.yysval.e));
      if (str) {
          ((*yyvalp).attrNames)->push_back(AttrName(str->s));
          delete str;
      } else
          ((*yyvalp).attrNames)->push_back(AttrName((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yysemantics.yysval.e)));
    }
    break;

  case 74:
/* Line 868 of glr.c  */
#line 485 "src/libexpr/parser.y"
    { ((*yyvalp).id) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yysemantics.yysval.id); }
    break;

  case 75:
/* Line 868 of glr.c  */
#line 486 "src/libexpr/parser.y"
    { ((*yyvalp).id) = "or"; }
    break;

  case 76:
/* Line 868 of glr.c  */
#line 490 "src/libexpr/parser.y"
    { ((*yyvalp).e) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (3))].yystate.yysemantics.yysval.e); }
    break;

  case 77:
/* Line 868 of glr.c  */
#line 491 "src/libexpr/parser.y"
    { ((*yyvalp).e) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (3))].yystate.yysemantics.yysval.e); }
    break;

  case 78:
/* Line 868 of glr.c  */
#line 495 "src/libexpr/parser.y"
    { ((*yyvalp).list) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yysemantics.yysval.list); (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yysemantics.yysval.list)->elems.push_back((((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yysemantics.yysval.e)); /* !!! dangerous */ }
    break;

  case 79:
/* Line 868 of glr.c  */
#line 496 "src/libexpr/parser.y"
    { ((*yyvalp).list) = new ExprList; }
    break;

  case 80:
/* Line 868 of glr.c  */
#line 501 "src/libexpr/parser.y"
    { ((*yyvalp).formals) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval.formals); addFormal(CUR_POS, ((*yyvalp).formals), *(((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval.formal)); }
    break;

  case 81:
/* Line 868 of glr.c  */
#line 503 "src/libexpr/parser.y"
    { ((*yyvalp).formals) = new Formals; addFormal(CUR_POS, ((*yyvalp).formals), *(((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yysemantics.yysval.formal)); ((*yyvalp).formals)->ellipsis = false; }
    break;

  case 82:
/* Line 868 of glr.c  */
#line 505 "src/libexpr/parser.y"
    { ((*yyvalp).formals) = new Formals; ((*yyvalp).formals)->ellipsis = false; }
    break;

  case 83:
/* Line 868 of glr.c  */
#line 507 "src/libexpr/parser.y"
    { ((*yyvalp).formals) = new Formals; ((*yyvalp).formals)->ellipsis = true; }
    break;

  case 84:
/* Line 868 of glr.c  */
#line 511 "src/libexpr/parser.y"
    { ((*yyvalp).formal) = new Formal(data->symbols.create((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yysemantics.yysval.id)), 0); }
    break;

  case 85:
/* Line 868 of glr.c  */
#line 512 "src/libexpr/parser.y"
    { ((*yyvalp).formal) = new Formal(data->symbols.create((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval.id)), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval.e)); }
    break;


/* Line 868 of glr.c  */
#line 1832 "src/libexpr/parser-tab.cc"
      default: break;
    }

  return yyok;
# undef yyerrok
# undef YYABORT
# undef YYACCEPT
# undef YYERROR
# undef YYBACKUP
# undef yyclearin
# undef YYRECOVERING
}


/*ARGSUSED*/ static void
yyuserMerge (int yyn, YYSTYPE* yy0, YYSTYPE* yy1)
{
  YYUSE (yy0);
  YYUSE (yy1);

  switch (yyn)
    {
      
      default: break;
    }
}

                              /* Bison grammar-table manipulation.  */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, void * scanner, nix::ParseData * data)
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  YYUSE (scanner);
  YYUSE (data);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
        break;
    }
}

/** Number of symbols composing the right hand side of rule #RULE.  */
static inline int
yyrhsLength (yyRuleNum yyrule)
{
  return yyr2[yyrule];
}

static void
yydestroyGLRState (char const *yymsg, yyGLRState *yys, void * scanner, nix::ParseData * data)
{
  if (yys->yyresolved)
    yydestruct (yymsg, yystos[yys->yylrState],
                &yys->yysemantics.yysval, &yys->yyloc, scanner, data);
  else
    {
#if YYDEBUG
      if (yydebug)
        {
          if (yys->yysemantics.yyfirstVal)
            YYFPRINTF (stderr, "%s unresolved ", yymsg);
          else
            YYFPRINTF (stderr, "%s incomplete ", yymsg);
          yy_symbol_print (stderr, yystos[yys->yylrState],
                           YY_NULL, &yys->yyloc, scanner, data);
          YYFPRINTF (stderr, "\n");
        }
#endif

      if (yys->yysemantics.yyfirstVal)
        {
          yySemanticOption *yyoption = yys->yysemantics.yyfirstVal;
          yyGLRState *yyrh;
          int yyn;
          for (yyrh = yyoption->yystate, yyn = yyrhsLength (yyoption->yyrule);
               yyn > 0;
               yyrh = yyrh->yypred, yyn -= 1)
            yydestroyGLRState (yymsg, yyrh, scanner, data);
        }
    }
}

/** Left-hand-side symbol for rule #RULE.  */
static inline yySymbol
yylhsNonterm (yyRuleNum yyrule)
{
  return yyr1[yyrule];
}

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-123)))

/** True iff LR state STATE has only a default reduction (regardless
 *  of token).  */
static inline yybool
yyisDefaultedState (yyStateNum yystate)
{
  return yypact_value_is_default (yypact[yystate]);
}

/** The default reduction for STATE, assuming it has one.  */
static inline yyRuleNum
yydefaultAction (yyStateNum yystate)
{
  return yydefact[yystate];
}

#define yytable_value_is_error(Yytable_value) \
  (!!((Yytable_value) == (-67)))

/** Set *YYACTION to the action to take in YYSTATE on seeing YYTOKEN.
 *  Result R means
 *    R < 0:  Reduce on rule -R.
 *    R = 0:  Error.
 *    R > 0:  Shift to state R.
 *  Set *CONFLICTS to a pointer into yyconfl to 0-terminated list of
 *  conflicting reductions.
 */
static inline void
yygetLRActions (yyStateNum yystate, int yytoken,
                int* yyaction, const short int** yyconflicts)
{
  int yyindex = yypact[yystate] + yytoken;
  if (yypact_value_is_default (yypact[yystate])
      || yyindex < 0 || YYLAST < yyindex || yycheck[yyindex] != yytoken)
    {
      *yyaction = -yydefact[yystate];
      *yyconflicts = yyconfl;
    }
  else if (! yytable_value_is_error (yytable[yyindex]))
    {
      *yyaction = yytable[yyindex];
      *yyconflicts = yyconfl + yyconflp[yyindex];
    }
  else
    {
      *yyaction = 0;
      *yyconflicts = yyconfl + yyconflp[yyindex];
    }
}

static inline yyStateNum
yyLRgotoState (yyStateNum yystate, yySymbol yylhs)
{
  int yyr;
  yyr = yypgoto[yylhs - YYNTOKENS] + yystate;
  if (0 <= yyr && yyr <= YYLAST && yycheck[yyr] == yystate)
    return yytable[yyr];
  else
    return yydefgoto[yylhs - YYNTOKENS];
}

static inline yybool
yyisShiftAction (int yyaction)
{
  return 0 < yyaction;
}

static inline yybool
yyisErrorAction (int yyaction)
{
  return yyaction == 0;
}

                                /* GLRStates */

/** Return a fresh GLRStackItem.  Callers should call
 * YY_RESERVE_GLRSTACK afterwards to make sure there is sufficient
 * headroom.  */

static inline yyGLRStackItem*
yynewGLRStackItem (yyGLRStack* yystackp, yybool yyisState)
{
  yyGLRStackItem* yynewItem = yystackp->yynextFree;
  yystackp->yyspaceLeft -= 1;
  yystackp->yynextFree += 1;
  yynewItem->yystate.yyisState = yyisState;
  return yynewItem;
}

/** Add a new semantic action that will execute the action for rule
 *  RULENUM on the semantic values in RHS to the list of
 *  alternative actions for STATE.  Assumes that RHS comes from
 *  stack #K of *STACKP. */
static void
yyaddDeferredAction (yyGLRStack* yystackp, size_t yyk, yyGLRState* yystate,
                     yyGLRState* rhs, yyRuleNum yyrule)
{
  yySemanticOption* yynewOption =
    &yynewGLRStackItem (yystackp, yyfalse)->yyoption;
  yynewOption->yystate = rhs;
  yynewOption->yyrule = yyrule;
  if (yystackp->yytops.yylookaheadNeeds[yyk])
    {
      yynewOption->yyrawchar = yychar;
      yynewOption->yyval = yylval;
      yynewOption->yyloc = yylloc;
    }
  else
    yynewOption->yyrawchar = YYEMPTY;
  yynewOption->yynext = yystate->yysemantics.yyfirstVal;
  yystate->yysemantics.yyfirstVal = yynewOption;

  YY_RESERVE_GLRSTACK (yystackp);
}

                                /* GLRStacks */

/** Initialize SET to a singleton set containing an empty stack.  */
static yybool
yyinitStateSet (yyGLRStateSet* yyset)
{
  yyset->yysize = 1;
  yyset->yycapacity = 16;
  yyset->yystates = (yyGLRState**) YYMALLOC (16 * sizeof yyset->yystates[0]);
  if (! yyset->yystates)
    return yyfalse;
  yyset->yystates[0] = YY_NULL;
  yyset->yylookaheadNeeds =
    (yybool*) YYMALLOC (16 * sizeof yyset->yylookaheadNeeds[0]);
  if (! yyset->yylookaheadNeeds)
    {
      YYFREE (yyset->yystates);
      return yyfalse;
    }
  return yytrue;
}

static void yyfreeStateSet (yyGLRStateSet* yyset)
{
  YYFREE (yyset->yystates);
  YYFREE (yyset->yylookaheadNeeds);
}

/** Initialize STACK to a single empty stack, with total maximum
 *  capacity for all stacks of SIZE.  */
static yybool
yyinitGLRStack (yyGLRStack* yystackp, size_t yysize)
{
  yystackp->yyerrState = 0;
  yynerrs = 0;
  yystackp->yyspaceLeft = yysize;
  yystackp->yyitems =
    (yyGLRStackItem*) YYMALLOC (yysize * sizeof yystackp->yynextFree[0]);
  if (!yystackp->yyitems)
    return yyfalse;
  yystackp->yynextFree = yystackp->yyitems;
  yystackp->yysplitPoint = YY_NULL;
  yystackp->yylastDeleted = YY_NULL;
  return yyinitStateSet (&yystackp->yytops);
}


#if YYSTACKEXPANDABLE
# define YYRELOC(YYFROMITEMS,YYTOITEMS,YYX,YYTYPE) \
  &((YYTOITEMS) - ((YYFROMITEMS) - (yyGLRStackItem*) (YYX)))->YYTYPE

/** If STACK is expandable, extend it.  WARNING: Pointers into the
    stack from outside should be considered invalid after this call.
    We always expand when there are 1 or fewer items left AFTER an
    allocation, so that we can avoid having external pointers exist
    across an allocation.  */
static void
yyexpandGLRStack (yyGLRStack* yystackp)
{
  yyGLRStackItem* yynewItems;
  yyGLRStackItem* yyp0, *yyp1;
  size_t yynewSize;
  size_t yyn;
  size_t yysize = yystackp->yynextFree - yystackp->yyitems;
  if (YYMAXDEPTH - YYHEADROOM < yysize)
    yyMemoryExhausted (yystackp);
  yynewSize = 2*yysize;
  if (YYMAXDEPTH < yynewSize)
    yynewSize = YYMAXDEPTH;
  yynewItems = (yyGLRStackItem*) YYMALLOC (yynewSize * sizeof yynewItems[0]);
  if (! yynewItems)
    yyMemoryExhausted (yystackp);
  for (yyp0 = yystackp->yyitems, yyp1 = yynewItems, yyn = yysize;
       0 < yyn;
       yyn -= 1, yyp0 += 1, yyp1 += 1)
    {
      *yyp1 = *yyp0;
      if (*(yybool *) yyp0)
        {
          yyGLRState* yys0 = &yyp0->yystate;
          yyGLRState* yys1 = &yyp1->yystate;
          if (yys0->yypred != YY_NULL)
            yys1->yypred =
              YYRELOC (yyp0, yyp1, yys0->yypred, yystate);
          if (! yys0->yyresolved && yys0->yysemantics.yyfirstVal != YY_NULL)
            yys1->yysemantics.yyfirstVal =
              YYRELOC (yyp0, yyp1, yys0->yysemantics.yyfirstVal, yyoption);
        }
      else
        {
          yySemanticOption* yyv0 = &yyp0->yyoption;
          yySemanticOption* yyv1 = &yyp1->yyoption;
          if (yyv0->yystate != YY_NULL)
            yyv1->yystate = YYRELOC (yyp0, yyp1, yyv0->yystate, yystate);
          if (yyv0->yynext != YY_NULL)
            yyv1->yynext = YYRELOC (yyp0, yyp1, yyv0->yynext, yyoption);
        }
    }
  if (yystackp->yysplitPoint != YY_NULL)
    yystackp->yysplitPoint = YYRELOC (yystackp->yyitems, yynewItems,
                                 yystackp->yysplitPoint, yystate);

  for (yyn = 0; yyn < yystackp->yytops.yysize; yyn += 1)
    if (yystackp->yytops.yystates[yyn] != YY_NULL)
      yystackp->yytops.yystates[yyn] =
        YYRELOC (yystackp->yyitems, yynewItems,
                 yystackp->yytops.yystates[yyn], yystate);
  YYFREE (yystackp->yyitems);
  yystackp->yyitems = yynewItems;
  yystackp->yynextFree = yynewItems + yysize;
  yystackp->yyspaceLeft = yynewSize - yysize;
}
#endif

static void
yyfreeGLRStack (yyGLRStack* yystackp)
{
  YYFREE (yystackp->yyitems);
  yyfreeStateSet (&yystackp->yytops);
}

/** Assuming that S is a GLRState somewhere on STACK, update the
 *  splitpoint of STACK, if needed, so that it is at least as deep as
 *  S.  */
static inline void
yyupdateSplit (yyGLRStack* yystackp, yyGLRState* yys)
{
  if (yystackp->yysplitPoint != YY_NULL && yystackp->yysplitPoint > yys)
    yystackp->yysplitPoint = yys;
}

/** Invalidate stack #K in STACK.  */
static inline void
yymarkStackDeleted (yyGLRStack* yystackp, size_t yyk)
{
  if (yystackp->yytops.yystates[yyk] != YY_NULL)
    yystackp->yylastDeleted = yystackp->yytops.yystates[yyk];
  yystackp->yytops.yystates[yyk] = YY_NULL;
}

/** Undelete the last stack that was marked as deleted.  Can only be
    done once after a deletion, and only when all other stacks have
    been deleted.  */
static void
yyundeleteLastStack (yyGLRStack* yystackp)
{
  if (yystackp->yylastDeleted == YY_NULL || yystackp->yytops.yysize != 0)
    return;
  yystackp->yytops.yystates[0] = yystackp->yylastDeleted;
  yystackp->yytops.yysize = 1;
  YYDPRINTF ((stderr, "Restoring last deleted stack as stack #0.\n"));
  yystackp->yylastDeleted = YY_NULL;
}

static inline void
yyremoveDeletes (yyGLRStack* yystackp)
{
  size_t yyi, yyj;
  yyi = yyj = 0;
  while (yyj < yystackp->yytops.yysize)
    {
      if (yystackp->yytops.yystates[yyi] == YY_NULL)
        {
          if (yyi == yyj)
            {
              YYDPRINTF ((stderr, "Removing dead stacks.\n"));
            }
          yystackp->yytops.yysize -= 1;
        }
      else
        {
          yystackp->yytops.yystates[yyj] = yystackp->yytops.yystates[yyi];
          /* In the current implementation, it's unnecessary to copy
             yystackp->yytops.yylookaheadNeeds[yyi] since, after
             yyremoveDeletes returns, the parser immediately either enters
             deterministic operation or shifts a token.  However, it doesn't
             hurt, and the code might evolve to need it.  */
          yystackp->yytops.yylookaheadNeeds[yyj] =
            yystackp->yytops.yylookaheadNeeds[yyi];
          if (yyj != yyi)
            {
              YYDPRINTF ((stderr, "Rename stack %lu -> %lu.\n",
                          (unsigned long int) yyi, (unsigned long int) yyj));
            }
          yyj += 1;
        }
      yyi += 1;
    }
}

/** Shift to a new state on stack #K of STACK, corresponding to LR state
 * LRSTATE, at input position POSN, with (resolved) semantic value SVAL.  */
static inline void
yyglrShift (yyGLRStack* yystackp, size_t yyk, yyStateNum yylrState,
            size_t yyposn,
            YYSTYPE* yyvalp, YYLTYPE* yylocp)
{
  yyGLRState* yynewState = &yynewGLRStackItem (yystackp, yytrue)->yystate;

  yynewState->yylrState = yylrState;
  yynewState->yyposn = yyposn;
  yynewState->yyresolved = yytrue;
  yynewState->yypred = yystackp->yytops.yystates[yyk];
  yynewState->yysemantics.yysval = *yyvalp;
  yynewState->yyloc = *yylocp;
  yystackp->yytops.yystates[yyk] = yynewState;

  YY_RESERVE_GLRSTACK (yystackp);
}

/** Shift stack #K of YYSTACK, to a new state corresponding to LR
 *  state YYLRSTATE, at input position YYPOSN, with the (unresolved)
 *  semantic value of YYRHS under the action for YYRULE.  */
static inline void
yyglrShiftDefer (yyGLRStack* yystackp, size_t yyk, yyStateNum yylrState,
                 size_t yyposn, yyGLRState* rhs, yyRuleNum yyrule)
{
  yyGLRState* yynewState = &yynewGLRStackItem (yystackp, yytrue)->yystate;

  yynewState->yylrState = yylrState;
  yynewState->yyposn = yyposn;
  yynewState->yyresolved = yyfalse;
  yynewState->yypred = yystackp->yytops.yystates[yyk];
  yynewState->yysemantics.yyfirstVal = YY_NULL;
  yystackp->yytops.yystates[yyk] = yynewState;

  /* Invokes YY_RESERVE_GLRSTACK.  */
  yyaddDeferredAction (yystackp, yyk, yynewState, rhs, yyrule);
}

/** Pop the symbols consumed by reduction #RULE from the top of stack
 *  #K of STACK, and perform the appropriate semantic action on their
 *  semantic values.  Assumes that all ambiguities in semantic values
 *  have been previously resolved.  Set *VALP to the resulting value,
 *  and *LOCP to the computed location (if any).  Return value is as
 *  for userAction.  */
static inline YYRESULTTAG
yydoAction (yyGLRStack* yystackp, size_t yyk, yyRuleNum yyrule,
            YYSTYPE* yyvalp, YYLTYPE *yylocp, void * scanner, nix::ParseData * data)
{
  int yynrhs = yyrhsLength (yyrule);

  if (yystackp->yysplitPoint == YY_NULL)
    {
      /* Standard special case: single stack.  */
      yyGLRStackItem* rhs = (yyGLRStackItem*) yystackp->yytops.yystates[yyk];
      YYASSERT (yyk == 0);
      yystackp->yynextFree -= yynrhs;
      yystackp->yyspaceLeft += yynrhs;
      yystackp->yytops.yystates[0] = & yystackp->yynextFree[-1].yystate;
      return yyuserAction (yyrule, yynrhs, rhs, yystackp,
                           yyvalp, yylocp, scanner, data);
    }
  else
    {
      /* At present, doAction is never called in nondeterministic
       * mode, so this branch is never taken.  It is here in
       * anticipation of a future feature that will allow immediate
       * evaluation of selected actions in nondeterministic mode.  */
      int yyi;
      yyGLRState* yys;
      yyGLRStackItem yyrhsVals[YYMAXRHS + YYMAXLEFT + 1];
      yys = yyrhsVals[YYMAXRHS + YYMAXLEFT].yystate.yypred
        = yystackp->yytops.yystates[yyk];
      if (yynrhs == 0)
        /* Set default location.  */
        yyrhsVals[YYMAXRHS + YYMAXLEFT - 1].yystate.yyloc = yys->yyloc;
      for (yyi = 0; yyi < yynrhs; yyi += 1)
        {
          yys = yys->yypred;
          YYASSERT (yys);
        }
      yyupdateSplit (yystackp, yys);
      yystackp->yytops.yystates[yyk] = yys;
      return yyuserAction (yyrule, yynrhs, yyrhsVals + YYMAXRHS + YYMAXLEFT - 1,
                           yystackp, yyvalp, yylocp, scanner, data);
    }
}

#if !YYDEBUG
# define YY_REDUCE_PRINT(Args)
#else
# define YY_REDUCE_PRINT(Args)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print Args;               \
} while (YYID (0))

/*----------------------------------------------------------.
| Report that the RULE is going to be reduced on stack #K.  |
`----------------------------------------------------------*/

/*ARGSUSED*/ static inline void
yy_reduce_print (yyGLRStack* yystackp, size_t yyk, yyRuleNum yyrule,
                 YYSTYPE* yyvalp, YYLTYPE *yylocp, void * scanner, nix::ParseData * data)
{
  int yynrhs = yyrhsLength (yyrule);
  yybool yynormal __attribute__ ((__unused__)) =
    (yystackp->yysplitPoint == YY_NULL);
  yyGLRStackItem* yyvsp = (yyGLRStackItem*) yystackp->yytops.yystates[yyk];
  int yylow = 1;
  int yyi;
  YYUSE (yyvalp);
  YYUSE (yylocp);
  YYUSE (scanner);
  YYUSE (data);
  YYFPRINTF (stderr, "Reducing stack %lu by rule %d (line %lu):\n",
             (unsigned long int) yyk, yyrule - 1,
             (unsigned long int) yyrline[yyrule]);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
                       &(((yyGLRStackItem const *)yyvsp)[YYFILL ((yyi + 1) - (yynrhs))].yystate.yysemantics.yysval)
                       , &(((yyGLRStackItem const *)yyvsp)[YYFILL ((yyi + 1) - (yynrhs))].yystate.yyloc)                       , scanner, data);
      YYFPRINTF (stderr, "\n");
    }
}
#endif

/** Pop items off stack #K of STACK according to grammar rule RULE,
 *  and push back on the resulting nonterminal symbol.  Perform the
 *  semantic action associated with RULE and store its value with the
 *  newly pushed state, if FORCEEVAL or if STACK is currently
 *  unambiguous.  Otherwise, store the deferred semantic action with
 *  the new state.  If the new state would have an identical input
 *  position, LR state, and predecessor to an existing state on the stack,
 *  it is identified with that existing state, eliminating stack #K from
 *  the STACK.  In this case, the (necessarily deferred) semantic value is
 *  added to the options for the existing state's semantic value.
 */
static inline YYRESULTTAG
yyglrReduce (yyGLRStack* yystackp, size_t yyk, yyRuleNum yyrule,
             yybool yyforceEval, void * scanner, nix::ParseData * data)
{
  size_t yyposn = yystackp->yytops.yystates[yyk]->yyposn;

  if (yyforceEval || yystackp->yysplitPoint == YY_NULL)
    {
      YYSTYPE yysval;
      YYLTYPE yyloc;

      YY_REDUCE_PRINT ((yystackp, yyk, yyrule, &yysval, &yyloc, scanner, data));
      YYCHK (yydoAction (yystackp, yyk, yyrule, &yysval, &yyloc, scanner, data));
      YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyrule], &yysval, &yyloc);
      yyglrShift (yystackp, yyk,
                  yyLRgotoState (yystackp->yytops.yystates[yyk]->yylrState,
                                 yylhsNonterm (yyrule)),
                  yyposn, &yysval, &yyloc);
    }
  else
    {
      size_t yyi;
      int yyn;
      yyGLRState* yys, *yys0 = yystackp->yytops.yystates[yyk];
      yyStateNum yynewLRState;

      for (yys = yystackp->yytops.yystates[yyk], yyn = yyrhsLength (yyrule);
           0 < yyn; yyn -= 1)
        {
          yys = yys->yypred;
          YYASSERT (yys);
        }
      yyupdateSplit (yystackp, yys);
      yynewLRState = yyLRgotoState (yys->yylrState, yylhsNonterm (yyrule));
      YYDPRINTF ((stderr,
                  "Reduced stack %lu by rule #%d; action deferred.  Now in state %d.\n",
                  (unsigned long int) yyk, yyrule - 1, yynewLRState));
      for (yyi = 0; yyi < yystackp->yytops.yysize; yyi += 1)
        if (yyi != yyk && yystackp->yytops.yystates[yyi] != YY_NULL)
          {
            yyGLRState *yysplit = yystackp->yysplitPoint;
            yyGLRState *yyp = yystackp->yytops.yystates[yyi];
            while (yyp != yys && yyp != yysplit && yyp->yyposn >= yyposn)
              {
                if (yyp->yylrState == yynewLRState && yyp->yypred == yys)
                  {
                    yyaddDeferredAction (yystackp, yyk, yyp, yys0, yyrule);
                    yymarkStackDeleted (yystackp, yyk);
                    YYDPRINTF ((stderr, "Merging stack %lu into stack %lu.\n",
                                (unsigned long int) yyk,
                                (unsigned long int) yyi));
                    return yyok;
                  }
                yyp = yyp->yypred;
              }
          }
      yystackp->yytops.yystates[yyk] = yys;
      yyglrShiftDefer (yystackp, yyk, yynewLRState, yyposn, yys0, yyrule);
    }
  return yyok;
}

static size_t
yysplitStack (yyGLRStack* yystackp, size_t yyk)
{
  if (yystackp->yysplitPoint == YY_NULL)
    {
      YYASSERT (yyk == 0);
      yystackp->yysplitPoint = yystackp->yytops.yystates[yyk];
    }
  if (yystackp->yytops.yysize >= yystackp->yytops.yycapacity)
    {
      yyGLRState** yynewStates;
      yybool* yynewLookaheadNeeds;

      yynewStates = YY_NULL;

      if (yystackp->yytops.yycapacity
          > (YYSIZEMAX / (2 * sizeof yynewStates[0])))
        yyMemoryExhausted (yystackp);
      yystackp->yytops.yycapacity *= 2;

      yynewStates =
        (yyGLRState**) YYREALLOC (yystackp->yytops.yystates,
                                  (yystackp->yytops.yycapacity
                                   * sizeof yynewStates[0]));
      if (yynewStates == YY_NULL)
        yyMemoryExhausted (yystackp);
      yystackp->yytops.yystates = yynewStates;

      yynewLookaheadNeeds =
        (yybool*) YYREALLOC (yystackp->yytops.yylookaheadNeeds,
                             (yystackp->yytops.yycapacity
                              * sizeof yynewLookaheadNeeds[0]));
      if (yynewLookaheadNeeds == YY_NULL)
        yyMemoryExhausted (yystackp);
      yystackp->yytops.yylookaheadNeeds = yynewLookaheadNeeds;
    }
  yystackp->yytops.yystates[yystackp->yytops.yysize]
    = yystackp->yytops.yystates[yyk];
  yystackp->yytops.yylookaheadNeeds[yystackp->yytops.yysize]
    = yystackp->yytops.yylookaheadNeeds[yyk];
  yystackp->yytops.yysize += 1;
  return yystackp->yytops.yysize-1;
}

/** True iff Y0 and Y1 represent identical options at the top level.
 *  That is, they represent the same rule applied to RHS symbols
 *  that produce the same terminal symbols.  */
static yybool
yyidenticalOptions (yySemanticOption* yyy0, yySemanticOption* yyy1)
{
  if (yyy0->yyrule == yyy1->yyrule)
    {
      yyGLRState *yys0, *yys1;
      int yyn;
      for (yys0 = yyy0->yystate, yys1 = yyy1->yystate,
           yyn = yyrhsLength (yyy0->yyrule);
           yyn > 0;
           yys0 = yys0->yypred, yys1 = yys1->yypred, yyn -= 1)
        if (yys0->yyposn != yys1->yyposn)
          return yyfalse;
      return yytrue;
    }
  else
    return yyfalse;
}

/** Assuming identicalOptions (Y0,Y1), destructively merge the
 *  alternative semantic values for the RHS-symbols of Y1 and Y0.  */
static void
yymergeOptionSets (yySemanticOption* yyy0, yySemanticOption* yyy1)
{
  yyGLRState *yys0, *yys1;
  int yyn;
  for (yys0 = yyy0->yystate, yys1 = yyy1->yystate,
       yyn = yyrhsLength (yyy0->yyrule);
       yyn > 0;
       yys0 = yys0->yypred, yys1 = yys1->yypred, yyn -= 1)
    {
      if (yys0 == yys1)
        break;
      else if (yys0->yyresolved)
        {
          yys1->yyresolved = yytrue;
          yys1->yysemantics.yysval = yys0->yysemantics.yysval;
        }
      else if (yys1->yyresolved)
        {
          yys0->yyresolved = yytrue;
          yys0->yysemantics.yysval = yys1->yysemantics.yysval;
        }
      else
        {
          yySemanticOption** yyz0p = &yys0->yysemantics.yyfirstVal;
          yySemanticOption* yyz1 = yys1->yysemantics.yyfirstVal;
          while (YYID (yytrue))
            {
              if (yyz1 == *yyz0p || yyz1 == YY_NULL)
                break;
              else if (*yyz0p == YY_NULL)
                {
                  *yyz0p = yyz1;
                  break;
                }
              else if (*yyz0p < yyz1)
                {
                  yySemanticOption* yyz = *yyz0p;
                  *yyz0p = yyz1;
                  yyz1 = yyz1->yynext;
                  (*yyz0p)->yynext = yyz;
                }
              yyz0p = &(*yyz0p)->yynext;
            }
          yys1->yysemantics.yyfirstVal = yys0->yysemantics.yyfirstVal;
        }
    }
}

/** Y0 and Y1 represent two possible actions to take in a given
 *  parsing state; return 0 if no combination is possible,
 *  1 if user-mergeable, 2 if Y0 is preferred, 3 if Y1 is preferred.  */
static int
yypreference (yySemanticOption* y0, yySemanticOption* y1)
{
  yyRuleNum r0 = y0->yyrule, r1 = y1->yyrule;
  int p0 = yydprec[r0], p1 = yydprec[r1];

  if (p0 == p1)
    {
      if (yymerger[r0] == 0 || yymerger[r0] != yymerger[r1])
        return 0;
      else
        return 1;
    }
  if (p0 == 0 || p1 == 0)
    return 0;
  if (p0 < p1)
    return 3;
  if (p1 < p0)
    return 2;
  return 0;
}

static YYRESULTTAG yyresolveValue (yyGLRState* yys,
                                   yyGLRStack* yystackp, void * scanner, nix::ParseData * data);


/** Resolve the previous N states starting at and including state S.  If result
 *  != yyok, some states may have been left unresolved possibly with empty
 *  semantic option chains.  Regardless of whether result = yyok, each state
 *  has been left with consistent data so that yydestroyGLRState can be invoked
 *  if necessary.  */
static YYRESULTTAG
yyresolveStates (yyGLRState* yys, int yyn,
                 yyGLRStack* yystackp, void * scanner, nix::ParseData * data)
{
  if (0 < yyn)
    {
      YYASSERT (yys->yypred);
      YYCHK (yyresolveStates (yys->yypred, yyn-1, yystackp, scanner, data));
      if (! yys->yyresolved)
        YYCHK (yyresolveValue (yys, yystackp, scanner, data));
    }
  return yyok;
}

/** Resolve the states for the RHS of OPT, perform its user action, and return
 *  the semantic value and location.  Regardless of whether result = yyok, all
 *  RHS states have been destroyed (assuming the user action destroys all RHS
 *  semantic values if invoked).  */
static YYRESULTTAG
yyresolveAction (yySemanticOption* yyopt, yyGLRStack* yystackp,
                 YYSTYPE* yyvalp, YYLTYPE *yylocp, void * scanner, nix::ParseData * data)
{
  yyGLRStackItem yyrhsVals[YYMAXRHS + YYMAXLEFT + 1];
  int yynrhs = yyrhsLength (yyopt->yyrule);
  YYRESULTTAG yyflag =
    yyresolveStates (yyopt->yystate, yynrhs, yystackp, scanner, data);
  if (yyflag != yyok)
    {
      yyGLRState *yys;
      for (yys = yyopt->yystate; yynrhs > 0; yys = yys->yypred, yynrhs -= 1)
        yydestroyGLRState ("Cleanup: popping", yys, scanner, data);
      return yyflag;
    }

  yyrhsVals[YYMAXRHS + YYMAXLEFT].yystate.yypred = yyopt->yystate;
  if (yynrhs == 0)
    /* Set default location.  */
    yyrhsVals[YYMAXRHS + YYMAXLEFT - 1].yystate.yyloc = yyopt->yystate->yyloc;
  {
    int yychar_current = yychar;
    YYSTYPE yylval_current = yylval;
    YYLTYPE yylloc_current = yylloc;
    yychar = yyopt->yyrawchar;
    yylval = yyopt->yyval;
    yylloc = yyopt->yyloc;
    yyflag = yyuserAction (yyopt->yyrule, yynrhs,
                           yyrhsVals + YYMAXRHS + YYMAXLEFT - 1,
                           yystackp, yyvalp, yylocp, scanner, data);
    yychar = yychar_current;
    yylval = yylval_current;
    yylloc = yylloc_current;
  }
  return yyflag;
}

#if YYDEBUG
static void
yyreportTree (yySemanticOption* yyx, int yyindent)
{
  int yynrhs = yyrhsLength (yyx->yyrule);
  int yyi;
  yyGLRState* yys;
  yyGLRState* yystates[1 + YYMAXRHS];
  yyGLRState yyleftmost_state;

  for (yyi = yynrhs, yys = yyx->yystate; 0 < yyi; yyi -= 1, yys = yys->yypred)
    yystates[yyi] = yys;
  if (yys == YY_NULL)
    {
      yyleftmost_state.yyposn = 0;
      yystates[0] = &yyleftmost_state;
    }
  else
    yystates[0] = yys;

  if (yyx->yystate->yyposn < yys->yyposn + 1)
    YYFPRINTF (stderr, "%*s%s -> <Rule %d, empty>\n",
               yyindent, "", yytokenName (yylhsNonterm (yyx->yyrule)),
               yyx->yyrule - 1);
  else
    YYFPRINTF (stderr, "%*s%s -> <Rule %d, tokens %lu .. %lu>\n",
               yyindent, "", yytokenName (yylhsNonterm (yyx->yyrule)),
               yyx->yyrule - 1, (unsigned long int) (yys->yyposn + 1),
               (unsigned long int) yyx->yystate->yyposn);
  for (yyi = 1; yyi <= yynrhs; yyi += 1)
    {
      if (yystates[yyi]->yyresolved)
        {
          if (yystates[yyi-1]->yyposn+1 > yystates[yyi]->yyposn)
            YYFPRINTF (stderr, "%*s%s <empty>\n", yyindent+2, "",
                       yytokenName (yyrhs[yyprhs[yyx->yyrule]+yyi-1]));
          else
            YYFPRINTF (stderr, "%*s%s <tokens %lu .. %lu>\n", yyindent+2, "",
                       yytokenName (yyrhs[yyprhs[yyx->yyrule]+yyi-1]),
                       (unsigned long int) (yystates[yyi - 1]->yyposn + 1),
                       (unsigned long int) yystates[yyi]->yyposn);
        }
      else
        yyreportTree (yystates[yyi]->yysemantics.yyfirstVal, yyindent+2);
    }
}
#endif

/*ARGSUSED*/ static YYRESULTTAG
yyreportAmbiguity (yySemanticOption* yyx0,
                   yySemanticOption* yyx1, YYLTYPE *yylocp, void * scanner, nix::ParseData * data)
{
  YYUSE (yyx0);
  YYUSE (yyx1);

#if YYDEBUG
  YYFPRINTF (stderr, "Ambiguity detected.\n");
  YYFPRINTF (stderr, "Option 1,\n");
  yyreportTree (yyx0, 2);
  YYFPRINTF (stderr, "\nOption 2,\n");
  yyreportTree (yyx1, 2);
  YYFPRINTF (stderr, "\n");
#endif

  yyerror (yylocp, scanner, data, YY_("syntax is ambiguous"));
  return yyabort;
}

/** Starting at and including state S1, resolve the location for each of the
 *  previous N1 states that is unresolved.  The first semantic option of a state
 *  is always chosen.  */
static void
yyresolveLocations (yyGLRState* yys1, int yyn1,
                    yyGLRStack *yystackp, void * scanner, nix::ParseData * data)
{
  if (0 < yyn1)
    {
      yyresolveLocations (yys1->yypred, yyn1 - 1, yystackp, scanner, data);
      if (!yys1->yyresolved)
        {
          yyGLRStackItem yyrhsloc[1 + YYMAXRHS];
          int yynrhs;
          yySemanticOption *yyoption = yys1->yysemantics.yyfirstVal;
          YYASSERT (yyoption != YY_NULL);
          yynrhs = yyrhsLength (yyoption->yyrule);
          if (yynrhs > 0)
            {
              yyGLRState *yys;
              int yyn;
              yyresolveLocations (yyoption->yystate, yynrhs,
                                  yystackp, scanner, data);
              for (yys = yyoption->yystate, yyn = yynrhs;
                   yyn > 0;
                   yys = yys->yypred, yyn -= 1)
                yyrhsloc[yyn].yystate.yyloc = yys->yyloc;
            }
          else
            {
              /* Both yyresolveAction and yyresolveLocations traverse the GSS
                 in reverse rightmost order.  It is only necessary to invoke
                 yyresolveLocations on a subforest for which yyresolveAction
                 would have been invoked next had an ambiguity not been
                 detected.  Thus the location of the previous state (but not
                 necessarily the previous state itself) is guaranteed to be
                 resolved already.  */
              yyGLRState *yyprevious = yyoption->yystate;
              yyrhsloc[0].yystate.yyloc = yyprevious->yyloc;
            }
          {
            int yychar_current = yychar;
            YYSTYPE yylval_current = yylval;
            YYLTYPE yylloc_current = yylloc;
            yychar = yyoption->yyrawchar;
            yylval = yyoption->yyval;
            yylloc = yyoption->yyloc;
            YYLLOC_DEFAULT ((yys1->yyloc), yyrhsloc, yynrhs);
            yychar = yychar_current;
            yylval = yylval_current;
            yylloc = yylloc_current;
          }
        }
    }
}

/** Resolve the ambiguity represented in state S, perform the indicated
 *  actions, and set the semantic value of S.  If result != yyok, the chain of
 *  semantic options in S has been cleared instead or it has been left
 *  unmodified except that redundant options may have been removed.  Regardless
 *  of whether result = yyok, S has been left with consistent data so that
 *  yydestroyGLRState can be invoked if necessary.  */
static YYRESULTTAG
yyresolveValue (yyGLRState* yys, yyGLRStack* yystackp, void * scanner, nix::ParseData * data)
{
  yySemanticOption* yyoptionList = yys->yysemantics.yyfirstVal;
  yySemanticOption* yybest = yyoptionList;
  yySemanticOption** yypp;
  yybool yymerge = yyfalse;
  YYSTYPE yysval;
  YYRESULTTAG yyflag;
  YYLTYPE *yylocp = &yys->yyloc;

  for (yypp = &yyoptionList->yynext; *yypp != YY_NULL; )
    {
      yySemanticOption* yyp = *yypp;

      if (yyidenticalOptions (yybest, yyp))
        {
          yymergeOptionSets (yybest, yyp);
          *yypp = yyp->yynext;
        }
      else
        {
          switch (yypreference (yybest, yyp))
            {
            case 0:
              yyresolveLocations (yys, 1, yystackp, scanner, data);
              return yyreportAmbiguity (yybest, yyp, yylocp, scanner, data);
              break;
            case 1:
              yymerge = yytrue;
              break;
            case 2:
              break;
            case 3:
              yybest = yyp;
              yymerge = yyfalse;
              break;
            default:
              /* This cannot happen so it is not worth a YYASSERT (yyfalse),
                 but some compilers complain if the default case is
                 omitted.  */
              break;
            }
          yypp = &yyp->yynext;
        }
    }

  if (yymerge)
    {
      yySemanticOption* yyp;
      int yyprec = yydprec[yybest->yyrule];
      yyflag = yyresolveAction (yybest, yystackp, &yysval, yylocp, scanner, data);
      if (yyflag == yyok)
        for (yyp = yybest->yynext; yyp != YY_NULL; yyp = yyp->yynext)
          {
            if (yyprec == yydprec[yyp->yyrule])
              {
                YYSTYPE yysval_other;
                YYLTYPE yydummy;
                yyflag = yyresolveAction (yyp, yystackp, &yysval_other, &yydummy, scanner, data);
                if (yyflag != yyok)
                  {
                    yydestruct ("Cleanup: discarding incompletely merged value for",
                                yystos[yys->yylrState],
                                &yysval, yylocp, scanner, data);
                    break;
                  }
                yyuserMerge (yymerger[yyp->yyrule], &yysval, &yysval_other);
              }
          }
    }
  else
    yyflag = yyresolveAction (yybest, yystackp, &yysval, yylocp, scanner, data);

  if (yyflag == yyok)
    {
      yys->yyresolved = yytrue;
      yys->yysemantics.yysval = yysval;
    }
  else
    yys->yysemantics.yyfirstVal = YY_NULL;
  return yyflag;
}

static YYRESULTTAG
yyresolveStack (yyGLRStack* yystackp, void * scanner, nix::ParseData * data)
{
  if (yystackp->yysplitPoint != YY_NULL)
    {
      yyGLRState* yys;
      int yyn;

      for (yyn = 0, yys = yystackp->yytops.yystates[0];
           yys != yystackp->yysplitPoint;
           yys = yys->yypred, yyn += 1)
        continue;
      YYCHK (yyresolveStates (yystackp->yytops.yystates[0], yyn, yystackp
                             , scanner, data));
    }
  return yyok;
}

static void
yycompressStack (yyGLRStack* yystackp)
{
  yyGLRState* yyp, *yyq, *yyr;

  if (yystackp->yytops.yysize != 1 || yystackp->yysplitPoint == YY_NULL)
    return;

  for (yyp = yystackp->yytops.yystates[0], yyq = yyp->yypred, yyr = YY_NULL;
       yyp != yystackp->yysplitPoint;
       yyr = yyp, yyp = yyq, yyq = yyp->yypred)
    yyp->yypred = yyr;

  yystackp->yyspaceLeft += yystackp->yynextFree - yystackp->yyitems;
  yystackp->yynextFree = ((yyGLRStackItem*) yystackp->yysplitPoint) + 1;
  yystackp->yyspaceLeft -= yystackp->yynextFree - yystackp->yyitems;
  yystackp->yysplitPoint = YY_NULL;
  yystackp->yylastDeleted = YY_NULL;

  while (yyr != YY_NULL)
    {
      yystackp->yynextFree->yystate = *yyr;
      yyr = yyr->yypred;
      yystackp->yynextFree->yystate.yypred = &yystackp->yynextFree[-1].yystate;
      yystackp->yytops.yystates[0] = &yystackp->yynextFree->yystate;
      yystackp->yynextFree += 1;
      yystackp->yyspaceLeft -= 1;
    }
}

static YYRESULTTAG
yyprocessOneStack (yyGLRStack* yystackp, size_t yyk,
                   size_t yyposn, YYLTYPE *yylocp, void * scanner, nix::ParseData * data)
{
  int yyaction;
  const short int* yyconflicts;
  yyRuleNum yyrule;

  while (yystackp->yytops.yystates[yyk] != YY_NULL)
    {
      yyStateNum yystate = yystackp->yytops.yystates[yyk]->yylrState;
      YYDPRINTF ((stderr, "Stack %lu Entering state %d\n",
                  (unsigned long int) yyk, yystate));

      YYASSERT (yystate != YYFINAL);

      if (yyisDefaultedState (yystate))
        {
          yyrule = yydefaultAction (yystate);
          if (yyrule == 0)
            {
              YYDPRINTF ((stderr, "Stack %lu dies.\n",
                          (unsigned long int) yyk));
              yymarkStackDeleted (yystackp, yyk);
              return yyok;
            }
          YYCHK (yyglrReduce (yystackp, yyk, yyrule, yyfalse, scanner, data));
        }
      else
        {
          yySymbol yytoken;
          yystackp->yytops.yylookaheadNeeds[yyk] = yytrue;
          if (yychar == YYEMPTY)
            {
              YYDPRINTF ((stderr, "Reading a token: "));
              yychar = YYLEX;
            }

          if (yychar <= YYEOF)
            {
              yychar = yytoken = YYEOF;
              YYDPRINTF ((stderr, "Now at end of input.\n"));
            }
          else
            {
              yytoken = YYTRANSLATE (yychar);
              YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
            }

          yygetLRActions (yystate, yytoken, &yyaction, &yyconflicts);

          while (*yyconflicts != 0)
            {
              size_t yynewStack = yysplitStack (yystackp, yyk);
              YYDPRINTF ((stderr, "Splitting off stack %lu from %lu.\n",
                          (unsigned long int) yynewStack,
                          (unsigned long int) yyk));
              YYCHK (yyglrReduce (yystackp, yynewStack,
                                  *yyconflicts, yyfalse, scanner, data));
              YYCHK (yyprocessOneStack (yystackp, yynewStack,
                                        yyposn, yylocp, scanner, data));
              yyconflicts += 1;
            }

          if (yyisShiftAction (yyaction))
            break;
          else if (yyisErrorAction (yyaction))
            {
              YYDPRINTF ((stderr, "Stack %lu dies.\n",
                          (unsigned long int) yyk));
              yymarkStackDeleted (yystackp, yyk);
              break;
            }
          else
            YYCHK (yyglrReduce (yystackp, yyk, -yyaction,
                                yyfalse, scanner, data));
        }
    }
  return yyok;
}

/*ARGSUSED*/ static void
yyreportSyntaxError (yyGLRStack* yystackp, void * scanner, nix::ParseData * data)
{
  if (yystackp->yyerrState != 0)
    return;
#if ! YYERROR_VERBOSE
  yyerror (&yylloc, scanner, data, YY_("syntax error"));
#else
  {
  yySymbol yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);
  size_t yysize0 = yytnamerr (YY_NULL, yytokenName (yytoken));
  size_t yysize = yysize0;
  yybool yysize_overflow = yyfalse;
  char* yymsg = YY_NULL;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULL;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected").  */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[yystackp->yytops.yystates[0]->yylrState];
      yyarg[yycount++] = yytokenName (yytoken);
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for this
             state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;
          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytokenName (yyx);
                {
                  size_t yysz = yysize + yytnamerr (YY_NULL, yytokenName (yyx));
                  yysize_overflow |= yysz < yysize;
                  yysize = yysz;
                }
              }
        }
    }

  switch (yycount)
    {
#define YYCASE_(N, S)                   \
      case N:                           \
        yyformat = S;                   \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
    }

  {
    size_t yysz = yysize + strlen (yyformat);
    yysize_overflow |= yysz < yysize;
    yysize = yysz;
  }

  if (!yysize_overflow)
    yymsg = (char *) YYMALLOC (yysize);

  if (yymsg)
    {
      char *yyp = yymsg;
      int yyi = 0;
      while ((*yyp = *yyformat))
        {
          if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
            {
              yyp += yytnamerr (yyp, yyarg[yyi++]);
              yyformat += 2;
            }
          else
            {
              yyp++;
              yyformat++;
            }
        }
      yyerror (&yylloc, scanner, data, yymsg);
      YYFREE (yymsg);
    }
  else
    {
      yyerror (&yylloc, scanner, data, YY_("syntax error"));
      yyMemoryExhausted (yystackp);
    }
  }
#endif /* YYERROR_VERBOSE */
  yynerrs += 1;
}

/* Recover from a syntax error on *YYSTACKP, assuming that *YYSTACKP->YYTOKENP,
   yylval, and yylloc are the syntactic category, semantic value, and location
   of the lookahead.  */
/*ARGSUSED*/ static void
yyrecoverSyntaxError (yyGLRStack* yystackp, void * scanner, nix::ParseData * data)
{
  size_t yyk;
  int yyj;

  if (yystackp->yyerrState == 3)
    /* We just shifted the error token and (perhaps) took some
       reductions.  Skip tokens until we can proceed.  */
    while (YYID (yytrue))
      {
        yySymbol yytoken;
        if (yychar == YYEOF)
          yyFail (yystackp, &yylloc, scanner, data, YY_NULL);
        if (yychar != YYEMPTY)
          {
            /* We throw away the lookahead, but the error range
               of the shifted error token must take it into account.  */
            yyGLRState *yys = yystackp->yytops.yystates[0];
            yyGLRStackItem yyerror_range[3];
            yyerror_range[1].yystate.yyloc = yys->yyloc;
            yyerror_range[2].yystate.yyloc = yylloc;
            YYLLOC_DEFAULT ((yys->yyloc), yyerror_range, 2);
            yytoken = YYTRANSLATE (yychar);
            yydestruct ("Error: discarding",
                        yytoken, &yylval, &yylloc, scanner, data);
          }
        YYDPRINTF ((stderr, "Reading a token: "));
        yychar = YYLEX;
        if (yychar <= YYEOF)
          {
            yychar = yytoken = YYEOF;
            YYDPRINTF ((stderr, "Now at end of input.\n"));
          }
        else
          {
            yytoken = YYTRANSLATE (yychar);
            YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
          }
        yyj = yypact[yystackp->yytops.yystates[0]->yylrState];
        if (yypact_value_is_default (yyj))
          return;
        yyj += yytoken;
        if (yyj < 0 || YYLAST < yyj || yycheck[yyj] != yytoken)
          {
            if (yydefact[yystackp->yytops.yystates[0]->yylrState] != 0)
              return;
          }
        else if (! yytable_value_is_error (yytable[yyj]))
          return;
      }

  /* Reduce to one stack.  */
  for (yyk = 0; yyk < yystackp->yytops.yysize; yyk += 1)
    if (yystackp->yytops.yystates[yyk] != YY_NULL)
      break;
  if (yyk >= yystackp->yytops.yysize)
    yyFail (yystackp, &yylloc, scanner, data, YY_NULL);
  for (yyk += 1; yyk < yystackp->yytops.yysize; yyk += 1)
    yymarkStackDeleted (yystackp, yyk);
  yyremoveDeletes (yystackp);
  yycompressStack (yystackp);

  /* Now pop stack until we find a state that shifts the error token.  */
  yystackp->yyerrState = 3;
  while (yystackp->yytops.yystates[0] != YY_NULL)
    {
      yyGLRState *yys = yystackp->yytops.yystates[0];
      yyj = yypact[yys->yylrState];
      if (! yypact_value_is_default (yyj))
        {
          yyj += YYTERROR;
          if (0 <= yyj && yyj <= YYLAST && yycheck[yyj] == YYTERROR
              && yyisShiftAction (yytable[yyj]))
            {
              /* Shift the error token.  */
              /* First adjust its location.*/
              YYLTYPE yyerrloc;
              yystackp->yyerror_range[2].yystate.yyloc = yylloc;
              YYLLOC_DEFAULT (yyerrloc, (yystackp->yyerror_range), 2);
              YY_SYMBOL_PRINT ("Shifting", yystos[yytable[yyj]],
                               &yylval, &yyerrloc);
              yyglrShift (yystackp, 0, yytable[yyj],
                          yys->yyposn, &yylval, &yyerrloc);
              yys = yystackp->yytops.yystates[0];
              break;
            }
        }
      yystackp->yyerror_range[1].yystate.yyloc = yys->yyloc;
      if (yys->yypred != YY_NULL)
        yydestroyGLRState ("Error: popping", yys, scanner, data);
      yystackp->yytops.yystates[0] = yys->yypred;
      yystackp->yynextFree -= 1;
      yystackp->yyspaceLeft += 1;
    }
  if (yystackp->yytops.yystates[0] == YY_NULL)
    yyFail (yystackp, &yylloc, scanner, data, YY_NULL);
}

#define YYCHK1(YYE)                                                          \
  do {                                                                       \
    switch (YYE) {                                                           \
    case yyok:                                                               \
      break;                                                                 \
    case yyabort:                                                            \
      goto yyabortlab;                                                       \
    case yyaccept:                                                           \
      goto yyacceptlab;                                                      \
    case yyerr:                                                              \
      goto yyuser_error;                                                     \
    default:                                                                 \
      goto yybuglab;                                                         \
    }                                                                        \
  } while (YYID (0))


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void * scanner, nix::ParseData * data)
{
  int yyresult;
  yyGLRStack yystack;
  yyGLRStack* const yystackp = &yystack;
  size_t yyposn;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY;
  yylval = yyval_default;
  yylloc = yyloc_default;

  if (! yyinitGLRStack (yystackp, YYINITDEPTH))
    goto yyexhaustedlab;
  switch (YYSETJMP (yystack.yyexception_buffer))
    {
    case 0: break;
    case 1: goto yyabortlab;
    case 2: goto yyexhaustedlab;
    default: goto yybuglab;
    }
  yyglrShift (&yystack, 0, 0, 0, &yylval, &yylloc);
  yyposn = 0;

  while (YYID (yytrue))
    {
      /* For efficiency, we have two loops, the first of which is
         specialized to deterministic operation (single stack, no
         potential ambiguity).  */
      /* Standard mode */
      while (YYID (yytrue))
        {
          yyRuleNum yyrule;
          int yyaction;
          const short int* yyconflicts;

          yyStateNum yystate = yystack.yytops.yystates[0]->yylrState;
          YYDPRINTF ((stderr, "Entering state %d\n", yystate));
          if (yystate == YYFINAL)
            goto yyacceptlab;
          if (yyisDefaultedState (yystate))
            {
              yyrule = yydefaultAction (yystate);
              if (yyrule == 0)
                {
               yystack.yyerror_range[1].yystate.yyloc = yylloc;
                  yyreportSyntaxError (&yystack, scanner, data);
                  goto yyuser_error;
                }
              YYCHK1 (yyglrReduce (&yystack, 0, yyrule, yytrue, scanner, data));
            }
          else
            {
              yySymbol yytoken;
              if (yychar == YYEMPTY)
                {
                  YYDPRINTF ((stderr, "Reading a token: "));
                  yychar = YYLEX;
                }

              if (yychar <= YYEOF)
                {
                  yychar = yytoken = YYEOF;
                  YYDPRINTF ((stderr, "Now at end of input.\n"));
                }
              else
                {
                  yytoken = YYTRANSLATE (yychar);
                  YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
                }

              yygetLRActions (yystate, yytoken, &yyaction, &yyconflicts);
              if (*yyconflicts != 0)
                break;
              if (yyisShiftAction (yyaction))
                {
                  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
                  yychar = YYEMPTY;
                  yyposn += 1;
                  yyglrShift (&yystack, 0, yyaction, yyposn, &yylval, &yylloc);
                  if (0 < yystack.yyerrState)
                    yystack.yyerrState -= 1;
                }
              else if (yyisErrorAction (yyaction))
                {
               yystack.yyerror_range[1].yystate.yyloc = yylloc;
                  yyreportSyntaxError (&yystack, scanner, data);
                  goto yyuser_error;
                }
              else
                YYCHK1 (yyglrReduce (&yystack, 0, -yyaction, yytrue, scanner, data));
            }
        }

      while (YYID (yytrue))
        {
          yySymbol yytoken_to_shift;
          size_t yys;

          for (yys = 0; yys < yystack.yytops.yysize; yys += 1)
            yystackp->yytops.yylookaheadNeeds[yys] = yychar != YYEMPTY;

          /* yyprocessOneStack returns one of three things:

              - An error flag.  If the caller is yyprocessOneStack, it
                immediately returns as well.  When the caller is finally
                yyparse, it jumps to an error label via YYCHK1.

              - yyok, but yyprocessOneStack has invoked yymarkStackDeleted
                (&yystack, yys), which sets the top state of yys to NULL.  Thus,
                yyparse's following invocation of yyremoveDeletes will remove
                the stack.

              - yyok, when ready to shift a token.

             Except in the first case, yyparse will invoke yyremoveDeletes and
             then shift the next token onto all remaining stacks.  This
             synchronization of the shift (that is, after all preceding
             reductions on all stacks) helps prevent double destructor calls
             on yylval in the event of memory exhaustion.  */

          for (yys = 0; yys < yystack.yytops.yysize; yys += 1)
            YYCHK1 (yyprocessOneStack (&yystack, yys, yyposn, &yylloc, scanner, data));
          yyremoveDeletes (&yystack);
          if (yystack.yytops.yysize == 0)
            {
              yyundeleteLastStack (&yystack);
              if (yystack.yytops.yysize == 0)
                yyFail (&yystack, &yylloc, scanner, data, YY_("syntax error"));
              YYCHK1 (yyresolveStack (&yystack, scanner, data));
              YYDPRINTF ((stderr, "Returning to deterministic operation.\n"));
           yystack.yyerror_range[1].yystate.yyloc = yylloc;
              yyreportSyntaxError (&yystack, scanner, data);
              goto yyuser_error;
            }

          /* If any yyglrShift call fails, it will fail after shifting.  Thus,
             a copy of yylval will already be on stack 0 in the event of a
             failure in the following loop.  Thus, yychar is set to YYEMPTY
             before the loop to make sure the user destructor for yylval isn't
             called twice.  */
          yytoken_to_shift = YYTRANSLATE (yychar);
          yychar = YYEMPTY;
          yyposn += 1;
          for (yys = 0; yys < yystack.yytops.yysize; yys += 1)
            {
              int yyaction;
              const short int* yyconflicts;
              yyStateNum yystate = yystack.yytops.yystates[yys]->yylrState;
              yygetLRActions (yystate, yytoken_to_shift, &yyaction,
                              &yyconflicts);
              /* Note that yyconflicts were handled by yyprocessOneStack.  */
              YYDPRINTF ((stderr, "On stack %lu, ", (unsigned long int) yys));
              YY_SYMBOL_PRINT ("shifting", yytoken_to_shift, &yylval, &yylloc);
              yyglrShift (&yystack, yys, yyaction, yyposn,
                          &yylval, &yylloc);
              YYDPRINTF ((stderr, "Stack %lu now in state #%d\n",
                          (unsigned long int) yys,
                          yystack.yytops.yystates[yys]->yylrState));
            }

          if (yystack.yytops.yysize == 1)
            {
              YYCHK1 (yyresolveStack (&yystack, scanner, data));
              YYDPRINTF ((stderr, "Returning to deterministic operation.\n"));
              yycompressStack (&yystack);
              break;
            }
        }
      continue;
    yyuser_error:
      yyrecoverSyntaxError (&yystack, scanner, data);
      yyposn = yystack.yytops.yystates[0]->yyposn;
    }

 yyacceptlab:
  yyresult = 0;
  goto yyreturn;

 yybuglab:
  YYASSERT (yyfalse);
  goto yyabortlab;

 yyabortlab:
  yyresult = 1;
  goto yyreturn;

 yyexhaustedlab:
  yyerror (&yylloc, scanner, data, YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturn;

 yyreturn:
  if (yychar != YYEMPTY)
    yydestruct ("Cleanup: discarding lookahead",
                YYTRANSLATE (yychar), &yylval, &yylloc, scanner, data);

  /* If the stack is well-formed, pop the stack until it is empty,
     destroying its entries as we go.  But free the stack regardless
     of whether it is well-formed.  */
  if (yystack.yyitems)
    {
      yyGLRState** yystates = yystack.yytops.yystates;
      if (yystates)
        {
          size_t yysize = yystack.yytops.yysize;
          size_t yyk;
          for (yyk = 0; yyk < yysize; yyk += 1)
            if (yystates[yyk])
              {
                while (yystates[yyk])
                  {
                    yyGLRState *yys = yystates[yyk];
                 yystack.yyerror_range[1].yystate.yyloc = yys->yyloc;
                  if (yys->yypred != YY_NULL)
                      yydestroyGLRState ("Cleanup: popping", yys, scanner, data);
                    yystates[yyk] = yys->yypred;
                    yystack.yynextFree -= 1;
                    yystack.yyspaceLeft += 1;
                  }
                break;
              }
        }
      yyfreeGLRStack (&yystack);
    }

  /* Make sure YYID is used.  */
  return YYID (yyresult);
}

/* DEBUGGING ONLY */
#if YYDEBUG
static void yypstack (yyGLRStack* yystackp, size_t yyk)
  __attribute__ ((__unused__));
static void yypdumpstack (yyGLRStack* yystackp) __attribute__ ((__unused__));

static void
yy_yypstack (yyGLRState* yys)
{
  if (yys->yypred)
    {
      yy_yypstack (yys->yypred);
      YYFPRINTF (stderr, " -> ");
    }
  YYFPRINTF (stderr, "%d@%lu", yys->yylrState,
             (unsigned long int) yys->yyposn);
}

static void
yypstates (yyGLRState* yyst)
{
  if (yyst == YY_NULL)
    YYFPRINTF (stderr, "<null>");
  else
    yy_yypstack (yyst);
  YYFPRINTF (stderr, "\n");
}

static void
yypstack (yyGLRStack* yystackp, size_t yyk)
{
  yypstates (yystackp->yytops.yystates[yyk]);
}

#define YYINDEX(YYX)                                                         \
    ((YYX) == YY_NULL ? -1 : (yyGLRStackItem*) (YYX) - yystackp->yyitems)


static void
yypdumpstack (yyGLRStack* yystackp)
{
  yyGLRStackItem* yyp;
  size_t yyi;
  for (yyp = yystackp->yyitems; yyp < yystackp->yynextFree; yyp += 1)
    {
      YYFPRINTF (stderr, "%3lu. ",
                 (unsigned long int) (yyp - yystackp->yyitems));
      if (*(yybool *) yyp)
        {
          YYFPRINTF (stderr, "Res: %d, LR State: %d, posn: %lu, pred: %ld",
                     yyp->yystate.yyresolved, yyp->yystate.yylrState,
                     (unsigned long int) yyp->yystate.yyposn,
                     (long int) YYINDEX (yyp->yystate.yypred));
          if (! yyp->yystate.yyresolved)
            YYFPRINTF (stderr, ", firstVal: %ld",
                       (long int) YYINDEX (yyp->yystate
                                             .yysemantics.yyfirstVal));
        }
      else
        {
          YYFPRINTF (stderr, "Option. rule: %d, state: %ld, next: %ld",
                     yyp->yyoption.yyrule - 1,
                     (long int) YYINDEX (yyp->yyoption.yystate),
                     (long int) YYINDEX (yyp->yyoption.yynext));
        }
      YYFPRINTF (stderr, "\n");
    }
  YYFPRINTF (stderr, "Tops:");
  for (yyi = 0; yyi < yystackp->yytops.yysize; yyi += 1)
    YYFPRINTF (stderr, "%lu: %ld; ", (unsigned long int) yyi,
               (long int) YYINDEX (yystackp->yytops.yystates[yyi]));
  YYFPRINTF (stderr, "\n");
}
#endif
/* Line 2575 of glr.c  */
#line 515 "src/libexpr/parser.y"



#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <eval.hh>
#include <download.hh>
#include <store-api.hh>


namespace nix {


Expr * EvalState::parse(const char * text,
    const Path & path, const Path & basePath, StaticEnv & staticEnv)
{
    yyscan_t scanner;
    ParseData data(*this);
    data.basePath = basePath;
    data.path = data.symbols.create(path);

    yylex_init(&scanner);
    yy_scan_string(text, scanner);
    int res = yyparse(scanner, &data);
    yylex_destroy(scanner);

    if (res) {
      if (data.atEnd)
        throw IncompleteParseError(data.error);
      else
        throw ParseError(data.error);
    }

    data.result->bindVars(staticEnv);

    return data.result;
}


Path resolveExprPath(Path path)
{
    assert(path[0] == '/');

    /* If `path' is a symlink, follow it.  This is so that relative
       path references work. */
    struct stat st;
    while (true) {
        if (lstat(path.c_str(), &st))
            throw SysError(format("getting status of ‘%1%’") % path);
        if (!S_ISLNK(st.st_mode)) break;
        path = absPath(readLink(path), dirOf(path));
    }

    /* If `path' refers to a directory, append `/default.nix'. */
    if (S_ISDIR(st.st_mode))
        path = canonPath(path + "/default.nix");

    return path;
}


Expr * EvalState::parseExprFromFile(const Path & path)
{
    return parseExprFromFile(path, staticBaseEnv);
}


Expr * EvalState::parseExprFromFile(const Path & path, StaticEnv & staticEnv)
{
    return parse(readFile(path).c_str(), path, dirOf(path), staticEnv);
}


Expr * EvalState::parseExprFromString(const string & s, const Path & basePath, StaticEnv & staticEnv)
{
    return parse(s.c_str(), "(string)", basePath, staticEnv);
}


Expr * EvalState::parseExprFromString(const string & s, const Path & basePath)
{
    return parseExprFromString(s, basePath, staticBaseEnv);
}


void EvalState::addToSearchPath(const string & s, bool warn)
{
    size_t pos = s.find('=');
    string prefix;
    Path path;
    if (pos == string::npos) {
        path = s;
    } else {
        prefix = string(s, 0, pos);
        path = string(s, pos + 1);
    }

    if (isUri(path))
        path = makeDownloader()->downloadCached(store, path, true);

    path = absPath(path);
    if (pathExists(path)) {
        debug(format("adding path ‘%1%’ to the search path") % path);
        /* Resolve symlinks in the path to support restricted mode. */
        searchPath.push_back(std::pair<string, Path>(prefix, canonPath(path, true)));
    } else if (warn)
        printMsg(lvlError, format("warning: Nix search path entry ‘%1%’ does not exist, ignoring") % path);
}


Path EvalState::findFile(const string & path)
{
    return findFile(searchPath, path);
}


Path EvalState::findFile(SearchPath & searchPath, const string & path, const Pos & pos)
{
    for (auto & i : searchPath) {
        assert(!isUri(i.second));
        Path res;
        if (i.first.empty())
            res = i.second + "/" + path;
        else {
            if (path.compare(0, i.first.size(), i.first) != 0 ||
                (path.size() > i.first.size() && path[i.first.size()] != '/'))
                continue;
            res = i.second +
                (path.size() == i.first.size() ? "" : "/" + string(path, i.first.size()));
        }
        if (pathExists(res)) return canonPath(res);
    }
    format f = format(
        "file ‘%1%’ was not found in the Nix search path (add it using $NIX_PATH or -I)"
        + string(pos ? ", at %2%" : ""));
    f.exceptions(boost::io::all_error_bits ^ boost::io::too_many_args_bit);
    throw ThrownError(f % path % pos);
}


}
