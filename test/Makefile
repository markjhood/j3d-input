# @(#)Makefile 1.4 02/08/20 16:42:48

.SUFFIXES: .java .class
.java.class:
	javac $<

FILES_java  = InputTest.java
FILES_class = $(FILES_java:.java=.class)

all: $(FILES_class)
clean:
	rm -f *.class *.log prj.el
