/* endian.c | 2004 U.J.Ruetschi | GPL */

#include "endian.h"
#include <errno.h>

/* Assumes 32-bit unsigned ints */

int getendian(void)
{
  unsigned char mem[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };

  errno = 0;

  if (*((unsigned *) mem) == 0x01020304) return ENDIAN_BIG;
  if (*((unsigned *) mem) == 0x04030201) return ENDIAN_LITTLE;

  return 0;  /* unknown byte order or no 32-bit ints */
}

#ifdef TEST
#include <stdio.h>
int main(void)
{
  switch (getendian()) {
  	case ENDIAN_LITTLE: puts("little");  break;
  	case ENDIAN_BIG:    puts("big");     break;
  	default:            puts("unknown"); break;
  }
  return 0;
}
#endif
