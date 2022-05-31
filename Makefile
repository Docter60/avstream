#!smake

# /usr/include/make/commondefs and /usr/include/make/commonrules
# define some useful Makefile rules.  For example, they
# define a 'clean' target, so you can type 'make clean'
# to remove all .o files (or other files generated during
# compilation). See the file /usr/include/make/commonrules for
# documentation on what targets are supported.

include /usr/include/make/commondefs

PROGRAM = a.out
TARGETS = $(PROGRAM)

LLDLIBS =-lInventorDMBuffer -lInventorXt -lInventor \
         -lGLU -lGL -lXm -lXt -lX11 -lm -lvl -ldmedia \
         -laudio
C++FILES = avstream.c++

# Use compiler flag IV_STRICT to catch things in the code that are not
# recommended for use with Inventor 2.1
LC++DEFS = -DIV_STRICT

default: $(PROGRAM)

include $(COMMONRULES)

$(PROGRAM): $(OBJECTS)
	$(C++F) -o $@ $(OBJECTS) $(LDFLAGS)
