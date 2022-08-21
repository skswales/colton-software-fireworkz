ReadMe for Fireworkz Build (RISC OS)
------------------------------------

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Copyright (C) 2013-2016 Stuart Swales

Prerequisites
-------------

Acorn C/C++ Release 25 or later (!Amu, C compiler, headers, libraries, linker).

GNU Sed to generate the Makefiles (you will need the Colton Software build as
the one provided by PackMan doesn't work for this).

InfoZip (install with PackMan) for creating a zip of the final build.

Unzip (installed as InfoZip dependency) to populate the build directories.

Zip (install as InfoZip dependency) for creating a zip of the final build.


First-time configuration and build
----------------------------------

Ensure that the system has 'seen' AcornC/C++ and set that environment.

Run !!!Boot in Build to set up the 32-bit Fireworkz release build environment.

Drop a copy of !ArcFS (read-only) in Build.bld_mkdir.  This is needed to
unpack the archive file that is used to populate empty build directories.

You need to check out a copy of RISC_OSLib for Fireworkz to build.
See ^.external.RISC_OSLib.ReadMe for instructions on how to do this.

Edit 'configure' - set the URD macro to the directory containing t5.

Edit DDE's !Amu.Desc for a larger WimpSlot. I use 5024k - it's ample.

Run !Amu.

Drag 'configure' to !Amu. This will build the tools (MakeAOF/Mess/Resp) and
Makefiles that you use to build Fireworkz.

Please note that ObjMunge doesn't work on 32-bit only systems.  You will
therefore need to build the fully-bound Fireworkz, r32b, on such systems.

Then drag 'r32b.firewrkz.Makefile' to !Amu... and wait...  A Fireworkz build
takes about eighty minutes on a SA RISC PC.  Building on an ARMX6 takes about
five minutes. A Fireworkz Pro build takes about ten minutes longer on a SA
RISC PC, and about thirty seconds(!) on an ARMX6.


Subsequent builds
-----------------

After any reboot, you will need to run !!!Boot to set up the 32-bit
Fireworkz build environment again. This also depends on the system having
already 'seen' AcornC/C++.

To clean up a build, run bld_mkdir.clean_r32b

In order to build again, you will need to run bld_mkdir.mkdir_r32b

Then drag 'r32b.firewrkz.Makefile' to !Amu... and wait...


Creating a package for distribution
-----------------------------------

In firewrkz.r32b.MakeNN, ensure t5$Release is set appropriately in the Obey file MakeRiscPkg, then run it.

This will copy only those files needed for distribution to $.Temp.Fireworkz_<t5$Release>.

!Zip (from !InfoZip) is used to create the distributable file $.Temp.Fireworkz_<t5$Release>/zip.
