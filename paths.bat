@echo off
REM -- J3DINPUTHOME is the directory where this package is installed.  When
REM -- building, a lib directory will be created there to contain the jar and
REM -- shared library if it doesn't exist.  Otherwise the files in lib
REM -- directory should be copied to %J3DINPUTHOME%\lib.

set J3DINPUTHOME=h:\klia\export\home\j3dinput-1.2

REM -- TRACKDHOME is the directory where the native VRCO trackd API is
REM -- installed.  This directory should have include and lib directories
REM -- and possibly a lib64 directory.  The trackd API lib is not shipped
REM -- with this package and must be obtained from VRCO.

set TRACKDHOME=%J3DINPUTHOME%\trackdAPI\win32

REM -- Get opt class and lib files from install directory.

set CLASSPATH=%J3DINPUTHOME%\lib\j3dInput.jar;%CLASSPATH%
set PATH=%J3DINPUTHOME%\lib;%TRACKDHOME%\lib;%PATH%

REM -- The rest of this script is for building and development.
REM -- Get the opt class and lib files from the build directory

REM set CLASSPATH=%J3DINPUTHOME%\classes\opt;%CLASSPATH%
REM set PATH=%J3DINPUTHOME%\build\win32\objs\opt;%TRACKDHOME%\lib;%PATH%

REM -- Get the debug class and lib files from the build directory

REM set CLASSPATH=%J3DINPUTHOME%\classes\debug;%CLASSPATH%
REM set PATH=%J3DINPUTHOME%\build\win32\objs\debug;%TRACKDHOME%\lib;%PATH%

REM -- Build the win32 version of the package.

set BUILD=win32

REM -- confirm paths

echo PATH=%PATH%
echo CLASSPATH=%CLASSPATH%
