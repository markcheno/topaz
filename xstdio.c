/* xstdio.c */

/*
**
** ONELINER:   A replacement for formatted printing programs.
**
** COPYRIGHT:
**   Copyright (c) 1990 by D. R. Hipp.  This code is an original work
**   and has been prepared without reference to any prior
**   implementations of similar functions.  No part of this code is
**   subject to licensing restrictions of any telephone company or
**   university.
**
**   Permission is hereby granted for this code to be used by anyone
**   for any purpose under the following restrictions:
**     1.  No attempt shall be made to prevent others (especially the
**         author) from using this code.
**     2.  Changes to this code are to be clearly marked.
**     3.  The origins of this code are not to be misrepresented.
**     4.  The user agrees that the author is in no way responsible for
**         the correctness or usefulness of this program.
**
** DESCRIPTION:
**   This program is an enhanced replacement for the "printf" programs
**   found in the standard library.  The following enhancements are
**   supported:
**
**      +  Additional functions.  The standard set of "printf" functions
**         includes printf, fprintf, sprintf, vprintf, vfprintf, and
**         vsprintf.  This module adds the following:
**
**           *  x[v]snprintf -- Works like sprintf, but has an extra argument
**                              which is the size of the buffer written to.
**
**           *  xprintf --  Similar to printf.
**
**
**      +  A few extensions to the formatting notation are supported:
**
**           *  The %b field outputs an integer in binary notation.
**
**           *  The %c field now accepts a precision.  The character output
**              is repeated by the number of times the precision specifies.
**
**           *  The %' field works like %c, but takes as its character the
**              next character of the format string, instead of the next
**              argument.  For example,  printf("%.78'-")  prints 78 minus
**              signs, the same as  printf("%.78c",'-').
**
**      +  All functions (except converter) are fully reentrant.
*/

#include "common.h"
#include "xstdio.h"
#include "xstdlib.h"
#include "xstring.h"

#define EOF (-1)

/*
** Conversion types fall into various categories as defined by the
** following enumeration.
*/
enum e_type /* The type of the format field */
{    
   RADIX,            /* Integer types.  %d, %x, %o, and so forth */
   FLOAT,            /* Floating point.  %f */
   EXP,              /* Exponentional notation. %e and %E */
   GENERIC,          /* Floating or exponential, depending on exponent. %g */
   SIZE,             /* Return number of characters processed so far. %n */
   STRING,           /* Strings. %s */
   PERCENT,          /* Percent symbol. %% */
   CHAR,             /* Characters. %c */
   CHARLIT,          /* Literal characters.  %' */
   IOERROR           /* Used to indicate no such conversion type */
};

/*
** Each builtin conversion character (ex: the 'd' in "%d") is described
** by an instance of the following structure
*/
typedef struct s_info /* Information about each format field */
{   
  Int32  fmttype;            /* The format field code letter */
  Int32  base;               /* The base for radix conversion */
  char *charset;             /* The character set for conversion */
  Int32  flag_signed;        /* Is the quantity signed? */
  char *prefix;              /* Prefix on non-zero values in alt format */
  enum e_type type;          /* Conversion paradigm */
} info;

/*
** The following table is searched linearly, so it is good to put the
** most frequently used conversion types first.
*/
static const info fmtinfo[] = 
{
  {'d',  10,  "0123456789",       1,    0, RADIX,	},
  {'s',   0,  0,                  0,    0, STRING,	},
  {'c',   0,  0,                  0,    0, CHAR,	},
  {'o',   8,  "01234567",         0,  "0", RADIX,	},
  {'u',  10,  "0123456789",       0,    0, RADIX,	},
  {'x',  16,  "0123456789abcdef", 0, "x0", RADIX,	},
  {'X',  16,  "0123456789ABCDEF", 0, "X0", RADIX,	},
  {'f',   0,  0,                  1,    0, FLOAT,	},
  {'e',   0,  "e",                1,    0, EXP,		},
  {'E',   0,  "E",                1,    0, EXP,		},
  {'g',   0,  "e",                1,    0, GENERIC,	},
  {'G',   0,  "E",                1,    0, GENERIC,	},
  {'i',  10,  "0123456789",       1,    0, RADIX,	},
  {'n',   0,  0,                  0,    0, SIZE,	},
  {'%',   0,  0,                  0,    0, PERCENT,	},
  {'b',   2,  "01",               0, "b0", RADIX,	},  /* Binary notation */
  {'p',  10,  "0123456789",       0,    0, RADIX,	},  /* Pointers */
  {'\'',  0,  0,                  0,    0, CHARLIT,	}   /* Literal character */
};

