/*--- DsAFF.h ---*/

#ifndef __DIALDRAW_H__
#define __DIALDRAW_H__

#ifndef RC_INVOKED
#include <stdio.h>
#include "DsFF9.h"
#endif

#define DRAW_UNITS_PER_INCH 46080L

#define path_term 0L
#define path_move_2 2L
#define path_closeline 5L
#define path_bezier 6L
#define path_lineto 8L

#define draw_OBJFONTLIST 0L
#define draw_OBJTEXT 1L
#define draw_OBJPATH 2L
#define draw_OBJSPRITE 5L
#define draw_OBJGROUP 6L
#define draw_OBJTAGGED 7L
#define draw_OBJTEXTAREA 9L
#define draw_OBJTEXTCOL 10L
#define draw_OBJOPTIONS 11L
#define draw_OBJTEXTROT 12L
#define draw_OBJSPRITEROT 13L
#define draw_OBJWINFONTLIST 0x310L
#define draw_OBJDIB 0x311L
#define draw_OBJDIBROT 0x312L

#define draw_COLOURNONE -1L

typedef struct {
                long x0,y0,x1,y1;
               } draw_box;

typedef struct {
                char title[4];
                long majorstamp;
                long minorstamp;
                char progident[12];
                draw_box   bbox; /* 4 words */
               } draw_fileheader;

typedef long draw_pathwidth;

typedef struct {
                long x,y;
               } draw_objcoord;

typedef draw_objcoord far *HPDRAWCOORD;

typedef struct
{ unsigned char joincapwind;       /* 1 byte  */ /* bit 0..1 join         */
                                                 /* bit 2..3 end cap      */

                                                 /* bit 4..5 start cap    */
                                                 /* bit 6    winding rule */
                                                 /* bit 7    dashed       */
  unsigned char reserved8;         /* 1 byte  */
  unsigned char tricapwid;         /* 1 byte  */ /* 1/16th of line width */
  unsigned char tricaphei;         /* 1 byte  */ /* 1/16th of line width */
} draw_pathstyle;

typedef struct {
                unsigned char join;
                unsigned char lead_cap;
                unsigned char trail_cap;
                unsigned char reserved;
               } draw_capjoinstr;

typedef struct {
                draw_capjoinstr capjoin;          /* 1 word */
                unsigned long mitre_limit;   /* 1 word */
                unsigned long tricap_width;  /* 1 word */
                unsigned long tricap_height; /* 1 word */
               } draw_capjoinspec;

typedef struct
{ long dashstart;       /* 1 word */           /* distance into pattern */
  long dashcount;       /* 1 word */           /* number of elements    */
  long dashelements[6]; /* dashcount words */  /* elements of pattern   */
} draw_dashstr;

typedef long draw_tagtyp;
typedef long draw_sizetyp;
typedef long draw_coltyp;

typedef struct
{ draw_tagtyp    tag;        /* 1 word  */
  draw_sizetyp   size;       /* 1 word  */
  draw_box       bbox;       /* 4 words */
  draw_coltyp    fillcolour; /* 1 word  */
  draw_coltyp    pathcolour; /* 1 word  */
  draw_pathwidth pathwidth;  /* 1 word  */
  draw_pathstyle pathstyle;  /* 1 word  */
} draw_pathstrhdr;

typedef struct
{ draw_tagtyp    tag;        /* 1 word  */
  draw_sizetyp   size;       /* 1 word  */
  draw_box       bbox;       /* 4 words */
  draw_coltyp    fillcolour; /* 1 word  */
  draw_coltyp    pathcolour; /* 1 word  */
  draw_pathwidth pathwidth;  /* 1 word  */
  draw_pathstyle pathstyle;  /* 1 word  */

  draw_dashstr data;      /* optional dash pattern, then path elements */
  long PATH;
} draw_pathstr;

typedef struct
{
 char ch[12];
} draw_groupnametyp;

typedef struct
{ draw_tagtyp       tag;
  draw_sizetyp      size;
  draw_box          bbox;
  draw_groupnametyp name;
} draw_groustr;

