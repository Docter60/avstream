#!smake

include /usr/include/make/commondefs

LIB = libInventorDMBuffer.a

LC++OPTS = -woff 3262 -woff 3115 -woff 3666 -ptused

TARGETS = \
	SbDMBuffer \
	SoDMBufferBackground \
	SoDMBufferDMICMovieEngine \
	SoDMBufferEngine \
	SoDMBufferMovieEngine \
	SoDMBufferTexture2 \
	SoDMBufferVideoEngine \
	SoSFDMBufferImage \
	$(NULL)

C++FILES = $(TARGETS:=.c++)

LDIRT = $(LIB)

default: $(LIB)

include $(COMMONRULES)

$(LIB): $(OBJECTS)
	$(AR) crs $@ $(OBJECTS)
