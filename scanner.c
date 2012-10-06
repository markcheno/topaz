/* scanner.c */

#include "common.h"
#include "scanner.h"
#include "parser.h"
#include "xstdlib.h"
#include "xstdio.h"
#include "xstring.h"

/*-----------*/
/* Constants */
/*-----------*/

enum
{
	MAX_STRING	= 64,			/* max length of string literals  */
	SQUOTE 		= '\'',
	DQUOTE 		= '\"'
};

/*---------*/
/* Globals */
/*---------*/

TOKEN Token;

/*-----------------*/
/* Local variables */
/*-----------------*/

static Int32	ch;					/* last character scanned         */
static Int32	ival;				/* last integer scanned           */
static Float32	rval;				/* last real scanned              */
static char		sval[MAX_STRING];	/* last string scanned            */
static char		ident[MAX_TOKEN];	/* last indentifer scanned        */
static Pointer	bufptr;				/* input buffer pointer           */
static Int32	maxbuf;				/* program size                   */
static Int32	charnum;			/* number of characters processed */
static Int32	linenum;			/* current line number            */

static Pointer TokenName[] = /* needs to match TOKEN enum in scanner.h */
{
	/* Scanner tokens */
	"error", "variable", "const", "intconst", "realconst", "stringconst",
	"+", "-", "*", "/", "[", "]", "{", "}", "(", ")", "==", "<>", "<", 
    "<=", ">", ">=", ",", ";", ".", "=", "eof",
	
	/* Keywords */
	"def", "if", "then", "elsif", "else", "end", "unless", "case", "when", 
	"while", "do", "until", "return", "or", "and", "not", "mod", "break",
	"for", "class", "self", "super", "public", "protected", "private",
	"library",
	
	/* Reserved words */
	"module", "require", "include", "next", "in", "foreach", "static", 0
};

typedef struct Keyword Keyword;
struct Keyword
{
	Pointer word;
	TOKEN	token;
};

Keyword keyword[] = {
	
	{ "def",		tDEF		},
	{ "if",			tIF			},
	{ "then",		tTHEN		},
	{ "elsif",		tELSIF		},
	{ "else",		tELSE		},
	{ "end",		tEND		},
	{ "unless",		tUNLESS		},
	{ "case",		tCASE		},
	{ "when",		tWHEN		},
	{ "while",		tWHILE		},
	{ "do",			tDO			},
	{ "until",		tUNTIL		},
	{ "return",		tRETURN		},
	{ "or",			tOR			},
	{ "and",		tAND		},
	{ "not",		tNOT		},
	{ "mod",		tMOD		},
	{ "break",		tBREAK		},
	{ "for",		tFOR		},
	{ "class",		tCLASS		},
	{ "self",		tSELF		},
	{ "super",		tSUPER		},
	{ "public",		tPUBLIC		},
	{ "protected",	tPROTECTED	},
	{ "private",	tPRIVATE	},
	{ "library",    tLIBRARY    },
	{ 0,			0			}
};

Keyword reserved[] = {
	
	{ "module",		tMODULE	 	},
	{ "require",	tREQUIRE	},
	{ "include",	tINCLUDE	},
	{ "next",		tNEXT		},
	{ "in",			tIN			},
	{ "foreach",	tFOREACH	},
	{ "static", 	tSTATIC 	},
	{ 0,			0			}
};

/*------------*/
/* Prototypes */
/*------------*/

static void  NextCh(void)         scanner_sect;
static void  GetWord(void)        scanner_sect;
static void  GetNumber(void)      scanner_sect;
static void  GetStringConst(void) scanner_sect;
static void  GetSpecial(void)     scanner_sect;
static void  SkipWhite(void)      scanner_sect;

/*---------------------*/
/* Set up input stream */
/*---------------------*/

void ScnInit(Pointer buf,Int32 max)
{
	bufptr  = buf;
	maxbuf  = max;
	charnum = 0;
	linenum = 1;
	
	/* prime the pump */
	NextCh();
	NextToken();
}

