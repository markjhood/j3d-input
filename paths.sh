#! /bin/csh -f

# J3DINPUTHOME is the directory where this package is installed.  A
# lib directory will be created there to contain the jar and shared
# library if it doesn't exist. The default is the current directory.
#
if (! $?J3DINPUTHOME) setenv J3DINPUTHOME ${PWD}

# TRACKDHOME is the directory where the native VRCO trackd API is
# installed.  This directory should have include and lib directories
# and possibly a lib64 directory.  The trackd API lib is not shipped
# with this package and must be obtained from VRCO.
#
if (! $?TRACKDHOME) setenv TRACKDHOME ${PWD}/trackdAPI/solaris

# Defaults for CLASSPATH and LD_LIBRARY_PATH if they don't exist.
#
if (! $?CLASSPATH) setenv CLASSPATH .
if (! $?LD_LIBRARY_PATH) setenv LD_LIBRARY_PATH /usr/lib

# Get opt class and 32-bit lib files from install directory.
#
setenv CLASSPATH ${J3DINPUTHOME}/lib/j3dInput.jar:${CLASSPATH}
setenv LD_LIBRARY_PATH ${J3DINPUTHOME}/lib:${TRACKDHOME}/lib:${LD_LIBRARY_PATH}

#
# The rest of this script is used only for building and testing the package.
#
# BUILD must be one of the directory names in the build directory.
if (! $?BUILD) setenv BUILD solaris
#
# For 64-bit builds, DIR64 is the subdirectory of the 32-bit obj and
# lib directories where the corresponding 64-bit variants are found.
if (! $?DIR64) setenv DIR64 sparcv9
#
# Get class files from opt build.
# setenv CLASSPATH ${PWD}/classes/opt:${CLASSPATH}
# 
# Get 64-bit lib from opt build.
# setenv LD_LIBRARY_PATH ${PWD}/build/${BUILD}/objs/opt/${DIR64}:${TRACKDHOME}/lib64:${LD_LIBRARY_PATH}
#
# Get 32-bit lib from opt build.
# setenv LD_LIBRARY_PATH ${PWD}/build/${BUILD}/objs/opt:${TRACKDHOME}/lib:${LD_LIBRARY_PATH}
# 
# Get class files from debug build.
# setenv CLASSPATH ${PWD}/classes/debug:${CLASSPATH}
#
# Get 64-bit lib from debug build
# setenv LD_LIBRARY_PATH ${PWD}/build/${BUILD}/objs/debug/${DIR64}:${TRACKDHOME}/lib64:${LD_LIBRARY_PATH}
#
# Get 32-bit lib from debug build.
# setenv LD_LIBRARY_PATH ${PWD}/build/${BUILD}/objs/debug:${TRACKDHOME}/lib:${LD_LIBRARY_PATH}

echo using J3DINPUTHOME=${J3DINPUTHOME}
echo using TRACKDHOME=${TRACKDHOME}
echo using CLASSPATH=${CLASSPATH}
echo using LD_LIBRARY_PATH=${LD_LIBRARY_PATH}