#define NINFO  (sizeof(fmtinfo)/sizeof(info))  /* Size of the fmtinfo table */

# define BUFSIZE 1000  /* Size of the output buffer */

/*
** "*val" is a Float32 such that 0.1 <= *val < 10.0
** Return the ascii code for the leading digit of *val, then
** multiply "*val" by 10.0 to renormalize.
**
** Example:
**     input:     *val = 3.14159
**     output:    *val = 1.4159    function return = '3'
*/
static Int32 getdigit(Float32 *val){
  Int32 digit;
  Float32 d;
  digit = (Int32)*val;
  d = digit;
  digit += '0';
  *val = (*val - d)*10.0;
  return digit;
}

/*
** The root program.  All variations call this core.
**
** INPUTS:
**   func   This is a pointer to a function taking three arguments
**            1. A pointer to the list of characters to be output
**               (Note, this list is NOT null terminated.)
**            2. An integer number of characters to be output.
**               (Note: This number might be zero.)
**            3. A pointer to anything.  Same as the "arg" parameter.
**
**   arg    This is the pointer to anything which will be passed as the
**          third argument to "func".  Use it for whatever you like.
**
**   fmt    This is the format string, as in the usual printf.
**
**   ap     This is a pointer to a list of arguments.  Same as in
**          vfprintf.
**
** OUTPUTS:
**          The return value is the total number of characters sent to
**          the function "func".  Returns EOF on a error.
**
** Note that the order in which automatic variables are declared below
** seems to make a big difference in determining how fast this beast
** will run.
*/
static Int32 vxprintf(void (*func)(char*,Int32,void*),
void *arg, const char *format, va_list ap){
  register const char *fmt; /* The format string. */
  register Int32 c;           /* Next character in the format string */
  register char *bufpt;     /* Pointer to the conversion buffer */
  register Int32  precision;  /* Precision of the current field */
  register Int32  length;     /* Length of the field */
  register Int32  idx;        /* A general purpose loop counter */
  Int32 count;                /* Total number of characters output */
  Int32 width;                /* Width of the current field */
  Int32 flag_leftjustify;     /* True if "-" flag is present */
  Int32 flag_plussign;        /* True if "+" flag is present */
  Int32 flag_blanksign;       /* True if " " flag is present */
  Int32 flag_alternateform;   /* True if "#" flag is present */
  Int32 flag_zeropad;         /* True if field width constant starts with zero */
  UInt32 longvalue;          /* Value for integer types */
  Float32 realvalue;         /* Value for real types */
  const info *infop;        /* Pointer to the appropriate info structure */
  char buf[BUFSIZE];        /* Conversion buffer */
  char prefix;              /* Prefix character.  "+" or "-" or " " or 0. */
  Int32  errorflag = 0;       /* True if an error is encountered */
  enum e_type xtype;        /* Which of 3 different FP formats */
  static char spaces[]="                                                    ";
#define SPACESIZE (sizeof(spaces)-1)
  Int32  exp;                 /* exponent of real numbers */
  Float32 rounder;           /* Used for rounding floating point values */
  Int32 flag_dp;              /* True if decimal point should be shown */
  Int32 flag_rtz;             /* True if trailing zeros should be removed */

  fmt = format;                     /* Put in a register for speed */
  count = length = 0;
  bufpt = 0;
  for(; (c=(*fmt))!=0; ++fmt){
    if( c!='%' ){
      register Int32 amt;
      bufpt = (char *)fmt;
      amt = 1;
      while( (c=*++fmt)!='%' && c!=0 ) amt++;
      (*func)(bufpt,amt,arg);
      count += amt;
      if( c==0 ) break;
    }
    if( (c=(*++fmt))==0 ){
      errorflag = 1;
      (*func)("%",1,arg);
      count++;
      break;
    }
    /* Find out what flags are present */
    flag_leftjustify = flag_plussign = flag_blanksign = 
     flag_alternateform = flag_zeropad = 0;
    do{
      switch( c ){
        case '-':   flag_leftjustify = 1;     c = 0;   break;
        case '+':   flag_plussign = 1;        c = 0;   break;
        case ' ':   flag_blanksign = 1;       c = 0;   break;
        case '#':   flag_alternateform = 1;   c = 0;   break;
        case '0':   flag_zeropad = 1;         c = 0;   break;
        default:                                       break;
      }
    }while( c==0 && (c=*++fmt)!=0 );
    /* Get the field width */
    width = 0;
    if( c=='*' ){
      width = va_arg(ap,Int32);
      if( width<0 ){
        flag_leftjustify = 1;
        width = -width;
      }
      c = *++fmt;
    }else{
      while( xisdigit(c) ){
        width = width*10 + c - '0';
        c = *++fmt;
      }
    }
    /* Get the precision */
    if( c=='.' ){
      precision = 0;
      c = *++fmt;
      if( c=='*' ){
        precision = va_arg(ap,Int32);
        if( precision<0 ) precision = -precision;
        c = *++fmt;
      }else{
        while( xisdigit(c) ){
          precision = precision*10 + c - '0';
          c = *++fmt;
        }
      }
      /* Limit the precision to prevent overflowing buf[] during conversion */
      if( precision>BUFSIZE-40 ) precision = BUFSIZE-40;
    }else{
      precision = -1;
    }

    /* Fetch the info entry for the field */
    infop = 0;
    for(idx=0; idx<NINFO; idx++){
      if( c==fmtinfo[idx].fmttype ){
        infop = &fmtinfo[idx];
        break;
      }
    }
    /* No info entry found.  it must be an error.  Check it out. */
    if( infop==0 ){
      xtype = IOERROR;
    }else{
      xtype = infop->type;
    }
    /*
    ** At this point, variables are initialized as follows:
    **
    **   flag_alternateform          TRUE if a '#' is present.
    **   flag_plussign               TRUE if a '+' is present.
    **   flag_leftjustify            TRUE if a '-' is present or if the
    **                               field width was negative.
    **   flag_zeropad                TRUE if the width began with 0.
    **   flag_blanksign              TRUE if a ' ' is present.
    **   width                       The specified field width.  This is
    **                               always non-negative.  Zero is the default.
    **   precision                   The specified precision.  The default
    **                               is -1.
    **   xtype                       The class of the conversion.
    **   infop                       Pointer to the appropriate info struct.
    */
    switch( xtype ){
      case RADIX:
		longvalue = va_arg(ap,Int32);
        /* More sensible: turn off the prefix for octal (to prevent "00"),
        ** but leave the prefix for hex. */
        if( longvalue==0 && infop->base==8 ) flag_alternateform = 0;
        if( infop->flag_signed ){
          if( *(Int32*)&longvalue<0 ){
            longvalue = -*(Int32*)&longvalue;
            prefix = '-';
          }else if( flag_plussign )  prefix = '+';
          else if( flag_blanksign )  prefix = ' ';
          else                       prefix = 0;
        }else                        prefix = 0;
        if( flag_zeropad && precision<width-(prefix!=0) ){
          precision = width-(prefix!=0);
		}
        {
          register char *cset;      /* Use registers for speed */
          register Int32 base;
          cset = infop->charset;
          base = infop->base;
          bufpt = &buf[BUFSIZE];
          do{                                           /* Convert to ascii */
            *(--bufpt) = cset[longvalue%base];
            longvalue = longvalue/base;
          }while( longvalue>0 );
		}
        length = (Int32)&buf[BUFSIZE]-(Int32)bufpt;
        for(idx=precision-length; idx>0; idx--){
          *(--bufpt) = '0';                             /* Zero pad */
		}
        if( prefix ) *(--bufpt) = prefix;               /* Add sign */
        if( flag_alternateform && infop->prefix ){      /* Add "0" or "0x" */
          char *pre, x;
          pre = infop->prefix;
          if( *bufpt!=pre[0] ){
            for(pre=infop->prefix; (x=*pre)!=0; pre++) *(--bufpt) = x;
		  }
        }
        length = (Int32)&buf[BUFSIZE]-(Int32)bufpt;
        break;
      case FLOAT:
      case EXP:
      case GENERIC:
	  	realvalue = va_arg(ap,Float64);
        if( precision<0 ) precision = 4;         /* Set default precision */
        if( precision>BUFSIZE-10 ) precision = BUFSIZE-10;
        if( realvalue<0.0 ){
          realvalue = -realvalue;
          prefix = '-';
		}else{
          if( flag_plussign )          prefix = '+';
          else if( flag_blanksign )    prefix = ' ';
          else                         prefix = 0;
		}
        if( infop->type==GENERIC && precision>0 ) precision--;
        for(idx=precision, rounder=0.5; idx>0; idx--, rounder*=0.1);
        if( infop->type==FLOAT ) realvalue += rounder;
        /* Normalize realvalue to within 10.0 > realvalue >= 1.0 */
        exp = 0;
        while( realvalue>=1e8 ){ realvalue *= 1e-8; exp+=8; }
        while( realvalue>=10.0 ){ realvalue *= 0.1; exp++; }
        while( realvalue<1e-8 ){ realvalue *= 1e8; exp-=8; }
        while( realvalue<1.0 ){ realvalue *= 10.0; exp--; }
        bufpt = buf;
        /*
        ** If the field type is GENERIC, then convert to either EXP
        ** or FLOAT, as appropriate.
        */
        if( xtype==GENERIC ){
          flag_rtz = !flag_alternateform;
            if( exp<-4 || exp>precision ){
            xtype = EXP;
          }else{
            precision = precision - exp;
            realvalue += rounder;
            xtype = FLOAT;
          }
		}
        /*
        ** The "exp+precision" test causes output to be of type EXP if
        ** the precision is too large to fit in buf[].
        */
        if( xtype==FLOAT && exp+precision<BUFSIZE-30 ){
          flag_rtz = 0;
          flag_dp = (precision>0 || flag_alternateform);
          if( prefix ) *(bufpt++) = prefix;         /* Sign */
          if( exp<0 )  *(bufpt++) = '0';            /* Digits before "." */
          else for(; exp>=0; exp--) *(bufpt++) = getdigit(&realvalue);
          if( flag_dp ) *(bufpt++) = '.';           /* The decimal point */
          for(exp++; exp<0 && precision>0; precision--, exp++){
            *(bufpt++) = '0';
          }
          while( (precision--)>0 ) *(bufpt++) = getdigit(&realvalue);
          *(bufpt--) = 0;                           /* Null terminate */
          if( flag_rtz && flag_dp ){     /* Remove trailing zeros and "." */
            while( bufpt>=buf && *bufpt=='0' ) *(bufpt--) = 0;
            if( bufpt>=buf && *bufpt=='.' ) *(bufpt--) = 0;
          }
          bufpt++;                            /* point to next free slot */
		}else{    /* EXP */
          flag_rtz = 0;
          flag_dp = (precision>0 || flag_alternateform);
          realvalue += rounder;
          if( prefix ) *(bufpt++) = prefix;   /* Sign */
          *(bufpt++) = getdigit(&realvalue);  /* First digit */
          if( flag_dp ) *(bufpt++) = '.';     /* Decimal point */
          while( (precision--)>0 ) *(bufpt++) = getdigit(&realvalue);
          bufpt--;                            /* point to last digit */
          if( flag_rtz && flag_dp ){          /* Remove tail zeros */
            while( bufpt>=buf && *bufpt=='0' ) *(bufpt--) = 0;
            if( bufpt>=buf && *bufpt=='.' ) *(bufpt--) = 0;
          }
          bufpt++;                            /* point to next free slot */
          *(bufpt++) = infop->charset[0];
          if( exp<0 ){ *(bufpt++) = '-'; exp = -exp; } /* sign of exp */
          else       { *(bufpt++) = '+'; }
          if( exp>=100 ) *(bufpt++) = (exp/100)+'0';   /* 100's digit */
          *(bufpt++) = exp/10+'0';                     /* 10's digit */
          *(bufpt++) = exp%10+'0';                     /* 1's digit */
		}
        /* The converted number is in buf[] and zero terminated. Output it.
        ** Note that the number is in the usual order, not reversed as with
        ** integer conversions. */
        length = (Int32)bufpt-(Int32)buf;
        bufpt = buf;
        break;
      case SIZE:
        *(va_arg(ap,Int32*)) = count;
        length = width = 0;
        break;
      case PERCENT:
        buf[0] = '%';
        bufpt = buf;
        length = 1;
        break;
      case CHARLIT:
      case CHAR:
        c = buf[0] = (xtype==CHAR ? va_arg(ap,Int32) : *++fmt);
        if( precision>=0 ){
          for(idx=1; idx<precision; idx++) buf[idx] = c;
          length = precision;
	}else{
          length =1;
	}
        bufpt = buf;
        break;
      case STRING:
        bufpt = va_arg(ap,char*);
        if( bufpt==0 ) bufpt = "(null)";
        length = xstrlen(bufpt);
        if( precision>=0 && precision<length ) length = precision;
        break;
      case IOERROR:
        buf[0] = '%';
        buf[1] = c;
        errorflag = 0;
        idx = 1+(c!=0);
        (*func)("%",idx,arg);
        count += idx;
        if( c==0 ) fmt--;
        break;
    }/* End switch over the format type */
    /*
    ** The text of the conversion is pointed to by "bufpt" and is
    ** "length" characters long.  The field width is "width".  Do
    ** the output.
    */
    if( !flag_leftjustify ){
      register Int32 nspace;
      nspace = width-length;
      if( nspace>0 ){
        count += nspace;
        while( nspace>=SPACESIZE ){
          (*func)(spaces,SPACESIZE,arg);
          nspace -= SPACESIZE;
        }
        if( nspace>0 ) (*func)(spaces,nspace,arg);
      }
    }
    if( length>0 ){
      (*func)(bufpt,length,arg);
      count += length;
    }
    if( flag_leftjustify ){
      register Int32 nspace;
      nspace = width-length;
      if( nspace>0 ){
        count += nspace;
        while( nspace>=SPACESIZE ){
          (*func)(spaces,SPACESIZE,arg);
          nspace -= SPACESIZE;
        }
        if( nspace>0 ) (*func)(spaces,nspace,arg);
      }
    }
  }/* End for loop over the format string */
  return errorflag ? EOF : count;
} /* End of function */

