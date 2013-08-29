/* Driver template for the LEMON parser generator.
** The author disclaims copyright to this source code.
*/
/* First off, code is included that follows the "include" declaration
** in the input grammar file. */
#include <stdio.h>
#line 5 "smalltalk.y"

#include <assert.h>
#import "../Foundation/Foundation.h"
#import "../LanguageKit/LanguageKit.h"
#import "SmalltalkParser.h"

static NSDictionary *DeclRefClasses;

static LKDeclRef *RefForSymbol(NSString *aSymbol)
{
	if (nil == DeclRefClasses)
	{
		DeclRefClasses = [[NSDictionary dictionaryWithObjectsAndKeys:
			[LKSelfRef class], @"self",
			[LKSuperRef class], @"super",
			[LKBlockSelfRef class], @"blockContext",
			[LKNilRef class], @"nil",
			[LKNilRef class], @"Nil",
			nil] retain];
	}
	Class cls = [DeclRefClasses objectForKey: aSymbol];
	//LKDeclRef *ref = nil;
	if (nil == cls)
	{
		cls = [LKDeclRef class];
	}
	return [cls referenceWithSymbol: aSymbol];
}

#line 38 "smalltalk.c"
/* Next is all token values, in a form suitable for use by makeheaders.
** This section will be null unless lemon is run with the -m switch.
*/
/* 
** These constants (all generated automatically by the parser generator)
** specify the various kinds of tokens (terminals) that the parser
** understands. 
**
** Each symbol here is a terminal symbol in the grammar.
*/
/* Make sure the INTERFACE macro is defined.
*/
#ifndef INTERFACE
# define INTERFACE 1
#endif
/* The next thing included is series of defines which control
** various aspects of the generated parser.
**    YYCODETYPE         is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 terminals
**                       and nonterminals.  "int" is used otherwise.
**    YYNOCODE           is a number of type YYCODETYPE which corresponds
**                       to no legal terminal or nonterminal number.  This
**                       number is used to fill in empty slots of the hash 
**                       table.
**    YYFALLBACK         If defined, this indicates that one or more tokens
**                       have fall-back values which should be used if the
**                       original value of the token will not parse.
**    YYACTIONTYPE       is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 rules and
**                       states combined.  "int" is used otherwise.
**    SmalltalkParseTOKENTYPE     is the data type used for minor tokens given 
**                       directly to the parser from the tokenizer.
**    YYMINORTYPE        is the data type used for all minor tokens.
**                       This is typically a union of many types, one of
**                       which is SmalltalkParseTOKENTYPE.  The entry in the union
**                       for base tokens is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
**    SmalltalkParseARG_SDECL     A static variable declaration for the %extra_argument
**    SmalltalkParseARG_PDECL     A parameter declaration for the %extra_argument
**    SmalltalkParseARG_STORE     Code to store %extra_argument into yypParser
**    SmalltalkParseARG_FETCH     Code to extract %extra_argument from yypParser
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
*/
#define YYCODETYPE unsigned char
#define YYNOCODE 63
#define YYACTIONTYPE unsigned char
#define SmalltalkParseTOKENTYPE id
typedef union {
  int yyinit;
  SmalltalkParseTOKENTYPE yy0;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define SmalltalkParseARG_SDECL SmalltalkParser *p;
#define SmalltalkParseARG_PDECL ,SmalltalkParser *p
#define SmalltalkParseARG_FETCH SmalltalkParser *p = yypParser->p
#define SmalltalkParseARG_STORE yypParser->p = p
#define YYNSTATE 143
#define YYNRULE 87
#define YY_NO_ACTION      (YYNSTATE+YYNRULE+2)
#define YY_ACCEPT_ACTION  (YYNSTATE+YYNRULE+1)
#define YY_ERROR_ACTION   (YYNSTATE+YYNRULE)

/* The yyzerominor constant is used to initialize instances of
** YYMINORTYPE objects to zero. */
static const YYMINORTYPE yyzerominor = { 0 };

/* Define the yytestcase() macro to be a no-op if is not already defined
** otherwise.
**
** Applications can choose to define yytestcase() in the %include section
** to a macro that can assist in verifying code coverage.  For production
** code the yytestcase() macro should be turned off.  But it is useful
** for testing.
*/
#ifndef yytestcase
# define yytestcase(X)
#endif


/* Next are the tables used to determine what action to take based on the
** current state and lookahead token.  These tables are used to implement
** functions that take a state number and lookahead value and return an
** action integer.  
**
** Suppose the action integer is N.  Then the action is determined as
** follows
**
**   0 <= N < YYNSTATE                  Shift N.  That is, push the lookahead
**                                      token onto the stack and goto state N.
**
**   YYNSTATE <= N < YYNSTATE+YYNRULE   Reduce by rule N-YYNSTATE.
**
**   N == YYNSTATE+YYNRULE              A syntax error has occurred.
**
**   N == YYNSTATE+YYNRULE+1            The parser accepts its input.
**
**   N == YYNSTATE+YYNRULE+2            No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as
**
**      yy_action[ yy_shift_ofst[S] + X ]
**
** If the index value yy_shift_ofst[S]+X is out of range or if the value
** yy_lookahead[yy_shift_ofst[S]+X] is not equal to X or if yy_shift_ofst[S]
** is equal to YY_SHIFT_USE_DFLT, it means that the action is not in the table
** and that yy_default[S] should be used instead.  
**
** The formula above is for computing the action when the lookahead is
** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
** a reduce action) then the yy_reduce_ofst[] array is used in place of
** the yy_shift_ofst[] array and YY_REDUCE_USE_DFLT is used in place of
** YY_SHIFT_USE_DFLT.
**
** The following are the tables generated in this section:
**
**  yy_action[]        A single table containing all actions.
**  yy_lookahead[]     A table containing the lookahead for each entry in
**                     yy_action.  Used to detect hash collisions.
**  yy_shift_ofst[]    For each state, the offset into yy_action for
**                     shifting terminals.
**  yy_reduce_ofst[]   For each state, the offset into yy_action for
**                     shifting non-terminals after a reduce.
**  yy_default[]       Default action for each state.
*/
static const YYACTIONTYPE yy_action[] = {
 /*     0 */    72,  133,  131,   69,   23,   17,   12,    2,  188,   59,
 /*    10 */   132,   88,    6,   91,  134,  124,   62,    5,  130,   20,
 /*    20 */    22,   83,  141,  138,  137,   61,   63,   64,  129,  142,
 /*    30 */    70,   44,   65,  144,   22,  103,   60,  101,   24,   22,
 /*    40 */    83,  141,  138,  137,  102,   63,   64,  129,  142,  109,
 /*    50 */   136,  133,  131,  108,   87,   17,   66,   24,   56,   53,
 /*    60 */     7,   75,   86,   79,  134,  124,   62,    5,   13,   20,
 /*    70 */    43,   83,  141,  138,  137,   61,   63,   64,  129,  142,
 /*    80 */   231,   11,   80,   33,   97,  112,  115,  145,   14,   89,
 /*    90 */    93,   56,   67,   81,   75,    1,   46,  118,  104,   99,
 /*   100 */   119,   58,   85,   14,   14,   77,    2,   88,   59,  132,
 /*   110 */   143,  117,  117,  125,  114,  113,   48,  111,  107,   42,
 /*   120 */    77,   77,   88,   88,   74,  110,  117,    1,   15,    2,
 /*   130 */    88,   59,  132,   68,   84,   77,   78,   95,    2,   57,
 /*   140 */    59,  132,    1,   71,  139,  127,   52,    2,   73,   59,
 /*   150 */   132,   75,  135,   35,  106,   60,  101,   96,   22,   37,
 /*   160 */    82,   50,   38,   31,   39,   36,   49,   50,   21,   19,
 /*   170 */    41,   18,   10,   45,   40,   28,    8,  121,    9,   29,
 /*   180 */    26,   51,   27,   30,   94,   55,    4,   54,   25,  126,
 /*   190 */   142,  123,   32,  128,  100,  140,   16,  232,   23,   68,
 /*   200 */    92,  232,   90,  120,  232,  232,   47,  189,   98,   34,
 /*   210 */   122,  116,    3,  232,  232,  232,  232,   76,  232,  105,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     9,   10,   11,   49,   18,   14,   53,   53,   22,   55,
 /*    10 */    56,   20,   21,    9,   23,   24,   25,   26,   52,   28,
 /*    20 */    54,    1,    2,    3,    4,    5,    6,    7,    8,    9,
 /*    30 */    50,   51,   52,    0,   54,   50,   51,   52,   18,   54,
 /*    40 */     1,    2,    3,    4,    5,    6,    7,    8,    9,   33,
 /*    50 */     9,   10,   11,   37,    9,   14,   57,   18,   42,   12,
 /*    60 */    61,   45,   17,   16,   23,   24,   25,   26,   53,   28,
 /*    70 */    34,    1,    2,    3,    4,    5,    6,    7,    8,    9,
 /*    80 */    31,   32,   33,    6,   35,   36,   37,    0,    1,    7,
 /*    90 */    37,   42,   44,    6,   45,   47,    9,    9,   10,   11,
 /*   100 */    38,   48,   49,    1,    1,   18,   53,   20,   55,   56,
 /*   110 */     0,    9,    9,   35,   36,   37,    6,   15,   15,    9,
 /*   120 */    18,   18,   20,   20,   44,   49,    9,   47,   53,   53,
 /*   130 */    20,   55,   56,   13,   49,   18,    1,   17,   53,   44,
 /*   140 */    55,   56,   47,   49,    9,   38,    9,   53,   42,   55,
 /*   150 */    56,   45,   17,   14,   50,   51,   52,   59,   54,   17,
 /*   160 */     7,    8,   39,   58,   59,   17,    7,    8,   43,   43,
 /*   170 */    34,   43,   40,   46,   41,   60,    5,    7,   40,    6,
 /*   180 */     5,    9,    5,   14,   19,    9,   22,   13,    5,    5,
 /*   190 */     9,    9,   14,    5,   15,   29,   53,   62,   18,   13,
 /*   200 */     9,   62,   15,    9,   62,   62,   13,   22,   19,   14,
 /*   210 */     9,   15,   22,   62,   62,   62,   62,   18,   62,   27,
};
#define YY_SHIFT_USE_DFLT (-15)
#define YY_SHIFT_MAX 82
static const short yy_shift_ofst[] = {
 /*     0 */    87,   -9,   20,   39,   39,   41,   41,   41,   41,  102,
 /*    10 */   103,  110,   70,   70,  117,  181,  181,  186,  -15,  -15,
 /*    20 */   -15,  -15,   41,   41,   41,   41,   88,   88,  120,  137,
 /*    30 */   142,  148,  148,  137,  148,  -15,  -15,  -15,  -15,  -15,
 /*    40 */   135,  153,   47,  159,  -14,   45,   47,  171,  173,  170,
 /*    50 */   172,  175,  177,  174,  176,  169,  178,  179,  165,  164,
 /*    60 */   180,  183,  182,  184,  188,  185,  166,  187,  191,  189,
 /*    70 */   190,  192,  193,  195,  196,  199,  194,  201,    4,  139,
 /*    80 */    33,   77,   82,
};
#define YY_REDUCE_USE_DFLT (-48)
#define YY_REDUCE_MAX 39
static const short yy_reduce_ofst[] = {
 /*     0 */    49,   53,  -20,  -15,  104,   94,   85,  -46,   76,   16,
 /*    10 */    16,   78,  -34,  -34,  106,  -34,  -34,  105,   80,   95,
 /*    20 */    -1,   48,  143,  -47,   15,   75,   62,  107,   98,   36,
 /*    30 */   123,  125,  126,  136,  128,  132,  127,  133,  138,  115,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   230,  179,  206,  230,  230,  230,  230,  226,  230,  230,
 /*    10 */   230,  230,  190,  191,  230,  217,  193,  223,  183,  183,
 /*    20 */   229,  183,  230,  230,  230,  230,  230,  230,  230,  230,
 /*    30 */   162,  176,  176,  230,  176,  168,  178,  165,  168,  225,
 /*    40 */   230,  230,  230,  230,  209,  230,  171,  230,  230,  230,
 /*    50 */   230,  230,  230,  230,  230,  230,  230,  230,  180,  204,
 /*    60 */   188,  198,  230,  199,  200,  216,  230,  230,  230,  227,
 /*    70 */   230,  230,  210,  230,  230,  172,  230,  230,  230,  230,
 /*    80 */   230,  230,  230,  194,  186,  185,  175,  177,  184,  147,
 /*    90 */   220,  164,  221,  182,  181,  222,  224,  149,  228,  158,
 /*   100 */   169,  189,  198,  208,  157,  218,  207,  160,  167,  166,
 /*   110 */   187,  159,  151,  152,  150,  153,  170,  171,  156,  154,
 /*   120 */   173,  146,  174,  215,  214,  148,  201,  155,  202,  203,
 /*   130 */   216,  213,  205,  212,  211,  161,  210,  197,  196,  163,
 /*   140 */   219,  195,  192,
};
#define YY_SZ_ACTTAB (int)(sizeof(yy_action)/sizeof(yy_action[0]))

/* The next table maps tokens into fallback tokens.  If a construct
** like the following:
** 
**      %fallback ID X Y Z.
**
** appears in the grammar, then ID becomes a fallback token for X, Y,
** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
** but it does not parse, the type of the token is changed to ID and
** the parse is retried before an error is thrown.
*/
#ifdef YYFALLBACK
static const YYCODETYPE yyFallback[] = {
};
#endif /* YYFALLBACK */

/* The following structure represents a single element of the
** parser's stack.  Information stored includes:
**
**   +  The state number for the parser at this level of the stack.
**
**   +  The value of the token stored at this level of the stack.
**      (In other words, the "major" token.)
**
**   +  The semantic value stored at this level of the stack.  This is
**      the information used by the action routines in the grammar.
**      It is sometimes called the "minor" token.
*/
struct yyStackEntry {
  YYACTIONTYPE stateno;  /* The state-number */
  YYCODETYPE major;      /* The major token value.  This is the code
                         ** number for the token at this stack level */
  YYMINORTYPE minor;     /* The user-supplied minor token value.  This
                         ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct yyParser {
  int yyidx;                    /* Index of top element in stack */
#ifdef YYTRACKMAXSTACKDEPTH
  int yyidxMax;                 /* Maximum value of yyidx */
#endif
  int yyerrcnt;                 /* Shifts left before out of the error */
  SmalltalkParseARG_SDECL                /* A place to hold %extra_argument */
#if YYSTACKDEPTH<=0
  int yystksz;                  /* Current side of the stack */
  yyStackEntry *yystack;        /* The parser's stack */
#else
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
#endif
};
typedef struct yyParser yyParser;

#ifndef NDEBUG
#include <stdio.h>
static FILE *yyTraceFILE = 0;
static char *yyTracePrompt = 0;
#endif /* NDEBUG */

#ifndef NDEBUG
/* 
** Turn parser tracing on by giving a stream to which to write the trace
** and a prompt to preface each trace message.  Tracing is turned off
** by making either argument NULL 
**
** Inputs:
** <ul>
** <li> A FILE* to which trace output should be written.
**      If NULL, then tracing is turned off.
** <li> A prefix string written at the beginning of every
**      line of trace output.  If NULL, then tracing is
**      turned off.
** </ul>
**
** Outputs:
** None.
*/
void SmalltalkParseTrace(FILE *TraceFILE, char *zTracePrompt);
void SmalltalkParseTrace(FILE *TraceFILE, char *zTracePrompt){
  yyTraceFILE = TraceFILE;
  yyTracePrompt = zTracePrompt;
  if( yyTraceFILE==0 ) yyTracePrompt = 0;
  else if( yyTracePrompt==0 ) yyTraceFILE = 0;
}
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
static const char *const yyTokenName[] = { 
  "$",             "PLUS",          "MINUS",         "STAR",        
  "SLASH",         "EQ",            "LT",            "GT",          
  "COMMA",         "WORD",          "STRING",        "NUMBER",      
  "SUBCLASS",      "COLON",         "LSQBRACK",      "RSQBRACK",    
  "EXTEND",        "BAR",           "KEYWORD",       "STOP",        
  "COMMENT",       "RETURN",        "SEMICOLON",     "SYMBOL",      
  "FLOATNUMBER",   "AT",            "LPAREN",        "RPAREN",      
  "LBRACE",        "RBRACE",        "error",         "file",        
  "module",        "method",        "pragma_dict",   "subclass",    
  "category",      "comment",       "pragma_value",  "ivar_list",   
  "method_list",   "ivars",         "signature",     "local_list",  
  "statement_list",  "keyword_signature",  "locals",        "statements",  
  "statement",     "expression",    "message",       "keyword_message",
  "simple_message",  "simple_expression",  "binary_selector",  "cascade_expression",
  "keyword_expression",  "expression_list",  "argument_list",  "argument",    
  "arguments",     "expressions", 
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "file ::= module",
 /*   1 */ "file ::= method",
 /*   2 */ "file ::=",
 /*   3 */ "module ::= module LT LT pragma_dict GT GT",
 /*   4 */ "module ::= LT LT pragma_dict GT GT",
 /*   5 */ "module ::= module subclass",
 /*   6 */ "module ::= subclass",
 /*   7 */ "module ::= module category",
 /*   8 */ "module ::= category",
 /*   9 */ "module ::= module comment",
 /*  10 */ "module ::= comment",
 /*  11 */ "pragma_dict ::= pragma_dict COMMA WORD EQ pragma_value",
 /*  12 */ "pragma_dict ::= WORD EQ pragma_value",
 /*  13 */ "pragma_value ::= WORD",
 /*  14 */ "pragma_value ::= STRING",
 /*  15 */ "pragma_value ::= NUMBER",
 /*  16 */ "subclass ::= WORD SUBCLASS COLON WORD LSQBRACK ivar_list method_list RSQBRACK",
 /*  17 */ "category ::= WORD EXTEND LSQBRACK method_list RSQBRACK",
 /*  18 */ "ivar_list ::= BAR ivars BAR",
 /*  19 */ "ivar_list ::=",
 /*  20 */ "ivars ::= ivars WORD",
 /*  21 */ "ivars ::= ivars PLUS WORD",
 /*  22 */ "ivars ::=",
 /*  23 */ "method_list ::= method_list method",
 /*  24 */ "method_list ::= method_list comment",
 /*  25 */ "method_list ::=",
 /*  26 */ "method ::= signature LSQBRACK local_list statement_list RSQBRACK",
 /*  27 */ "method ::= PLUS signature LSQBRACK local_list statement_list RSQBRACK",
 /*  28 */ "signature ::= WORD",
 /*  29 */ "signature ::= keyword_signature",
 /*  30 */ "keyword_signature ::= keyword_signature KEYWORD WORD",
 /*  31 */ "keyword_signature ::= KEYWORD WORD",
 /*  32 */ "local_list ::= BAR locals BAR",
 /*  33 */ "local_list ::=",
 /*  34 */ "locals ::= locals WORD",
 /*  35 */ "locals ::=",
 /*  36 */ "statement_list ::= statements",
 /*  37 */ "statement_list ::= statements statement",
 /*  38 */ "statements ::= statements statement STOP",
 /*  39 */ "statements ::= statements comment",
 /*  40 */ "statements ::=",
 /*  41 */ "comment ::= COMMENT",
 /*  42 */ "statement ::= expression",
 /*  43 */ "statement ::= RETURN expression",
 /*  44 */ "statement ::= WORD COLON EQ expression",
 /*  45 */ "message ::= keyword_message",
 /*  46 */ "message ::= simple_message",
 /*  47 */ "keyword_message ::= keyword_message KEYWORD simple_expression",
 /*  48 */ "keyword_message ::= KEYWORD simple_expression",
 /*  49 */ "simple_message ::= WORD",
 /*  50 */ "simple_message ::= binary_selector simple_expression",
 /*  51 */ "binary_selector ::= PLUS",
 /*  52 */ "binary_selector ::= MINUS",
 /*  53 */ "binary_selector ::= STAR",
 /*  54 */ "binary_selector ::= SLASH",
 /*  55 */ "binary_selector ::= EQ",
 /*  56 */ "binary_selector ::= LT",
 /*  57 */ "binary_selector ::= GT",
 /*  58 */ "binary_selector ::= LT EQ",
 /*  59 */ "binary_selector ::= GT EQ",
 /*  60 */ "binary_selector ::= COMMA",
 /*  61 */ "expression ::= cascade_expression",
 /*  62 */ "expression ::= keyword_expression",
 /*  63 */ "expression ::= simple_expression",
 /*  64 */ "cascade_expression ::= cascade_expression SEMICOLON message",
 /*  65 */ "cascade_expression ::= simple_expression message SEMICOLON message",
 /*  66 */ "keyword_expression ::= simple_expression keyword_message",
 /*  67 */ "simple_expression ::= WORD",
 /*  68 */ "simple_expression ::= SYMBOL",
 /*  69 */ "simple_expression ::= STRING",
 /*  70 */ "simple_expression ::= NUMBER",
 /*  71 */ "simple_expression ::= FLOATNUMBER",
 /*  72 */ "simple_expression ::= AT WORD",
 /*  73 */ "simple_expression ::= simple_expression simple_message",
 /*  74 */ "simple_expression ::= simple_expression EQ EQ simple_expression",
 /*  75 */ "simple_expression ::= LPAREN expression RPAREN",
 /*  76 */ "simple_expression ::= LBRACE expression_list RBRACE",
 /*  77 */ "simple_expression ::= LSQBRACK argument_list local_list statement_list RSQBRACK",
 /*  78 */ "argument ::= COLON WORD",
 /*  79 */ "argument_list ::= argument arguments BAR",
 /*  80 */ "argument_list ::=",
 /*  81 */ "arguments ::= arguments argument",
 /*  82 */ "arguments ::=",
 /*  83 */ "expression_list ::= expressions",
 /*  84 */ "expression_list ::= expressions expression",
 /*  85 */ "expressions ::= expressions expression STOP",
 /*  86 */ "expressions ::=",
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.
*/
static void yyGrowStack(yyParser *p){
  int newSize;
  yyStackEntry *pNew;

  newSize = p->yystksz*2 + 100;
  pNew = realloc(p->yystack, newSize*sizeof(pNew[0]));
  if( pNew ){
    p->yystack = pNew;
    p->yystksz = newSize;
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sStack grows to %d entries!\n",
              yyTracePrompt, p->yystksz);
    }
#endif
  }
}
#endif

