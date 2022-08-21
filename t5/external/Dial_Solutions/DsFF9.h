# ifndef __DsFF9_h
# define __DsFF9_h

/*---
N.B. If you use DSSPRITE.DLL then you must have a copy of 'RO.INI' in your
     windows directory. This file contains information about default palette
     colours for RiscOS and information about graphics modes.
---*/

typedef struct /* Format of a sprite area control block */
{
  long size;    /* size of area in bytes */
  long number;  /* number of sprites */
  long sproff;  /* byte offset to 1st sprite */
  long freeoff; /* byte offset to 1st free word (i.e. byte after last sprite) */
} sprite_area;
typedef sprite_area * psprite_area;

typedef struct /* Format of a sprite header */
{
  long next;      /*  Offset to next sprite                */
  char name[12];  /*  Sprite name (trailing zeroes)        */
  long width;     /*  Width in words-1      (0..639)       */
  long height;    /*  Height in scanlines-1 (0..255/511)   */
  long lbit;      /*  First bit used (left end of row)     */
  long rbit;      /*  Last bit used (right end of row)     */
  long image;     /*  Offset to sprite image               */
  long mask;      /*  Offset to transparency mask (= image if no mask) */
  long mode;      /*  Mode sprite was defined in           */
                  /*  Palette data optionally follows here */
                  /*  in memory                            */
} sprite_header;
typedef sprite_header * psprite_header;

typedef struct
{
 HGLOBAL himage; /* bitmap data */
 HGLOBAL hmask; /* mask data */
} big_icon;

typedef struct
{
 long a,b,c,d;   /* 16.16 fixed point values in 2x2 matrix */
} sprite_matrix;
typedef sprite_matrix far * lpsprite_matrix;

typedef void * sprite_ptr;

/*--- SPRITEGETBITS
Gets palette index of the sprite pixel at (bits/bpp,ybase) and returns it in 'col' (unsigned long)
bpp        = sprite bits per pixel
bits       = "unsigned long", is preset to number of bits from start of scan line of pixel @ (x,y) (= p->lbit + x * bpp)
mask       = pre-calculated (1 << bpp) - 1
ybase      = address of start of scan line for this pixel (= (long *)imagestart + y * wpl)
---*/
#define SPRITEGETBITS(bpp,col,bits,mask,ybase) \
 { \
  col = (*((long *)(ybase) + ((bits) >> 5)) >> ((bits) & 31)) & (mask); \
 }

/*--- SPRITEWIDTHBITS
Gets the width of the sprite in bits
p = psprite_header
---*/
#define SPRITEWIDTHBITS(p) ((((p)->width+1) << 5) - (p)->lbit - (31 - (p)->rbit))

/*--- SPRITEWIDTH
Gets the width of the sprite in pixels
p = psprite_header
bpp = bits per pixel
---*/
#define SPRITEWIDTH(p,bpp) (SPRITEWIDTHBITS(p)/bpp)

/*--- SPRITEHEIGHT
Gets the height of the sprite in pixels
p = psprite_header
---*/
#define SPRITEHEIGHT(p) ((p)->height + 1)

/*---
SpriteBPP : returns the bits per pixel for the given RiscOS screen mode or 0 if it doesn't know
---*/
int FAR PASCAL SpriteBPP(long mode);

/*---
Sprite_ModeEigenValues : sets 'xeig' and 'yeig' to eigen values for given mode
                         returns FALSE if 'ro.ini' does not contain information on the given mode
---*/
BOOL FAR PASCAL Sprite_ModeEigenValues(long mode,int far *xeig,int far *yeig);

/*---
SpriteGetBits : returns palette index of the pixel at x,y
---*/
unsigned long FAR PASCAL SpriteGetBits(psprite_header p,long x,long y,int bpp,unsigned long wpl,BOOL mask);

/*---
SpriteGetPixel : returns the colour of the pixel at x,y
                 or 0 if the co-ordinates are out of range
                 or 0 if it doesn't know the bpp value for the sprite's mode
---*/
COLORREF FAR PASCAL SpriteGetPixel(psprite_header p,long x,long y,HGLOBAL paltab,long palsize,int bpp);

/*---
SpritePut : displays given sprite on given device context with top left of sprite at x,y
---*/
void FAR PASCAL SpritePut(HDC dc,psprite_header p,int x,int y);

/*---
SpritetoDIB : converts given sprite to a DIB
              returns NULL if it fails
              if 'dc' is non-zero then the sprite palette table is matched with the given device
              if 'mask' is TRUE then the sprite's mask is converted instead of its image
---*/
HGLOBAL FAR PASCAL SpriteToDIB(psprite_header p,HDC dc,BOOL mask);

/*---
SpriteBuildPaletteTable : builds a table of COLORREF values for the given sprite
                          if 'dc' is non-zero then maps palette colours to the nearest on the given device
                          if sprite has no palette table then uses default RiscOS wimp palette
                          if 'mask' is TRUE, then sets 1st colour in palette table to black and last colour to white
---*/
HGLOBAL FAR PASCAL SpriteBuildPaletteTable(psprite_header p,HDC dc,long far *palsize,BOOL mask);

/*---
SpriteToIcon : Converts a sprite to a bitmap and a mask compatible with the given device context
---*/
BOOL FAR PASCAL SpriteToIcon(psprite_header p,HDC dc,big_icon far *bi);

/*---
SpriteToTransformedIcon : Converts a sprite to a bitmap and a mask compatible with the given device context
                          rotating and scaling the sprite as indicated by the given transformation matrix
---*/
BOOL FAR PASCAL SpriteToTransformedIcon(psprite_header p,HDC dc,lpsprite_matrix far *t,big_icon far *bi);

/*---
SpritePutIcon : Copies given icon to given device context with top left of icon at (x,y)
---*/
void FAR PASCAL SpritePutIcon(big_icon far *bi,HDC dc,int x,int y);

/*---
Sprite_DefPal2 : returns a pointer to 2 COLORREF values which represent the
                  default RiscOS wimp palette for monochrome sprites
---*/
void FAR PASCAL Sprite_DefPal2(COLORREF far * far *c);

/*---
Sprite_DefPal4 : returns a pointer to 4 COLORREF values which represent the
                  default RiscOS wimp palette for 2bpp sprites
---*/
void FAR PASCAL Sprite_DefPal4(COLORREF far * far *c);

/*---
Sprite_DefPal16 : returns a pointer to 16 COLORREF values which represent the
                  default RiscOS wimp palette
---*/
void FAR PASCAL Sprite_DefPal16(COLORREF far * far *c);

/*---
Sprite_DefPal256 : returns a pointer to 256 COLORREF values which represent the
                   default RiscOS 256 colour palette
---*/
void FAR PASCAL Sprite_DefPal256(COLORREF far * far *c);

# endif

/* end of DsFF9.h */
