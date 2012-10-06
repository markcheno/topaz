/* xstring.c */

#include "common.h"
#include "xstring.h"
#include "xstdlib.h"

/*-------------------*/
/* Standard c strtol */
/*-------------------*/

Int32 xstrtol(const char *cp,char **endp,UInt16 base)
{
	Int32 sign;
	Int32 result=0,value;
	
	if( !xisdigit(*cp) )
	{
		if( (*cp) == '-' )
			sign = -1;
		else
			sign = 1;
		cp++;
	}
	else 
		sign = 1;
		
	if(!base)
	{
		base = 10;
		if( *cp == '0' )
		{
			base = 8;
			cp++;
			if( (*cp == 'x') && xisxdigit(cp[1]) )
			{
				cp++;
				base = 16;
			}
		}
	}

	while( xisxdigit(*cp) && 
	       (value=xisdigit(*cp) ? *cp-'0' : (xislower(*cp) ? xtoupper(*cp) : *cp)-'A'+10) < base)
	{
		result = result*base + value;
		cp++;
	}
	
	if(endp)
		*endp = (char*)cp;
	
	return (result*sign);
}

/*--------------------------------------------------------------------------*/
/* xstrtod --                                                                */
/*                                                                          */
/*	This procedure converts a floating-point number from an ASCII           */
/*	decimal representation to internal double-precision format.             */
/*                                                                          */
/* Results:                                                                 */
/*	The return value is the double-precision floating-point                 */
/*	representation of the characters in string.  If endPtr isn't            */
/*	NULL, then *endPtr is filled in with the address of the                 */
/*	next character after the last one that was part of the                  */
/*	floating-point number.                                                  */
/*                                                                          */
/* Side effects:                                                            */
/*	None.                                                                   */
/*                                                                          */
/* string: A decimal ASCII floating-point number, optionally preceded       */
/*         by white space. Must have form "-I.FE-X", where I is the         */
/*         integer part of the mantissa, F is the fractional part of        */
/*         the mantissa, and X is the exponent.  Either of the signs        */
/*         may be "+", "-", or omitted.  Either I or F may be omitted,      */
/*         or both. The decimal point isn't necessary unless F is present.  */
/*         The "E" may actually be an "e".  E and X may both be omitted     */
/*         (but not just one).                                              */
/*                                                                          */
/* endPtr: If non-NULL, store terminating character's address here.         */
/*                                                                          */
/* Copyright (c) 1988-1993 The Regents of the University of California.     */
/* Copyright (c) 194 Sun Microsystems, Inc.                                 */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/*---------------------------------------------*/
/* Largest possible base 10 exponent.          */
/* Any exponent larger than this will already  */
/* produce underflow or overflow, so there's   */
/* no need to worry about additional digits.   */
/*---------------------------------------------*/

static Int32 maxExponent = 255;	

/*---------------------------------------------*/
/* Table giving binary powers of 10.           */
/* Entry is 10^2^i. Used to convert decimal    */
/* exponents into floating-point numbers.      */
/*---------------------------------------------*/

static Float32 powersOf10[] =
{	
    10.,		
    100.,	
    1.0e4,
    1.0e8,
    1.0e16,
    1.0e32,
    1.0e64,
    1.0e128,
    1.0e256
};

Float32 xstrtod(const char *string,char **endPtr)
{
	Int32 sign, expSign=FALSE;
	Float32 fraction, dblExp, *d;
	register const char *p;
	register Int32 c;
	Int32 exp=0;
	Int32 fracExp=0;		
    Int32 mantSize;
    Int32 decPt;
    const char *pExp;

    /* Strip off leading blanks and check for a sign. */
    p = string;
    while( xisspace((unsigned char)*p) )
	{
		p += 1;
	}
	
	if( *p == '-' )
	{
		sign = TRUE;
		p += 1;
	}
	else
	{
		if( *p == '+' ) p += 1;
		sign = FALSE;
	}

	/* Count the number of digits in the mantissa */
	decPt = -1;
	for( mantSize=0; ; mantSize += 1 )
	{
		c = *p;
		if( !xisdigit(c) )
		{
			if( (c != '.') || (decPt >= 0) ) break;
			decPt = mantSize;
		}
		p += 1;
	}

	/*
	 * Now suck up the digits in the mantissa.  Use two integers to
	 * collect 9 digits each (this is faster than using floating-point).
	 * If the mantissa has more than 18 digits, ignore the extras, since
	 * they can't affect the value anyway.
	 */
    
	pExp = p;
	p -= mantSize;
    if( decPt < 0 )
	{
		decPt = mantSize;
	}
	else
	{
		mantSize -= 1;			/* One of the digits was the point. */
	}
	
	if( mantSize > 18 )
	{
		fracExp = decPt - 18;
		mantSize = 18;
	} 
	else
	{
		fracExp = decPt - mantSize;
	}
	
	if( mantSize == 0 )
	{
		fraction = 0.0;
		p = string;
		goto done;
	}
	else 
	{
		Int32 frac1, frac2;
		
		frac1 = 0;
		for ( ; mantSize>9; mantSize -= 1)
		{
			c = *p;
			p += 1;
			if( c == '.' )
			{
				c = *p;
				p += 1;
			}
			frac1 = 10*frac1 + (c - '0');
		}
		
		frac2 = 0;
		for( ; mantSize>0; mantSize -= 1 )
		{
			c = *p;
			p += 1;
			if( c == '.' )
			{
				c = *p;
				p += 1;
			}
			frac2 = 10*frac2 + (c - '0');
		}
		fraction = (1.0e9 * frac1) + frac2;
    }

    /* Skim off the exponent */
	p = pExp;
	if( (*p == 'E') || (*p == 'e') )
	{
		p += 1;
		if( *p == '-' )
		{
			expSign = TRUE;
			p += 1;
		} 
		else
		{
			if( *p == '+' ) p += 1;
			expSign = FALSE;
		}
		while( xisdigit((unsigned char)*p) )
		{
			exp = exp * 10 + (*p - '0');
			p += 1;
		}
	}
	
	if(expSign)
	{
		exp = fracExp - exp;
	}
	else
	{
		exp = fracExp + exp;
	}

	/*
	 * Generate a floating-point number that represents the exponent.
	 * Do this by processing the exponent one bit at a time to combine
	 * many powers of 2 of 10. Then combine the exponent with the
	 * fraction.
	 */
    
	if( exp < 0 )
	{
		expSign = TRUE;
		exp = -exp;
	}
	else
	{
		expSign = FALSE;
	}
	
	if( exp > maxExponent )
	{
		exp = maxExponent;
	}
	
	dblExp = 1.0;
	for( d=powersOf10; exp != 0; exp >>= 1, d += 1 )
	{
		if( exp & 01 ) dblExp *= *d;
	}
	
	if(expSign)
	{
		fraction /= dblExp;
	} 
	else
	{
		fraction *= dblExp;
	}

done:
	if( endPtr != NULL )
	{
		*endPtr = (char*)p;
	}

	if(sign)
	{
		return -fraction;
	}

	/* xprintf("xstrtod: %f\n",fraction); */
	
	return fraction;
}
