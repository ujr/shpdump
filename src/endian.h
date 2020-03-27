/* endian.h | 2004 U.J.Ruetschi | GPL */

#ifndef _ENDIAN_H_
#define _ENDIAN_H_

#define ENDIAN_BIG     1   /* big endian: msb at lowest mem addr */
#define ENDIAN_LITTLE  2   /* little endian: lsb at lowest mem addr */

extern int getendian(void);  /* return 0 if byte order unknown */

#endif /* _ENDIAN_H_ */