/* 
** This function allocates a new parser.
** The only argument is a pointer to a function which works like
** malloc.
**
** Inputs:
** A pointer to the function used to allocate memory.
**
** Outputs:
** A pointer to a parser.  This pointer is used in subsequent calls
** to SmalltalkParse and SmalltalkParseFree.
*/
void *SmalltalkParseAlloc(void *(*mallocProc)(size_t));
void *SmalltalkParseAlloc(void *(*mallocProc)(size_t)){
  yyParser *pParser;
  pParser = (yyParser*)(*mallocProc)( (size_t)sizeof(yyParser) );
  if( pParser ){
    pParser->yyidx = -1;
#ifdef YYTRACKMAXSTACKDEPTH
    pParser->yyidxMax = 0;
#endif
#if YYSTACKDEPTH<=0
    pParser->yystack = NULL;
    pParser->yystksz = 0;
    yyGrowStack(pParser);
#endif
  }
  return pParser;
}

/* The following function deletes the value associated with a
** symbol.  The symbol can be either a terminal or nonterminal.
** "yymajor" is the symbol code, and "yypminor" is a pointer to
** the value.
*/
static void yy_destructor(
  yyParser *yypParser,    /* The parser */
  YYCODETYPE yymajor,     /* Type code for object to destroy */
  YYMINORTYPE *yypminor   /* The object to be destroyed */
){
//  SmalltalkParseARG_FETCH;
  switch( yymajor ){
    /* Here is inserted the actions which take place when a
    ** terminal or non-terminal is destroyed.  This can happen
    ** when the symbol is popped from the stack during a
    ** reduce or during error processing or when a parser is 
    ** being destroyed before it is finished parsing.
    **
    ** Note: during a reduce, the only symbols destroyed are those
    ** which appear on the RHS of the rule, but which are not used
    ** inside the C code.
    */
    default:  break;   /* If no destructor action specified: do nothing */
  }
}

