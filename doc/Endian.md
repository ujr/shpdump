
Endian (byte ordering)
======================

Memory is addressed on a byte-by-byte basis:
we may write `mem[i]` to denote the `i`th byte of
some memory area such as main memory or a disk file.
Values larger than one byte, like those of type
`long` or `float` in the C language, are called
multi-byte values. They raise the question how
to order those bytes in memory.

There are two common **byte ordering** conventions:

**Big-endian** stores the most significant
byte of a multi-byte value in `mem[i]`, the
second most significant byte in `mem[i+1]`,
and so on; that is, the “big end” comes first.

**Little-endian** store the least significant
byte of a multi-byte value in `mem[i]`, the
second least significant byte in `mem[i+1]`,
and so on; that is, the “little end” comes first.

Byte ordering is a property of processors, but
also of file formats. For example, Intel processors
use little-endian when storing a register in memory;
PowerPC processors use big-endian. The TIFF image
file format comes in two flavours: one uses little-endian,
the other uses big-endian. Plain ASCII files are
not affected by byte ordering, because each character
occupies only one byte.


Determining byte ordering at run-time
-------------------------------------

The code in *endian.c* shows how the host processor's
byte-ordering can be determined at run-time.


Byte-order invariant code
-------------------------

It is possible to write code for reading and writing memory
such that it is independent of the host machine's byte ordering.
This works by explicitly writing the individual bytes
of a multi-byte value. For example:

```C
unsigned long getintbig(void)
{
    unsigned long value;
    unsigned char bytes[4];
    int i;

    for (i = 0; i < 4; i++)
        bytes[i] = getbyte();

    value  = bytes[0]; value <<= 8;
    value += bytes[1]; value <<= 8;
    value += bytes[2]; value <<= 8;
    value += bytes[3];

    return value;
}
```


Network byte order
------------------

This is a synonym for big-endian, because the
widely-used TCP/IP protocol stack uses big-endian
for all multi-byte values, such as IP addresses
(32 bits) and port numbers (16 bits).

The header `<netinet/in.h>` provides four routines
to convert between the host machine's byte ordering
and network byte order. They are called `htonl`,
`htons`, `ntohl`, and `ntohs`, where `h` stands for
host, `n` for network, `l` for unsigned long int,
and `s` for unsigned short int. They are usually
implemented as macros that expand to no-ops on
big-endian machines and some byte-reversing on
little-endian machines, that is, something like

```C
#define htons(x) ((((unsigned short)(x) & 0xff00) >> 8) | \
                   ((unsigned short)(x) & 0x00ff) << 8))
```
