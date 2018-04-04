# j3d-input
An archive of open source Java 3D 6DOF virtual reality input trackers, circa 2003.

I've put this here only for educational and historical purposes; it's highly unlikely that
anybody now will have access to the hardware and software environments to actually build the
package.  The original README file follows.

# Overview

This is the source code for the com.sun.j3d.input package.  It includes:
TrackdInputDevice, an implementation of the Java 3D InputDevice interface
for the VRCO trackd sensor tracking API; LogitechTracker, an InputDevice
implementation for the Logitech ultrasonic head and mouse trackers;
Mouse2DValuator, a simple mouse input device encapsulated as a 2D valuator;
EventDrivenInputDevice, an experimental interface that extends InputDevice;
and Gameport, an InputDevice implementation for PC-style analog joysticks
attached to a workstation gameport serial interface device.

Some of these classes use native code and require the native j3dInput
shared library somewhere in the application library search path.
TrackdInputDevice also requires a VRCO trackd software installation; in
particular, the trackdAPI shared library must exist somewhere in the
application library search path.

Trackd is a middleware standard that abstracts most of the popular motion
tracking sensor devices into a common API.  TrackdInputDevice uses this
generic interface to allow integration of any trackd-supported device into
Java 3D applications.  This package doesn't include the trackdAPI libraries;
they must be obtained directly from VRCO.

LogitechTracker and Gameport are extensions of the SerialDevice class.
This class uses standard serial ports to communicate with the devices.

# Supported platforms

Currently this package has been built only on Solaris using the Sun compiler,
but it shouldn't be difficult to port the native code to other Unix variants or
to gcc.  The Makefile in build/solaris and the supplied source code should
provide good starting points.  The file serial.c contains most of the
OS-specific code.  The SWAPBYTES preprocessor definition should be set
according to the native byte order of the host hardware in redbarron.h.

TrackdInputDevice should be portable to Windows systems.  There is a
Makefile for win32 in build/win32 which will successfully build the jar and
dll files using Visual C++ nmake on the commandline, but currently the win32
LoadLibrary call in TrackdInputDevice.c fails at runtime with error 0x1e7
(487): "Attempt to access invalid address."  There may be a problem with the
trackdAPI_MT.dll shipped by VRCO; j3dinput.dll loads fine if there are no
references to trackdAPI symbols.  However, trackdAPI_MT.dll does work for
building the example reader shipped by VRCO, so it may be another problem
entirely.

# Building and testing

Edit paths.sh (or paths.bat for win32) and review the values for
J3DINPUTHOME, CLASSPATH, and LD_LIBRARY_PATH.  TRACKDHOME must be set to the
root directory of the trackd software installation; the build will look for
the lib and include subdirectories in that directory.

To build and install on Unix system variants, source paths.sh and run 'make'
in this directory.  You can alternatively cd to build/<platform> and run
'make' from there.  The Makefiles in the build directories provide 'opt' and
'debug' targets, but you will need to set paths appropriately in the
paths.sh script to directly access the opt and debug files.

Building the win32 version is currently broken.  If you feel like trying to
fix it, run paths.bat, cd to build\win32, run 'nmake', and go from there.

The 'test' directory currently contains only the InputTest program.
It is built by default by the top-level Makefile, but can also be built by
going to the test directory and running 'make'.  See the class comments for
details. 

# Installation

The jar and shared library files for com.sun.j3d.input are copied into the
lib directory when using the default Makefile targets. The root of the lib
directory can be specified with J3DINPUTHOME, or the files can just be
copied to standard local directories on the usual CLASSPATH,
LD_LIBRARY_PATH, LIB, or SHLIB_PATH.

To use TrackdInputDevice, the trackdAPI shared library supplied by VRCO must
be installed in a standard system library directory or somewhere on the
user's path.

# Applications

com.sun.j3d.utils.behaviors.vp.WandViewBehavior (shipped with Java 3D 1.3)
provides a rich set of view manipulations that can be mapped to 6 degree of
freedom InputDevice implementations such as TrackdInputDevice and
LogitechTracker.  
