This is the sourcecode tree for Kerberos Telnet for Windows 95/98/NT,
version V2.00.

Depending on the version of you Borland C++ compiler you have to use
different versions of the makefiles, Borland have changed the linking
process between 5.0 and 5.02.

If you have Borland C++ 5.0B installed on you computer, then everything
should build if you give the command 'make -f makefile.bcp' in this
directory.

If you are using Borland C++ 5.02 you should use the command
'make -f makefile.b52' in this directory to build all programs.

You will find the results in the WBIN directory.

You will best find the structure in the tree by looking into the top
makefile.bcp (or makefile.b52), it is very simple..... In the different
source directories you will find Borland .IDE-files, these IDE-files can
be used to build the libraries and the application, but watch out:
the IDE-files might build with debugging and they usally don't leave
the results in the WBIN directory.

If you find a bug (and maybe a correction for it), please send
a note about it to me: thn@stacken.kth.se

Happy hacking!

/Thomas
