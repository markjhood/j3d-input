# @(#)Makefile 1.17 03/05/21 14:23:43
# 
# Makefile for building the win32 version of the com.sun.j3d.input
# package.  This builds both the Java and native binaries using the
# `nmake' Program Maintenance Utility from Microsoft.
#
# Supported targets:
# 
# install (default)
# Builds the optimized class and object files, the .dll native
# library, and installs installs the jar and .dll files in
# $(J3DINPUTHOME)\lib.
#
# opt debug
# Builds either optimized or debug class and object files and the .dll
# native library.  The application class and library paths must be set 
# to access the class and .dll files directly from the objs directory where 
# are built.
#
# javadoc
# Generates javadoc files and stores them in the top level html directory.
#
# clean
# Deletes all non-distributed files except for the javadoc and
# installed jar and lib.
#
# clean-lib
# Invokes the clean target and then deletes the installed jar and lib.
#
# clean-all
# Deletes the top-level html and lib directories as well.
#
# NOTE: This Makefile currently doesn't create a working
# TrackdInputDevice implementation.  All objects are built without
# errors, and at runtime j3dInput.dll loads correctly, but the
# LoadLibrary("trackdAPI_MT.dll") call in the native code fails with
# GetLastError returning 0x1e7 (487 decimal): "Attempt to access
# invalid address."
#
# It seems like it could be a problem with trackdAPI_MT.dll from VRCO,
# but it does get loaded correctly when building and running the
# example program shipped by VRCO.  I've tried building without the
# /DELAYLOAD option, but then the Java System.loadLibrary("j3dInput")
# call fails with the same error message.
#
# If anyone manages to get this working, please let me know...
# mark.hood `at' sun.com.

PKGNAME    = com.sun.j3d.input
PKGDIR     = com\sun\j3d\input
PKGHFILE   = com_sun_j3d_input
JARNAME    = j3dInput.jar
LIBNAME    = j3dInput.dll

# No pattern replacement in macros, just suffix replacement, so we do
# this explicitly instead.
FILES_java  = $(SRCDIR)\TrackdInputDevice.java \
              $(SRCDIR)\Mouse2DValuator.java \
              $(SRCDIR)\EventDrivenInputDevice.java
FILES_class = $(CLASSDIR)\$(PKGDIR)\TrackdInputDevice.class \
              $(CLASSDIR)\$(PKGDIR)\Mouse2DValuator.class \
              $(CLASSDIR)\$(PKGDIR)\EventDrivenInputDevice.class
FILES_html  = $(HTMLDIR)\$(PKGDIR)\TrackdInputDevice.html \
              $(HTMLDIR)\$(PKGDIR)\Mouse2DValuator.html \
              $(HTMLDIR)\$(PKGDIR)\EventDrivenInputDevice.html
FILES_obj   = $(OBJDIR)\TrackdInputDevice.obj
FILES_javah = TrackdInputDevice.h

# Command macros for Windows.
CC      = cl
JAR     = jar
JAVAC   = javac
JAVAH   = javah
JAVADOC = javadoc
MKDIR   = md
MAKE    = nmake
LD      = link

# The next are Unix ports.  Need the equivalent -f and -r functionality
# for deleting files, and the copy command doesn't seem to update the
# timestamp.
CP      = cp
RM      = rm -f
RMDIR   = rm -rf

# No conditional macros, so we do this instead.
!if "$(VARIANT)" == "opt"
JAVACFLAGS = -O
CFLAGS  = -nologo -MT -W3 -GX -Ox -YX -FD
DEFINES = -DWIN32 -DNDEBUG -D_WINDOWS
!else
JAVACFLAGS = -g
CFLAGS  = -nologo -MTd -W3 -Gm -GX -Zi -Od -YX -FD
DEFINES = -DWIN32 -DDEBUG -D_WINDOWS
!endif

INCLUDES = -I$(TRACKDHOME)\include \
           -I$(JAVAHOME)\include -I$(JAVAHOME)\include\win32
