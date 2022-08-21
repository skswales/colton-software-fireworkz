@rem This Source Code Form is subject to the terms of the Mozilla Public
@rem License, v. 2.0. If a copy of the MPL was not distributed with this
@rem file, You can obtain one at http://mozilla.org/MPL/2.0/.

@rem Copyright (C) 2015-2016 Stuart Swales

chcp 1252

call prefer-unix

if exist "%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" call "%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86
if exist "%ProgramFiles%\Microsoft Visual Studio 14.0\VC\vcvarsall.bat"      call "%ProgramFiles%\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86

@rem display environment set for development
set

title fireworkz_trunk

cd \fireworkz\trunk