typedef unsigned char draw_fontref;    /* 1 byte */
typedef struct
{ draw_fontref fontref;             /* 1 byte  */
  char         reserved8;           /* 1 byte  */
  short        reserved16;          /* 2 bytes */
} draw_textstyle;   /* 1 word */

typedef long draw_fontsize;  /* 4 bytes */

typedef struct
{ draw_tagtyp    tag;        /* 1 word  */
  draw_sizetyp   size;       /* 1 word  */
  draw_box       bbox;       /* 4 words */
  draw_coltyp    textcolour; /* 1 word  */
  draw_coltyp    background; /* 1 word  */
  draw_textstyle textstyle;  /* 1 word  */
  draw_fontsize  fsizex;     /* 1 word, unsigned */
  draw_fontsize  fsizey;     /* 1 word, unsigned */
  draw_objcoord  coord;      /* 2 words */
} draw_textstrhdr;

typedef struct
{ draw_tagtyp    tag;        /* 1 word  */
  draw_sizetyp   size;       /* 1 word  */
  draw_box       bbox;       /* 4 words */
  draw_coltyp    textcolour; /* 1 word  */
  draw_coltyp    background; /* 1 word  */
  draw_textstyle textstyle;  /* 1 word  */
  draw_fontsize  fsizex;     /* 1 word, unsigned */
  draw_fontsize  fsizey;     /* 1 word, unsigned */
  draw_objcoord  coord;      /* 2 words */

  char           text[1];   /* String, null terminated */
} draw_textstr;

typedef unsigned long draw_fontflags;

typedef struct
{
 long a,b,c,d,e,f;
} draw_matrix;

typedef draw_matrix far * LPDRAWMATRIX;
typedef draw_matrix far * HPDRAWMATRIX;

typedef struct
{ draw_tagtyp    tag;        /* 1 word  */
  draw_sizetyp   size;       /* 1 word  */
  draw_box       bbox;       /* 4 words */
  draw_matrix    transform;  /* 6 words */
  draw_fontflags flags;      /* 1 word */
  draw_coltyp    textcolour; /* 1 word  */
  draw_coltyp    background; /* 1 word  */
  draw_textstyle textstyle;  /* 1 word  */
  draw_fontsize  fsizex;     /* 1 word, unsigned */
  draw_fontsize  fsizey;     /* 1 word, unsigned */
  draw_objcoord  coord;      /* 2 words */
} draw_transformedtextstrhdr;

typedef struct
{ draw_tagtyp    tag;        /* 1 word  */
  draw_sizetyp   size;       /* 1 word  */
  draw_box       bbox;       /* 4 words */
  draw_matrix    transform;  /* 6 words */
  draw_fontflags flags;      /* 1 word */
  draw_coltyp    textcolour; /* 1 word  */
  draw_coltyp    background; /* 1 word  */
  draw_textstyle textstyle;  /* 1 word  */
  draw_fontsize  fsizex;     /* 1 word, unsigned */
  draw_fontsize  fsizey;     /* 1 word, unsigned */
  draw_objcoord  coord;      /* 2 words */

  char           text[1];   /* String, null terminated */
} draw_transformedtextstr;

typedef struct
{ draw_tagtyp    tag;       /* 1 word  */
  draw_sizetyp   size;      /* 1 word  */
} draw_fontliststrhdr;

typedef struct
{ draw_tagtyp    tag;       /* 1 word  */
  draw_sizetyp   size;      /* 1 word  */

  draw_fontref   fontref;       /*  check size        */
  char           fontname[1];   /* String, null terminated */
} draw_fontliststr;

#define DRAW_MAXFONTNAME 32
/* this can be cast to a LOGFONT */
typedef struct
{
 int            lfHeight;
 int            lfWidth;
 int            lfEscapement;
 int            lfOrientation;
 int            lfWeight;
 unsigned char  lfItalic;
 unsigned char  lfUnderline;
 unsigned char  lfStrikeOut;
 unsigned char  lfCharSet;
 unsigned char  lfOutPrecision;
 unsigned char  lfClipPrecision;
 unsigned char  lfQuality;
 unsigned char  lfPitchAndFamily;
 char  lfFaceName[DRAW_MAXFONTNAME];
} draw_winfontstr;

