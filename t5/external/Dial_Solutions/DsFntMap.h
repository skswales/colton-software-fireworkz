/*--- DsFntMap.h ---*/

#ifndef __DSFNTMAP_H__
#define __DSFNTMAP_H__

#define ID_FONTNAME                 101

BOOL FAR PASCAL FontMap_RiscOSToLogFont(LPSTR riscos_font,LOGFONT far *lf);
/*---
Description: returns equivalent Windows font to given risc os font name
---*/

void FAR PASCAL FontMap_LogFontToRiscOS(LOGFONT far *lf,LPSTR riscos_font);
/*---
Description: returns name of risc os font which maps to given logfont
             invents a name if no match is found (e.g. "Arial.Bold.Italic")
             riscos_font is assumed to point to "enough" memory (256 bytes should always be more than enough)
---*/

BOOL FAR PASCAL FontMap_GetEquivalentLogFont(HWND parent,LPSTR riscos_font,LOGFONT far *lf);
/*---
Description: Displays dialog box which asks user to select an equivalent windows font
             for the given riscos font name then returns the selected font in 'lf'
---*/

void FAR PASCAL FontMap_Add(LPSTR riscos_font,LOGFONT far *lf);
/*---
Description: adds the given riscos font and logfont to the font mapping table
             (and also to fontmap.ini) if they are not already there
---*/

#endif