/*---------------------------------------------*/
/* Advance to next character from input buffer */
/*---------------------------------------------*/

static void NextCh(void)
{	
	if( *bufptr == '\0' || charnum == maxbuf )
	{
		ch = -1;
		return;
	}
	ch = *bufptr++;
	if( ch == '\n' ) linenum++;
	charnum++;
}

/*------------------------------------*/
/* Return current line being compiled */
/*------------------------------------*/

Int32 GetLineNum(void)
{
	return linenum-1;
}

/*-------------------------------------------------*/
/* Return the current offset into the input buffer */
/*-------------------------------------------------*/

Int32 GetCharNum(void)
{
	return charnum;
}

/*---------------------------------------------*/
/* Return next NON-SPACE character from input  */
/* buffer WITHOUT advancing the buffer pointer */           
/*---------------------------------------------*/

Int32 LookAheadChar(void)
{	
	Int32 tmpch = ch;
	Pointer tmpptr = bufptr;
		
	if( tmpch == '\0' || charnum == maxbuf )
	{
		return -1;
	}

  	while( (tmpch == ' '  || tmpch == '\t' || tmpch == '\r' || 
		    tmpch == '\n' || tmpch == '#') && tmpch != -1 )
  	{
		tmpch = *tmpptr++;
  	}
	
	return tmpch;
}

/*-----------------------------------*/
/* Skip the next NON-SPACE character */
/*-----------------------------------*/

Int32 SkipChar(void)
{
	Int32 tmp;
	
	SkipWhite();
	tmp = ch;
	NextCh();
	
	return tmp;
}

/*----------------------------------------------*/
/* Extract the next token from the input stream */
/*----------------------------------------------*/

Int32 NextToken(void)
{
	SkipWhite();
		
	if( xisalpha(ch) )
	{
		GetWord();
	}
	else if( xisdigit(ch) )
	{
		GetNumber();
	}
	else if( ch == SQUOTE || ch == DQUOTE )
	{		
		GetStringConst();
	}
	else if( ch == -1 )
	{
		Token = tEOF;
	}
	else
	{
		GetSpecial();
	}
	
	/* Printf("token: %s\n",TokenName[Token]); */
	
	return Token;
}

/*------------------------------------------------------------*/
/* Extract a word from the input stream and scan for keywords */
/*------------------------------------------------------------*/

static void GetWord(void)
{		
	Int32 i=0;

	if( xisupper(ch) )
		Token = tCONST;
	else
		Token = tVARIABLE;

	ident[i++] = ch;
	NextCh();
	
	/* allow underscores in words */
	while( (xisalnum(ch) || ch=='_' ) && i<MAX_TOKEN-1 )
	{
		ident[i++] = ch;
		NextCh();
	}
		
	ident[i] = '\0';
	
	/* check for keywords */
	for( i=0; keyword[i].word != 0; i++ )
	{
		if( xstrcmp(keyword[i].word,ident) == 0 )
		{
			Token = keyword[i].token;
			return;
		}
	}

	/* check for reserved words */
	for( i=0; reserved[i].word != 0; i++ )
	{
		if( xstrcmp(reserved[i].word,ident) == 0 )
		{
			Token = tERROR;
			compileError("reserved word - %s",reserved[i].word); 
			return;
		}
	}
}

/*------------------------------------------------------*/
/* Extract an Integer/Real Number from the input stream */
/*------------------------------------------------------*/

