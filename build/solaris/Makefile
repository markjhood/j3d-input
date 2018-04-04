# @(#)Makefile 1.39 03/05/21 14:20:30
# 
# Makefile for building the Solaris version of the com.sun.j3d.input package.
# This builds both the Java and native binaries.
#
# Supported targets:
# 
# install (default)
# Builds the optimized class and object files, builds the .so native
# libraries, and installs the jar and .so files in
# $(J3DINPUTHOME)/lib.
#
# opt debug
# Builds either optimized or debug class and object files and the .so
# native libraries.  The application class and library paths must be set 
# to access the class and .so files directly from the objs directory where 
# are built.
#
# javadoc
# Generates javadoc files and stores them in the top level html directory.
#
# clean
# Deletes all non-distributed files except for the javadoc and the
# installed jar and lib.
#
# clean-lib
# Invokes the clean target and then deletes the installed jar and lib.
#
# clean-all
# Deletes the top-level lib and html directories as well.

PKGNAME    = com.sun.j3d.input
PKGDIR     = com/sun/j3d/input
PKGHFILE   = com_sun_j3d_input
JARNAME    = j3dInput.jar
LIBNAME    = libj3dInput.so

FILES_java   = TrackdInputDevice.java LogitechTracker.java \
               Gameport.java SerialDevice.java Mouse2DValuator.java \
               EventDrivenInputDevice.java
FILES_jni    = TrackdInputDevice.c LogitechTracker.c \
               Gameport.c SerialDevice.c
FILES_c      = $(FILES_jni) redbarron.c gameport.c serial.c
FILES_h      = redbarron.h gameport.h serial.h

# ../../lib is the default directory for installation unless J3DINPUTHOME
# is defined in the environment.
J3DINPUTHOME:sh = echo ${J3DINPUTHOME:-../..}
LIBDIR = $(J3DINPUTHOME)/lib
LIBDIR64 = $(LIBDIR)/sparcv9

# Location of the native trackd API library.
# The default is ../../trackdAPI/solaris if TRACKDHOME 
# is undefined in the environment.
TRACKDHOME:sh = echo ${TRACKDHOME:-../../trackdAPI/solaris}

# Location of the JDK.  
# The default is /usr/java if JAVAHOME is undefined in the environment.
JAVAHOME:sh = echo ${JAVAHOME:-/usr/java}

# Add source directory to current CLASSPATH.
CURPATH:sh = echo ${CLASSPATH:-.}
CLASSPATH = ../../src:$(CURPATH)

CC      = cc 
CP      = cp
JAR     = jar
JAVAC   = javac
JAVAH   = javah
JAVADOC = javadoc
RM      = rm -f
RMDIR   = rm -rf
MKDIR   = mkdir -p

CFLAGS     = -v -xchip=ultra -DSOLARIS
CFLAGS32   = -xarch=v8a
CFLAGS64   = -xarch=v9a
CPPFLAGS   = -I$(TRACKDHOME)/include
CPPFLAGS  += -I$(JAVAHOME)/include -I$(JAVAHOME)/include/solaris
LDFLAGS32  = -L$(TRACKDHOME)/lib
LDFLAGS64  = -L$(TRACKDHOME)/lib64

# -lCstd and -lCrun below are for TrackdInputDevice.c.  ld won't find the
# libCstd and libCrun libraries in the standard Solaris configuration
# (they don't have symlinks from .so -> .so.1), so the cc compiler
# driver is used (via LINK.c).  The cc compiler driver needs the
# -xarch flag even for linking or it will fail with 64-bit libs.  The
# libCstd and libCrun symbols seem to be statically linked, however;
# the resulting .so is over 400K just for TrackdInputDevice.
LIBS = -ltrackdAPI -lCstd -lCrun -lm -lnls -ldl -lc

# -z defs for development to ensure all symbols are resolved.
# -z lazyload for production:  the native code may be built into a shared lib 
# along with unrelated code, and we don't want to require libtrackdAPI.so.
LDFLAGS = -G -z lazyload

# Default build variant is opt.
VARIANT = opt

# Set conditional macros depending upon build.
opt := VARIANT = opt
opt := JAVACFLAGS += -O
opt := CFLAGS += -O

debug := VARIANT = debug
debug := JAVACFLAGS += -g
debug := CFLAGS += -g -DDEBUG

