@rem Build\windows\setup.cmd

@rem This Source Code Form is subject to the terms of the Mozilla Public
@rem License, v. 2.0. If a copy of the MPL was not distributed with this
@rem file, You can obtain one at https://mozilla.org/MPL/2.0/.

@rem Copyright © 2013-2022 Stuart Swales

@rem Execute from %FIREWORKZ_ROOT%\t5\external directory

pushd %~dp0
IF NOT EXIST setup-windows.cmd EXIT

call ..\Build\windows\t5_tbt.bat

set COLTSOFT_CS_FREE=..\..\..\..\coltsoft\%FIREWORKZ_TBT%\cs-free
set COLTSOFT_CS_NONFREE=..\..\..\..\coltsoft\%FIREWORKZ_TBT%\cs-nonfree

IF NOT EXIST %COLTSOFT_CS_FREE% EXIT
IF NOT EXIST %COLTSOFT_CS_NONFREE% EXIT

mklink /j %CD%\cs-free    %COLTSOFT_CS_FREE%
mklink /j %CD%\cs-nonfree %COLTSOFT_CS_NONFREE%

set COLTSOFT_CS_FREE=%CD%\cs-free
set COLTSOFT_CS_NONFREE=%CD%\cs-nonfree

@rem Patched copy of BTTNCUR from 'Inside OLE 2' for Fireworkz

@set BTTNCURP_FILE=.\Microsoft\InsideOLE2\BTTNCURP\BTTNCUR.C
@if exist %BTTNCURP_FILE% (@del /F /Q %BTTNCURP_FILE%)
@set BTTNCURP_FILE=.\Microsoft\InsideOLE2\BTTNCURP\BTTNCUR.H
@if exist %BTTNCURP_FILE% (@del /F /Q %BTTNCURP_FILE%)
@set BTTNCURP_FILE=.\Microsoft\InsideOLE2\BTTNCURP\BTTNCURI.H
@if exist %BTTNCURP_FILE% (@del /F /Q %BTTNCURP_FILE%)

set BTTNCUR_SOURCE=.\Microsoft\InsideOLE2\BTTNCUR
if exist %COLTSOFT_CS_NONFREE%\Microsoft\InsideOLE2\BTTNCUR (set BTTNCUR_SOURCE=%COLTSOFT_CS_NONFREE%\Microsoft\InsideOLE2\BTTNCUR)

@rem it helps to rename the patch utility on Windows 8 etc.
%COLTSOFT_CS_FREE%\GNU\win32\paatch.exe -b %BTTNCUR_SOURCE%\BTTNCUR.C -o .\Microsoft\InsideOLE2\BTTNCURP\BTTNCUR.C -i .\Microsoft\InsideOLE2\BTTNCURP\BTTNCUR.C.patch --verbose
%COLTSOFT_CS_FREE%\GNU\win32\paatch.exe -b %BTTNCUR_SOURCE%\BTTNCUR.H -o .\Microsoft\InsideOLE2\BTTNCURP\BTTNCUR.H -i .\Microsoft\InsideOLE2\BTTNCURP\BTTNCUR.H.patch --verbose
copy /Y                                    %BTTNCUR_SOURCE%\BTTNCURI.H   .\Microsoft\InsideOLE2\BTTNCURP

@rem Make the patched source files read-only to avoid accidental mods...
attrib +r .\Microsoft\InsideOLE2\BTTNCURP\BTTNCUR.C
attrib +r .\Microsoft\InsideOLE2\BTTNCURP\BTTNCUR.H
attrib +r .\Microsoft\InsideOLE2\BTTNCURP\BTTNCURI.H

popd

pause