LDFLAGS  = -nologo -dll -subsystem:windows -pdb:none -machine:I386 \
           delayimp.lib -DELAYLOAD:trackdAPI_MT.dll \
           -libpath:$(TRACKDHOME)\lib
LIBS     = trackdAPI_MT.lib

TOPDIR   = ..\..
SRCDIR   = $(TOPDIR)\src\$(PKGDIR)
OBJDIR   = objs\$(VARIANT)
LIBDIR   = $(J3DINPUTHOME)\lib
HTMLDIR  = $(TOPDIR)\html
CLASSDIR = $(TOPDIR)\classes\$(VARIANT)
BINDIRS  = $(OBJDIR) $(CLASSDIR) $(LIBDIR)

FILES_lib   = $(OBJDIR)\$(LIBNAME)
INSTALL_jar = $(LIBDIR)\$(JARNAME) 
INSTALL_lib = $(LIBDIR)\$(LIBNAME)

#
# Targets and rules.  
#
install:
	@$(MAKE) /nologo do-install VARIANT=opt
opt:     
	@$(MAKE) /nologo do-opt VARIANT=opt
debug:   
	@$(MAKE) /nologo do-debug VARIANT=debug

javadoc: $(FILES_html)

clean:
	$(RM) *.idb *.pch *.pdb
	$(RM) $(FILES_javah) $(SRCDIR)\$(PKGHFILE)* $(SRCDIR)\prj.el
	$(RMDIR) objs $(TOPDIR)\classes

clean-lib: clean
	$(RM) $(INSTALL_jar) $(INSTALL_lib)

clean-all: clean
	$(RMDIR) $(HTMLDIR) $(TOPDIR)\lib

do-install:      $(BINDIRS) $(INSTALL_jar) $(INSTALL_lib)
do-opt do-debug: $(BINDIRS) $(FILES_class) $(FILES_lib)

# No pattern matching in rules, so we use suffix rules instead.
.SUFFIXES: .java .class

{$(SRCDIR)}.java{$(CLASSDIR)\$(PKGDIR)}.class:
	$(JAVAC) -d $(CLASSDIR) $(JAVACFLAGS) $<

{$(SRCDIR)}.c{$(OBJDIR)}.obj:
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -Fo$@ -c $<

$(FILES_lib): $(FILES_obj)
	$(LD) $(LDFLAGS) $(FILES_obj) $(LIBS) -out:$@

$(FILES_obj): $(FILES_javah)

$(INSTALL_jar): $(FILES_class)
	$(RM) $(INSTALL_jar)
	$(JAR) cvf $(INSTALL_jar) -C $(CLASSDIR) $(PKGDIR)

$(INSTALL_lib): $(FILES_lib)
	$(RM) $(INSTALL_lib)
	$(CP) $(FILES_lib) $@

$(FILES_html): $(FILES_java)
	$(JAVADOC) -d $(HTMLDIR) $(FILES_java)

# Copies the .h without the package prefix to create the target that
# satisfies this rule.  Non-prefixed .h is needed so its basename can
# be concatenated with the package name to create the class name.
$(FILES_javah): $(FILES_class)
	$(RM) $@ $(SRCDIR)\$(PKGHFILE)_$@
	$(JAVAH) -classpath $(CLASSDIR) -d $(SRCDIR) $(PKGNAME).$*
	$(CP) $(SRCDIR)\$(PKGHFILE)_$@ $@

# Have to explicitly check for directory existence before creating one.
$(BINDIRS):
	if not exist objs $(MKDIR) objs
	if not exist objs\$(VARIANT) $(MKDIR) objs\$(VARIANT)
	if not exist $(J3DINPUTHOME) $(MKDIR) $(J3DINPUTHOME)
	if not exist $(J3DINPUTHOME)\lib $(MKDIR) $(J3DINPUTHOME)\lib
	if not exist $(TOPDIR)\classes $(MKDIR) $(TOPDIR)\classes
	if not exist $(TOPDIR)\classes\$(VARIANT) \
	     $(MKDIR) $(TOPDIR)\classes\$(VARIANT)
