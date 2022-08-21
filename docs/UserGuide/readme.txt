Windows
-------

The HTML Help is distributed as a compiled HTML Help file with Fireworkz.

Download and install a copy of the Microsoft HTML Help Workshop (it's free). Just the htmlhelp.exe will suffice. Should be v1.3.

On a checked-out copy on a Windows system, start a command prompt.

    cd docs\UserGuide

    firewrkz.hhp

This will bring up the HTML Help Workshop with the Fireworkz help project loaded.

Press the 'Save all files and Compile' button (the one below the 'Save' button).

If it asks you to save a log file, just press Cancel.

This generates the compiled help file, firewrkz.chm, which should be checked in when OK.

Or why not just run

	"c:\Program Files\HTML Help Workshop\hhc.exe" firewrkz.hhp

which runs the HTML Help Compiler directly?


RISC OS
-------

The HTML Help can be distributed as a ZIP archive with !Fireworkz.


On a checked-out copy on a Unix system

	cd docs/UserGuide

	./make_HTMLhelp_zip

then copy the resulting ZIP archive, HTMLHelp,a91, to ../../t5/resource/.



In either case, remember to check-in the new binary.
