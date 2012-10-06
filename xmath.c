/* xmath.c */

#include "common.h"
#include "xmath.h"

/*----------------------------------------------------------*/
/* Cephes Math Library Release 2.2:  June, 1992             */
/* Copyright 1985, 1987, 1988, 1992 by Stephen L. Moshier   */
/* Direct inquiries to 30 Frost Street, Cambridge, MA 02140 */
/*                                                          */
/* MCC - Note:                                              */
/* original code referenced IBMPC,DEC,MIEEE defines         */
/* MIEEE - works for Palm                                   */
/* IBMPC - works for Linux                                  */
/*----------------------------------------------------------*/

/*-----------*/
/* Constants */
/*-----------*/

static Float32 MAXNUMF = 1.7014117331926442990585209174225846272e38f;
static Float32 T24M1 = 16777215.;
static Float32 PI180 = 0.0174532925199432957692; /* pi/180 */

/*------------*/
/* Prototypes */
/*------------*/

static Float32 ldexp(Float32 x,Int32 pw2)          xmath_sect;
static Float32 frexp(Float32 x,Int32 *pw2)         xmath_sect;
static Float32 tancotdg( Float32 xx, Int32 cotlg ) xmath_sect;

/*-----------------------------------*/
/* Circular sine of angle in degrees */
/*-----------------------------------*/

Float32 xsindg(Float32 xx)
{
	Float32 x,y,z;
	Int32 j,sign=1;

	x = xx;
	if( xx < 0 )
	{
		sign = -1;
		x = -xx;
	}
	
	if( x > T24M1 )
		return 0.0f;

	j = 0.022222222222222222222f * x; /* integer part of x/45 */
	y = j;
	
	/* map zeros to origin */
	if( j & 1 )
	{
		j += 1;
		y += 1.0f;
	}
	
	j &= 7; /* octant modulo 360 degrees */
	
	/* reflect in x axis */
	if( j > 3)
	{
		sign = -sign;
		j -= 4;
	}

	x = x - y * 45.0f;
	x *= PI180;	/* multiply by pi/180 to convert to radians */
	z = x * x;
	
	if( (j==1) || (j==2) )
	{
		y = ((2.443315711809948E-005f  * z
		    - 1.388731625493765E-003f) * z
		    + 4.166664568298827E-002f) * z * z;
		y -= 0.5f * z;
		y += 1.0f;
	}
	else
	{
		y = ((-1.9515295891E-4f  * z
		     + 8.3321608736E-3f) * z
		     - 1.6666654611E-1f) * z * x;
		y += x;
	}

	if( sign < 0 )
		y = -y;
	
	return y;
}

/*-------------------------------------*/
/* Circular cosine of angle in degrees */
/*-------------------------------------*/

Float32 xcosdg( Float32 xx )
{
	Float32 x,y,z;
	Int32 j,sign=1;

	x = xx;
	if( x < 0 )
		x = -x;

	if( x > T24M1 )
	{
		return 0.0f;
	}

	j = 0.02222222222222222222222f * x; /* integer part of x/PIO4 */
	y = j;
	
	/* integer and fractional part modulo one octant */
	if( j & 1 )	/* map zeros to origin */
	{
		j += 1;
		y += 1.0f;
	}
	
	j &= 7;
	if( j > 3)
	{
		j -=4;
		sign = -sign;
	}

	if( j > 1 )
		sign = -sign;

	x = x - y * 45.0f; /* x mod 45 degrees */
	x *= PI180;	/* multiply by pi/180 to convert to radians */
	z = x * x;

	if( (j==1) || (j==2) )
	{
		y = (((-1.9515295891E-4f  * z
	          + 8.3321608736E-3f) * z
    		  - 1.6666654611E-1f) * z * x) + x;
	}
	else
	{
		y = ((  2.443315711809948E-005f  * z
			  - 1.388731625493765E-003f) * z
			  + 4.166664568298827E-002f) * z * z;
		y -= 0.5f * z;
		y += 1.0f;
	}
	
	if( sign < 0 )
		y = -y;
		
	return y;
}

