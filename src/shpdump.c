/* shpdump - dump an ESRI shapefile as plain text
 *
 * Copyright (c) 2004-2008 by Urs-Jakob Ruetschi.
 * Licensed under the terms of the GNU General Public License.
 *
 * Usage: shpdump [-V] [-p prec] [-ghvx] [shapefile]
 *
 * Read from stdin or the file given on the command line a shapefile
 * and dump it to stdout in a plain text representation that is easy
 * to read for humans and machines.  Any complaints go to stderr.
 *
 * Options:
 *
 *   -V  identify program and version to stdout and exit 0
 *   -g  dump in Arc GENERATE format
 *   -h  header only: quit after dump of shapefile header
 *   -v  verbose: dump more stuff about the shapefile
 *   -x  report inconsistencies in the shapefile to stderr
 *   -p  use given precision (digits after decimal point; deflt 2)
 *
 * Exit codes:
 *
 *     0  ok
 *     1  invalid (with -x only) or not fully supported shapefile
 *   111  temporary error, e.g. troubles reading or writing
 *   127  permanent error, e.g. invalid command line args
 *
 * ujr/2004-03-20 started.
 * ujr/2005-08-02 applied patch by David Benbennick, maintenance.
 * ujr/2007-06-12 added support for PolyLineZ shapes.
 * ujr/2008-08-27 improved code readability in dumpshape();
 *                shptype() now names all known shape types.
 */

static char id[] = "shpdump by ujr/2008-07-27\n";
static char usage[] = "Usage: shpdump [-V] [-p prec] [-ghvx] [shapefile]\n";

#define FAILSOFT 111  /* temporary error */
#define FAILHARD 127  /* permanent error */

#include <assert.h>
#include <errno.h>
#include <float.h>   /* DBL_MIN, DBL_MAX */
#include <stdio.h>
#include <stdlib.h>  /* calloc, free, atoi */
#include <string.h>
#include <unistd.h>  /* getopt if _POSIX_C_SOURCE >= 2 */

#include "endian.h"
#include "shapefile.h"

int header(void);            /* parse and dump header, return shape type */
int dumpshape(void);         /* dump next shape, return type */
char *shptype(int type);     /* translate shape code to description */
void dumppoint(Integer id);
void dumpline(Integer id);
void dumppolygon(Integer id);
void dumpmultipoint(Integer id);
void dumplinez(Integer id);

Integer getint(void);        /* read integer, little endian */
Integer getintbig(void);     /* read integer, big endian */
Double getdouble(void);      /* read double, little endian */
Point getpoint(void);        /* read two doubles, little endian */

void putint(const char *label, Integer value);
void putname(const char *label, const char *name);
void putrange(const char *label, Double min, Double max);
void putbbox(Double xmin, Double ymin, Double xmax, Double ymax);
void putpoint(Double xcoord, Double ycoord, int gflag);
void putpointz(Double x, Double y, Double z, Double m, int gflag);

void bboxinit(BoundingBox *bbox);  /* make empty bbox */
void bboxadd(BoundingBox *bbox, Double xcoord, Double ycoord);
int bboxok(BoundingBox *, Double xmin, Double ymin, Double xmax, Double ymax);
#define bboxok(bb, minx, miny, maxx, maxy) \
	((bb)->xmin == (minx) && (bb)->ymin == (miny) && \
	 (bb)->xmax == (maxx) && (bb)->ymax == (maxy))

/* I/O helpers: put... to stdout, log... to stderr */
#define setin(s) ((freopen((s), "rb", stdin) == NULL) ? -1 : 0)
#define getbyte() getchar()
#define putstr(s) fputs(s, stdout)
static void logstr(const char *s);
static void logline(const char *s);
static void logbuf(const char *s, size_t len);
static int shipout(FILE *fp, const char *buf, size_t len);

/* Reporting the unexpected */
void die(int code, const char *info);
void warn(const char *info);
#define usage(x) do { logline(usage); errno=0; die(FAILHARD, (x)); } while (0)

int endian;
int vflag=0, gflag=0, hflag=0, xflag=0, prec=2;
unsigned long length, tally;  /* in 16-bit words */
unsigned long warnings=0;
BoundingBox headerbbox, actualbbox;