typedef struct
{
 int n;
 draw_winfontstr font;
} draw_winfontentry;

typedef struct
{
 draw_tagtyp tag;
 draw_sizetyp size;
 draw_box bbox;
 sprite_header sh;
} draw_spritestr;

typedef struct
{
 draw_tagtyp tag;
 draw_sizetyp size;
 draw_box bbox;
 draw_matrix transform;
 sprite_header sh;
} draw_transformedspritestr;

typedef struct
{
 draw_tagtyp tag;
 draw_sizetyp size;
 draw_box bbox;
 draw_tagtyp tagid;
} draw_tagstr;

typedef struct {
                draw_tagtyp    tag;       /* 1 word  */
                draw_sizetyp   size;      /* 1 word  */
                draw_box       bbox;      /* 4 words */
               } draw_objhdr;

typedef struct {
                HGLOBAL data;
                long    length;
               } draw_diag;

typedef struct {
                long errnum;
                char errmess[252];
               } draw_error;      /* same as RiscOS OS error */

typedef char far * HPDRAWDATA;
typedef draw_objhdr far * HPDRAWOBJHDR;
typedef long far * HPLONG;
typedef draw_pathstr far * HPDRAWPATH;
typedef draw_box far * HPDRAWBOX;
typedef draw_textstr far * HPDRAWTEXT;

#define DRAW_COLOURDITHER 0x0001
#define DRAW_COLOURNEAREST 0x0002
#define DRAW_COLOURFIXED 0x0004
#define DRAW_COLOURNOFILL 0x0008
#define DRAW_COLOURNOEDGE 0x0010
#define DRAW_COLOURINCLUDENONE 0x0020
#define DRAW_COLOURDASHGROUPS 0x0040
#define DRAW_COLOURIGNOREWIDTH 0x0080

#define DRAWOBJ_FIRST -1L
#define DRAWOBJ_LAST -2L

#ifdef DIALDRAW_INTERNAL
/*--- don't reference anything in this section ---*/
#define IDS_OUTOFMEMORY    100
#define IDS_INVALIDBEZIER  101
#define IDS_INVALIDCOMMAND 102

extern HDC draw_dc;
extern draw_matrix draw_identity_matrix;
extern HPDRAWMATRIX draw_transform;
extern HANDLE hInst;
extern unsigned int MetaFile_dpi;
#ifdef DEBUG
extern BOOL dump;
extern FILE *dump_file;
#endif

BOOL Draw_RenderSprite(HDC dc,HPDRAWOBJHDR o,HPDRAWMATRIX t,draw_error far *e);
BOOL Draw_RenderTransformedSprite(HDC dc,HPDRAWOBJHDR o,HPDRAWMATRIX t,draw_error far *e);
BOOL Draw_RenderDIB(HDC dc,HPDRAWOBJHDR o,HPDRAWMATRIX t,draw_error far *e);
void FAR PASCAL DrawPath_RestoreMappingMode(void);
BOOL Draw_SetAFontTable(draw_objhdr far *obj,draw_error far *e);
void FAR PASCAL Draw_UpdateFontMap(HWND parent,draw_diag far *d);
void FAR PASCAL Draw_AddWinFontTable(draw_diag far *d);
void FAR PASCAL DrawText_SetFontTablePointers(HPDRAWDATA data);
#endif

/*---
round: rounds given number to nearest long integer
---*/
long FAR PASCAL round(double r);

/*---
Draw_BoxesOverlap: Returns TRUE if given boxes overlap
                   FALSE if they don't
---*/
BOOL FAR PASCAL Draw_BoxesOverlap(HPDRAWBOX b1,HPDRAWBOX b2);

/*---
Draw_GetDiagBbox: sets 'b' to the bounding box of the given drawfile
---*/
BOOL FAR PASCAL Draw_GetDiagBbox(draw_diag far *d,draw_box far *b);

