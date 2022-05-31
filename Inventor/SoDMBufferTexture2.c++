/****************************************************************************
*                                                                          *
*               Copyright (c) 1996-97-98, Silicon Graphics, Inc.           *
*                                                                          *
*   Permission to use,  copy, modify, distribute, and sell this software   *
*   and its documentation for any purpose is hereby granted without fee,   *
*   provided that the name of  Silicon  Graphics  may not be used in any   *
*   advertising  or  publicity  relating  to  the  software  without the   *
*   specific, prior written permission of Silicon Graphics.                *
*                                                                          *
*   THIS SOFTWARE IS PROVIDED "AS-IS"  AND WITHOUT WARRANTY OF ANY KIND,   *
*   EXPRESS,  IMPLIED OR OTHERWISE,  INCLUDING  WITHOUT LIMITATION,  ANY   *
*   WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.       *
*                                                                          *
*   IN  NO  EVENT  SHALL  SILICON  GRAPHICS  BE  LIABLE FOR ANY SPECIAL,   *
*   INCIDENTAL,  INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,  OR  ANY   *
*   DAMAGES  WHATSOEVER  RESULTING  FROM  LOSS OF USE,  DATA OR PROFITS,   *
*   WHETHER OR NOT ADVISED OF  THE  POSSIBILITY  OF  DAMAGE,  AND ON ANY   *
*   THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR   *
*   PERFORMANCE OF THIS SOFTWARE.                                          *
*                                                                          *
****************************************************************************/



#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoGLTextureEnabledElement.h>
#include <Inventor/elements/SoTextureQualityElement.h>
#include <Inventor/elements/SoTextureCoordinateElement.h>
#include <Inventor/nodes/SoDMBufferTexture2.h>

#include <stdio.h>
#include <Inventor/misc/checks.h>

SO_NODE_SOURCE(SoDMBufferTexture2);

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructor
//
// Use: public

GLuint SoDMBufferTexture2::DMBUFFER_TEXTURE_OBJECT_ID = 0;

void
SoDMBufferTexture2::initClass() {
    if (getClassTypeId().isBad())
	SoTexture2::initClass();

    SO_NODE_INIT_CLASS(SoDMBufferTexture2, SoTexture2, "SoTexture2");
}

SoDMBufferTexture2::SoDMBufferTexture2(): SoTexture2()
//
////////////////////////////////////////////////////////////////////////
{
    SO_NODE_CONSTRUCTOR(SoDMBufferTexture2);

    SO_NODE_ADD_FIELD(image, ( NULL ));
    SO_NODE_ADD_FIELD(transform,( 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 ));
    doOnce = TRUE;

    textureID = DMBUFFER_TEXTURE_OBJECT_ID++;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Destructor
//
// Use: private

SoDMBufferTexture2::~SoDMBufferTexture2()
//
////////////////////////////////////////////////////////////////////////
{
    SoTexture2::~SoTexture2();
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Performs typical action
//
// Use: extender

void
SoDMBufferTexture2::doAction(SoAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    SoState *	state = action->getState();

    if (image.isIgnored() ||
	SoTextureOverrideElement::getImageOverride(state))
	return; // Texture being overriden or this node ignored
    if (isOverride()) {
	SoTextureOverrideElement::setImageOverride(state, TRUE);
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Performs GL rendering on a texture node.
//
// Use: extender

void
SoDMBufferTexture2::GLRender(SoGLRenderAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    SoState *	state = action->getState();

    if (image.isIgnored() ||
	SoTextureOverrideElement::getImageOverride(state))
	return; // Texture being overriden or this node ignored
    if (isOverride()) {
	SoTextureOverrideElement::setImageOverride(state, TRUE);
    }

    SbDMBuffer *dmBuffer = image.getValue();

    if ( dmBuffer != NULL ){

	DMbuffer texture = dmBuffer->getValue();
	if ( texture == NULL ){
	    dmBuffer->releaseValue();
	    return;
	}

	SbVec2s size = dmBuffer->getSize();

	glBindTextureEXT( GL_TEXTURE_2D, textureID );
	if ( doOnce ){
	    // More than one texture in the scene? 
	    // Tell current context what size to expect...
	    glTexImage2D(GL_TEXTURE_2D, 0, dmBuffer->getGLInternalFormat(),
			 size[0], size[1],
			 0,
			 dmBuffer->getGLFormat(),
			 dmBuffer->getGLDataType(),
			 NULL );
	    doOnce = FALSE;
	}

	int result;
	result = glXAssociateDMPbufferSGIX( dmBuffer->getDisplay(), 
					    dmBuffer->getPBuffer(), 
					    dmBuffer->getFormat(), 
					    texture);
	XCHECK( result, "Unable to associate glPbuffer with allocated video dmBuffer.\n");
	dmBuffer->releaseValue();

	Bool success;
	success = glXMakeCurrentReadSGI( dmBuffer->getDisplay(), glXGetCurrentDrawable(), 
					 dmBuffer->getPBuffer(), glXGetCurrentContext());
	XCHECK( success == True, "Unable to make glPbuffer the current readable!\n");

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS.getValue());
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT.getValue());
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, model.getValue());
 	glBlendColorEXT( blendColor.getValue()[0],
 			 blendColor.getValue()[1],
 			 blendColor.getValue()[2],
 			 blendColor.getValue()[3] );

	glCopyTexSubImage2DEXT(GL_TEXTURE_2D, 0, 
			       0, 0,
			       0, 0,				// x, y
			       size[0] , size[1]
			       );
      
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glMatrixMode(GL_TEXTURE);
	glLoadMatrixf(*transform.getValue().getValue());
	glMatrixMode(GL_MODELVIEW);
      
	SoGLTextureEnabledElement::set(state, TRUE);
	SoTextureCoordinateElement::setDefault(state, this);
    }
}
