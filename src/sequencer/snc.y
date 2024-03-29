/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
%{
/**************************************************************************
			GTA PROJECT   AT division
	Copyright, 1990, The Regents of the University of California.
		         Los Alamos National Laboratory
	snc.y,v 1.2 1995/06/27 15:26:07 wright Exp
	ENVIRONMENT: UNIX
	HISTORY:
20nov91,ajk	Added new "option" statement.
08nov93,ajk	Implemented additional declarations (see VC_CLASS in parse.h).
08nov93,ajk	Implemented assignment of array elements to pv's.
02may93,ajk	Removed "parameter" definition for functions, and added "%prec"
		qualifications to some "expr" definitions.
31may94,ajk	Changed method for handling global C code.
20jul95,ajk	Added "unsigned" types (see UNSIGNED token).
11jul96,ajk	Added character constants (CHAR_CONST).
13jan98,wfl	Added "down a level" handling of compound expressions
09jun98,wfl	Permitted pre-processor "#" lines between state-sets
***************************************************************************/
/*	SNC - State Notation Compiler.
 *	The general structure of a state program is:
 *		program-name
 *		declarations
 *		ss  { state { event { action ...} new-state } ... } ...
 *
 *	The following yacc definitions call the various parsing routines, which
 *	are coded in the file "parse.c".  Their major action is to build
 *	a structure for each SNL component (state, event, action, etc.) and
 *	build linked lists from these structures.  The linked lists have a
 *	hierarchical structure corresponding to the SNL block structure.
 *	For instance, a "state" structure is linked to a chain of "event",
 *	structures, which are, in turn, linked to a chain of "action"
 *	structures.
 *	The lexical analyser (see snc_lax.l) reads the input
 *	stream and passes tokens to the yacc-generated code.  The snc_lex
 *	and parsing routines may also pass data elements that take on one
 *	of the types defined under the %union definition.
 *
 */
#include	<stdio.h>
#include	<ctype.h>
#include	"parse.h"

#ifndef	TRUE
#define	TRUE	1
#define	FALSE	0
#endif /* TRUE */

extern	int line_num; /* input file line no. */
%}

%start	state_program

%union
{
	int	ival;
	char	*pchar;
	void	*pval;
	Expr	*pexpr;
}
%token	<pchar>	STATE STATE_SET
%token	<pchar>	NUMBER NAME
%token	<pchar>	CHAR_CONST
%token	<pchar>	DEBUG_PRINT
%token	PROGRAM EXIT OPTION
%token	R_SQ_BRACKET L_SQ_BRACKET
%token	BAD_CHAR L_BRACKET R_BRACKET
%token	COLON SEMI_COLON EQUAL
%token	L_PAREN R_PAREN PERIOD POINTER COMMA OR AND
%token	MONITOR ASSIGN TO WHEN
%token	UNSIGNED CHAR SHORT INT LONG FLOAT DOUBLE STRING_DECL
%token	EVFLAG SYNC
%token	ASTERISK AMPERSAND
%token	AUTO_INCR AUTO_DECR
%token	PLUS MINUS SLASH GT GE EQ LE LT NE NOT BIT_OR BIT_AND
%token	L_SHIFT R_SHIFT COMPLEMENT MODULO
%token	PLUS_EQUAL MINUS_EQUAL MULT_EQUAL DIV_EQUAL AND_EQUAL OR_EQUAL
%token	MODULO_EQUAL LEFT_EQUAL RIGHT_EQUAL CMPL_EQUAL
%token	<pchar>	STRING
%token	<pchar>	C_STMT
%token	IF ELSE WHILE FOR BREAK
%token	PP_SYMBOL CR
%type	<ival>	type
%type	<pchar>	subscript binop asgnop unop
%type	<pexpr> state_set_list state_set state_list state transition_list transition
%type	<pexpr> expr compound_expr assign_list bracked_expr
%type	<pexpr> statement stmt_list compound_stmt if_stmt else_stmt while_stmt
%type	<pexpr> for_stmt escaped_c_list
/* precidence rules for expr evaluation */
%right	EQUAL COMMA
%left	OR AND
%left	GT GE EQ NE LE LT
%left	PLUS MINUS
%left	ASTERISK SLASH
%left	NOT UOP	/* unary operators: e.g. -x */
%left	SUBSCRIPT

%%	/* Begin rules */