/*---
Draw_RenderDiag: draws the given drawfile on the given device context
                 r : pointer to rectangle to redraw in device context units
                 t : transformation matrix which maps the drawfile onto the device context
                 e : any returned errors (not properly implemented yet, just use function return value
                     to determine success or not)
---*/
BOOL FAR PASCAL Draw_RenderDiag(draw_diag far *d,HDC dc,LPRECT r,HPDRAWMATRIX t,draw_error far *e);

/*---
Draw_RenderRange: draws the given range of objects on the given device context
                  r,t,e    : as for Draw_RenderDiag
                  from, to : offsets to first and last object to draw
                             -can use DRAWOBJ_FIRST and DRAWOBJ_LAST as arguments
                  Note that no checking is done on 'from' or 'to' so it is up to the caller
                  to make sure they are valid offsets
---*/
BOOL FAR PASCAL Draw_RenderRange(draw_diag far *d,HDC dc,LPRECT r,HPDRAWMATRIX t,long from,long to,draw_error far *e);

/*---
Draw_SetColourMapping called as follows:
 ...(DRAW_COLOURDITHER,(COLORREF)0)  ==> matches colours using dithering
                                         uses colours in drawfile
 ...(DRAW_COLOURNEAREST,(COLORREF)0) ==> matches colours to nearest in palette
                                         uses colours in drawfile
 ...(DRAW_COLOURDITHER | DRAW_COLOURFIXED,RGB(r,g,b))
                                     ==> matches colours using dithering
                                         renders all objects in given colour
 ...(DRAW_COLOURNEAREST | DRAW_COLOURFIXED,RGB(r,g,b))
                                     ==> matches colours to nearest in palette
                                         renders all objects in given colour
 ...({nearest/fixed/dither} | DRAW_COLOURNOFILL,colour)
                                     ==> does not fill paths regardless of what is stored in path
 ...((nearest/fixed/dither} | DRAW_COLOURNOEDGE,colour)
                                     ==> does not draw path outline regardless of what is stored in path
N.B. always call this before calling Draw_RenderDiag or Draw_RenderRange because another
     application may change the mapping
---*/
void FAR PASCAL Draw_SetColourMapping(UINT maptype,COLORREF fixed);

/*---
Draw_MatrixMultiply: result = t1 x t2
                     result can be the same pointer as t1 or t2 with no side effects
---*/
void FAR PASCAL Draw_MatrixMultiply(HPDRAWMATRIX t1,HPDRAWMATRIX t2,HPDRAWMATRIX result);

/*---
Draw_FirstRenderable: returns the offset to the first renderable object in the given drawfile
---*/
BOOL FAR PASCAL Draw_FirstRenderable(draw_diag far *d,long far *offset);

/*---
Draw_RenderDiagInMetaFile : as for Draw_RenderDiag except that a windows metafile is created
---*/
HMETAFILE FAR PASCAL Draw_RenderDiagInMetaFile(draw_diag far *d,LPRECT r,HPDRAWMATRIX t);

/*---
Draw_VerifyDiag : checks for errors in the given drawfile
                  parent : parent window handle
                  rebind : currently ignored
                  AddWinFontTable : if TRUE, adds a windows font table object to the drawfile
                                    (of type draw_OBJWINFONTLIST)
                  UpdateFontMap   : if TRUE, then the user will be asked to select fonts which
                                    have the same characteristics as any risc os fonts which are
                                    not in 'Fontmap.INI'
---*/
BOOL FAR PASCAL Draw_VerifyDiag(HWND parent,draw_diag far *d,BOOL rebind,BOOL AddWinFontTable,BOOL UpdateFontMap,draw_error far *e);

/*---
Draw_ObjectHasBbox : returns TRUE if the object has a bounding box
                     (actually only returns FALSE if the tag is one of draw_OBJFONTLIST,
                      draw_OBJWINFONTLIST, or draw_OBJOPTIONS)
---*/
BOOL FAR PASCAL Draw_ObjectHasBbox(long tag);

typedef struct {
                HGLOBAL buffer;
                void far *ptr;
                long length;
               } draw_buffer;

/*--- flags for allocating from global heap ---*/
#define DRAW_ALLOCFLAGS GMEM_MOVEABLE

