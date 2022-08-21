@rem This Source Code Form is subject to the terms of the Mozilla Public
@rem License, v. 2.0. If a copy of the MPL was not distributed with this
@rem file, You can obtain one at http://mozilla.org/MPL/2.0/.

@rem Copyright (C) 2013-2021 Stuart Swales

pushd %~dp0
IF NOT EXIST BLDCD.BAT EXIT
@
@rem The echo F | xcopy /f is needed to copy and rename at the same time.
@
rmdir /s /q                                                        InnoSetup
@
rmdir /s /q                                                        CD
mkdir                                                              CD\
@if errorlevel 1 goto failed
@
@rem copy newly built stuff i.e. the main executable
xcopy             "..\..\..\Build\windows\Release XP ANSI\firewrkz.exe" CD\
@if errorlevel 1 goto failed
@
@rem copy release notes
@echo F | xcopy ..\..\..\firewrkz\RelNotesW.htm                    CD\RelNotes.htm
@if errorlevel 1 goto failed
@
@rem copy standard stuff
xcopy             ..\..\..\firewrkz\windows\DISC1-x86.SRC\*.*      CD\     /s /EXCLUDE:..\..\..\firewrkz\windows\DISC1-x86.SRC\EXCLUDE.TXT
@if errorlevel 1 goto failed
@
@
mkdir                                                              CD\Resources\
@
@rem language-independent files
mkdir                                                              CD\Resources\Neutral\
@
@rem copy other files (markers)
mkdir                                                              CD\Resources\Neutral\Markers\
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\Resources\Neutral\Markers\S1?aff       CD\Resources\Neutral\Markers\S1.aff
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\Resources\Neutral\Markers\S2?aff       CD\Resources\Neutral\Markers\S2.aff
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\Resources\Neutral\Markers\S3?aff       CD\Resources\Neutral\Markers\S3.aff
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\Resources\Neutral\Markers\S4?aff       CD\Resources\Neutral\Markers\S4.aff
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\Resources\Neutral\Markers\S5?aff       CD\Resources\Neutral\Markers\S5.aff
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\Resources\Neutral\Markers\S6?aff       CD\Resources\Neutral\Markers\S6.aff
@if errorlevel 1 goto failed
@
@rem copy other files (pictures)
mkdir                                                              CD\Resources\Neutral\Pictures\
@echo F | xcopy /f ..\..\..\Resources\Neutral\Pictures\S1?aff      CD\Resources\Neutral\Pictures\S1.aff
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\Resources\Neutral\Pictures\S2?aff      CD\Resources\Neutral\Pictures\S2.aff
@if errorlevel 1 goto failed
@
@rem copy other files (compound file helpers)
mkdir                                                              CD\Resources\Neutral\Support\
@echo F | xcopy /f ..\..\..\Resources\Neutral\Support\cfbf?ffd     CD\Resources\Neutral\Support\cfbf
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\Resources\Neutral\Support\cfbf-mini?ffd CD\Resources\Neutral\Support\cfbf-mini
@if errorlevel 1 goto failed
@
@rem language-independent files [end]
@
@rem language-dependent files (UK)
mkdir                                                              CD\Resources\UK\
@
@rem copy other files (built on RISC OS)
@echo F | xcopy /f ..\..\..\firewrkz\Resources\Windows\UK\config.txt CD\Resources\UK\
@if errorlevel 1 goto failed
@
@rem copy other files (compiled HTML help)
@echo F | xcopy /f ..\..\..\..\docs\UserGuide\firewrkz.chm         CD\Resources\UK\
@if errorlevel 1 goto failed
@
@rem copy other files (Dial Solutions DLLs)
xcopy             ..\..\..\external\Dial_Solutions\*.dll           CD\Resources\UK\DLL\
@
@rem language-dependent files (UK) [end]
@
@
@rem CD\DefaultUser directory is used to populate each user's AppData and Templates directories
@
@rem copy other files (standard templates)
@rem mkdir                                                         CD\DefaultUser\
@rem mkdir                                                         CD\DefaultUser\AppData\
mkdir                                                              CD\DefaultUser\AppData\Templates\
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\Resources\UK\Templates\Letter?c1d      CD\DefaultUser\AppData\Templates\Letter.fwt
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\Resources\UK\Templates\Sheet?c1d       CD\DefaultUser\AppData\Templates\Sheet.fwt
@if errorlevel 1 goto failed
@
@rem copy other files (single default template)
mkdir                                                              CD\DefaultUser\Templates\
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\Resources\UK\Templates\Sheet?c1d       CD\DefaultUser\Templates\firewrkz.fwt
@if errorlevel 1 goto failed
@
@
@rem these files are copied to the Windows directory by the installer
mkdir                                                              CD\Install\Windows\
@if errorlevel 1 goto failed
xcopy             ..\..\..\external\Dial_Solutions\RO.ini          CD\Install\Windows\
@if errorlevel 1 goto failed
@
@
@popd
@exit /b
@
:failed
@popd
@echo *** copy failed ***
@pause
@exit /b 1