state_program 	/* define a state program */
:	program_name definitions state_set_list			{ program($3); }
|	program_name definitions state_set_list global_c	{ program($3); }
|	pp_code program_name definitions state_set_list		{ program($4); }
|	pp_code program_name definitions state_set_list global_c{ program($4); }
|	error { snc_err("state program"); }
;

program_name 	/* program name */
:	PROGRAM NAME { program_name($2, ""); }
|	PROGRAM NAME L_PAREN STRING R_PAREN { program_name($2, $4); }
;

definitions 	/* definitions block */
:	defn_stmt
|	definitions defn_stmt
;

defn_stmt	/* individual definitions for SNL (preceeds state sets) */
:	assign_stmt
|	monitor_stmt
|	decl_stmt
|	debug_stmt
|	sync_stmt
|	option_stmt
|	defn_c_stmt
|	pp_code
|	error { snc_err("definitions/declarations"); }
;

assign_stmt	/* assign <var name> to <db name>; */
:	ASSIGN NAME TO STRING SEMI_COLON { assign_single($2, $4); }

		/* assign <var name>[<n>] to <db name>; */
|	ASSIGN NAME subscript TO STRING SEMI_COLON { assign_subscr($2, $3, $5); }

		/* assign <var name> to {<db name>, ... }; */
|	ASSIGN NAME TO L_BRACKET assign_list R_BRACKET SEMI_COLON
			{ assign_list($2, $5); }
;

assign_list	/* {"<db name>", .... } */ 
:	STRING				{ $$ = expression(E_STRING, $1, 0, 0); }
|	assign_list COMMA STRING
			{ $$ = link_expr($1, expression(E_STRING, $3, 0, 0)); }
;

monitor_stmt	/* variable to be monitored; delta is optional */
:	MONITOR NAME SEMI_COLON			{ monitor_stmt($2, NULL); }
|	MONITOR NAME subscript SEMI_COLON	{ monitor_stmt($2, $3); }
;

subscript	/* e.g. [10] */
:	L_SQ_BRACKET NUMBER R_SQ_BRACKET	{ $$ = $2; }
;

debug_stmt
:	DEBUG_PRINT NUMBER SEMI_COLON		{ set_debug_print($2); }
;

decl_stmt	/* variable declarations (e.g. float x[20];)
		 * decl_stmt(type, class, name, <1-st dim>, <2-nd dim>, value) */
:	type NAME SEMI_COLON
			{ decl_stmt($1, VC_SIMPLE,  $2,  NULL, NULL, NULL); }

|	type NAME EQUAL NUMBER SEMI_COLON
			{ decl_stmt($1, VC_SIMPLE,  $2,  NULL, NULL, $4  ); }
 
|	type NAME subscript SEMI_COLON
			{ decl_stmt($1, VC_ARRAY1,  $2,  $3,   NULL, NULL); }

|	type NAME subscript subscript SEMI_COLON
			{ decl_stmt($1, VC_ARRAY2,  $2,  $3,   $4,   NULL); }

|	type ASTERISK NAME SEMI_COLON
			{ decl_stmt($1, VC_POINTER, $3,  NULL, NULL, NULL); }

|	type ASTERISK NAME subscript SEMI_COLON
			{ decl_stmt($1, VC_ARRAYP,  $3,  $4,   NULL, NULL); }
;

type		/* types for variables defined in SNL */
:	CHAR		{ $$ = V_CHAR; }
|	SHORT		{ $$ = V_SHORT; }
|	INT		{ $$ = V_INT; }
|	LONG		{ $$ = V_LONG; }
|	UNSIGNED CHAR	{ $$ = V_UCHAR; }
|	UNSIGNED SHORT	{ $$ = V_USHORT; }
|	UNSIGNED INT	{ $$ = V_UINT; }
|	UNSIGNED LONG	{ $$ = V_ULONG; }
|	FLOAT		{ $$ = V_FLOAT; }
|	DOUBLE		{ $$ = V_DOUBLE; }
|	STRING_DECL	{ $$ = V_STRING; }
|	EVFLAG		{ $$ = V_EVFLAG; }
;

sync_stmt	/* sync <variable> <event flag> */
:	SYNC NAME TO NAME SEMI_COLON		{ sync_stmt($2, NULL, $4); }
|	SYNC NAME    NAME SEMI_COLON		{ sync_stmt($2, NULL, $3); }
|	SYNC NAME subscript TO NAME SEMI_COLON	{ sync_stmt($2, $3,   $5); }
|	SYNC NAME subscript    NAME SEMI_COLON	{ sync_stmt($2, $3,   $4); }
;