/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
**
** Return the major token number for the symbol popped.
*/
static int yy_pop_parser_stack(yyParser *pParser){
  YYCODETYPE yymajor;
  yyStackEntry *yytos = &pParser->yystack[pParser->yyidx];

  if( pParser->yyidx<0 ) return 0;
#ifndef NDEBUG
  if( yyTraceFILE && pParser->yyidx>=0 ){
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yytos->major]);
  }
#endif
  yymajor = yytos->major;
  yy_destructor(pParser, yymajor, &yytos->minor);
  pParser->yyidx--;
  return yymajor;
}

/* 
** Deallocate and destroy a parser.  Destructors are all called for
** all stack elements before shutting the parser down.
**
** Inputs:
** <ul>
** <li>  A pointer to the parser.  This should be a pointer
**       obtained from SmalltalkParseAlloc.
** <li>  A pointer to a function used to reclaim memory obtained
**       from malloc.
** </ul>
*/
void SmalltalkParseFree(
						void *p,                    /* The parser to be deleted */
						void (*freeProc)(void*)     /* Function used to reclaim memory */
						);
void SmalltalkParseFree(
  void *p,                    /* The parser to be deleted */
  void (*freeProc)(void*)     /* Function used to reclaim memory */
){
  yyParser *pParser = (yyParser*)p;
  if( pParser==0 ) return;
  while( pParser->yyidx>=0 ) yy_pop_parser_stack(pParser);
#if YYSTACKDEPTH<=0
  free(pParser->yystack);
#endif
  (*freeProc)((void*)pParser);
}

