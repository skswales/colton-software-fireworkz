@rem This Source Code Form is subject to the terms of the Mozilla Public
@rem License, v. 2.0. If a copy of the MPL was not distributed with this
@rem file, You can obtain one at http://mozilla.org/MPL/2.0/.

@rem Copyright (C) 2013-2020 Stuart Swales

pushd %~dp0
IF NOT EXIST BLDCD2.BAT EXIT

set OUT_DIR=fwin-22407

rmdir /s /q                                            %OUT_DIR%
mkdir                                                  %OUT_DIR%\
@if errorlevel 1 goto failed
@
@rem copy newly built stuff ie. the Setup EXE
xcopy .\InnoSetup\*.exe                                %OUT_DIR%\
@if errorlevel 1 goto failed
@
@echo F | xcopy ..\..\..\firewrkz\RelNotesW.htm        %OUT_DIR%\RelNotes.htm
@if errorlevel 1 goto failed
@
@echo D | xcopy /s ..\..\..\..\docs\GSGuide\windows    %OUT_DIR%\StartGuide\
@if errorlevel 1 goto failed
@
@explorer .
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
