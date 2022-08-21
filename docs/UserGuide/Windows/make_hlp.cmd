@rem This Source Code Form is subject to the terms of the Mozilla Public
@rem License, v. 2.0. If a copy of the MPL was not distributed with this
@rem file, You can obtain one at http://mozilla.org/MPL/2.0/.

@rem Copyright (C) 2020 Stuart Swales

pushd %~dp0
IF NOT EXIST make_hlp.cmd EXIT
@
@rem The echo F | xcopy /f is needed to copy and rename at the same time.
@
del firewrkz.chm
rmdir /s /q content
@
svn export ../content content
@
@echo Copying Windows-specific index.htm for HTML Help
@del                       content\index.htm
@echo F | xcopy indexW.htm content\index.htm
@if errorlevel 1 goto failed
@
@echo Copying Windows-specific CSS for HTML Help
@del                       content\common\guide.css
@echo F | xcopy guideW.css content\common\guide.css
@if errorlevel 1 goto failed
@
@echo Copying Windows-specific images for HTML Help
@echo F | xcopy /Y ..\..\GSGuide\windows\content\images\theatre_13.png                content\common\images\the_13.png
@if errorlevel 1 goto failed
@
@copy           ..\..\GSGuide\windows\content\images\toolbar\bold.png                 content\common\images\toolbar\
@if errorlevel 1 goto failed
@copy           ..\..\GSGuide\windows\content\images\toolbar\box.png                  content\common\images\toolbar\
@if errorlevel 1 goto failed
@copy           ..\..\GSGuide\windows\content\images\toolbar\chart.png                content\common\images\toolbar\
@if errorlevel 1 goto failed
@copy           ..\..\GSGuide\windows\content\images\toolbar\copy.png                 content\common\images\toolbar\
@if errorlevel 1 goto failed
@copy           ..\..\GSGuide\windows\content\images\toolbar\cut.png                  content\common\images\toolbar\
@if errorlevel 1 goto failed
@copy           ..\..\GSGuide\windows\content\images\toolbar\effects.png              content\common\images\toolbar\
@if errorlevel 1 goto failed
@copy           ..\..\GSGuide\windows\content\images\toolbar\fill_d.png               content\common\images\toolbar\
@if errorlevel 1 goto failed
@copy           ..\..\GSGuide\windows\content\images\toolbar\fill_r.png               content\common\images\toolbar\
@if errorlevel 1 goto failed
@echo F | xcopy /Y ..\..\GSGuide\windows\content\images\toolbar\function_selector.png content\common\images\toolbar\selfnc.png
@if errorlevel 1 goto failed
@copy           ..\..\GSGuide\windows\content\images\toolbar\italic.png               content\common\images\toolbar\
@if errorlevel 1 goto failed
@copy           ..\..\GSGuide\windows\content\images\toolbar\just_c.png               content\common\images\toolbar\
@if errorlevel 1 goto failed
@copy           ..\..\GSGuide\windows\content\images\toolbar\just_r.png               content\common\images\toolbar\
@if errorlevel 1 goto failed
@copy           ..\..\GSGuide\windows\content\images\toolbar\paste.png                content\common\images\toolbar\
@if errorlevel 1 goto failed
@copy           ..\..\GSGuide\windows\content\images\toolbar\print.png                content\common\images\toolbar\
@if errorlevel 1 goto failed
@copy           ..\..\GSGuide\windows\content\images\toolbar\save.png                 content\common\images\toolbar\
@if errorlevel 1 goto failed
@copy           ..\..\GSGuide\windows\content\images\toolbar\search.png               content\common\images\toolbar\
@if errorlevel 1 goto failed
@echo F | xcopy /Y ..\..\GSGuide\windows\content\images\toolbar\selection_off.png     content\common\images\toolbar\seloff.png
@if errorlevel 1 goto failed
@echo F | xcopy /Y ..\..\GSGuide\windows\content\images\toolbar\selection_on.png      content\common\images\toolbar\selon.png
@if errorlevel 1 goto failed
@copy           ..\..\GSGuide\windows\content\images\toolbar\sort.png                 content\common\images\toolbar\
@if errorlevel 1 goto failed
@copy           ..\..\GSGuide\windows\content\images\toolbar\spell_check.png          content\common\images\toolbar\
@if errorlevel 1 goto failed
@copy           ..\..\GSGuide\windows\content\images\toolbar\style.png                content\common\images\toolbar\
@if errorlevel 1 goto failed
@copy           ..\..\GSGuide\windows\content\images\toolbar\sum.png                  content\common\images\toolbar\
@if errorlevel 1 goto failed
@copy           ..\..\GSGuide\windows\content\images\toolbar\table.png                content\common\images\toolbar\
@if errorlevel 1 goto failed
@copy           ..\..\GSGuide\windows\content\images\toolbar\tbar.png                 content\common\images\toolbar\
@if errorlevel 1 goto failed
@copy           ..\..\GSGuide\windows\content\images\toolbar\tick.png                 content\common\images\toolbar\
@if errorlevel 1 goto failed
@echo F | xcopy /Y ..\..\GSGuide\windows\content\images\toolbar\view_control.png      content\common\images\toolbar\view_c.png
@if errorlevel 1 goto failed
@
@rem 32-bit on 64-bit lives in (x86)
IF EXIST      "%ProgramFiles%\HTML Help Workshop\hhc.exe"      "%ProgramFiles%\HTML Help Workshop\hhc.exe" firewrkz.hhp
IF EXIST "%ProgramFiles(x86)%\HTML Help Workshop\hhc.exe" "%ProgramFiles(x86)%\HTML Help Workshop\hhc.exe" firewrkz.hhp
@IF NOT EXIST firewrkz.chm @goto failed_chm
@
@echo .
@echo Test this locally-built firewrkz.chm and then, when happy, copy up into UserGuide and commit
@sleep 2
firewrkz.chm
@
@popd
@exit /b
@
:failed_chm
@echo HTML Help file firewrkz.chm has not been built
@echo Try loading project into HTML Help Workshop manually
@goto failed

:failed
@popd
@echo *** failed ***
@pause
@exit /b 1
