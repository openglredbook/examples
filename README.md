OpenGL Red Book Example Code
============================

This is the example code for the OpenGL Programming Guide (Red Book), 9th Edition.
It is mostly an import of code from previous editions with minor updates
to match some of the descriptions in the latest edition of the book.
We will be adding more samples and updating existing ones over time.
Treat this as a live repository. Pull requests and issues are welcome.
Go forth and fork!

Building
--------

We are using CMake as our build system. To build the samples, enter
the "build" subdirectory and enter "cmake -G "{your generator here}" .."
Project files or makefiles will be produced in the build subdirectory.
We have tested Unix makefiles ("Unix Makefiles") and Visual Studio project files.
Visual Studio 2013 ("Visual Studio 12") was used to develop most of the samples.
An update to add support for 2015 will land shortly. Code should compile, but the
templates for the project files are not in the repository at this time.
Other compilers and IDEs should work, but we haven't tested them.

Running
-------

Download the media pack from the download area in the repository and unpack it
into the bin/media directory. This contains model files and textures used by the
applications. Without this, most applications won't work and will quite possibly
crash. Don't file bugs about that if you don't have the media files.
Otherwise, the samples should compile and run out-of-the box. Of course, make sure your
OpenGL drivers are up to date and that your hardware is capable of running OpenGL 4.5.
That's what this book is about. Some systems don't support OpenGL 4.5 and will therefore
not run some of the samples. Trying to run such samples on a machine that doesn't have
OpenGL 4.5 support will fail. For example, if a platform were limited to, say OpenGL 4.1,
then the samples wouldn't work on that platform. Please don't file bugs about that either.
Error checking in these applications is minimal. If you don't have media files or if
your OpenGL drivers are out of date, they'll probably fail spectacularly.