/*
** Return the peak depth of the stack for a parser.
*/
#ifdef YYTRACKMAXSTACKDEPTH
int SmalltalkParseStackPeak(void *p){
  yyParser *pParser = (yyParser*)p;
  return pParser->yyidxMax;
}
#endif

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_shift_action(
  yyParser *pParser,        /* The parser */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
  int stateno = pParser->yystack[pParser->yyidx].stateno;
 
  if( stateno>YY_SHIFT_MAX || (i = yy_shift_ofst[stateno])==YY_SHIFT_USE_DFLT ){
    return yy_default[stateno];
  }
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
  if( i<0 || i>=YY_SZ_ACTTAB || yy_lookahead[i]!=iLookAhead ){
    if( iLookAhead>0 ){
#ifdef YYFALLBACK
      YYCODETYPE iFallback;            /* Fallback token */
      if( iLookAhead<sizeof(yyFallback)/sizeof(yyFallback[0])
             && (iFallback = yyFallback[iLookAhead])!=0 ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
             yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[iFallback]);
        }
#endif
        return yy_find_shift_action(pParser, iFallback);
      }
#endif
#ifdef YYWILDCARD
      {
        int j = i - iLookAhead + YYWILDCARD;
        if( j>=0 && j<YY_SZ_ACTTAB && yy_lookahead[j]==YYWILDCARD ){
#ifndef NDEBUG
          if( yyTraceFILE ){
            fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
               yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[YYWILDCARD]);
          }
#endif /* NDEBUG */
          return yy_action[j];
        }
      }
#endif /* YYWILDCARD */
    }
    return yy_default[stateno];
  }else{
    return yy_action[i];
  }
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_reduce_action(
  int stateno,              /* Current state number */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
#ifdef YYERRORSYMBOL
  if( stateno>YY_REDUCE_MAX ){
    return yy_default[stateno];
  }
#else
  assert( stateno<=YY_REDUCE_MAX );
#endif
  i = yy_reduce_ofst[stateno];
  assert( i!=YY_REDUCE_USE_DFLT );
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
#ifdef YYERRORSYMBOL
  if( i<0 || i>=YY_SZ_ACTTAB || yy_lookahead[i]!=iLookAhead ){
    return yy_default[stateno];
  }
#else
  assert( i>=0 && i<YY_SZ_ACTTAB );
  assert( yy_lookahead[i]==iLookAhead );
#endif
  return yy_action[i];
}

/*
** The following routine is called if the stack overflows.
*/
static void yyStackOverflow(yyParser *yypParser, YYMINORTYPE *yypMinor){
   SmalltalkParseARG_FETCH;
   yypParser->yyidx--;
#ifndef NDEBUG
   if( yyTraceFILE ){
     fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
   }
#endif
   while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
   /* Here code is inserted which will execute if the parser
   ** stack every overflows */
   SmalltalkParseARG_STORE; /* Suppress warning about unused %extra_argument var */
}

/*
** Perform a shift action.
*/
static void yy_shift(
  yyParser *yypParser,          /* The parser to be shifted */
  int yyNewState,               /* The new state to shift in */
  int yyMajor,                  /* The major token to shift in */
  YYMINORTYPE *yypMinor         /* Pointer to the minor token to shift in */
){
  yyStackEntry *yytos;
  yypParser->yyidx++;
#ifdef YYTRACKMAXSTACKDEPTH
  if( yypParser->yyidx>yypParser->yyidxMax ){
    yypParser->yyidxMax = yypParser->yyidx;
  }
#endif
#if YYSTACKDEPTH>0 
  if( yypParser->yyidx>=YYSTACKDEPTH ){
    yyStackOverflow(yypParser, yypMinor);
    return;
  }
#else
  if( yypParser->yyidx>=yypParser->yystksz ){
    yyGrowStack(yypParser);
    if( yypParser->yyidx>=yypParser->yystksz ){
      yyStackOverflow(yypParser, yypMinor);
      return;
    }
  }
#endif
  yytos = &yypParser->yystack[yypParser->yyidx];
  yytos->stateno = (YYACTIONTYPE)yyNewState;
  yytos->major = (YYCODETYPE)yyMajor;
  yytos->minor = *yypMinor;
#ifndef NDEBUG
  if( yyTraceFILE && yypParser->yyidx>0 ){
    int i;
    fprintf(yyTraceFILE,"%sShift %d\n",yyTracePrompt,yyNewState);
    fprintf(yyTraceFILE,"%sStack:",yyTracePrompt);
    for(i=1; i<=yypParser->yyidx; i++)
      fprintf(yyTraceFILE," %s",yyTokenName[yypParser->yystack[i].major]);
    fprintf(yyTraceFILE,"\n");
  }
#endif
}