int main(int argc, char *argv[])
{
  extern int optind, opterr;
  extern char *optarg;
  int c, type; /* of shapefile */

  opterr = 0;
  while ((c=getopt(argc, argv, "gGhHp:vVxX")) > 0) switch (c) {
  	case 'g': gflag = 1; break;  /* GENERATE format */
  	case 'G': gflag = 0; break;
  	case 'h': hflag = 1; break;  /* header only */
  	case 'H': hflag = 0; break;
  	case 'x': xflag = 1; break;  /* report inconsistencies */
  	case 'X': xflag = 0; break;
  	case 'p': prec = atoi(optarg); if (prec < 0) prec = 0; break;
  	case 'v': vflag += 1; break;  /* verbose */
  	case 'V': putstr(id); return 0;
  	default:  usage("invalid option");
  }
  argc -= optind;
  argv += optind;

  assert(sizeof(Integer) == 4);
  assert(sizeof(Double) == 8);

  if (argc > 1) usage("too many arguments");
  if (argc > 0 && *argv) {
    char buf[256];
    const char *filename;
  	const char *p = strrchr(*argv, '/');
  	const char *q = strrchr(*argv, '.');
  	if (!q || (p && (q < p))) { /* append suffix */
  		size_t len = strlen(*argv);
  		if (len > sizeof(buf) - 5) die(FAILHARD, "filename too long");
  		strcpy(buf, *argv); strcpy(buf+len, ".shp");
      filename = buf;
  	}
  	else filename = *argv;
  	if (setin(filename) < 0) die(FAILHARD, filename);
  	if (vflag > 1) putname("filename", filename);
  }

  endian = getendian();
  switch (endian) { char *p;  /* weird but legal */
  	case ENDIAN_LITTLE: p = "little"; goto goon;
  	case ENDIAN_BIG: p = "big"; goto goon;
  	default: die(FAILHARD, "unknown machine byte order");
  	goon: if (vflag > 1) putname("endian", p);
  }

  type = header();
  if (hflag) return 0; /* header only */
  bboxinit(&actualbbox);
  switch (type) {
  	case SHP_TYPE_NULL:
  	case SHP_TYPE_POINT:
  	case SHP_TYPE_POLYLINE:
  	case SHP_TYPE_POLYGON:
  	case SHP_TYPE_POLYLINEZ:
  		break;
  	default: warn("type not supported, just scanning");
  }

  while (tally < length) {
    int shape = dumpshape();
  	if (xflag && (shape != type) && (shape != SHP_TYPE_NULL))
  		warn("unexpected shape type");
  }
  if (gflag) printf("END\n");  /* last line in GENERATE file */
  if (xflag) {
  	if (tally != length) warn("inconsistent file");
  	if (!bboxok(&headerbbox, actualbbox.xmin, actualbbox.ymin,
  	                         actualbbox.xmax, actualbbox.ymax))
  		warn("invalid global bounding box (xrange/yrange)");
  }

  return (xflag && warnings > 0) ? 1 : 0;
}

int header(void)
{
  Integer magic, version, type;
  Double minX, maxX, minY, maxY, minZ, maxZ, minM, maxM;
  int vngflag = (vflag && !gflag);

  magic = getintbig();
  (void) getintbig();  /* unused */
  (void) getintbig();  /* unused */
  (void) getintbig();  /* unused */
  (void) getintbig();  /* unused */
  (void) getintbig();  /* unused */
  length = getintbig();
  version = getint();
  type = getint();
  minX = getdouble();  headerbbox.xmin = minX;
  minY = getdouble();  headerbbox.ymin = minY;
  maxX = getdouble();  headerbbox.xmax = maxX;
  maxY = getdouble();  headerbbox.ymax = maxY;
  minZ = getdouble();
  maxZ = getdouble();
  minM = getdouble();
  maxM = getdouble();

  if (vngflag) putint("magic", magic);
  if (xflag && (magic != SHP_MAGIC)) warn("invalid file code");

  if (vngflag) putint("version", version);
  if (xflag && (version != 1000)) warn("unknown file version");

  if (vngflag) putint("length", length*2); /* in bytes */

  if (!gflag) putname("type", shptype(type));

  if (vngflag) putrange("xrange", minX, maxX);
  if (xflag && (minX > maxX)) warn("global xrange has min > max");

  if (vngflag) putrange("yrange", minY, maxY);
  if (xflag && (minY > maxY)) warn("global yrange has min > max");

  switch (type) {
    case 11: case 13: case 15: case 18: /* shapes in XYZ space with M */
  	if (vngflag) putrange("zrange", minZ, maxZ);
  	if (xflag && (minZ > maxZ)) warn("global zrange has min > max");
  	/* FALLTHRU */
    case 21: case 23: case 25: case 28: /* measured shapes in XY space */
  	if (vngflag) putrange("mrange", minM, maxM);
  	if (xflag && (minM > maxM)) warn("global mrange has min > max");
  	break;
  }

  tally = 100/2;  /* number of 16-bit words handled */
  return (int) type;
}