TOPDIR   = ../..
SRCDIR   = $(TOPDIR)/src/$(PKGDIR)
OBJDIR32 = objs/$(VARIANT)
OBJDIR64 = $(OBJDIR32)/sparcv9
CLASSDIR = $(TOPDIR)/classes/$(VARIANT)
HTMLDIR  = $(TOPDIR)/html
BINDIRS  = $$(CLASSDIR) $$(OBJDIR32) $$(OBJDIR64)

FILES_src   = $(FILES_java:%=$(SRCDIR)/%)
FILES_class = $(FILES_java:%.java=$(CLASSDIR)/$(PKGDIR)/%.class)
FILES_html  = $(FILES_java:%.java=$(HTMLDIR)/$(PKGDIR)/%.html)
FILES_javah = $(FILES_jni:%.c=$(SRCDIR)/$(PKGHFILE)_%.h)
FILES_incl  = $(FILES_javah) $(FILES_h:%=$(SRCDIR)/%)
FILES_obj32 = $(FILES_c:%.c=$(OBJDIR32)/%.o)
FILES_obj64 = $(FILES_c:%.c=$(OBJDIR64)/%.o)
FILES_lib32 = $(OBJDIR32)/$(LIBNAME)
FILES_lib64 = $(OBJDIR64)/$(LIBNAME)
INSTALL_jar = $(LIBDIR)/$(JARNAME) 
INSTALL_lib = $(LIBDIR)/$(LIBNAME) $(LIBDIR64)/$(LIBNAME)

#
# Targets and rules.  
#
install:   opt $(INSTALL_jar) $(INSTALL_lib)
opt debug: $(BINDIRS) $$(FILES_class) $$(FILES_lib32) $$(FILES_lib64)
javadoc:   $(FILES_html)

clean:
	$(RM) $(FILES_javah) $(SRCDIR)/prj.el
	$(RMDIR) objs $(TOPDIR)/classes

clean-lib: clean
	$(RM) $(INSTALL_jar) $(INSTALL_lib)

clean-all: clean
	$(RMDIR) $(HTMLDIR) $(TOPDIR)/lib

$$(CLASSDIR)/$(PKGDIR)/%.class: $(SRCDIR)/%.java
	$(JAVAC) -classpath $(CLASSPATH) -d $(CLASSDIR) $(JAVACFLAGS) $<

$(SRCDIR)/$(PKGHFILE)_%.h: $$(CLASSDIR)/$(PKGDIR)/%.class
	$(RM) $@
	$(JAVAH) -classpath $(CLASSDIR) -d $(SRCDIR) $(PKGNAME).$*

$$(OBJDIR32)/%.o: $(FILES_incl) $(SRCDIR)/%.c
	$(COMPILE.c) $(CFLAGS32) -o $@ $<

$$(OBJDIR64)/%.o: $(FILES_incl) $(SRCDIR)/%.c
	$(COMPILE.c) $(CFLAGS64) -o $@ $<

$(FILES_html): $(FILES_src)
	$(JAVADOC) -d $(HTMLDIR) $(FILES_src)

$(INSTALL_jar): $(LIBDIR) $$(FILES_class)
	$(RM) $(INSTALL_jar)
	$(JAR) cvf $(INSTALL_jar) -C $(CLASSDIR) $(PKGDIR)

$(INSTALL_lib): $(LIBDIR) $(LIBDIR64) $$(FILES_lib32) $$(FILES_lib64)
	$(RM) $(INSTALL_lib)
	$(CP) $(FILES_lib32) $(LIBDIR)
	$(CP) $(FILES_lib64) $(LIBDIR64)

$(BINDIRS) $(LIBDIR) $(LIBDIR64) $(HTMLDIR):
	$(MKDIR) $@

objs/opt/$(LIBNAME) objs/debug/$(LIBNAME): $$(FILES_obj32)
	$(LINK.c) $(LDFLAGS32) $(CFLAGS32) $(FILES_obj32) $(LIBS) -o $@

objs/opt/sparcv9/$(LIBNAME) objs/debug/sparcv9/$(LIBNAME): $$(FILES_obj64)
	$(LINK.c) $(LDFLAGS64) $(CFLAGS64) $(FILES_obj64) $(LIBS) -o $@