/* The following table contains information about every rule that
** is used during the reduce.
*/
static const struct {
  YYCODETYPE lhs;         /* Symbol on the left-hand side of the rule */
  unsigned char nrhs;     /* Number of right-hand side symbols in the rule */
} yyRuleInfo[] = {
  { 31, 1 },
  { 31, 1 },
  { 31, 0 },
  { 32, 6 },
  { 32, 5 },
  { 32, 2 },
  { 32, 1 },
  { 32, 2 },
  { 32, 1 },
  { 32, 2 },
  { 32, 1 },
  { 34, 5 },
  { 34, 3 },
  { 38, 1 },
  { 38, 1 },
  { 38, 1 },
  { 35, 8 },
  { 36, 5 },
  { 39, 3 },
  { 39, 0 },
  { 41, 2 },
  { 41, 3 },
  { 41, 0 },
  { 40, 2 },
  { 40, 2 },
  { 40, 0 },
  { 33, 5 },
  { 33, 6 },
  { 42, 1 },
  { 42, 1 },
  { 45, 3 },
  { 45, 2 },
  { 43, 3 },
  { 43, 0 },
  { 46, 2 },
  { 46, 0 },
  { 44, 1 },
  { 44, 2 },
  { 47, 3 },
  { 47, 2 },
  { 47, 0 },
  { 37, 1 },
  { 48, 1 },
  { 48, 2 },
  { 48, 4 },
  { 50, 1 },
  { 50, 1 },
  { 51, 3 },
  { 51, 2 },
  { 52, 1 },
  { 52, 2 },
  { 54, 1 },
  { 54, 1 },
  { 54, 1 },
  { 54, 1 },
  { 54, 1 },
  { 54, 1 },
  { 54, 1 },
  { 54, 2 },
  { 54, 2 },
  { 54, 1 },
  { 49, 1 },
  { 49, 1 },
  { 49, 1 },
  { 55, 3 },
  { 55, 4 },
  { 56, 2 },
  { 53, 1 },
  { 53, 1 },
  { 53, 1 },
  { 53, 1 },
  { 53, 1 },
  { 53, 2 },
  { 53, 2 },
  { 53, 4 },
  { 53, 3 },
  { 53, 3 },
  { 53, 5 },
  { 59, 2 },
  { 58, 3 },
  { 58, 0 },
  { 60, 2 },
  { 60, 0 },
  { 57, 1 },
  { 57, 2 },
  { 61, 3 },
  { 61, 0 },
};