int dumpshape(void)
{
  Integer recnum = getintbig();  /* record number */
  Integer reclen = getintbig();  /* record length in 16-bit words */
  Integer type = getint();  /* record type, 0=Null, 1=Point, etc */

  tally += 4;  /* record header size in words */
  tally += reclen;  /* record contents in words */

  reclen *= 2;  /* convert to bytes */
  reclen -= sizeof(Integer);  /* type already read */

  switch (type) {
  	case SHP_TYPE_NULL: printf("null " FINT "\n", recnum); break;
  	case SHP_TYPE_POINT: dumppoint(recnum); break;
  	case SHP_TYPE_POLYLINE: dumpline(recnum); break;
  	case SHP_TYPE_POLYGON: dumppolygon(recnum); break;
  	case SHP_TYPE_POLYLINEZ: dumplinez(recnum); break;
  	default: printf("shape "FINT" type "FINT" bytes "FINT"\n", recnum, type, reclen);
  	         while (--reclen > 0) (void) getbyte(); /* skip record */
  }

  return type;
}

void dumppoint(Integer id)
{
  Double xcoord = getdouble();
  Double ycoord = getdouble();

  if (gflag) printf(FINT",", id);
  else printf("point "FINT, id);
  putpoint(xcoord, ycoord, gflag);
  bboxadd(&actualbbox, xcoord, ycoord);
}

void dumpline(Integer id)
{
  BoundingBox bbox;
  Double xmin, xmax;
  Double ymin, ymax;
  Integer nparts, npoints;
  Integer *parts;
  Point *points;
  Integer i, j;

  xmin = getdouble();
  ymin = getdouble();
  xmax = getdouble();
  ymax = getdouble();

  nparts = getint();
  npoints = getint();

  parts = (Integer *) calloc(nparts, sizeof(Integer));
  if (parts == NULL) die(FAILSOFT, "out of memory");
  for (i = 0; i < nparts; i++) parts[i] = getint();

  points = (Point *) calloc(npoints, sizeof(Point));
  if (points == NULL) die(FAILSOFT, "out of memory");
  for (i = 0; i < npoints; i++) points[i] = getpoint();

  bboxinit(&bbox);
  if (gflag) printf(FINT"\n", id);
  else printf("line "FINT" parts "FINT" points "FINT"\n", id, nparts, npoints);

  for (i = 0, j = 1; i < npoints; i++) {
  	if ((j < nparts) && (i == parts[j])) { ++j;
  		printf("part\n");
  	}
  	putpoint(points[i].x, points[i].y, gflag);
  	bboxadd(&bbox, points[i].x, points[i].y);
  }

  free(points);
  free(parts);

  if (gflag) printf("END\n");
  else if (vflag) putbbox(xmin, ymin, xmax, ymax);

  if (xflag && !bboxok(&bbox, xmin, ymin, xmax, ymax))
  	warn("invalid bbox");
  bboxadd(&actualbbox, bbox.xmin, bbox.ymin);
  bboxadd(&actualbbox, bbox.xmax, bbox.ymax);
}