/*------------------------------------------------*/
/* Circular tangent/cotangent of angle in degrees */
/*------------------------------------------------*/

Float32 xtandg( Float32 x )
{
	return tancotdg(x,0);
}

Float32 xcotdg( Float32 x )
{
	if( x==0.0f )
		return MAXNUMF;
	
	return tancotdg(x,1);
}

static Float32 tancotdg( Float32 xx, Int32 cotlg )
{
	Float32 x, y, z, zz;
	Int32 j;
	Int32 sign;

	/* make argument positive but save the sign */
	if( xx < 0.0f )
	{
		x = -xx;
		sign = -1;
	}
	else
	{
		x = xx;
		sign = 1;
	}

	if( x > T24M1 )
		return 0.0f;

	/* compute x mod PIO4 */
	j = 0.022222222222222222222f * x; /* integer part of x/45 */
	y = j;

	/* map zeros and singularities to origin */
	if( j & 1 )
	{
		j += 1;
		y += 1.0f;
	}

	z = x - y * 45.0f;
	z *= PI180;	/* multiply by pi/180 to convert to radians */
	zz = z * z;

	if( x > 1.0e-4 )
	{
		y = ((((( 9.38540185543E-3f * zz
			+ 3.11992232697E-3f)    * zz
			+ 2.44301354525E-2f)    * zz
			+ 5.34112807005E-2f)    * zz
			+ 1.33387994085E-1f)    * zz
			+ 3.33331568548E-1f)    * zz * z
			+ z;
	}
	else
	{
		y = z;
	}

	if( j & 2 )
	{
		if( cotlg )
			y = -y;
		else
		{
			if( y != 0.0f )
				y = -1.0f/y;
			else
				y = MAXNUMF;
		}
	}
	else
	{
		if( cotlg )
		{
			if( y != 0.0f )
				y = 1.0f/y;
			else
				y = MAXNUMF;
		}
	}

	if( sign < 0 )
		y = -y;

	return y;
}

/*-----------------------------------------*/
/* Smallest integral value not less than x */
/*-----------------------------------------*/

Float32 xceil(Float32 x)
{
	Float32 y;

	y = xfloor(x);
	
	if( y < x )
		y += 1.0f;

	return y;
}

/*-------------------------------------------*/
/* Largest integral value not greater than x */
/*-------------------------------------------*/

/* Bit clearing masks: */
static UInt16 bmask[] = {
	0xffff, 0xfffe, 0xfffc, 0xfff8, 0xfff0,
	0xffe0, 0xffc0, 0xff80, 0xff00, 0xfe00,
	0xfc00, 0xf800, 0xf000, 0xe000, 0xc000,
	0x8000, 0x0000
};

Float32 xfloor(Float32 x)
{
	UInt16 *p;
	Float32 y;
	Int32 e;

	y = x;

	/* find the exponent (power of 2) */
	p = (unsigned short *)&y + 1;
	e = (( *p >> 7) & 0xff) - 0x7f;
	p -= 1;

	if( e < 0 )
	{
		if( y < 0 )
			return -1.0f;
		else
			return 0.0f;
	}

	e = (24-1) - e;
	
	/* clean out 16 bits at a time */
	while( e >= 16 )
	{
		*p++ = 0;
		e -= 16;
	}

	/* clear the remaining bits */
	if( e > 0 )
		*p &= bmask[e];

	if( (x<0) && (y!=x) )
		y -= 1.0f;

	return y;
}

/*------------------------------*/
/* Returns the square root of x */
/*------------------------------*/

