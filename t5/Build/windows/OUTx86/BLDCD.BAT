@rem This Source Code Form is subject to the terms of the Mozilla Public
@rem License, v. 2.0. If a copy of the MPL was not distributed with this
@rem file, You can obtain one at http://mozilla.org/MPL/2.0/.

@rem Copyright (C) 2013-2016 Stuart Swales

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
mkdir                                                              CD\System\Neutral\
@
@rem copy other files (built on RISC OS)
@echo F | xcopy /f ..\..\..\firewrkz\resource\windows\UK\config    CD\System\UK\config.txt
@if errorlevel 1 goto failed
@
@rem copy other files (markers)
mkdir                                                              CD\System\Neutral\Markers\
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\AppData\Neutral\Markers\s1?aff         CD\System\Neutral\Markers\S1.aff
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\AppData\Neutral\Markers\s2?aff         CD\System\Neutral\Markers\S2.aff
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\AppData\Neutral\Markers\s3?aff         CD\System\Neutral\Markers\S3.aff
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\AppData\Neutral\Markers\s4?aff         CD\System\Neutral\Markers\S4.aff
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\AppData\Neutral\Markers\s5?aff         CD\System\Neutral\Markers\S5.aff
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\AppData\Neutral\Markers\s6?aff         CD\System\Neutral\Markers\S6.aff
@if errorlevel 1 goto failed
@
@rem copy other files (pictures)
mkdir                                                              CD\System\Neutral\Pictures\
@echo F | xcopy /f ..\..\..\AppData\Neutral\Pictures\s1?aff        CD\System\Neutral\Pictures\S1.aff
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\AppData\Neutral\Pictures\s2?aff        CD\System\Neutral\Pictures\S2.aff
@if errorlevel 1 goto failed
@
@rem copy other files (compound file helpers)
mkdir                                                              CD\System\Neutral\Support\
@echo F | xcopy /f ..\..\..\AppData\Neutral\Support\cfbf?ffd       CD\System\Neutral\Support\cfbf
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\AppData\Neutral\Support\cfbf-mini?ffd  CD\System\Neutral\Support\cfbf-mini
@if errorlevel 1 goto failed
@
mkdir                                                              CD\Resources\
mkdir                                                              CD\Resources\Neutral\
mkdir                                                              CD\Resources\UK\
@
@rem copy other files (compiled HTML help)
@echo F | xcopy /f ..\..\..\..\docs\UserGuide\firewrkz.chm         CD\Resources\UK\
@if errorlevel 1 goto failed
@
@rem copy other files (Dial Solutions)
xcopy             ..\..\..\external\Dial_Solutions\*.dll           CD\Resources\UK\DLL\
@rem
mkdir                                                              CD\Windows\
@if errorlevel 1 goto failed
xcopy             ..\..\..\external\Dial_Solutions\RO.ini          CD\Windows\
@if errorlevel 1 goto failed
@
@rem copy other files (standard templates)
@rem mkdir                                                         CD\DefaultUser\
@rem mkdir                                                         CD\DefaultUser\AppData\
mkdir                                                              CD\DefaultUser\AppData\Templates\
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\AppData\UK\Template\Letter?c1d         CD\DefaultUser\AppData\Templates\Letter.fwt
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\AppData\UK\Template\Sheet?c1d          CD\DefaultUser\AppData\Templates\Sheet.fwt
@if errorlevel 1 goto failed
@
@rem copy other files (default template)
mkdir                                                              CD\DefaultUser\Templates\
@if errorlevel 1 goto failed
@echo F | xcopy /f ..\..\..\AppData\UK\Template\Sheet?c1d          CD\DefaultUser\Templates\firewrkz.fwt
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