/*---
Draw_AllocBuffer : allocates 'size' bytes from the global heap
                   b->buffer : gets set to handle for the allocated memory
                   b->length : gets set to 'size'
                   b->ptr    : gets set to NULL
---*/
BOOL FAR PASCAL Draw_AllocBuffer(draw_buffer far *b,long size);

/*---
Draw_LockBuffer : locks the memory
                  b->ptr : gets set to point to the locked memory
---*/
BOOL FAR PASCAL Draw_LockBuffer(draw_buffer far *b);

/*---
Draw_UnlockBuffer : unlocks the memory
                    does NOT set b->ptr to NULL as we don't know if memory is still being used
                    (lock count is only used for discardable memory)
---*/
void FAR PASCAL Draw_UnlockBuffer(draw_buffer far *b);

/*---
Draw_FreeBuffer : frees the memory
                  b->buffer : gets set to NULL
---*/
void FAR PASCAL Draw_FreeBuffer(draw_buffer far *b);

/*---
Draw_ExtendBuffer : changes the size of memory allocated by 'by'
                    b->buffer : may change
                    b->length : += by
                    b->ptr    : gets reset but I don't think that this will ever change
                                if you are running in protected mode
---*/
BOOL FAR PASCAL Draw_ExtendBuffer(draw_buffer far *b,long by);

/*---
hmemmovedown : huge model version of memmove
               copies 'count' bytes from 'src' to 'dest'
               the 2 blocks of memory can overlap, but only if 'dest' is below 'src'
---*/
void FAR PASCAL hmemmovedown(void far *dest,void far *src,unsigned long count);

/*---
hmemmoveup : as for hmemmovedown
             except that if the 2 blocks of memory overlap, then 'dest' must be above 'src'
---*/
void FAR PASCAL hmemmoveup(void far *dest,void far *src,unsigned long count);

/*---
IsValidHmemmove : returns FALSE if a subsequent call to hmemmoveup or hmemmovedown with
                  the same arguments would cause a general protection fault
---*/
BOOL FAR PASCAL IsValidHmemmove(void far *dest,void far *src,unsigned long count);

/*---
Draw_TransformPoint : transforms 'in' to 'out' using 't'
                      'in' and 'out' can be the same pointer with no side effects
---*/
void FAR PASCAL Draw_TransformPoint(HPDRAWCOORD in,HPDRAWCOORD out,HPDRAWMATRIX t);

/*---
Draw_TransformPoints: as for Draw_TransformPoint except transforms n points
---*/
void FAR PASCAL Draw_TransformPoints(HPDRAWCOORD in,HPDRAWCOORD out,HPDRAWMATRIX t,long n);

/*---
Draw_TransformX : as for Draw_TransformPoint except that only in->x is transformed
                  (useful for drawing grids)
---*/
void FAR PASCAL Draw_TransformX(HPDRAWCOORD in,HPDRAWCOORD out,HPDRAWMATRIX t);

/*---
Draw_TransformY : as for Draw_TransformPoint except that only in->y is transformed
---*/
void FAR PASCAL Draw_TransformY(HPDRAWCOORD in,HPDRAWCOORD out,HPDRAWMATRIX t);

/*---
Draw_Colour : maps given draw colour to a COLORREF as defined by previous call to Draw_SetColourMapping

Draw_ColourDither, Draw_ColourNearest, and Draw_ColourFixed
do not call these functions, use a call to Draw_Colour instead
---*/
extern COLORREF (FAR PASCAL *Draw_Colour)(draw_coltyp colour);

/*---
Draw_ColourDither : converts given draw colour to a COLORREF
---*/
COLORREF FAR PASCAL Draw_ColourDither(draw_coltyp colour);

/*---
Draw_ColourNearest : converts given draw colour to the nearest colour in the current device context
---*/
COLORREF FAR PASCAL Draw_ColourNearest(draw_coltyp colour);

/*---
Draw_ColourFixed : ignores given colour and sets colour to that specified in last call to Draw_SetColourMapping
---*/
COLORREF FAR PASCAL Draw_ColourFixed(draw_coltyp colour);

