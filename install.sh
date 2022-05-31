#!/bin/sh

INVENTOR_PATH=Inventor/
NODES_PATH=nodes/
ENGINES_PATH=engines/
FIELDS_PATH=fields/
MISC_PATH=misc/

INCLUDE_PATH=/usr/include/
INCLUDE_INVENTOR_PATH=$INCLUDE_PATH$INVENTOR_PATH
INCLUDE_NODES_PATH=$INCLUDE_INVENTOR_PATH$NODES_PATH
INCLUDE_ENGINES_PATH=$INCLUDE_INVENTOR_PATH$ENGINES_PATH
INCLUDE_FIELDS_PATH=$INCLUDE_INVENTOR_PATH$FIELDS_PATH
INCLUDE_MISC_PATH=$INCLUDE_INVENTOR_PATH$MISC_PATH

LIB=libInventorDMBuffer.a
LIB_PATH=/usr/lib/
LIB32_PATH=/usr/lib32/

cd Inventor
# Install Inventor plugin header files
cp SbDMBuffer.h $INCLUDE_INVENTOR_PATH
cp SoDMBufferBackground.h SoDMBufferTexture2.h $INCLUDE_NODES_PATH
cp SoDMBufferDMICMovieEngine.h SoDMBufferEngine.h SoDMBufferMovieEngine.h SoDMBufferVideoEngine.h $INCLUDE_ENGINES_PATH
cp SoSFDMBufferImage.h $INCLUDE_FIELDS_PATH
cp checks.h $INCLUDE_MISC_PATH

# Make the Inventor plugin library
make

# Install symbolic links pointing to library
unlink $LIB_PATH$LIB
unlink $LIB32_PATH$LIB
ln -s $PWD/$LIB $LIB_PATH$LIB
ln -s $PWD/$LIB $LIB32_PATH$LIB

# Build the avstream project
cd ..
make
