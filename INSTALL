Here are the instructions for building the Polymorph screensaver from source.
See [README](README.md) for how to run or install the screensaver after it is
built.

## Setting up the build environment.

You need a recent MinGW-W64 toolchain and GNU Make. To get them, install MSYS2
from Sourceforge, then launch the MINGW32 or MINGW64 shell and get the necessary
packages using the commands:

    pacman -Sqyu ${MINGW_PACKAGE_PREFIX}-gcc
    pacman -Sqyu ${MINGW_PACKAGE_PREFIX}-make

If you want to minify the shader sources, you also need to have Perl installed.

## Compile commands.

To compile, enter this command in the MINGW32 or MINGW64 shell:

    mingw32-make

You can set variables on the Make command line to influence Make's behaviour.
For example:

* To target 32-bit Windows:

        mingw32-make PLATFORM=x86

* To specify the location of Perl if it is not on the path:

        mingw32-make PERL=c:\Strawberry\perl\bin\perl.exe

* To skip shader minification (e.g., if Perl is not available):

        mingw32-make shaders=full