/*---
ColorRefToDraw : converts given COLORREF to draw colour format
---*/
draw_coltyp FAR PASCAL ColorRefToDraw(COLORREF c);

/*---
DrawToColorRef : converts given draw colour to a COLORREF
---*/
COLORREF FAR PASCAL DrawToColorRef(draw_coltyp colour);

/*---
Draw_InverseTransform : calculates the inverse transformation of 't' and returns it in 'inv'
                        't' and 'inv' can be the same pointer with no side effects
---*/
void FAR PASCAL Draw_InverseTransform(HPDRAWMATRIX t,HPDRAWMATRIX inv);

/*---
DrawPath_SetOptimumFlatness : sets the largest flatness value for the given device context
                              such that curves will still appear smooth
                              the mapping mode for the device is taken into account
---*/
void FAR PASCAL DrawPath_SetOptimumFlatness(HDC dc,HPDRAWMATRIX t);

/*---
DrawPath_Init : allocates internal buffers so that Draw_RenderPath can work
---*/
BOOL FAR PASCAL DrawPath_Init(void);

/*---
DrawPath_End : frees internal buffers
---*/
void FAR PASCAL DrawPath_End(void);

/*---
Path_HeaderSize : calculates the size of the given path's header in bytes
---*/
long FAR PASCAL Path_HeaderSize(HPDRAWPATH path);

/*---
Draw_RenderPath : used by other Draw_Render... functions, do not call
---*/
BOOL FAR PASCAL Draw_RenderPath(HPDRAWOBJHDR obj,draw_error far *e);

/*---
Draw_Stroke : render the given path on the given device context
              dc : device context
              path : pointer to start of path (NOT the path header)
              fill_style : can be 0 for default
                         & 3  = 0 ==> non-zero winding number
                                1     negative winding number
                                2     even-odd winding number
                                3     positive winding number
                         & 4  = 0     don't plot non-boundary exterior pixels
                                4     plot non-boundary exterior pixels
                         & 8  = 0     don't plot boundary exterior pixels
                                8     plot boundary exterior pixels
                         & 16 = 0     don't plot boundary interior pixels
                                16    plot boundary interior pixels
                         & 32 = 0     don't plot non-boundary interior pixels
                                32    plot non-boundary interior pixels
                         N.B. All other bits reserved and must be set to zero
                              This implementation only takes account of:
                               if(fs & 3) mode = WINDING else ALTERNATE
                               if(fs & 8) draw outline
                               if(fs & 32) fill interior
              t          : transformation matrix or NULL for identity matrix
              flatness   : can be 0 for default
              thickness  : ditto
              cjspec     : pointer to line cap and join spec (can be NULL)
              dash       : pointer to line style (NULL if no line style)
              e          : pointer to an error (only valid if function returns FALSE)
---*/
BOOL FAR PASCAL Draw_Stroke(HDC dc,HPLONG path,unsigned long fill_style,
                            HPDRAWMATRIX t,unsigned long flatness,
                            unsigned long thickness,
                            draw_capjoinspec far *cjspec,
                            draw_dashstr far *dash,
                            draw_error far *e);

/*---
Draw_FlattenPath : converts an input path into a flattened output path
                   flatness can be 0 for default
                   'out' can be 0 if you only want the length of the flattened path
                   returns length of flattened path in bytes
---*/
long FAR PASCAL Draw_FlattenPath(HPLONG in,long far * far *out,long flatness,unsigned long FillStyle);

/*---
Draw_TransformPath : convert an input path into a transformed output path
---*/
void FAR PASCAL Draw_TransformPath(HPLONG in,HPLONG output,HPDRAWMATRIX transform);

/*---
Draw_PathBbox : calculates given path's bounding box
---*/
BOOL FAR PASCAL Draw_PathBbox(HPLONG path,unsigned long fill_style,unsigned long thickness,draw_dashstr far *dash,draw_capjoinspec far *cjspec,HPDRAWBOX bbox,draw_error far *e);

/*---
Draw_SetPathBbox : sets the bounding box for the given path object
---*/
BOOL FAR PASCAL Draw_SetPathBbox(HPDRAWPATH path);

