ReadMe for Fireworkz Build (RISC OS)
------------------------------------

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Copyright (C) 2013-2021 Stuart Swales

Prerequisites
-------------

ROOL DDE Release 25 or later (!Amu, C compiler, headers, libraries, linker).

Edit DDE's !Amu.Desc for a larger WimpSlot. I use 5024k - it's ample.

GNU Sed to generate the Makefiles (you will need the Colton Software build as
the one provided by PackMan doesn't work for this).

InfoZip (install with PackMan) to provide Unzip and Zip.

Unzip (installed as InfoZip dependency) to populate the build directories.

Zip (install as InfoZip dependency) for creating a zip of the final build.


First-time configuration and build
----------------------------------

Ensure that the system has 'seen' AcornC/C++ and set that environment.

Run !!!Boot in t5.Build to set up the 32-bit Fireworkz release build environment.

Edit 'configure' - set the URD macro to the directory containing t5.

Run !Amu and drag 'configure' to !Amu. This will build the tools
(MakeAOF/MakeMess/MakeResp) and Makefiles that you use to build Fireworkz.

Then drag 'r32.firewrkz.Makefile' to !Amu... and wait...  A Fireworkz build
takes about eighty minutes on a SA RISC PC.  Building on an ARMX6 takes about
five minutes. A Fireworkz Pro build takes about ten minutes longer on a SA
RISC PC, and about thirty seconds(!) on an ARMX6.

Please note that the ObjMunge tool only works on 26-bit systems.  In order to
build the modular Fireworkz, r32m, you will need to build on such a system.


Subsequent builds
-----------------

After any reboot, you will need to run !!!Boot to set up the 32-bit
Fireworkz build environment again. This also depends on the system having
already 'seen' AcornC/C++.

To clean up a build, run bld_mkdir.r32_clean

In order to build again, you will need to run bld_mkdir.r32_mkdir

Then drag 'r32.firewrkz.Makefile' to !Amu... and wait...


Creating a package for distribution
-----------------------------------

In firewrkz.r32.MakeNN, ensure t5$Release is set appropriately in the Obey file MakeRiscPkg, then run it.

This will copy only those files needed for distribution to $.Temp.Fireworkz_<t5$Release>.

Zip is used to create the distributable file $.Temp.Fireworkz_<t5$Release>/zip.
