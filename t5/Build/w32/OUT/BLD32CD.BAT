@rem This Source Code Form is subject to the terms of the Mozilla Public
@rem License, v. 2.0. If a copy of the MPL was not distributed with this
@rem file, You can obtain one at http://mozilla.org/MPL/2.0/.

@rem Copyright (C) 2013-2015 Stuart Swales

@rem Assumes N: is mapped to the root of your development tree i.e. the dir containing \fireworkz
@rem You can easily do this with a batch file dropped in your Startup folder
@rem     SUBST N: %USERPROFILE%\colton-software-development
@rem or some appropriate equivalent for your own development environment.
@rem If not, you just have to navigate to the directory contining this batch file before executing it
pushd N:\fireworkz\fireworkz__2_00__branch\t5\Build\w32\OUT
IF NOT EXIST BLD32CD.BAT EXIT
@
@rem The echo F | xcopy /f is needed to copy and rename at the same time.
@
rmdir /s /q                                                    InnoSetup
@
rmdir /s /q                                                    CD
mkdir                                                          CD\
@if errorlevel 1 goto failed
@
@rem copy standard stuff
xcopy             ..\..\..\firewrkz\w32\DISC1-32.SRC\*.*       CD\    /s /EXCLUDE:..\..\..\firewrkz\w32\DISC1-32.SRC\EXCLUDE.TXT
@if errorlevel 1 goto failed
@
@rem copy other files (compound file helpers)
echo F | xcopy /f ..\..\..\resource\cfbf?ffd                   CD\System\UK\cfbf
@if errorlevel 1 goto failed
echo F | xcopy /f ..\..\..\resource\cfbf-mini?ffd              CD\System\UK\cfbf-mini
@if errorlevel 1 goto failed
@
@rem copy other files (built on RISC OS)
echo F | xcopy /f ..\..\..\firewrkz\resource\windows\uk\config CD\System\UK\config.txt
@if errorlevel 1 goto failed
@
@rem copy other files (compiled help)
echo F | xcopy /f ..\..\..\..\docs\htmlhelp\firewrkz.chm       CD\System\UK\
@if errorlevel 1 goto failed
@
@rem copy other files (markers)
mkdir                                                          CD\System\UK\Markers\
@if errorlevel 1 goto failed
echo F | xcopy /f ..\..\..\resource\markers\s1?aff             CD\System\UK\Markers\S1.AFF
@if errorlevel 1 goto failed
echo F | xcopy /f ..\..\..\resource\markers\s2?aff             CD\System\UK\Markers\S2.AFF
@if errorlevel 1 goto failed
echo F | xcopy /f ..\..\..\resource\markers\s3?aff             CD\System\UK\Markers\S3.AFF
@if errorlevel 1 goto failed
echo F | xcopy /f ..\..\..\resource\markers\s4?aff             CD\System\UK\Markers\S4.AFF
@if errorlevel 1 goto failed
echo F | xcopy /f ..\..\..\resource\markers\s5?aff             CD\System\UK\Markers\S5.AFF
@if errorlevel 1 goto failed
echo F | xcopy /f ..\..\..\resource\markers\s6?aff             CD\System\UK\Markers\S6.AFF
@if errorlevel 1 goto failed
@
@rem copy other files (pictures)
mkdir                                                          CD\System\UK\Pictures\
echo F | xcopy /f ..\..\..\resource\pictures\s1?aff            CD\System\UK\Pictures\S1.AFF
@if errorlevel 1 goto failed
echo F | xcopy /f ..\..\..\resource\pictures\s2?aff            CD\System\UK\Pictures\S2.AFF
@if errorlevel 1 goto failed
@
@rem copy other files (Dial Solutions)
xcopy             ..\..\..\external\Dial_Solutions\*.dll       CD\System\UK\DLL\
@rem
mkdir                                                          CD\Windows\
@if errorlevel 1 goto failed
xcopy             ..\..\..\external\Dial_Solutions\RO.ini      CD\Windows\
@if errorlevel 1 goto failed
@
@rem copy other files (standard templates)
@rem mkdir                                                     CD\DefaultUser\
@rem mkdir                                                     CD\DefaultUser\ApplicationData\
mkdir                                                          CD\DefaultUser\ApplicationData\Templates\
@if errorlevel 1 goto failed
echo F | xcopy /f ..\..\..\resource\template\Letter?c1d        CD\DefaultUser\ApplicationData\Templates\Letter.fwt
@if errorlevel 1 goto failed
echo F | xcopy /f ..\..\..\resource\template\SheetUK?c1d       CD\DefaultUser\ApplicationData\Templates\Sheet.fwt
@if errorlevel 1 goto failed
@
@rem copy other files (foreign templates)
@rem mkdir                                                          CD\DefaultUser\ApplicationData\ForeignTemplates\
@rem @if errorlevel 1 goto failed
@rem echo F | xcopy /f ..\..\..\resource\foreigntem\SheetFgn?c1d    CD\DefaultUser\ApplicationData\ForeignTemplates\SheetFgn.fwt
@rem @if errorlevel 1 goto failed
@rem echo F | xcopy /f ..\..\..\resource\foreigntem\TextFgn?c1d     CD\DefaultUser\ApplicationData\ForeignTemplates\TextFgn.fwt
@rem @if errorlevel 1 goto failed
@rem echo F | xcopy /f ..\..\..\resource\foreigntem\SheetXLS?c1d    CD\DefaultUser\ApplicationData\ForeignTemplates\SheetXLS.fwt
@rem @if errorlevel 1 goto failed
@
@rem copy other files (default template)
mkdir                                                          CD\DefaultUser\Templates\
@if errorlevel 1 goto failed
echo F | xcopy /f ..\..\..\resource\template\SheetUK?c1d       CD\DefaultUser\Templates\firewrkz.fwt
@if errorlevel 1 goto failed
@
@rem copy newly built stuff i.e. the main executable
xcopy             ..\..\..\Build\w32\Release\firewrkz.exe      CD\
@if errorlevel 1 goto failed
@
@popd
@
@pause
@exit /b
@
:failed
@echo *** copy failed ***
@popd
@
@pause
@exit /b 1