static void p_fout(register char *txt, register Int32 amt, register void *arg){
  register Int32 c;
  while( amt-->0 ){
    c = *txt++;
    /* putc(c,(FILE *)arg); */
	xputchar(c);
  }
}

Int32 xprintf(const char *fmt, ...){
  va_list ap;
  va_start(ap,fmt);
  /* return vxprintf(p_fout,(void*)stdout,fmt,ap); */
  return vxprintf(p_fout,NULL,fmt,ap);
}

typedef struct s_strargument {    /* Describes the string being written to */
  char *next;                        /* Next free slot in the string */
  char *last;                        /* Last available slot in the string */
} sarg;

static void sout(char *txt, Int32 amt, void *arg){
  register char *head;
  register char *t;  
  register Int32 a;
  register char *tail;
  a = amt;
  t = txt;
  head = ((sarg*)arg)->next;
  tail = ((sarg*)arg)->last;
  if( tail ){
    while( a-->0 && head<tail ) *(head++) = *(t++);
  }else{
    while( a-->0 ) *(head++) = *(t++);
  }
  *head = 0;
  ((sarg*)arg)->next = head;
}

Int32 xsnprintf(char *buf, Int32 n, const char *fmt, ...){
  va_list ap;
  sarg arg;
  va_start(ap,fmt);
  arg.next = buf;
  arg.last = &buf[n-1];
  return vxprintf(sout,(void*)&arg,fmt,ap);
}

