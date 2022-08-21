@rem This Source Code Form is subject to the terms of the Mozilla Public
@rem License, v. 2.0. If a copy of the MPL was not distributed with this
@rem file, You can obtain one at http://mozilla.org/MPL/2.0/.

@rem Copyright (C) 2013-2016 Stuart Swales

chcp 1252

call prefer-unix

call "C:\Program Files\Microsoft Visual Studio 8\VC\vcvarsall.bat" x86

@rem call "C:\Program Files\Microsoft SDKs\Windows\v6.0\bin\setenv.cmd" /XP /x86 /Debug

@rem display environment set for development
set

title fireworkz_trunk

cd \fireworkz\trunk