@rem Build\windows\setup.cmd

@rem This Source Code Form is subject to the terms of the Mozilla Public
@rem License, v. 2.0. If a copy of the MPL was not distributed with this
@rem file, You can obtain one at http://mozilla.org/MPL/2.0/.

@rem Copyright (C) 2013-2016 Stuart Swales

@rem Execute from top-level t5 directory

set COLTSOFT_CS_NONFREE=..\..\..\coltsoft\trunk\cs-nonfree

mkdir                                                                 .\external\Microsoft\Excel97SDK\INCLUDE
copy /Y %COLTSOFT_CS_NONFREE%\Microsoft\Excel_97_SDK\INCLUDE\XLCALL.H .\external\Microsoft\Excel97SDK\INCLUDE\xlcall.h

@rem Patched copy of BTTNCUR from 'Inside OLE 2' for Fireworkz

@set BTTNCURP_FILE=.\external\Microsoft\InsideOLE2\BTTNCURP\BTTNCUR.C
@if exist %BTTNCURP_FILE% (@del /F /Q %BTTNCURP_FILE%)
@set BTTNCURP_FILE=.\external\Microsoft\InsideOLE2\BTTNCURP\BTTNCUR.H
@if exist %BTTNCURP_FILE% (@del /F /Q %BTTNCURP_FILE%)
@set BTTNCURP_FILE=.\external\Microsoft\InsideOLE2\BTTNCURP\BTTNCURI.H
@if exist %BTTNCURP_FILE% (@del /F /Q %BTTNCURP_FILE%)

set BTTNCUR_SOURCE=.\external\Microsoft\InsideOLE2\BTTNCUR
if exist %COLTSOFT_CS_NONFREE%\Microsoft\InsideOLE2\BTTNCUR (set BTTNCUR_SOURCE=%COLTSOFT_CS_NONFREE%\Microsoft\InsideOLE2\BTTNCUR)

@rem it helps to rename the patch utility on Windows 8 etc
Build\windows\gnu-paatch -b %BTTNCUR_SOURCE%\BTTNCUR.C -o .\external\Microsoft\InsideOLE2\BTTNCURP\BTTNCUR.C -i .\external\Microsoft\InsideOLE2\BTTNCURP\BTTNCUR.C.patch --verbose
Build\windows\gnu-paatch -b %BTTNCUR_SOURCE%\BTTNCUR.H -o .\external\Microsoft\InsideOLE2\BTTNCURP\BTTNCUR.H -i .\external\Microsoft\InsideOLE2\BTTNCURP\BTTNCUR.H.patch --verbose
copy /Y                     %BTTNCUR_SOURCE%\BTTNCURI.H   .\external\Microsoft\InsideOLE2\BTTNCURP

@rem Make the patched source files read-only to avoid accidental mods...
attrib +r .\external\Microsoft\InsideOLE2\BTTNCURP\BTTNCUR.C
attrib +r .\external\Microsoft\InsideOLE2\BTTNCURP\BTTNCUR.H
attrib +r .\external\Microsoft\InsideOLE2\BTTNCURP\BTTNCURI.H