static void GetNumber(void)
{
	Int32 i=0;
	char num[MAX_TOKEN+1];

	ival = 0; 
	rval = 0;
	Token = tINTCONST;

	/* check for octal/hex notation */
	if( ch == '0' )
	{
		num[i++] = ch;
		NextCh();

		if( ch == 'x' ) 
		{
			Token = tINTCONST;
		
			num[i++] = ch; 
			NextCh();

			while( xisxdigit(ch) && ch != -1 && i < MAX_TOKEN-1 )
			{
				num[i++] = ch;
				NextCh();	
			}
		}
	}

	while( xisdigit(ch) && ch != -1 && i < MAX_TOKEN-1 )
	{
		num[i++] = ch; 
		NextCh();	
	}

	
	if( ch == '.' ) 
	{
		Token = tREALCONST;
		
		num[i++] = ch; 
		NextCh();

		while( xisdigit(ch) && ch != -1 && i < MAX_TOKEN-1 )
		{
			num[i++] = ch;
			NextCh();	
		}
	}
	
	if( ch == 'e' || ch == 'E' )
	{
		Token = tREALCONST;
		
		num[i++] = ch; 
		NextCh();	

		if( ch == '+' || ch == '-' )
		{
			num[i++] = ch; 
			NextCh();	
		}
		while( xisdigit(ch) && ch != -1 && i < MAX_TOKEN-1 )
		{
			num[i++] = ch;
			NextCh();
		}
	}


	num[i] = '\0';
	
	switch(Token)
	{
	case tREALCONST:
		/* xprintf("Scanned real: '%s'\n",num); */
		rval = xstrtod(&num[0],NULL);
		break;

	case tINTCONST:  
		/* xprintf("Scanned integer: '%s'\n",num); */
		ival = xstrtol(&num[0],NULL,0);
		break;
		
	default: 
		break;
	}	
}

/*-------------------------------------------------*/
/* Extract a string constant from the input stream */
/*-------------------------------------------------*/

static void GetStringConst(void)
{
	Int32 len=0;
	Int32 done=FALSE;
	
	Token = tSTRINGCONST;
		
	do
	{
		NextCh();
		if( ch == SQUOTE || ch == DQUOTE )
		{
			NextCh();
			if( ch == SQUOTE || ch == DQUOTE )
				sval[len++] = ch;
			else
				done = TRUE;
		}
		else
		{
			if( ch == '\\' )
			{
				NextCh();	/* do not save the '\' */
				switch(ch)
				{
				case 'n':  sval[len++]='\n'; break;
				case 't':  sval[len++]='\t'; break;
				case 'r':  sval[len++]='\r'; break;
				case '"':  sval[len++]='"';  break;
				case '\'': sval[len++]='\''; break;
				default :  sval[len++]='\\'; break;
				}
			}
			else
				sval[len++] = ch;
		}
	}
	while( !done && len < MAX_STRING );

	if( len == MAX_STRING || len == 0 )
	{
		compileError("invalid string constant");
		sval[0] = '\0';
	}
    else
    {
	    sval[len] = '\0';
    }
	
	/* Printf("scanned string const: %s\n",&sval[0]); */
}	

/*------------------------------------------------------------*/
/* Extract a special character sequence from the input stream */
/*------------------------------------------------------------*/