Int32 xvsnprintf(char *buf, Int32 n, const char *fmt, va_list ap){
  sarg arg;
  arg.next = buf;
  arg.last = &buf[n-1];
  return vxprintf(sout,(void*)&arg,fmt,ap);
}

/*
** The following are for malloc-printf.  Space is obtained from malloc
** to hold the string which results, and a pointer to this space is
** returned.  NULL is returned if we run out of space.
*/

typedef struct s_mprintfarg {
  char *buf;
  Int32  size;
  char *next;
} marg;

static void mout(char *txt, Int32 amt, void *arg){
  register char *head = 0;
  register char *t;
  register Int32 a;
  marg *mp;
  mp = (marg*)arg;
  a = amt;
  t = txt;
  if( mp->size==0 ){
    mp->size = amt+1;
    head = mp->buf = xmalloc( mp->size );
    if( head ){
      while( a-->0 ) *(head++) = *(t++);
      *head = 0;
    }
  }else{
    if( mp->buf!=0 ){
      mp->buf = xrealloc(mp->buf,mp->size+amt);
      if( mp->buf ){
        head = &(mp->buf[mp->size-1]);
        while( a-->0 ) *(head++) = *(t++);
        *head = 0;
        mp->size += amt;
      }
    }
  }
  mp->next = head;
}
char *xmprintf(const char *fmt, ...){
  va_list ap;
  marg mp;
  va_start(ap,fmt);
  mp.size = 0;
  mp.buf = mp.next = 0;
  vxprintf(mout,(void*)&mp,fmt,ap);
  return mp.buf;
}
char *xvmprintf(const char *fmt, va_list ap){
  marg mp;
  mp.size = 0;
  mp.buf = mp.next = 0;
  vxprintf(mout,(void*)&mp,fmt,ap);
  return mp.buf;
}


