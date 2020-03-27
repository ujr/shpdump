
Shapefile Dumper
================

Command line utility to dump Esri Shapefiles in human-readable
format. Written a long time ago to peek into those files.


Esri Shapefiles
---------------

Shapefiles are a file format for storing vector GIS data,
developed by Esri around 1990 for their ArcView 2.x software.
A shapefile consists of several files, all with the same
basename, but different extensions. The .shp (shape data),
.shx (index), .dbf (attribute data) files are mandatory,
others are optional (e.g., .prj with spatial reference
information).

Shapefiles are widely used, mainly because of Esri's public
specification in the [Shapefile Technical Description][shp]
White Paper ([local copy][local]).

More information can be found, as usual, at [Wikipedia][wiki].
Note that, somewhat weirdly, shapefiles (.shp) use a mixture
of big- and little-endian [byte ordering](./doc/Endian.md).


The shpdump tool
----------------

The command line tool here was written a long time ago
to dump shapefiles to a human-readable format. It only
deals with the shape file proper (.shp), not with any
of the other files, and it handles only a common subset
of all shape types.

### Building

A plain `make` in the top level directory should do.

### Usage

**shpdump** \[-V] \[-p *prec*] \[-ghvx] \[*shapefile*]

Read from stdin or the file given on the command line a shapefile
and dump it to stdout in a plain text representation that is easy
to read for humans and machines.  Any complaints go to stderr.

Options:

    -V  identify program and version to stdout and exit 0  
    -g  dump in Arc GENERATE format  
    -h  header only: quit after dump of shapefile header  
    -v  verbose: dump more stuff about the shapefile  
    -x  report inconsistencies in the shapefile to stderr  
    -p  use given precision (digits after decimal point; deflt 2)

Exit codes:

      0  ok
      1  invalid (with -x only) or not fully supported shapefile
    111  temporary error, e.g. troubles reading or writing
    127  permanent error, e.g. invalid command line args

### Miscellaneous

Please refer to the documentation in [doc/shpdump.html](./doc/shpdump.html).

The file [src/shpdump.pl](src/shpdump.pl) contains
a simple Perl CGI frontend.

The links in the documentation and in the Perl script
probably no longer exist.

The dot files have recently been added before publishing to GitHub.


[shp]: https://www.esri.com/library/whitepapers/pdfs/shapefile.pdf
[local]: ./doc/shapefile.pdf
[wiki]: https://en.wikipedia.org/wiki/Shapefile
