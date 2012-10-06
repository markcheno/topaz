/* xmath.h */
#ifndef XMATH_H
#define XMATH_H

#define xfabs(x) ( (x) < 0.0f ? -(x) : (x) )

/*------------------*/
/* Public Interface */
/*------------------*/

Float32 xsindg(Float32 xx) xmath_sect;
Float32 xcosdg(Float32 xx) xmath_sect;
Float32 xtandg(Float32 x)  xmath_sect;
Float32 xcotdg(Float32 x)  xmath_sect;
Float32 xfloor(Float32 x)  xmath_sect;
Float32 xceil(Float32 x)   xmath_sect;
Float32 xsqrt(Float32 xx)  xmath_sect;

#endif
