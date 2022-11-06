@rem This Source Code Form is subject to the terms of the Mozilla Public
@rem License, v. 2.0. If a copy of the MPL was not distributed with this
@rem file, You can obtain one at https://mozilla.org/MPL/2.0/.

@rem Copyright © 2013-2022 Stuart Swales

pushd %~dp0
IF NOT EXIST BLDCD2.BAT EXIT

set BLDCD2_DIR=fwin-23201

rmdir /s /q                                            %BLDCD2_DIR%
@del                                                   %BLDCD2_DIR%.zip

mkdir                                                  %BLDCD2_DIR%\
@if errorlevel 1 goto failed
@
@rem copy newly built stuff ie. the Setup EXE
xcopy .\InnoSetup\*.exe                                %BLDCD2_DIR%\
@if errorlevel 1 goto failed
@
@echo F | xcopy ..\..\..\firewrkz\RelNotesW.htm        %BLDCD2_DIR%\RelNotes.htm
@if errorlevel 1 goto failed
@
@echo D | xcopy /s ..\..\..\..\docs\GSGuide\windows    %BLDCD2_DIR%\StartGuide\
@if errorlevel 1 goto failed
@del                                                   %BLDCD2_DIR%\StartGuide\.htaccess
@
mkdir                                                  %BLDCD2_DIR%\Examples\
call ..\..\..\..\examples\_make_w32.cmd                %BLDCD2_DIR%\Examples
@
@rem zip this for distribution
pushd %BLDCD2_DIR%
..\..\..\..\external\cs-free\GNU\win32\zip.exe -2 -n .exe:.png -r -v ..\%BLDCD2_DIR%.zip .
@if errorlevel 1 goto failed2
popd
@
@rem remove the temporary directories
@rem rmdir /s /q                                            CD
@rem rmdir /s /q                                            InnoSetup
@
@explorer .
@
@popd
@exit /b
@
:failed2
@popd
:failed
@popd
@echo *** copy failed ***
@pause
@exit /b 1