static void GetSpecial(void)
{
	if( ch == '(' ) { NextCh(); Token=tLPAREN;	return; }
	if( ch == ')' ) { NextCh(); Token=tRPAREN;	return; }
	if( ch == '[' ) { NextCh(); Token=tLBRACK;	return; }
	if( ch == ']' ) { NextCh(); Token=tRBRACK;	return; }
	if( ch == '{' ) { NextCh(); Token=tLCURLY;	return; }
	if( ch == '}' ) { NextCh(); Token=tRCURLY;	return; }
	if( ch == ',' ) { NextCh(); Token=tCOMMA;	return; }
	if( ch == ';' ) { NextCh(); Token=tSEMI;	return; }
	if( ch == '.' ) { NextCh(); Token=tPERIOD;	return; }
	if( ch == '%' ) { NextCh(); Token=tMOD;		return; }
	if( ch == '+' ) { NextCh(); Token=tADD;		return; }
	if( ch == '-' ) { NextCh(); Token=tSUB;		return; }
	if( ch == '*' ) { NextCh(); Token=tMUL;		return; }
	if( ch == '/' ) { NextCh(); Token=tDIV;		return; }
	
	/* catch "||" */
	if( ch == '|' )
	{
		NextCh();
		if( ch == '|' )
		{
			NextCh();
			Token = tOR;
		}
		else
		{
			Token = tERROR;
		}
		return;
	}

	/* catch "&&" */
	if( ch == '&' )
	{
		NextCh();
		if( ch == '&' )
		{
			NextCh();
			Token = tAND;
			return;
		}
	}

	/* catch "=" or "==" */
	if( ch == '=' )
	{
		NextCh();
		if( ch == '=' )
		{
			NextCh();
			Token = tEQL;
		}
		else
		{
			Token = tEQUALS;
		}
		return;
	}
	
	/* catch ">" or ">=" */
	if( ch == '>' )
	{
		NextCh();
		if( ch == '=' )
		{
			NextCh();
			Token = tGEQ;
		}
		else
		{
			Token = tGTR;
		}
		return;
	}
	
	/* catch "<" or "<=" or "<>" */
	if( ch == '<' )
	{
		NextCh();
		if( ch == '=' )
		{
			NextCh(); 
			Token = tLEQ;
			return;
		}
		if( ch == '>' )
		{
			NextCh(); 
			Token = tNEQ;
			return;
		}
		else 
		{
			Token = tLSS;
		}
		return;
	}

	/* catch "!" or "!=" */
	if( ch == '!' )
	{
		NextCh();
		if( ch == '=' )
		{
			NextCh();
			Token = tNEQ;
		}
		else
		{
			Token = tNOT;
		}
		return;        
	}

	Token = tERROR;
	compileError("unknown character - %c",ch);
}

/*-----------------*/
/* Skip over token */
/*-----------------*/

Int32 SkipToken(TOKEN tok)
{
	if( Token == tok )
	{
		NextToken();
	}
	else
	{
		compileError("expected '%s' (skip)",TokenName[tok]);
		return FALSE;
	}
	
	return TRUE;
}

/*-------------------------------*/
/* Match token or generate error */
/*-------------------------------*/

Int32 MatchToken(TOKEN tok)
{
	if( Token != tok )
	{
		compileError("expected '%s' (match)",TokenName[tok]);
		return FALSE;
	}
	
	return TRUE;
}

/*---------------------------------------------*/
/* Determine if current token is in token list */
/*---------------------------------------------*/

Int32 TokenIn(TOKEN token_list[])
{
	TOKEN *tokenptr;
			
	if( token_list == 0 ) return FALSE;
	
	for( tokenptr=&token_list[0]; *tokenptr; ++tokenptr )
	{
		if( Token == *tokenptr ) return TRUE;
	}
	
	return FALSE;	
}

/*---------------------------------*/
/* Return the last integer scanned */
/*---------------------------------*/

Int32 GetInt(void) { return ival; }

/*------------------------------*/
/* Return the last real scanned */
/*------------------------------*/

Float32 GetReal(void) { return rval; }

/*---------------------------------------------------*/
/* Return pointer to the last string literal scanned */
/*---------------------------------------------------*/

Pointer GetString(void) { return &sval[0]; }

/*-----------------------------------------------*/
/* Return pointer to the last identifier scanned */
/*-----------------------------------------------*/

Pointer GetIdent(void) { return &ident[0]; }

/*-------------------------------*/
/* Skip Over Leading White Space */
/*-------------------------------*/

static void SkipWhite(void)
{
  	while( (ch == ' '  || ch == '\t' || ch == '\r' || 
		    ch == '\n' || ch == '#') && ch != -1 )
  	{
		/* skip comments */
		if( ch == '#' )
		{
			while( ch != '\n' && ch != -1 )
			{
				NextCh();
			}
		}
		NextCh();
  	}
}