/*---
DrawText_Init : internal use, do not call
---*/
BOOL FAR PASCAL DrawText_Init(void);

/*---
DrawText_End : internal use, do not call
---*/
void FAR PASCAL DrawText_End(void);

/*---
Draw_RenderText : internal use, do not call
---*/
BOOL FAR PASCAL Draw_RenderText(draw_objhdr far *obj,draw_error far *e);

/*---
Draw_RenderTransformedText : internal use, do not call
---*/
BOOL FAR PASCAL Draw_RenderTransformedText(draw_objhdr far *obj,draw_error far *e);

/*---
Draw_GetLogFont : finds the appropriate LOGFONT for the given font number
---*/
void FAR PASCAL Draw_GetLogFont(int n,LOGFONT far *lf);

/*---
Draw_SetLogFontSize : sets the given LOGFONT width & height
                      width and height are in draw units
---*/
void FAR PASCAL Draw_SetLogFontSize(long width,long height,HPDRAWMATRIX t,LOGFONT far *lf);

/*---
Draw_TextDescent : calculates the text descent of the given LOGFONT in draw units
                   (descent = distance that font extends below text base line)
---*/
long FAR PASCAL Draw_TextDescent(HWND w,LOGFONT far *lf,HPDRAWMATRIX inv_tx);

/*---
Draw_SetFontTables : sets up the offsets to the font table objects for the given draw file
                     use before calling Draw_RenderRange
---*/
BOOL FAR PASCAL Draw_SetFontTables(draw_diag far *d);

/*---
Draw_GetAccurateTextBbox : calculates the bounding box for the given text object
---*/
void FAR PASCAL Draw_GetAccurateTextBbox(HWND w,draw_diag far *d,long offset,HPDRAWMATRIX tx,HPDRAWMATRIX inv_tx,draw_box far *bb);

/*---
Draw_SetAccurateTextBbox : sets the bounding box for the given text object
---*/
void FAR PASCAL Draw_SetAccurateTextBbox(HWND w,draw_diag far *d,long offset,HPDRAWMATRIX tx,HPDRAWMATRIX inv_tx);

/*---
Draw_SetTextBbox : sets the text bounding box such that it would be ok when viewed at 100%
                   on the screen
---*/
void FAR PASCAL Draw_SetTextBbox(draw_diag far *d,long offset);

/*---
Draw_GetTextSlab : Calculates the 4 points which define the smallest rectangle which surrounds
                   the given text object
---*/
void FAR PASCAL Draw_GetTextSlab(HWND w,draw_diag far *d,long offset,HPDRAWMATRIX tx,HPDRAWMATRIX inv_tx,HPDRAWCOORD p4);

/*---
Draw_GetTextBaseline : Calculates the end points of the given text object's base line
---*/
void FAR PASCAL Draw_GetTextBaseline(HWND w, draw_diag far *d,long offset,HPDRAWMATRIX tx,HPDRAWMATRIX inv_tx,HPDRAWCOORD p2);

/*---
Draw_GetRiscOSToWindowsTextWidthFactor : returns a value which RiscOS font widths are scaled by
                                         before being displayed
                                         this value is currently 0.4
(this is used to ensure that text objects are as similar as possible when viewed on both platforms)
---*/
double FAR PASCAL Draw_GetRiscOSToWindowsTextWidthFactor(void);

/*---
Draw_GetSpriteSlab : Calculates the 4 points (p[0] - p[3]) which specify the smallest rectangle
                     which surrounds the given sprite or transformed sprite object
---*/
void FAR PASCAL Draw_GetSpriteSlab(draw_diag far *d,long offset,HPDRAWCOORD p);

/*---
Draw_SetSpriteBbox : sets the given rotated sprites' bounding box
---*/
void FAR PASCAL Draw_SetSpriteBbox(draw_diag far *d,long offset);

/*---
Draw_SetMetaFileDPI : sets smoothness of curves when creating a metafile
---*/
void FAR PASCAL Draw_SetMetaFileDPI(unsigned int dpi);

#endif
