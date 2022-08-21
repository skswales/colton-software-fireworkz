@rem This Source Code Form is subject to the terms of the Mozilla Public
@rem License, v. 2.0. If a copy of the MPL was not distributed with this
@rem file, You can obtain one at http://mozilla.org/MPL/2.0/.

@rem Copyright (C) 2013-2015 Stuart Swales

pushd N:\fireworkz\fireworkz__2_00__branch\t5\Build\w32\OUT

rmdir /s /q                                fwin-200yy
mkdir                                      fwin-200yy\
@
xcopy ..\..\..\firewrkz\w32\DISC1-32.SRC\RelNotes.htm  fwin-200yy\
@
@rem copy newly built stuff ie. the Setup EXE
xcopy .\InnoSetup\setup*.exe               fwin-200yy\
@
@explorer .
@
popd
@
@pause