defn_c_stmt	/* escaped C in definitions */
:	escaped_c_list		{ defn_c_stmt($1); }
;

option_stmt	/* option +/-<option>;  e.g. option +a; */
:	OPTION PLUS NAME SEMI_COLON	{ option_stmt($3, TRUE); }
|	OPTION MINUS NAME SEMI_COLON	{ option_stmt($3, FALSE); }
;

state_set_list 	/* a program body is one or more state sets */
:	state_set			{ $$ = $1; }
|	state_set_list state_set	{ $$ = link_expr($1, $2); }
;

state_set 	/* define a state set */
:	STATE_SET NAME L_BRACKET state_list R_BRACKET
				{ $$ = expression(E_SS, $2, $4, 0); }
|	EXIT L_BRACKET stmt_list R_BRACKET
				{ exit_code($3); $$ = 0; }
|	pp_code			{ $$ = 0; }
|	error { snc_err("state set"); }
;

state_list /* define a state set body (one or more states) */
:	state				{ $$ = $1; }
|	state_list state		{ $$ = link_expr($1, $2); }
|	error { snc_err("state set"); }
;

state	/* a block that defines a single state */
:	STATE NAME L_BRACKET transition_list R_BRACKET
				{ $$ = expression(E_STATE, $2, $4, 0); }
|	error { snc_err("state block"); }
;

transition_list	/* all transitions for one state */
:	transition			{ $$ = $1; }
|	transition_list transition	{ $$ = link_expr($1, $2); }
|	error { snc_err("transition"); }
;

transition	/* define a transition ("when" statment ) */
:	WHEN L_PAREN expr R_PAREN L_BRACKET stmt_list R_BRACKET STATE NAME
			{ $$ = expression(E_WHEN, $9, $3, $6); }
;

expr	/* general expr: e.g. (-b+2*a/(c+d)) != 0 || (func1(x,y) < 5.0) */
	/* Expr *expression(int type, char *value, Expr *left, Expr *right) */
:	compound_expr			{ $$ = expression(E_COMMA, "", $1, 0); }
|	expr binop expr %prec UOP	{ $$ = expression(E_BINOP, $2, $1, $3); }
|	expr asgnop expr		{ $$ = expression(E_ASGNOP, $2, $1, $3); }
|	unop expr  %prec UOP		{ $$ = expression(E_UNOP, $1, $2, 0); }
|	AUTO_INCR expr  %prec UOP	{ $$ = expression(E_PRE, "++", $2, 0); }
|	AUTO_DECR expr  %prec UOP	{ $$ = expression(E_PRE, "--", $2, 0); }
|	expr AUTO_INCR  %prec UOP	{ $$ = expression(E_POST, "++", $1, 0); }
|	expr AUTO_DECR  %prec UOP	{ $$ = expression(E_POST, "--", $1, 0); }
|	NUMBER				{ $$ = expression(E_CONST, $1, 0, 0); }
|	CHAR_CONST			{ $$ = expression(E_CONST, $1, 0, 0); }
|	STRING				{ $$ = expression(E_STRING, $1, 0, 0); }
|	NAME				{ $$ = expression(E_VAR, $1, 0, 0); }
|	NAME L_PAREN expr R_PAREN	{ $$ = expression(E_FUNC, $1, $3, 0); }
|	EXIT L_PAREN expr R_PAREN	{ $$ = expression(E_FUNC, "exit", $3, 0); }
|	L_PAREN expr R_PAREN		{ $$ = expression(E_PAREN, "", $2, 0); }
|	expr bracked_expr %prec SUBSCRIPT { $$ = expression(E_SUBSCR, "", $1, $2); }
|	/* empty */			{ $$ = 0; }
;

compound_expr
:	expr COMMA expr			{ $$ = link_expr($1, $3); }
|	compound_expr COMMA expr	{ $$ = link_expr($1, $3); }
;

bracked_expr	/* e.g. [k-1] */
:	L_SQ_BRACKET expr R_SQ_BRACKET	{ $$ = $2; }
;

unop	/* Unary operators */
:	PLUS		{ $$ = "+"; }
|	MINUS		{ $$ = "-"; }
|	ASTERISK	{ $$ = "*"; }
|	AMPERSAND	{ $$ = "&"; }
|	NOT		{ $$ = "!"; }
;

