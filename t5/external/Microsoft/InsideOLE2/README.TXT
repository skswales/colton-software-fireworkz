external/Microsoft/InsideOLE2/
==============================

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Copyright (C) 2013-2020 Stuart Swales

Updated samples from the Inside OLE 2 book by Kraig Brockschmidt (Microsoft Press).

Available from http://support.microsoft.com/kb/113255

[Article ID: 113255 - Last Review: August 9, 2004 - Revision: 3.3]
[SAMPLE: Corrections for Inside OLE 2 Sample Code]
[This code has been updated to be compatible with OLE 2.01, and also to fix some problems in the original code]

Copy the inole2.exe file into this directory.

CD here in a command prompt then

inole2.exe -d

to unzip, creating directories.

NB Fireworkz only uses files from the BTTNCUR directory.

In order to patch the BTTNCUR files, you will need a copy of GNU Win32 patch, which can be obtained from:

http://sourceforge.net/projects/gnuwin32/files/patch/

Just take a copy of patch.exe from the patch-xxx-bin.zip (no need for the full setup) and rename it as gnu-paatch.exe (yes, with the deliberate mis-spelling) in Build\w32.