void dumplinez(Integer id)
{
  BoundingBox bbox;
  Double xmin, xmax;
  Double ymin, ymax;
  Double zmin, zmax;
  Double mmin, mmax;
  Integer nparts, npoints;
  Integer *parts;
  Point *points;
  Double *zvalues;
  Double *mvalues;
  Integer i, j;

  xmin = getdouble();
  ymin = getdouble();
  xmax = getdouble();
  ymax = getdouble();

  nparts = getint();
  npoints = getint();

  parts = (Integer *) calloc(nparts, sizeof(Integer));
  if (parts == NULL) die(FAILSOFT, "out of memory");
  for (i = 0; i < nparts; i++) parts[i] = getint();

  points = (Point *) calloc(npoints, sizeof(Point));
  if (points == NULL) die(FAILSOFT, "out of memory");
  for (i = 0; i < npoints; i++) points[i] = getpoint();

  zmin = getdouble();
  zmax = getdouble();

  zvalues = (Double *) calloc(npoints, sizeof(Double));
  if (zvalues == NULL) die(FAILSOFT, "out of memory");
  for (i = 0; i < npoints; i++) zvalues[i] = getdouble();

  /* Note: Shapes with Z always include M */

  mmin = getdouble();
  mmax = getdouble();

  mvalues = (Double *) calloc(npoints, sizeof(Double));
  if (mvalues == NULL) die(FAILSOFT, "out of memory");
  for (i = 0; i < npoints; i++) mvalues[i] = getdouble();

  bboxinit(&bbox);
  if (gflag) printf(FINT"\n", id);
  else printf("line "FINT" parts "FINT" points "FINT"\n", id, nparts, npoints);

  for (i = 0, j = 1; i < npoints; i++) {
  	if ((j < nparts) && (i == parts[j])) { ++j;
  		printf("part\n");
  	}
  	putpointz(points[i].x, points[i].y, zvalues[i], mvalues[i], gflag);
  	bboxadd(&bbox, points[i].x, points[i].y);
  }

  free(mvalues);
  free(zvalues);
  free(points);
  free(parts);

  if (gflag) printf("END\n");
  else if (vflag) {
  	putbbox(xmin, ymin, xmax, ymax);
  	putrange("zrange", zmin, zmax);
  	putrange("mrange", mmin, mmax);
  }
  if (xflag && !bboxok(&bbox, xmin, ymin, xmax, ymax))
  	warn("invalid bbox");
  bboxadd(&actualbbox, bbox.xmin, bbox.ymin);
  bboxadd(&actualbbox, bbox.xmax, bbox.ymax);
}

void dumppolygon(Integer id)
{
  BoundingBox bbox;
  Double xmin, xmax;
  Double ymin, ymax;
  Integer nparts, npoints;
  Integer *parts;
  Point *points;
  Integer i, j;

  xmin = getdouble();
  ymin = getdouble();
  xmax = getdouble();
  ymax = getdouble();

  nparts = getint();
  npoints = getint();

  parts = (Integer *) calloc(nparts, sizeof(Integer));
  if (parts == NULL) die(FAILSOFT, "out of memory");
  for (i = 0; i < nparts; i++) parts[i] = getint();

  points = (Point *) calloc(npoints, sizeof(Point));
  if (points == NULL) die(FAILSOFT, "out of memory");
  for (i = 0; i < npoints; i++) points[i] = getpoint();

  bboxinit(&bbox);
  if (gflag) printf(FINT"\n", id);
  else printf("polygon "FINT" parts "FINT" points "FINT"\n", id, nparts, npoints);

  for (i = 0, j = 1; i < npoints; i++) {
  	if ((j < nparts) && (i == parts[j])) { ++j;
  		printf("part\n");
  	}
  	putpoint(points[i].x, points[i].y, gflag);
  	bboxadd(&bbox, points[i].x, points[i].y);
  }

  free(points);
  free(parts);

  if (gflag) printf("END\n");
  else if (vflag) putbbox(xmin, ymin, xmax, ymax);

  if (xflag && !bboxok(&bbox, xmin, ymin, xmax, ymax))
  	warn("invalid bbox");
  bboxadd(&actualbbox, bbox.xmin, bbox.ymin);
  bboxadd(&actualbbox, bbox.xmax, bbox.ymax);
}

void dumpmultipoint(Integer id); /* etc: TODO */

/* die: complain to stderr, then exit code */
void die(int code, const char *info)
{
  fflush(stdout);
  logstr("shpdump: ");
  logstr(info ? (char *) info : "error");
  if (errno) {
  	logstr(": ");
  	logstr(strerror(errno));
  }
  logline(0);
  exit(code);
}

/* warn: write "! info" to stderr, inc counter */
void warn(const char *info)
{
  assert(info);
  fflush(stdout);
  logstr("! ");
  logline((char *) info);
  warnings++;
}

void bboxinit(BoundingBox *bbox)
{
  assert(bbox);

  bbox->xmin = bbox->ymin = DBL_MAX;  /* XXX +1.0/0.0 */
  bbox->xmax = bbox->ymax = DBL_MIN;  /* XXX -1.0/0.0 */
}

void bboxadd(BoundingBox *bbox, Double xcoord, Double ycoord)
{
  assert(bbox);

  if (xcoord < bbox->xmin) bbox->xmin = xcoord;
  if (xcoord > bbox->xmax) bbox->xmax = xcoord;

  if (ycoord < bbox->ymin) bbox->ymin = ycoord;
  if (ycoord > bbox->ymax) bbox->ymax = ycoord;
}

/* The only two data types in Shapefiles are Integer and Double.
 * Double is always stored in little endian byte order, Integer
 * occurs in both big and little endian order.
 */