static void yy_accept(yyParser*);  /* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
*/
static void yy_reduce(
  yyParser *yypParser,         /* The parser */
  int yyruleno                 /* Number of the rule by which to reduce */
){
  int yygoto;                     /* The next state */
  int yyact;                      /* The next action */
  YYMINORTYPE yygotominor;        /* The LHS of the rule reduced */
  yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  SmalltalkParseARG_FETCH;
  yymsp = &yypParser->yystack[yypParser->yyidx];
#ifndef NDEBUG
  if( yyTraceFILE && yyruleno>=0 
        && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
    fprintf(yyTraceFILE, "%sReduce [%s].\n", yyTracePrompt,
      yyRuleName[yyruleno]);
  }
#endif /* NDEBUG */

  /* Silence complaints from purify about yygotominor being uninitialized
  ** in some cases when it is copied into the stack after the following
  ** switch.  yygotominor is uninitialized when a rule reduces that does
  ** not set the value of its left-hand side nonterminal.  Leaving the
  ** value of the nonterminal uninitialized is utterly harmless as long
  ** as the value is never used.  So really the only thing this code
  ** accomplishes is to quieten purify.  
  **
  ** 2007-01-16:  The wireshark project (www.wireshark.org) reports that
  ** without this code, their parser segfaults.  I'm not sure what there
  ** parser is doing to make this happen.  This is the second bug report
  ** from wireshark this week.  Clearly they are stressing Lemon in ways
  ** that it has not been previously stressed...  (SQLite ticket #2172)
  */
  /*memset(&yygotominor, 0, sizeof(yygotominor));*/
  yygotominor = yyzerominor;


  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
      case 0: /* file ::= module */
      case 1: /* file ::= method */ yytestcase(yyruleno==1);
#line 43 "smalltalk.y"
{
	[p setAST:yymsp[0].minor.yy0];
}
#line 912 "smalltalk.c"
        break;
      case 3: /* module ::= module LT LT pragma_dict GT GT */
#line 53 "smalltalk.y"
{
	[yymsp[-5].minor.yy0 addPragmas:yymsp[-2].minor.yy0];
	yygotominor.yy0 = yymsp[-5].minor.yy0;
}
#line 920 "smalltalk.c"
        break;
      case 4: /* module ::= LT LT pragma_dict GT GT */
#line 58 "smalltalk.y"
{
	yygotominor.yy0 = [LKModule module];
	[yygotominor.yy0 addPragmas:yymsp[-2].minor.yy0];
}
#line 928 "smalltalk.c"
        break;
      case 5: /* module ::= module subclass */
#line 63 "smalltalk.y"
{
	[yymsp[-1].minor.yy0 addClass:yymsp[0].minor.yy0];
	yygotominor.yy0 = yymsp[-1].minor.yy0;
}
#line 936 "smalltalk.c"
        break;
      case 6: /* module ::= subclass */
#line 68 "smalltalk.y"
{
	yygotominor.yy0 = [LKModule module];
	[yygotominor.yy0 addClass:yymsp[0].minor.yy0];
}
#line 944 "smalltalk.c"
        break;
      case 7: /* module ::= module category */
#line 73 "smalltalk.y"
{
	[yymsp[-1].minor.yy0 addCategory:yymsp[0].minor.yy0];
	yygotominor.yy0 = yymsp[-1].minor.yy0;
}
#line 952 "smalltalk.c"
        break;
      case 8: /* module ::= category */
#line 78 "smalltalk.y"
{
	yygotominor.yy0 = [LKModule module];
	[yygotominor.yy0 addCategory:yymsp[0].minor.yy0];
}
#line 960 "smalltalk.c"
        break;
      case 9: /* module ::= module comment */
      case 18: /* ivar_list ::= BAR ivars BAR */ yytestcase(yyruleno==18);
      case 24: /* method_list ::= method_list comment */ yytestcase(yyruleno==24);
      case 32: /* local_list ::= BAR locals BAR */ yytestcase(yyruleno==32);
#line 83 "smalltalk.y"
{
	yygotominor.yy0 = yymsp[-1].minor.yy0;
}
#line 970 "smalltalk.c"
        break;
      case 10: /* module ::= comment */
#line 87 "smalltalk.y"
{
	yygotominor.yy0 = [LKModule module];
}
#line 977 "smalltalk.c"
        break;
      case 11: /* pragma_dict ::= pragma_dict COMMA WORD EQ pragma_value */
#line 92 "smalltalk.y"
{
	[yymsp[-4].minor.yy0 setObject:yymsp[0].minor.yy0 forKey:yymsp[-2].minor.yy0];
	yygotominor.yy0 = yymsp[-4].minor.yy0;
}
#line 985 "smalltalk.c"
        break;
      case 12: /* pragma_dict ::= WORD EQ pragma_value */
#line 97 "smalltalk.y"
{
	yygotominor.yy0 = [NSMutableDictionary dictionaryWithObject:yymsp[0].minor.yy0 forKey:yymsp[-2].minor.yy0];
}
#line 992 "smalltalk.c"
        break;
      case 13: /* pragma_value ::= WORD */
      case 14: /* pragma_value ::= STRING */ yytestcase(yyruleno==14);
      case 15: /* pragma_value ::= NUMBER */ yytestcase(yyruleno==15);
#line 101 "smalltalk.y"
{ yygotominor.yy0 = yymsp[0].minor.yy0; }
#line 999 "smalltalk.c"
        break;
      case 16: /* subclass ::= WORD SUBCLASS COLON WORD LSQBRACK ivar_list method_list RSQBRACK */
#line 106 "smalltalk.y"
{
	yygotominor.yy0 = [LKSubclass subclassWithName:yymsp[-4].minor.yy0
	                 superclassNamed:yymsp[-7].minor.yy0
	                           cvars:[yymsp[-2].minor.yy0 objectAtIndex:1]
	                           ivars:[yymsp[-2].minor.yy0 objectAtIndex:0]
	                         methods:yymsp[-1].minor.yy0];
}
#line 1010 "smalltalk.c"
        break;
      case 17: /* category ::= WORD EXTEND LSQBRACK method_list RSQBRACK */
#line 115 "smalltalk.y"
{
	yygotominor.yy0 = [LKCategoryDef categoryOnClassNamed:yymsp[-4].minor.yy0 methods:yymsp[-1].minor.yy0];
}
#line 1017 "smalltalk.c"
        break;
      case 20: /* ivars ::= ivars WORD */
#line 126 "smalltalk.y"
{
	/* First element is the list of instance variables. */
	[[yymsp[-1].minor.yy0 objectAtIndex:0] addObject:yymsp[0].minor.yy0];
	yygotominor.yy0 = yymsp[-1].minor.yy0;
}
#line 1026 "smalltalk.c"
        break;
      case 21: /* ivars ::= ivars PLUS WORD */
#line 132 "smalltalk.y"
{
	/* Second element is the list of class variables. */
	[[yymsp[-2].minor.yy0 objectAtIndex:1] addObject:yymsp[0].minor.yy0];
	yygotominor.yy0 = yymsp[-2].minor.yy0;
}
#line 1035 "smalltalk.c"
        break;
      case 22: /* ivars ::= */
#line 138 "smalltalk.y"
{
	/*
	Separate lists need to be built for instance and class variables.
	Put them in a fixed size array since we can only pass one value.
	*/
	yygotominor.yy0 = [NSArray arrayWithObjects: [NSMutableArray array],
	                               [NSMutableArray array],
	                               nil];
}
#line 1048 "smalltalk.c"
        break;
      case 23: /* method_list ::= method_list method */
      case 34: /* locals ::= locals WORD */ yytestcase(yyruleno==34);
      case 37: /* statement_list ::= statements statement */ yytestcase(yyruleno==37);
      case 39: /* statements ::= statements comment */ yytestcase(yyruleno==39);
      case 81: /* arguments ::= arguments argument */ yytestcase(yyruleno==81);
      case 84: /* expression_list ::= expressions expression */ yytestcase(yyruleno==84);
#line 149 "smalltalk.y"
{
	[yymsp[-1].minor.yy0 addObject:yymsp[0].minor.yy0];
	yygotominor.yy0 = yymsp[-1].minor.yy0;
}
#line 1061 "smalltalk.c"
        break;
      case 25: /* method_list ::= */
      case 35: /* locals ::= */ yytestcase(yyruleno==35);
      case 40: /* statements ::= */ yytestcase(yyruleno==40);
      case 82: /* arguments ::= */ yytestcase(yyruleno==82);
      case 86: /* expressions ::= */ yytestcase(yyruleno==86);
#line 158 "smalltalk.y"
{
	yygotominor.yy0 = [NSMutableArray array];
}
#line 1072 "smalltalk.c"
        break;
      case 26: /* method ::= signature LSQBRACK local_list statement_list RSQBRACK */
#line 163 "smalltalk.y"
{
	yygotominor.yy0 = [LKInstanceMethod methodWithSignature:yymsp[-4].minor.yy0 locals:yymsp[-2].minor.yy0 statements:yymsp[-1].minor.yy0];
}
#line 1079 "smalltalk.c"
        break;
      case 27: /* method ::= PLUS signature LSQBRACK local_list statement_list RSQBRACK */
#line 167 "smalltalk.y"
{
	yygotominor.yy0 = [LKClassMethod methodWithSignature:yymsp[-4].minor.yy0 locals:yymsp[-2].minor.yy0 statements:yymsp[-1].minor.yy0];
}
#line 1086 "smalltalk.c"
        break;
      case 28: /* signature ::= WORD */
      case 49: /* simple_message ::= WORD */ yytestcase(yyruleno==49);
#line 172 "smalltalk.y"
{
	yygotominor.yy0 = [LKMessageSend messageWithSelectorName:yymsp[0].minor.yy0];
}
#line 1094 "smalltalk.c"
        break;
      case 29: /* signature ::= keyword_signature */
      case 36: /* statement_list ::= statements */ yytestcase(yyruleno==36);
      case 42: /* statement ::= expression */ yytestcase(yyruleno==42);
      case 45: /* message ::= keyword_message */ yytestcase(yyruleno==45);
      case 46: /* message ::= simple_message */ yytestcase(yyruleno==46);
      case 61: /* expression ::= cascade_expression */ yytestcase(yyruleno==61);
      case 62: /* expression ::= keyword_expression */ yytestcase(yyruleno==62);
      case 63: /* expression ::= simple_expression */ yytestcase(yyruleno==63);
      case 78: /* argument ::= COLON WORD */ yytestcase(yyruleno==78);
      case 83: /* expression_list ::= expressions */ yytestcase(yyruleno==83);
#line 176 "smalltalk.y"
{
	yygotominor.yy0 = yymsp[0].minor.yy0;
}
#line 1110 "smalltalk.c"
        break;
      case 30: /* keyword_signature ::= keyword_signature KEYWORD WORD */
      case 47: /* keyword_message ::= keyword_message KEYWORD simple_expression */ yytestcase(yyruleno==47);
#line 180 "smalltalk.y"
{
	yygotominor.yy0 = yymsp[-2].minor.yy0;
	[yygotominor.yy0 addSelectorComponent:yymsp[-1].minor.yy0];
	[yygotominor.yy0 addArgument:yymsp[0].minor.yy0];
}
#line 1120 "smalltalk.c"
        break;
      case 31: /* keyword_signature ::= KEYWORD WORD */
#line 186 "smalltalk.y"
{ 
	yygotominor.yy0 = [LKMessageSend messageWithSelectorName:yymsp[-1].minor.yy0];
	[yygotominor.yy0 addArgument:yymsp[0].minor.yy0];
}
#line 1128 "smalltalk.c"
        break;
      case 38: /* statements ::= statements statement STOP */
      case 85: /* expressions ::= expressions expression STOP */ yytestcase(yyruleno==85);
#line 218 "smalltalk.y"
{
	[yymsp[-2].minor.yy0 addObject:yymsp[-1].minor.yy0];
	yygotominor.yy0 = yymsp[-2].minor.yy0;
}
#line 1137 "smalltalk.c"
        break;
      case 41: /* comment ::= COMMENT */
#line 233 "smalltalk.y"
{
	yygotominor.yy0 = [LKComment commentWithString:yymsp[0].minor.yy0];
}
#line 1144 "smalltalk.c"
        break;
      case 43: /* statement ::= RETURN expression */
#line 242 "smalltalk.y"
{
	yygotominor.yy0 = [LKReturn returnWithExpr:yymsp[0].minor.yy0];
}
#line 1151 "smalltalk.c"
        break;
      case 44: /* statement ::= WORD COLON EQ expression */
#line 246 "smalltalk.y"
{
	yygotominor.yy0 = [LKAssignExpr assignWithTarget: RefForSymbol(yymsp[-3].minor.yy0)
	                              expr: yymsp[0].minor.yy0];
}
#line 1159 "smalltalk.c"
        break;
      case 48: /* keyword_message ::= KEYWORD simple_expression */
      case 50: /* simple_message ::= binary_selector simple_expression */ yytestcase(yyruleno==50);
#line 272 "smalltalk.y"
{
	yygotominor.yy0 = [LKMessageSend messageWithSelectorName:yymsp[-1].minor.yy0];
	[yygotominor.yy0 addArgument:yymsp[0].minor.yy0];
}
#line 1168 "smalltalk.c"
        break;
      case 51: /* binary_selector ::= PLUS */
#line 288 "smalltalk.y"
{
	yygotominor.yy0 = @"plus:";
}
#line 1175 "smalltalk.c"
        break;
      case 52: /* binary_selector ::= MINUS */
#line 292 "smalltalk.y"
{
	yygotominor.yy0 = @"sub:";
}
#line 1182 "smalltalk.c"
        break;
      case 53: /* binary_selector ::= STAR */
#line 296 "smalltalk.y"
{
	yygotominor.yy0 = @"mul:";
}
#line 1189 "smalltalk.c"
        break;
      case 54: /* binary_selector ::= SLASH */
#line 300 "smalltalk.y"
{
	yygotominor.yy0 = @"div:";
}
#line 1196 "smalltalk.c"
        break;
      case 55: /* binary_selector ::= EQ */
#line 304 "smalltalk.y"
{
	yygotominor.yy0 = @"isEqual:";
}
#line 1203 "smalltalk.c"
        break;
      case 56: /* binary_selector ::= LT */
#line 308 "smalltalk.y"
{
	yygotominor.yy0 = @"isLessThan:";
}
#line 1210 "smalltalk.c"
        break;
      case 57: /* binary_selector ::= GT */
#line 312 "smalltalk.y"
{
	yygotominor.yy0 = @"isGreaterThan:";
}
#line 1217 "smalltalk.c"
        break;
      case 58: /* binary_selector ::= LT EQ */
#line 316 "smalltalk.y"
{
	yygotominor.yy0 = @"isLessThanOrEqualTo:";
}
#line 1224 "smalltalk.c"
        break;
      case 59: /* binary_selector ::= GT EQ */
#line 320 "smalltalk.y"
{
	yygotominor.yy0 = @"isGreaterThanOrEqualTo:";
}
#line 1231 "smalltalk.c"
        break;
      case 60: /* binary_selector ::= COMMA */
#line 324 "smalltalk.y"
{
	yygotominor.yy0 = @"comma:"; // concatenates strings or arrays (see Support/NSString+comma.m)
}
#line 1238 "smalltalk.c"
        break;
      case 64: /* cascade_expression ::= cascade_expression SEMICOLON message */
#line 342 "smalltalk.y"
{
	[yymsp[-2].minor.yy0 addMessage:yymsp[0].minor.yy0];
	yygotominor.yy0 = yymsp[-2].minor.yy0;
}
#line 1246 "smalltalk.c"
        break;
      case 65: /* cascade_expression ::= simple_expression message SEMICOLON message */
#line 347 "smalltalk.y"
{
	yygotominor.yy0 = [LKMessageCascade messageCascadeWithTarget:yymsp[-3].minor.yy0 messages:
		[NSMutableArray arrayWithObjects:yymsp[-2].minor.yy0, yymsp[0].minor.yy0, nil]];
}
#line 1254 "smalltalk.c"
        break;
      case 66: /* keyword_expression ::= simple_expression keyword_message */
#line 353 "smalltalk.y"
{
	if ([yymsp[-1].minor.yy0 isKindOfClass: [LKDeclRef class]] &&
		[@"C" isEqualToString: [yymsp[-1].minor.yy0 symbol]])
	{
		NSString *selector = [(LKMessageSend*)yymsp[0].minor.yy0 selector];
		NSArray *args = [yymsp[0].minor.yy0 arguments];
		if ([@"enumValue:" isEqualToString: selector])
		{
			yygotominor.yy0 = [[LKEnumReference alloc] initWithValue: [[args objectAtIndex: 0] symbol]
			                             inEnumeration: nil];
		}
		else if ([@"enum:value:" isEqualToString: selector])
		{
			yygotominor.yy0 = [[LKEnumReference alloc] initWithValue: [[args objectAtIndex: 0] symbol]
			                             inEnumeration: [[args objectAtIndex: 1] symbol]];
		}
		else
		{
			LKFunctionCall *call= [[LKFunctionCall new] autorelease];
			[call setFunctionName: [selector stringByReplacingOccurrencesOfString: @":" withString: @""]];
			if ([args count] == 1 && [[args objectAtIndex: 0] isKindOfClass: [LKArrayExpr class]])
			{
				[call setArguments: [(LKArrayExpr*)[args objectAtIndex: 0] elements]];
			}
			else
			{
				[call setArguments: [args mutableCopy]];
			}
			yygotominor.yy0 = call;
		}
	}
	else
	{
		[yymsp[0].minor.yy0 setTarget:yymsp[-1].minor.yy0];
		yygotominor.yy0 = yymsp[0].minor.yy0;
	}
}
#line 1295 "smalltalk.c"
        break;
      case 67: /* simple_expression ::= WORD */
#line 392 "smalltalk.y"
{
	if ([yymsp[0].minor.yy0 isEqualToString:@"true"])
	{
		yygotominor.yy0 = [LKNumberLiteral literalFromString:@"1"];
	}
	else if ([yymsp[0].minor.yy0 isEqualToString:@"false"])
	{
		yygotominor.yy0 = [LKNumberLiteral literalFromString:@"0"];
	}
	else
	{
		yygotominor.yy0 = RefForSymbol(yymsp[0].minor.yy0);
	}
}
#line 1313 "smalltalk.c"
        break;
      case 68: /* simple_expression ::= SYMBOL */
#line 407 "smalltalk.y"
{
	yygotominor.yy0 = [LKSymbolRef referenceWithSymbol: yymsp[0].minor.yy0];
}
#line 1320 "smalltalk.c"
        break;
      case 69: /* simple_expression ::= STRING */
#line 411 "smalltalk.y"
{
	yygotominor.yy0 = [LKStringLiteral literalFromString:yymsp[0].minor.yy0];
}
#line 1327 "smalltalk.c"
        break;
      case 70: /* simple_expression ::= NUMBER */
#line 415 "smalltalk.y"
{
	yygotominor.yy0 = [LKNumberLiteral literalFromString:yymsp[0].minor.yy0];
}
#line 1334 "smalltalk.c"
        break;
      case 71: /* simple_expression ::= FLOATNUMBER */
#line 419 "smalltalk.y"
{
	yygotominor.yy0 = [LKFloatLiteral literalFromString:yymsp[0].minor.yy0];
}
#line 1341 "smalltalk.c"
        break;
      case 72: /* simple_expression ::= AT WORD */
#line 423 "smalltalk.y"
{
	yygotominor.yy0 = [LKNumberLiteral literalFromSymbol:yymsp[0].minor.yy0];
}
#line 1348 "smalltalk.c"
        break;
      case 73: /* simple_expression ::= simple_expression simple_message */
#line 427 "smalltalk.y"
{
	[yymsp[0].minor.yy0 setTarget:yymsp[-1].minor.yy0];
	yygotominor.yy0 = yymsp[0].minor.yy0;
}
#line 1356 "smalltalk.c"
        break;
      case 74: /* simple_expression ::= simple_expression EQ EQ simple_expression */
#line 432 "smalltalk.y"
{
	yygotominor.yy0 = [LKCompare comparisonWithLeftExpression: yymsp[-3].minor.yy0
	                            rightExpression: yymsp[0].minor.yy0];
}
#line 1364 "smalltalk.c"
        break;
      case 75: /* simple_expression ::= LPAREN expression RPAREN */
#line 437 "smalltalk.y"
{
	[yymsp[-1].minor.yy0 setBracketed:YES];
	yygotominor.yy0 = yymsp[-1].minor.yy0;
}
#line 1372 "smalltalk.c"
        break;
      case 76: /* simple_expression ::= LBRACE expression_list RBRACE */
#line 442 "smalltalk.y"
{
	yygotominor.yy0 = [LKArrayExpr arrayWithElements:yymsp[-1].minor.yy0];
}
#line 1379 "smalltalk.c"
        break;
      case 77: /* simple_expression ::= LSQBRACK argument_list local_list statement_list RSQBRACK */
#line 446 "smalltalk.y"
{
	//FIXME: block locals
	yygotominor.yy0 = [LKBlockExpr blockWithArguments: yymsp[-3].minor.yy0
	                             locals: yymsp[-2].minor.yy0
	                         statements: yymsp[-1].minor.yy0];
/*
	yygotominor.yy0 = [LKBlockExpr blockWithArguments: [yymsp[-3].minor.yy0 objectAtIndex: 0]
	                             locals: [yymsp[-3].minor.yy0 objectAtIndex: 1]
	                         statements: yymsp[-1].minor.yy0];
*/
}
#line 1394 "smalltalk.c"
        break;
      case 79: /* argument_list ::= argument arguments BAR */
#line 474 "smalltalk.y"
{
	[yymsp[-1].minor.yy0 insertObject: yymsp[-2].minor.yy0 atIndex: 0];
	yygotominor.yy0 = yymsp[-1].minor.yy0;
}
#line 1402 "smalltalk.c"
        break;
      default:
      /* (2) file ::= */ yytestcase(yyruleno==2);
      /* (19) ivar_list ::= */ yytestcase(yyruleno==19);
      /* (33) local_list ::= */ yytestcase(yyruleno==33);
      /* (80) argument_list ::= */ yytestcase(yyruleno==80);
        break;
  };
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  yypParser->yyidx -= yysize;
  yyact = yy_find_reduce_action(yymsp[-yysize].stateno,(YYCODETYPE)yygoto);
  if( yyact < YYNSTATE ){
#ifdef NDEBUG
    /* If we are not debugging and the reduce action popped at least
    ** one element off the stack, then we can push the new element back
    ** onto the stack here, and skip the stack overflow test in yy_shift().
    ** That gives a significant speed improvement. */
    if( yysize ){
      yypParser->yyidx++;
      yymsp -= yysize-1;
      yymsp->stateno = (YYACTIONTYPE)yyact;
      yymsp->major = (YYCODETYPE)yygoto;
      yymsp->minor = yygotominor;
    }else
#endif
    {
      yy_shift(yypParser,yyact,yygoto,&yygotominor);
    }
  }else{
    assert( yyact == YYNSTATE + YYNRULE + 1 );
    yy_accept(yypParser);
  }
}

