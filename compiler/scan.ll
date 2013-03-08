/* -*-fundamental-*- */
/*
 *
 * Copyright (C) 2005 Max Krohn (my last name AT mit DOT edu)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */

%{
#define YYSTYPE YYSTYPE
#include "tame.hh"
#include "parse.hh"

#define YY_NO_UNPUT
#define YY_SKIP_YYWRAP
#define yywrap() 1

str filename = "(stdin)";
int lineno = 1;
static void switch_to_state (int i);
static int std_ret (int i);
static int tame_ret (int s, int t);
int get_yy_lineno () { return lineno ;}
str get_yy_loc ();
int tame_on = 1;
int lineno_return ();
int loc_return ();
int filename_return ();
%}

%option stack

ID	[a-zA-Z_][a-zA-Z_0-9]*
WSPACE	[ \t\v\f]
SYM	[{}<>;,():*\[\]]
DNUM 	[+-]?[0-9]+
XNUM 	[+-]?0x[0-9a-fA-F]

%x FULL_PARSE FN_ENTER VARS_ENTER 
%x TAME_BASE C_COMMENT CXX_COMMENT TAME
%x ID_OR_NUM NUM_ONLY HALF_PARSE PP PP_BASE PP_EQUALS BB
%x JOIN_LIST JOIN_LIST_BASE
%x TWAIT_ENTER TWAIT_BODY TWAIT_BODY_BASE
%x EXPR_LIST EXPR_LIST_BASE ID_LIST RETURN_PARAMS
%x EXPR_LIST_BR EXPR_LIST_BR_BASE
%x DEFRET_ENTER DEFRET_BASE DEFRET
%x TEMPLATE_ENTER TEMPLATE TEMPLATE_BASE
%x SIG_PARSE QUOTE

%%

<FN_ENTER,FULL_PARSE,SIG_PARSE,VARS_ENTER,ID_LIST,ID_OR_NUM,NUM_ONLY,HALF_PARSE,TWAIT_ENTER,JOIN_LIST,JOIN_LIST_BASE,EXPR_LIST,EXPR_LIST_BASE,DEFRET_ENTER>{
\n|\r\n|\r	++lineno;
{WSPACE}+	/*discard*/;
}

<ID_OR_NUM>{
{ID} 		{ yy_pop_state (); return std_ret (T_ID); }
{DNUM}|{XNUM}	{ yy_pop_state (); return std_ret (T_NUM); }
.		{ return yyerror ("expected an identifier or a number"); }
}

<NUM_ONLY>
{
{DNUM}|{XNUM}	{ yy_pop_state (); return std_ret (T_NUM); }
.		{ return yyerror ("expected a number"); }
}

<FULL_PARSE,HALF_PARSE,SIG_PARSE>{

const		return T_CONST;
volatile	return T_VOLATILE;
struct		return T_STRUCT;
typename	return T_TYPENAME;
void		return T_VOID;
char		return T_CHAR;
short		return T_SHORT;
int		return T_INT;
long		return T_LONG;
float		return T_FLOAT;
double		return T_DOUBLE;
signed		return T_SIGNED;
unsigned	return T_UNSIGNED;
static		return T_STATIC;
inline		return T_INLINE;
virtual		return T_VIRTUAL;
template	{ yy_push_state (TEMPLATE_ENTER); return T_TEMPLATE; }

{ID} 		{ return std_ret (T_ID); }
{DNUM}|{XNUM}	{ return std_ret (T_NUM); }

[}]		{ yy_pop_state (); return yytext[0]; }

[<>,:*&]	{ return yytext[0]; }
"::"		{ return T_2COLON; }
}

<FULL_PARSE,HALF_PARSE>{
[)]		{ yy_pop_state (); return yytext[0]; }
[;]		{ return yytext[0]; }
}

