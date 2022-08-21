Windows
-------

The HTML Help is distributed as a compiled HTML Help file with Fireworkz.

Download and install a copy of the Microsoft HTML Help Workshop (it's free).

Just the htmlhelp.exe will suffice. Should be v1.3.

On a checked-out copy on a Windows system, start a command prompt.

    cd docs\UserGuide\Windows

    make_help.cmd

This generates the compiled help file, firewrkz.chm, which should be copied to UserGuide and checked in when OK.


If the build fails, some examination is needed!

    firewrkz.hhp

This will bring up the HTML Help Workshop with the Fireworkz help project loaded.

Press the 'Save all files and Compile' button (the one below the 'Save' button).

If it asks you to save a log file, just press Cancel.