Integer getint(void)
{
  unsigned long value;
  unsigned char bytes[4];
  int byte, i;

  for (i = 0; i < 4; i++) {
  	if ((byte = getbyte()) < 0) { errno = 0;
  		die(FAILHARD, "unexpected end of file");
  	}
  	else bytes[i] = byte;
  }
  /* little endian */
  value  = bytes[3]; value <<= 8;
  value += bytes[2]; value <<= 8;
  value += bytes[1]; value <<= 8;
  value += bytes[0];

  return value;
}

Integer getintbig(void)
{
  unsigned long value;
  unsigned char bytes[4];
  int byte, i;

  for (i = 0; i < 4; i++) {
  	if ((byte = getbyte()) < 0) { errno = 0;
  		die(FAILHARD, "unexpected end of file");
  	}
  	else bytes[i] = byte;
  }
  /* big endian */
  value  = bytes[0]; value <<= 8;
  value += bytes[1]; value <<= 8;
  value += bytes[2]; value <<= 8;
  value += bytes[3];

  return value;
}

Double getdouble(void)
{
  unsigned char bytes[8];
  int byte, i;

  for (i = 0; i < 8; i++) {
  	if ((byte = getbyte()) < 0) { errno = 0;
  		die(FAILHARD, "unexpected end of file");
  	}
  	else switch (endian) {
  		case ENDIAN_BIG: bytes[7-i] = byte; break;
  		case ENDIAN_LITTLE: bytes[i] = byte; break;
  		default: abort();
  	}
  }
  return *(double *) bytes;
}

Point getpoint(void)
{
  Point point;
  point.x = getdouble();
  point.y = getdouble();
  return point;
}

/* Translate numeric shape types to descriptive strings.
 * Shapefiles may only contain shapes of this type and null shapes.
 * Null shapes have attributes in the dBASE file but no geometry.
 */
char *shptype(int type)
{
  switch (type) {
    case 0:  return "Null";
    case 1:  return "Point";
    case 3:  return "PolyLine";
    case 5:  return "Polygon";
    case 8:  return "MultiPoint";
    case 11: return "PointZ";
    case 13: return "PolyLineZ";
    case 15: return "PolygonZ";
    case 18: return "MultiPointZ";
    case 21: return "PointM";
    case 23: return "PolyLineM";
    case 25: return "PolygonM";
    case 28: return "MultiPointM";
    case 31: return "MultiPatch";
    default: return "Unknown";
  }
}

void putint(const char *label, Integer value)
{
  assert(label);
  printf("%s "FINT"\n", label, value);
}

void putname(const char *label, const char *name)
{
  assert(label);
  printf("%s %s\n", label, name ? name : "(null)");
}

void putrange(const char *label, Double min, Double max)
{
  assert(label);
  printf("%s %.*f %.*f\n", label, prec, min, prec, max);
}

void putbbox(Double xmin, Double ymin, Double xmax, Double ymax)
{
  printf("bbox %.*f %.*f %.*f %.*f\n",
  	prec, xmin, prec, ymin, prec, xmax, prec, ymax);
}

void putpoint(Double xcoord, Double ycoord, int gflag)
{
  if (gflag) printf("%.*f,%.*f\n", prec, xcoord, prec, ycoord);
  else printf(" %.*f %.*f\n", prec, xcoord, prec, ycoord);
}

void putpointz(Double x, Double y, Double z, Double m, int gflag)
{
  if (gflag) printf("%.*f,%.*f,%.*f,%.*f\n", prec,x, prec,y, prec,z, prec,m);
  else printf(" %.*f %.*f z %.*f m %.*f\n", prec,x, prec,y, prec,z, prec,m);
}

/* Logging (unbuffered to stderr) */

static void logstr(const char *s)
{
  if (s) logbuf(s, strlen(s));
}

static void logline(const char *s)
{
  if (s) {
    size_t len = strlen(s);
    logbuf(s, len);
    if (s[len-1] == '\n') return;
  }
  logbuf("\n", 1);
}

static void logbuf(const char *s, size_t len)
{
  /* silently exit on error: we couldn't report */
  if (shipout(stderr, s, len) < 0) exit(FAILHARD);
}

static int shipout(FILE *fp, const char *buf, size_t len)
{
  while (len > 0) { /* may write in chunks */
    size_t r = fwrite(buf, 1, len, fp);
    if (r < len && ferror(fp)) return -1;
    buf += r; len -= r;
  }
  return 0;
}