#ifdef TEST1
/****************************************************************************
*                      Test floating-point conversions                      *
****************************************************************************/

char outbuf[1000];
int  outidx = 0;

int fout(char *str, int amt){
  strncpy(&outbuf[outidx],str,amt);
  outidx += amt;
  return 0;
}

int testprintf(char *fmt, ...){
  va_list ap;
  va_start(ap,fmt);
  return vxprintf((void(*)(char*,int,void*))fout,0,fmt,ap);
}

/*
** compare number strings.  Ignore errors past the first 15 significant
** digits.  Return TRUE for a match, and FALSE for a mismatch.
*/
int rcmp(char *a, char *b){
  int digitcnt = 0;
  while( *a && *b ){
    if( (!isdigit(*a) || ++digitcnt<15) && *a!=*b ){
      return 0;
    }
    if( !isdigit(*a) && *a!='.' ) digitcnt = 0;
    a++;
    b++;
  }
  return *a==*b;
}

int main(){
  int count = 0;
  char sysout[1000];
  int sysrc;
  int vdrc;
  double value;
  int w,p;
  static char *format[] = { "%*.*g","%-*.*g","%#*.*g","%+*.*g", "% *.*G",
                            "%*.*e", "%#*.*E", "%*.*f", "%#*.*f", "%*%%*g"};
  int fcnt;
  for(fcnt=0; fcnt<sizeof(format)/sizeof(char*); fcnt++){
   for(w=0; w<=12; w += 3){
    for(p=0; p<=12; p += 3){
      for(value=1.234567890123456789e-32; value<1.3e+32; value *= 1e8 ){
        outidx = 0;
        sprintf(sysout,format[fcnt],w,p,value);
        vdrc = testprintf(format[fcnt],w,p,value);
        sysrc = strlen(sysout);
        outbuf[outidx] = 0;
        count++;
        if( vdrc!=sysrc || !rcmp(outbuf,sysout) ){
          printf("Count=%d  Format=%s  Width=%d  Precision=%d  Value=%.16g\n",
            count,format[fcnt],w,p,value);
          printf("  system:   [%s] returned %d\n",sysout,sysrc);
          printf("  vxprintf: [%s] returned %d\n",outbuf,vdrc);
        }
      }
    }
   }
  }
  printf("Checked %d combinations.\n",count);
  
  return 0;
}

