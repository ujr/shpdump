/* shapefile.h - definitions for ESRI shapefiles */

#ifndef _SHAPEFILE_H_
#define _SHAPEFILE_H_

#include <limits.h>

#define SHAPE_SUFFIX ".shp"    /* suffix for shape file (counties.shp) */
#define INDEX_SUFFIX ".shx"    /* suffix for index file (counties.shx) */
#define DBASE_SUFFIX ".dbf"    /* suffix for dBase file (counties.dbf) */

#define SHP_MAGIC 9994         /* magic file code */

#define SHP_TYPE_NULL          0   /* no geometric data */

#define SHP_TYPE_POINT         1   /* shape types in X,Y space */
#define SHP_TYPE_POLYLINE      3
#define SHP_TYPE_POLYGON       5
#define SHP_TYPE_MULTIPOINT    8

#define SHP_TYPE_POINTZ       11   /* shape types in X,Y,Z space */
#define SHP_TYPE_POLYLINEZ    13   /*  (they always include M) */
#define SHP_TYPE_POLYGONZ     15
#define SHP_TYPE_MULTIPOINTZ  18

#define SHP_TYPE_POINTM       21   /* measured shape types in X,Y */
#define SHP_TYPE_POLYLINEM    23
#define SHP_TYPE_POLYGONM     25
#define SHP_TYPE_MULTIPOINTM  28

#define SHP_TYPE_MULTIPATCH   31   /* surface patches */

#if INT_MAX == 0x7FFFFFFF
  #define INT32 int
  #define FINT "%d"
#elif LONG_MAX == 0x7FFFFFFF
  #define INT32 long
  #define FINT "%ld"
#else
  #error "Unsupported word size"
#endif

typedef INT32 Integer;  /* signed 32-bit integer (2's complement) */
typedef double Double;  /* IEEE double-prec (64 bit) floating point num */

typedef struct {
    Double  xmin, ymin;
    Double  xmax, ymax;
} BoundingBox;

typedef struct {
    Double  x, y;
} Point;

/*
 * The Index File (*.shx) consists of the same 100-byte header
 * as the main file (but the file length is the index file length
 * in 16-bit words) followed by fixed-length (8-byte) records.
 * Each index file record stores the offset and length of the
 * variable-size records in the main file.
 */

typedef struct {               /* record in the index file (.shx) */
    Integer  offset;           /* BIG, into main file, in 16-bit words (!) */
    Integer  contentLength;    /* BIG, same as in main file record header */
} IndexRecord;

typedef struct MainHeader {    /* main file header for .shp & .shx */
    Integer  fileCode;         /* BIG, always 9994 dec */
    Integer  unused[5];        /* BIG, always 0 dec */
    Integer  fileLength;       /* BIG, files length in 16-bit words (!) */
    Integer  version;          /* LITTLE, currently 1000 dec */
    Integer  shapeType;        /* LITTLE, type of shapes in file */
    BoundingBox bbox;          /* LITTLE, bounding box of all shapes */
    Double   zmin, zmax;       /* LITTLE, range of all Z values */
    Double   mmin, mmax;       /* LITTLE, range of all M values */
} MainHeader;

typedef struct {               /* precedes each shape in .shp */
    Integer  recordNumber;     /* BIG, record numbers begin at 1 */
    Integer  contentLength;    /* BIG, record's lengths in 16-bit words */
    Integer  shapeType;        /* LITTLE, already part of record's content! */
} RecordHeader;

typedef struct { /* ShapeType = TypePoint = 1 */
    Point  point;         /* LITTLE */
} ShapePoint;

typedef struct { /* ShapeType = TypeMultiPoint = 8 */
    BoundingBox bbox;     /* LITTLE, minimum enclosing rectangle */
    Integer numPoints;    /* LITTLE, number of points in array */
    Point *points;        /* LITTLE, array of points */
} ShapeMultiPoint;

typedef struct { /* ShapeType = TypePolyLine = 3 */
    BoundingBox bbox;     /* LITTLE, minimum enclosing rectangle */
    Integer  numParts;    /* LITTLE, number of parts */
    Integer  numPoints;   /* LITTLE, number of points */
    Integer  *parts;      /* LITTLE, 'numParts' indices into 'points' */
    Point    *points;     /* LITTLE, 'numPoints' points */
} ShapePolyLine;

typedef struct { /* ShapeType = TypePolygon = 5 */
    BoundingBox bbox;     /* LITTLE, minimum enclosing rectangle */
    Integer  numParts;    /* LITTLE, number of rings */
    Integer  numPoints;   /* LITTLE, number of points */
    Integer  *parts;      /* LITTLE, 'numParts' indices into 'points' */
    Point    *points;     /* LITTLE, 'numPoints' points */
} ShapePolygon;

#endif /* _SHAPEFILE_H_ */
