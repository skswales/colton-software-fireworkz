@rem This Source Code Form is subject to the terms of the Mozilla Public
@rem License, v. 2.0. If a copy of the MPL was not distributed with this
@rem file, You can obtain one at http://mozilla.org/MPL/2.0/.

@rem Copyright (C) 2013-2016 Stuart Swales

pushd %~dp0
IF NOT EXIST BLDCD.BAT EXIT
@
@rem The echo F | xcopy /f is needed to copy and rename at the same time.
@
rmdir /s /q                                                    InnoSetup
@
rmdir /s /q                                                    CD
mkdir                                                          CD\
@if errorlevel 1 goto failed
@
@rem copy newly built stuff i.e. the main executable
xcopy             "..\..\..\Build\windows\x64\Release\firewrkz.exe" CD\
@if errorlevel 1 goto failed
@
@rem copy standard stuff
xcopy             ..\..\..\firewrkz\windows\DISC1-x64.SRC\*.*  CD\    /s /EXCLUDE:..\..\..\firewrkz\windows\DISC1-x64.SRC\EXCLUDE.TXT
@if errorlevel 1 goto failed
@
@rem copy release notes
@echo F | xcopy ..\..\..\firewrkz\RelNotesW.htm                CD\RelNotes.htm
@if errorlevel 1 goto failed
@
@rem copy other files (compound file helpers)
@echo F | xcopy /f ..\..\..\resource\cfbf?ffd                  CD\System\UK\cfbf
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\resource\cfbf-mini?ffd             CD\System\UK\cfbf-mini
@if errorlevel 1 goto failed
@
@rem copy other files (built on RISC OS)
@echo F | xcopy /f ..\..\..\firewrkz\resource\windows\uk\config CD\System\UK\config.txt
@if errorlevel 1 goto failed
@
@rem copy other files (compiled help)
@echo F | xcopy /f ..\..\..\..\docs\htmlhelp\firewrkz.chm      CD\System\UK\
@if errorlevel 1 goto failed
@
@rem copy other files (markers)
mkdir                                                          CD\System\UK\Markers\
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\resource\markers\s1?aff            CD\System\UK\Markers\S1.aff
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\resource\markers\s2?aff            CD\System\UK\Markers\S2.aff
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\resource\markers\s3?aff            CD\System\UK\Markers\S3.aff
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\resource\markers\s4?aff            CD\System\UK\Markers\S4.aff
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\resource\markers\s5?aff            CD\System\UK\Markers\S5.aff
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\resource\markers\s6?aff            CD\System\UK\Markers\S6.aff
@if errorlevel 1 goto failed
@
@rem copy other files (pictures)
mkdir                                                          CD\System\UK\Pictures\
@echo F | xcopy /f ..\..\..\resource\pictures\s1?aff           CD\System\UK\Pictures\S1.aff
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\resource\pictures\s2?aff           CD\System\UK\Pictures\S2.aff
@if errorlevel 1 goto failed
@
@rem copy other files (Dial Solutions)
@rem xcopy             ..\..\..\external\Dial_Solutions\*.dll       CD\System\UK\DLL\
@rem
@rem mkdir                                                          CD\Windows\
@rem @if errorlevel 1 goto failed
@rem xcopy             ..\..\..\external\Dial_Solutions\RO.ini      CD\Windows\
@rem @if errorlevel 1 goto failed
@
@rem copy other files (standard templates)
@rem mkdir                                                     CD\DefaultUser\
@rem mkdir                                                     CD\DefaultUser\ApplicationData\
mkdir                                                          CD\DefaultUser\ApplicationData\Templates\
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\resource\template\Letter?c1d       CD\DefaultUser\ApplicationData\Templates\Letter.fwt
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\resource\template\SheetUK?c1d      CD\DefaultUser\ApplicationData\Templates\Sheet.fwt
@if errorlevel 1 goto failed
@
@rem copy other files (default template)
mkdir                                                          CD\DefaultUser\Templates\
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\resource\template\SheetUK?c1d      CD\DefaultUser\Templates\firewrkz.fwt
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