#endif

#ifdef TEST2
/****************************************************************************
*                         Test integer conversions                          *
****************************************************************************/

char outbuf[1000];
int  outidx = 0;

int fout(char *str, int amt){
  strncpy(&outbuf[outidx],str,amt);
  outidx += amt;
  return 0;
}

int testprintf(char *fmt, ...){
  va_list ap;
  va_start(ap,fmt);
  return vxprintf((void(*)(char*,int,void*))fout,0,fmt,ap);
}

int main(){
  static char letters[] = "dixXou%";
  static char *flags[] = { "", "-", " ", "+", "#", "-#", "+-", "+#", "# ",
                           "-+#", "# -+", 0 };
  static char *widths[] = { "", "0", "05", "1", "3", "10", "*", 0 };
  static char *precisions[] = { "", ".1", ".3", ".10", ".*", 0 };
  int values[] = { 0, 1, 10, 9999, -1, -10, -9999 };
  char format[100];
  char sysout[1000];
  int longflag;
  int sysrc;
  int vdrc;
  int lcnt;
  int fcnt;
  int wcnt;
  int pcnt;
  int vcnt;
  int v1, v2, v3;
  int count = 0;
  lcnt = fcnt = vcnt = longflag = wcnt = pcnt = v1 = v2 = v3 = 0;
  do{
    sprintf(format,"Ho ho ho %%%s%s%s%s%c hi hi hi",
       flags[fcnt],widths[wcnt],precisions[pcnt],
       longflag?"l":"",letters[lcnt]);
    v1 = values[vcnt];
    if( widths[wcnt][0]=='*' ){
      v2 = v1;
      v1 = 8;
    }
    if( precisions[pcnt][1]=='*' ){
      v3 = v2;
      v2 = v1;
      v1 = 7;
    }
    outidx = 0;
    if( longflag ){
      sprintf(sysout,format,(long)v1,v2,v3);
      vdrc = testprintf(format,(long)v1,v2,v3);
    }else{
      sprintf(sysout,format,v1,v2,v3);
      vdrc = testprintf(format,v1,v2,v3);
    }
    sysrc = strlen(sysout);
    outbuf[outidx] = 0;
    count++;
    if( count%1000==0 ){
      printf("\rChecked thru %d...   ",count);
      fflush(stdout);
    }
    if( vdrc!=sysrc || strcmp(outbuf,sysout)!=0 ){
      printf("Format: [%s]  Arguments: %d, %d, %d\n",format,v1,v2,v3);
      printf("  system:   [%s] returned %d\n",sysout,sysrc);
      printf("  vxprintf: [%s] returned %d\n",outbuf,vdrc);
    }
    vcnt++;
    if( vcnt>=sizeof(values)/sizeof(int) ){ vcnt = 0; lcnt++; }
    if( letters[lcnt]==0 ){ lcnt = 0; longflag++; }
    if( longflag==2 ){ longflag = 0; fcnt++; }
    if( flags[fcnt]==0 ){ fcnt = 0; wcnt++; }
    if( widths[wcnt]==0 ){ wcnt = 0; pcnt++; }
  }while( precisions[pcnt] );
  printf("\nChecked %d combinations.\n",count);
  
  return 0;
}
#endif