Float32 xsqrt( Float32 xx )
{
	Int32 e;
	Float32 f,x,y;

	f = xx;
	if( f <= 0.0f )
	{
		if( f < 0.0f )
			return 0.0f;
	}

	x = frexp( f, &e );	/* f = x * 2**e,   0.5 <= x < 1.0 */
	
	/* If power of 2 is odd, double x and decrement the power of 2. */
	if( e & 1 )
	{
		x = x + x;
		e -= 1;
	}

	e >>= 1;	/* The power of 2 of the square root. */

	if( x > 1.41421356237f )
	{
		/* x is between sqrt(2) and 2. */
		x = x - 2.0f;
		y = ((((( -9.8843065718E-4f * x
			  + 7.9479950957E-4f) * x
			  - 3.5890535377E-3f) * x
			  + 1.1028809744E-2f) * x
			  - 4.4195203560E-2f) * x
			  + 3.5355338194E-1f) * x
			  + 1.41421356237E0f;
		goto sqdon;
	}

	if( x > 0.707106781187f )
	{
		/* x is between sqrt(2)/2 and sqrt(2). */
		x = x - 1.0f;
		y = ((((( 1.35199291026E-2f * x
			  - 2.26657767832E-2f) * x
			  + 2.78720776889E-2f) * x
			  - 3.89582788321E-2f) * x
			  + 6.24811144548E-2f) * x
			  - 1.25001503933E-1f) * x * x
			  + 0.5f * x
			  + 1.0f;
		goto sqdon;
	}

	/* x is between 0.5 and sqrt(2)/2. */
	x = x - 0.5f;
	y = ((((( -3.9495006054E-1f * x
		  + 5.1743034569E-1f) * x
		  - 4.3214437330E-1f) * x
		  + 3.5310730460E-1f) * x
		  - 3.5354581892E-1f) * x
		  + 7.0710676017E-1f) * x
		  + 7.07106781187E-1f;

sqdon:
	y = ldexp(y,e);  /* y = y * 2**e */
	
	return y;
}

static Float32 frexp(Float32 x,Int32 *pw2)
{
	Float32 y;
	Int32 i, k;
	Int16 *q;

	y = x;
	q = (short*)&y+1;

	/* find the exponent (power of 2) */
	i  = ( *q >> 7) & 0xff;
	if( i==0 )
	{
		if( y==0.0f )
		{
			*pw2 = 0;
			return 0.0f;
		}
		
		/* Number is denormal or zero */
		/* Handle denormal number.    */
		do
		{
			y *= 2.0f;
			i -= 1;
			k  = ( *q >> 7) & 0xff;
		}
		while( k==0 );
		i = i + k;
	}
	
	i -= 0x7e;
	*pw2 = i;
	*q &= 0x807f;	/* strip all exponent bits */
	*q |= 0x3f00;	/* mantissa between 0.5 and 1 */
	
	return y;
}

static Float32 ldexp( Float32 x, Int32 pw2 )
{
	Float32 y;
	Int16 *q;
	Int32 e;

	y = x;

	q = (short*)&y + 1;

	while( (e = ( *q >> 7) & 0xff) == 0 )
	{
		if( y==0.0f )
			return 0.0f;
			
		/* Input is denormal. */
		if( pw2 > 0 )
		{
			y *= 2.0f;
			pw2 -= 1;
		}
		
		if( pw2 < 0 )
		{
			if( pw2 < -24 )
				return 0.0f;
				
			y *= 0.5f;
			pw2 += 1;
		}
		
		if( pw2==0 )
			return y;
	}

	e += pw2;

	/* Handle overflow */
	if( e > 255 )
		return MAXNUMF;

	*q &= 0x807f;

	/* Handle denormalized results */
	if( e < 1 )
	{
		if( e < -24 )
			return 0.0f;
			
		*q |= 0x80;
	
		while( e < 1 )
		{
			y *= 0.5f;
			e += 1;
		}
		e = 0;
	}
	
	*q |= (e & 0xff) << 7;
	
	return y;
}