/*
** The following code executes when the parse fails
*/
#ifndef YYNOERRORRECOVERY
static void yy_parse_failed(
  yyParser *yypParser           /* The parser */
){
  SmalltalkParseARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
  SmalltalkParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}
#endif /* YYNOERRORRECOVERY */

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
  yyParser *yypParser,           /* The parser */
  int yymajor,                   /* The major type of the error token */
  YYMINORTYPE yyminor            /* The minor type of the error token */
){
  SmalltalkParseARG_FETCH;
#define TOKEN (yyminor.yy0)
#line 252 "smalltalk.y"

	[NSException raise:@"ParserError" format:@"Parsing failed"];
#line 1471 "smalltalk.c"
  SmalltalkParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  yyParser *yypParser           /* The parser */
){
  SmalltalkParseARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
  SmalltalkParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "SmalltalkParseAlloc" which describes the current state of the parser.
** The second argument is the major token number.  The third is
** the minor token.  The fourth optional argument is whatever the
** user wants (and specified in the grammar) and is available for
** use by the action routines.
**
** Inputs:
** <ul>
** <li> A pointer to the parser (an opaque structure.)
** <li> The major token number.
** <li> The minor token number.
** <li> An option argument of a grammar-specified type.
** </ul>
**
** Outputs:
** None.
*/
void SmalltalkParse(
					void *yyp,                   /* The parser */
					int yymajor,                 /* The major token code number */
					SmalltalkParseTOKENTYPE yyminor       /* The value for the token */
					SmalltalkParseARG_PDECL               /* Optional %extra_argument parameter */
					);
void SmalltalkParse(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  SmalltalkParseTOKENTYPE yyminor       /* The value for the token */
  SmalltalkParseARG_PDECL               /* Optional %extra_argument parameter */
){
  YYMINORTYPE yyminorunion;
  int yyact;            /* The parser action. */
  int yyendofinput;     /* True if we are at the end of input */
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif
  yyParser *yypParser;  /* The parser */

  /* (re)initialize the parser, if necessary */
  yypParser = (yyParser*)yyp;
  if( yypParser->yyidx<0 ){
#if YYSTACKDEPTH<=0
    if( yypParser->yystksz <=0 ){
      /*memset(&yyminorunion, 0, sizeof(yyminorunion));*/
      yyminorunion = yyzerominor;
      yyStackOverflow(yypParser, &yyminorunion);
      return;
    }
#endif
    yypParser->yyidx = 0;
    yypParser->yyerrcnt = -1;
    yypParser->yystack[0].stateno = 0;
    yypParser->yystack[0].major = 0;
  }
  yyminorunion.yy0 = yyminor;
  yyendofinput = (yymajor==0);
  SmalltalkParseARG_STORE;

#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sInput %s\n",yyTracePrompt,yyTokenName[yymajor]);
  }
#endif

  do{
    yyact = yy_find_shift_action(yypParser,(YYCODETYPE)yymajor);
    if( yyact<YYNSTATE ){
      assert( !yyendofinput );  /* Impossible to shift the $ token */
      yy_shift(yypParser,yyact,yymajor,&yyminorunion);
      yypParser->yyerrcnt--;
      yymajor = YYNOCODE;
    }else if( yyact < YYNSTATE + YYNRULE ){
      yy_reduce(yypParser,yyact-YYNSTATE);
    }else{
      assert( yyact == YY_ERROR_ACTION );
#ifdef YYERRORSYMBOL
      int yymx;
#endif
#ifndef NDEBUG
      if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sSyntax Error!\n",yyTracePrompt);
      }
#endif
#ifdef YYERRORSYMBOL
      /* A syntax error has occurred.
      ** The response to an error depends upon whether or not the
      ** grammar defines an error token "ERROR".  
      **
      ** This is what we do if the grammar does define ERROR:
      **
      **  * Call the %syntax_error function.
      **
      **  * Begin popping the stack until we enter a state where
      **    it is legal to shift the error symbol, then shift
      **    the error symbol.
      **
      **  * Set the error count to three.
      **
      **  * Begin accepting and shifting new tokens.  No new error
      **    processing will occur until three tokens have been
      **    shifted successfully.
      **
      */
      if( yypParser->yyerrcnt<0 ){
        yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yymx = yypParser->yystack[yypParser->yyidx].major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        yy_destructor(yypParser, (YYCODETYPE)yymajor,&yyminorunion);
        yymajor = YYNOCODE;
      }else{
         while(
          yypParser->yyidx >= 0 &&
          yymx != YYERRORSYMBOL &&
          (yyact = yy_find_reduce_action(
                        yypParser->yystack[yypParser->yyidx].stateno,
                        YYERRORSYMBOL)) >= YYNSTATE
        ){
          yy_pop_parser_stack(yypParser);
        }
        if( yypParser->yyidx < 0 || yymajor==0 ){
          yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
          yy_parse_failed(yypParser);
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          YYMINORTYPE u2;
          u2.YYERRSYMDT = 0;
          yy_shift(yypParser,yyact,YYERRORSYMBOL,&u2);
        }
      }
      yypParser->yyerrcnt = 3;
      yyerrorhit = 1;
#elif defined(YYNOERRORRECOVERY)
      /* If the YYNOERRORRECOVERY macro is defined, then do not attempt to
      ** do any kind of error recovery.  Instead, simply invoke the syntax
      ** error routine and continue going as if nothing had happened.
      **
      ** Applications can set this macro (for example inside %include) if
      ** they intend to abandon the parse upon the first syntax error seen.
      */
      yy_syntax_error(yypParser,yymajor,yyminorunion);
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      yymajor = YYNOCODE;
      
#else  /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if( yypParser->yyerrcnt<=0 ){
        yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yypParser->yyerrcnt = 3;
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      if( yyendofinput ){
        yy_parse_failed(yypParser);
      }
      yymajor = YYNOCODE;
#endif
    }
  }while( yymajor!=YYNOCODE && yypParser->yyidx>=0 );
  return;
}