binop	/* Binary operators */
:	MINUS		{ $$ = "-"; }
|	PLUS		{ $$ = "+"; }
|	ASTERISK	{ $$ = "*"; }
|	SLASH 		{ $$ = "/"; }
|	GT		{ $$ = ">"; }
|	GE		{ $$ = ">="; }
|	EQ		{ $$ = "=="; }
|	NE		{ $$ = "!="; }
|	LE		{ $$ = "<="; }
|	LT		{ $$ = "<"; }
|	OR		{ $$ = "||"; }
|	AND		{ $$ = "&&"; }
|	L_SHIFT		{ $$ = "<<"; }
|	R_SHIFT		{ $$ = ">>"; }
|	BIT_OR		{ $$ = "|"; }
|	BIT_AND		{ $$ = "&"; }
|	COMPLEMENT	{ $$ = "^"; }
|	MODULO		{ $$ = "%"; }
|	PERIOD		{ $$ = "."; }	/* fudges structure elements */
|	POINTER		{ $$ = "->"; }	/* fudges ptr to structure elements */
;

asgnop	/* Assignment operators */
:	EQUAL		{ $$ = "="; }
|	PLUS_EQUAL	{ $$ = "+="; }
|	MINUS_EQUAL	{ $$ = "-="; }
|	AND_EQUAL	{ $$ = "&="; }
|	OR_EQUAL	{ $$ = "|="; }
|	DIV_EQUAL	{ $$ = "/="; }
|	MULT_EQUAL	{ $$ = "*="; }
|	MODULO_EQUAL	{ $$ = "%="; }
|	LEFT_EQUAL	{ $$ = "<<="; }
|	RIGHT_EQUAL	{ $$ = ">>="; }
|	CMPL_EQUAL	{ $$ = "^="; }
;

compound_stmt		/* compound statement e.g. { ...; ...; ...; } */
:	L_BRACKET stmt_list R_BRACKET { $$ = expression(E_CMPND, "",$2, 0); }
|	error { snc_err("action statements"); }	
;

stmt_list
:	statement		{ $$ = $1; }
|	stmt_list statement	{ $$ = link_expr($1, $2); }
|	/* empty */		{ $$ = 0; }
;

statement
:	compound_stmt			{ $$ = $1; }
|	expr SEMI_COLON			{ $$ = expression(E_STMT, "", $1, 0); }
|	BREAK SEMI_COLON		{ $$ = expression(E_BREAK, "", 0, 0); }
|	if_stmt				{ $$ = $1; }
|	else_stmt			{ $$ = $1; }
|	while_stmt			{ $$ = $1; }
|	for_stmt			{ $$ = $1; }
|	C_STMT				{ $$ = expression(E_TEXT, "", $1, 0); }
|	pp_code				{ $$ = 0; }
|	error 				{ snc_err("action statement"); }
;

if_stmt
:	IF L_PAREN expr R_PAREN statement { $$ = expression(E_IF, "", $3, $5); }
;

else_stmt
:	ELSE statement			{ $$ = expression(E_ELSE, "", $2, 0); }
;

while_stmt
:	WHILE L_PAREN expr R_PAREN statement { $$ = expression(E_WHILE, "", $3, $5); }
;

for_stmt
:	FOR L_PAREN expr SEMI_COLON expr SEMI_COLON expr R_PAREN statement
	{ $$ = expression(E_FOR, "", expression(E_X, "", $3, $5),
				expression(E_X, "", $7, $9) ); }
;

pp_code		/* pre-processor code (e.g. # 1 "test.st") */
:	PP_SYMBOL NUMBER STRING CR		{ pp_code($2, $3, ""); }
|	PP_SYMBOL NUMBER STRING NUMBER CR	{ pp_code($2, $3, $4); }
;

global_c
:	escaped_c_list	{ global_c_stmt($1); }
;

escaped_c_list
:	C_STMT			{ $$ = expression(E_TEXT, "", $1, 0); }
|	escaped_c_list C_STMT	{ $$ = link_expr($1, expression(E_TEXT, "", $2, 0)); }
;
%%
#include	"snc_lex.c"

static int yyparse (void);

/* yyparse() is static, so we create global access to it */
void Global_yyparse (void)
{
	yyparse ();
}