#ifdef TEST3
/****************************************************************************
*                         Test string conversions                           *
****************************************************************************/

char outbuf[1000];
int  outidx = 0;

int fout(char *str, int amt){
  strncpy(&outbuf[outidx],str,amt);
  outidx += amt;
  return 0;
}

int testprintf(char *fmt, ...){
  va_list ap;
  va_start(ap,fmt);
  return vxprintf((void(*)(char*,int,void*))fout,0,fmt,ap);
}

int main(){
  int count = 0;
  char sysout[1000];
  int sysrc;
  int vdrc;
  int i;
  int sysx;
  int vdx;
  for(i=0; i<50; i++){
    sprintf(sysout,"%*s%n--%c ho ho ho",i,(i&1)?"xxx":"yyyyyyyy",&sysx,i+'a');
    sysrc = strlen(sysout);
    outidx = 0;
    vdrc = testprintf("%*s%n--%c ho ho ho",i,(i&1)?"xxx":"yyyyyyyy",&vdx,i+'a');
    outbuf[outidx] = 0;
    count++;
    if( vdrc!=sysrc || strcmp(outbuf,sysout)!=0 || sysx!=vdx ){
      printf("  system:   [%s] returned %d with x=%d\n",sysout,sysrc,sysx);
      printf("  vxprintf: [%s] returned %d with x=%d\n",outbuf,vdrc,vdx);
    }
  }
  printf("Checked %d combinations.\n",count);
  
  return 0;
}
#endif
