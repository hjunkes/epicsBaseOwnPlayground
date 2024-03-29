/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/* postfixPvt.h
 *      Author:          Bob Dalesio
 *      Date:            9-21-88
 */

#ifndef INCpostfixPvth
#define INCpostfixPvth

#include <shareLib.h>

/*	defines for element table      */
#define		BAD_EXPRESSION	0
#define 	FETCH_A		1
#define		FETCH_B		2
#define		FETCH_C		3
#define		FETCH_D		4
#define		FETCH_E		5
#define		FETCH_F		6
#define		FETCH_G		7
#define		FETCH_H		8
#define		FETCH_I		9
#define		FETCH_J		10
#define		FETCH_K		11
#define		FETCH_L		12
#define		ACOS		13
#define		ASIN		14
#define		ATAN		15
#define		COS		16
#define		COSH		17
#define		SIN		18
#define		STORE_A		19
#define		STORE_B		20
#define		STORE_C		21
#define		STORE_D		22
#define		STORE_E		23
#define		STORE_F		24
#define		STORE_G		25
#define		STORE_H		26
#define		STORE_I		27
#define		STORE_J		28
#define		STORE_K		29
#define		STORE_L		30
#define		RIGHT_SHIFT	31
#define		LEFT_SHIFT	32
#define		SINH		33
#define		TAN		34
#define		TANH		35
#define		LOG_2		36
#define		COND_ELSE	37
#define		ABS_VAL		38
#define		UNARY_NEG	39
#define		SQU_RT		40
#define		EXP		41
#define		CEIL		42
#define		FLOOR		43
#define		LOG_10		44
#define		LOG_E		45
#define		RANDOM		46
#define		ADD		47
#define		SUB		48
#define		MULT		49
#define		DIV		50
#define		EXPON		51
#define		MODULO		52
#define		BIT_OR		53
#define		BIT_AND		54
#define		BIT_EXCL_OR	55
#define		GR_OR_EQ	56
#define		GR_THAN		57
#define		LESS_OR_EQ	58
#define		LESS_THAN	59
#define		NOT_EQ		60
#define		EQUAL		61
#define		REL_OR		62
#define		REL_AND		63
#define		REL_NOT		64
#define		BIT_NOT		65
#define		PAREN		66
#define		MAX		67
#define		MIN		68
#define		COMMA		69
#define		COND_IF		70
#define		COND_END	71
#define		CONSTANT	72
#define		CONST_PI	73
#define		CONST_D2R	74
#define		CONST_R2D	75
#define		NINT		76
#define		ATAN2		77
#define		END_STACK	127

#endif /* INCpostfixPvth */