<SIG_PARSE>{
[()]		{ return yytext[0]; }
[[]		{ yy_push_state(PP_BASE); return yytext[0]; }
[{]		{ switch_to_state(TAME_BASE); return yytext[0]; }
[;]		{ switch_to_state(INITIAL); return yytext[0]; }
}

<TEMPLATE_ENTER>{
\n|\r\n|\r	++lineno;
{WSPACE}+	/* discard */ ;
"<"		{ switch_to_state (TEMPLATE_BASE); return yytext[0]; }
.		{ return yyerror ("unexpected token after 'template'"); }
}

<TEMPLATE_BASE>{
[>]		{ yy_pop_state (); return yytext[0]; }
}

<TEMPLATE_BASE,TEMPLATE>{
\n		{ ++lineno; return std_ret (T_PASSTHROUGH); }
[<]		{ yy_push_state (TEMPLATE); return std_ret (T_PASSTHROUGH); }
[^<>\n]+	{ return std_ret (T_PASSTHROUGH); }
}

<TEMPLATE>{
[>]		{ yy_pop_state (); return std_ret (T_PASSTHROUGH); }
}

<HALF_PARSE>{
[([]		{ yy_push_state(PP_BASE); return yytext[0]; }
[=]		{ yy_push_state(PP_EQUALS); return yytext[0]; }
}

<FULL_PARSE>{
[({]		{ yy_push_state(FULL_PARSE); return yytext[0]; }
}

<FULL_PARSE,HALF_PARSE,SIG_PARSE>{
.		{ return yyerror ("illegal token found in parsed "
				  "environment"); }
}

<FN_ENTER>{
[(]		{ yy_push_state (FULL_PARSE); return yytext[0]; }
[{]		{ switch_to_state (TAME_BASE); return yytext[0]; }
.		{ return yyerror ("illegal token found in function "
				  "environment"); }
}

<PP_EQUALS>{
[,;]		{ yy_pop_state(); return yytext[0]; }
[)\]]		{ return std_ret(T_PASSTHROUGH); }
}

<PP_BASE>{
[)\]]		{ yy_pop_state(); return yytext[0]; } 
[;]		{ yy_pop_state(); return yytext[0]; }
}

<BB>{
[\]]		{ yy_pop_state(); return std_ret(T_PASSTHROUGH); }
}

<PP>{
[)]		{ yy_pop_state(); return std_ret(T_PASSTHROUGH); }
}

<PP,PP_BASE,PP_EQUALS,BB>{
\n			{ ++lineno; }
[(]			{ yy_push_state(PP); return std_ret(T_PASSTHROUGH); }
[[]			{ yy_push_state(BB); return std_ret(T_PASSTHROUGH); }
}

<PP,PP_BASE,BB>{
[^()[\]\n/_]+|[/_]	{ return std_ret(T_PASSTHROUGH); }
}

<PP_EQUALS>{
[^()[\]\n/_,;]+|[/_]	{ return std_ret(T_PASSTHROUGH); }
}

<PP,PP_BASE,PP_EQUALS,BB,TAME,TAME_BASE>{
__LINE__	{ return lineno_return (); }
__FILE__        { return filename_return (); }
__LOC__         { return loc_return (); }
}


<VARS_ENTER>{
[{]		{ switch_to_state(HALF_PARSE); return yytext[0]; }
.		{ return yyerror("illegal token found between 'tvars' and '{'");}
}


<TWAIT_ENTER>{
volatile	{ return T_VOLATILE; }
[(]		{ yy_push_state(JOIN_LIST_BASE); return yytext[0]; }
[{]		{ switch_to_state(TWAIT_BODY_BASE); return yytext[0]; }
[;]		{ yy_pop_state(); return yytext[0]; }
.		{ return yyerror("illegal token found after twait"); }

}

<JOIN_LIST_BASE,JOIN_LIST>{
[(]		{ yy_push_state (JOIN_LIST); return std_ret (T_PASSTHROUGH); }
[^(),/\n]+|"/"	{ return std_ret (T_PASSTHROUGH); }
}

<JOIN_LIST>{
[)]		{ yy_pop_state (); return std_ret (T_PASSTHROUGH); }
[,]		{ return std_ret (T_PASSTHROUGH); }
}

<JOIN_LIST_BASE>{
[,]		{ switch_to_state (ID_LIST); return yytext[0]; }
[)]		{ yy_pop_state (); return yytext[0]; }
}

<DEFRET_ENTER>{
\{		{ switch_to_state (DEFRET_BASE); return yytext[0]; }
.		{ return yyerror ("Expected '{' after DEFAULT_RETURN"); }
}

<DEFRET_BASE>{
\}		{ yy_pop_state (); return yytext[0]; }
\{		{ yy_push_state (DEFRET); return std_ret (T_PASSTHROUGH); }
}

<DEFRET>{
\{		{ yy_push_state (DEFRET); return std_ret (T_PASSTHROUGH); }
\}		{ yy_pop_state (); return std_ret (T_PASSTHROUGH); }
}

<DEFRET_BASE,DEFRET>{
\n		{ ++lineno; return std_ret (T_PASSTHROUGH); }
[^{}\n]+	{ return std_ret (T_PASSTHROUGH); }
}

<EXPR_LIST_BR_BASE>{
\]		{ yy_pop_state (); return yytext[0]; }
[,]		{ return yytext[0]; }
}

<EXPR_LIST_BR>{
\]		{ yy_pop_state (); return std_ret (T_PASSTHROUGH); }
[,]		{ return std_ret (T_PASSTHROUGH); }
}

<EXPR_LIST_BR_BASE,EXPR_LIST_BR>{
\[		   { yy_push_state (EXPR_LIST_BR); 
	             return std_ret (T_PASSTHROUGH); }
[^,\[\]/\n]+|"/"   { return std_ret (T_PASSTHROUGH); }
\n		   { ++lineno; return std_ret (T_PASSTHROUGH); }
}

<ID_LIST>{
{ID}		{ return std_ret (T_ID); }
[,]		{ return yytext[0]; }
[)]		{ yy_pop_state (); return yytext[0]; }
}

<EXPR_LIST_BASE,EXPR_LIST>{
[(]		{ yy_push_state (EXPR_LIST); return std_ret (T_PASSTHROUGH); }
[^(),/\n]+|"/"	{ return std_ret (T_PASSTHROUGH); }
}

<EXPR_LIST>{
[,]		{ return std_ret (T_PASSTHROUGH); }
[)]		{ yy_pop_state (); return std_ret (T_PASSTHROUGH); }
}

<EXPR_LIST_BASE>{
[)]		{ yy_pop_state (); return yytext[0]; }
[,]		{ return yytext[0]; }
}

<TWAIT_BODY_BASE,TWAIT_BODY>{
\n			{ ++lineno; return std_ret (T_PASSTHROUGH); }
[^ "gr\t{}\n/]+|[ \tgr/]	{ return std_ret (T_PASSTHROUGH); }
[{]			{ yy_push_state (TWAIT_BODY); 
			  return std_ret (T_PASSTHROUGH); }
goto/[ \t\n]		{ return yyerror ("cannot goto within twait{..}"); }
return/[ \t\n(;]	{ return yyerror ("cannot return withint twait{..}"); }
\"			{ yy_push_state(QUOTE);
			  return std_ret(T_PASSTHROUGH); }
}

<TWAIT_BODY_BASE>{
[}]		{ yy_pop_state (); return yytext[0]; }
}

<TWAIT_BODY>{
[}]		{ yy_pop_state (); return std_ret (T_PASSTHROUGH); }
}

<TAME,TAME_BASE>{
\n		{ yylval.str = lstr(lineno, yytext); ++lineno; return T_PASSTHROUGH; }

[^ \t{}"\n/trD_]+|[ \t/trD_] { yylval.str = lstr(lineno, yytext); return T_PASSTHROUGH; }

[{]		{ yylval.str = lstr(lineno, yytext); yy_push_state (TAME); 
		  return T_PASSTHROUGH; }

tvars/[ \t\n{/]	    { return tame_ret(VARS_ENTER, T_TVARS); }
DEFAULT_RETURN	    { return tame_ret(DEFRET_ENTER, T_DEFAULT_RETURN); }

return/[ \t\n(/;]   { yy_push_state(RETURN_PARAMS); return T_RETURN; }

\"		    { yy_push_state(QUOTE); return std_ret (T_PASSTHROUGH); }
}

<TAME,TAME_BASE,INITIAL>{
twait/[ \t\n({/]           { return tame_ret (TWAIT_ENTER, T_TWAIT); }
}

<QUOTE>{
[^\\"]+|\\\"	    { return std_ret (T_PASSTHROUGH); }
\\		    { return std_ret (T_PASSTHROUGH); }
\"		    { yy_pop_state (); return std_ret (T_PASSTHROUGH); }
}

<TAME>{
[}]		{ yylval.str = lstr(lineno, yytext); yy_pop_state ();
	    	  return T_PASSTHROUGH; }
}

<TAME_BASE>{
[}]		{ yy_pop_state (); return yytext[0]; }
}


<TAME,TAME_BASE,INITIAL>{
"//"		{ yy_push_state(CXX_COMMENT); return std_ret(T_COMMENT); }
"/*"		{ yy_push_state(C_COMMENT); return std_ret(T_COMMENT); }
}

<INITIAL>{
tamed/[ \t\n/]   { return tame_ret(SIG_PARSE, T_TAMED); }
[^t\n"/]+|[t/]   { yylval.str = lstr(lineno, yytext); return T_PASSTHROUGH ; }
\n		 { ++lineno; yylval.str = lstr(lineno, yytext); return T_PASSTHROUGH; }
\"		 { yy_push_state (QUOTE); return std_ret (T_PASSTHROUGH); }
}

<CXX_COMMENT>{
\n		{ ++lineno; yy_pop_state(); return std_ret(T_COMMENT); }
[^T\n]+|[T]	{ return std_ret(T_COMMENT); }
TAME_OFF	{ tame_on = 0; return std_ret(T_COMMENT); }
TAME_ON		{ tame_on = 1; return std_ret(T_COMMENT); }
}

<RETURN_PARAMS>{
\n			{ ++lineno; return std_ret (T_PASSTHROUGH); }
;			{ yy_pop_state (); return yytext[0]; }
[^\n;/]+|[/]		{ return std_ret (T_PASSTHROUGH); }
}

<C_COMMENT>{
TAME_OFF	{ tame_on = 0; return std_ret(T_COMMENT); }
TAME_ON		{ tame_on = 1; return std_ret(T_COMMENT); }
"*/"		{ yy_pop_state (); return std_ret(T_COMMENT); }
[^*\nT]+|[*T]	{ return std_ret(T_COMMENT); }
\n		{ ++lineno; return std_ret(T_COMMENT); }
}


<FULL_PARSE,SIG_PARSE,FN_ENTER,VARS_ENTER,HALF_PARSE,PP,PP_BASE,PP_EQUALS,BB,EXPR_LIST,EXPR_LIST_BASE,ID_LIST,RETURN_PARAMS,EXPR_LIST_BR,EXPR_LIST_BR_BASE,DEFRET_ENTER,TWAIT_BODY,TWAIT_BODY_BASE>{

"//"		{ yy_push_state(CXX_COMMENT); return std_ret(T_COMMENT); }
"/*"		{ yy_push_state(C_COMMENT); return std_ret(T_COMMENT); }

}

%%

void
switch_to_state (int s)
{
	yy_pop_state ();
	yy_push_state (s);
}

int
yyerror (str msg)
{
  warn << filename << ":" << lineno << ": " << msg << "\n";
  if (outputter)
    outputter->fail_exit();
  else
    exit(1);
}

int
yywarn (str msg)
{
  warn << filename << ":" << lineno << ": Warning: " << msg << "\n";
  return 0;
}

int
std_ret (int i)
{
  yylval.str = lstr(lineno, yytext);
  return i;
}

void
gcc_hack_use_static_functions ()
{
  assert (false);
  (void )yy_top_state ();
}

int
tame_ret (int s, int t)
{
  if (tame_on) {
    yy_push_state (s);
    return t;
  } else {
    return std_ret (T_PASSTHROUGH);
  }
}

str
get_yy_loc ()
{
   strbuf b;
   b << filename << ":" << lineno;
   return b.str();
}

int
lineno_return ()
{
   strbuf b; 
   b << lineno; 
   yylval.str = lstr (lineno, b.str());
   return T_PASSTHROUGH;
}

int
filename_return ()
{
  strbuf b; 
  b << "\"" << filename << "\"";
  yylval.str = lstr (lineno, b.str());
  return T_PASSTHROUGH; 
}

int
loc_return ()
{
   strbuf b;
   b << "\"" << filename << ":" << lineno << "\"";
   yylval.str = lstr (lineno, b.str());
   return T_PASSTHROUGH; 
}
