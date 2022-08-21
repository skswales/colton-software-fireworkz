@rem This Source Code Form is subject to the terms of the Mozilla Public
@rem License, v. 2.0. If a copy of the MPL was not distributed with this
@rem file, You can obtain one at http://mozilla.org/MPL/2.0/.

@rem Copyright (C) 2013-2018 Stuart Swales

chcp 1252

call prefer-unix

if exist "%ProgramFiles(x86)%\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" call "%ProgramFiles(x86)%\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86
if exist "%ProgramFiles%\Microsoft Visual Studio 12.0\VC\vcvarsall.bat"      call "%ProgramFiles%\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86

set FIREWORKZ_TBT=fireworkz__2_21__branch
@rem set FIREWORKZ_TBT=trunk

set FIREWORKZ_ROOT=N:\fireworkz\%FIREWORKZ_TBT%

@rem display environment set for development
set

title %FIREWORKZ_ROOT%

cd /D %FIREWORKZ_ROOT%
