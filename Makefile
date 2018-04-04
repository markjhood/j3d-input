# @(#)Makefile 1.12 02/08/20 18:05:41
# 
# Top-level Makefile for building the com.sun.j3d.input package and
# test programs on Unix system variants.  This builds both the Java
# and native binaries.
#
# $(BUILD) specifies the build to perform; it must be the name of one
# of the directories in ./build and defaults to solaris if not
# specified.  $(J3DINPUTHOME) defaults to the current directory if not
# specified.
#
# Supported targets:
# 
# all (default)
# Builds the optimized class, object, and .so native library files.
# Builds the jar file and copies the .so files into $(J3DINPUTHOME)/lib.  
# Generates the javadoc files into the top level html directory.
# Builds the programs in the test directory.
#
# javadoc
# Generates javadoc files and stores them in the top level html directory.
#
# tests
# Builds the test programs in the test directory.
# 
# clean
# Deletes all non-distributed files except the javadoc and the installed
# jar and lib.
#
# clean-lib
# Invokes the clean target and then deletes the installed jar and lib.
#
# clean-all
# Deletes the top-level html and lib directories as well.
#
# dist-src
# Invokes the clean target and generates a .tar.gz archive using the 
# current directory as the basename.
#
# dist-lib
# Invokes the clean target and generates a .tar.gz archive using the
# current directory as the basename.  The files in the top level lib
# directory are archived as well.

BUILD:sh = echo ${BUILD:-solaris}
CLASSPATH:sh = echo ${CLASSPATH:-.}
J3DINPUTHOME:sh = echo ${J3DINPUTHOME:-$PWD}
DIR:sh = basename $PWD

FILES_dist = src test build paths.sh paths.bat Makefile README
FILES_tar = $(FILES_dist:%=$(DIR)/%)
dist-lib := FILES_tar += $(DIR)/lib

all: $(BUILD) tests javadoc

$(BUILD):
	(cd build/$(BUILD); make)

tests:
	(cd test; make CLASSPATH=$(J3DINPUTHOME)/lib/j3dInput.jar:$(CLASSPATH))

javadoc:
	(cd build/$(BUILD); make javadoc)

dist-src: clean
	(cd ..; tar cvFf - $(FILES_tar)) | gzip -c > $(DIR)-src.tar.gz

dist-lib: $(BUILD) clean
	(cd ..; tar cvFf - $(FILES_tar)) | gzip -c > $(DIR)-lib.tar.gz

clean clean-lib clean-all:
	(cd build/$(BUILD); make $@)
	(cd test; make clean)

