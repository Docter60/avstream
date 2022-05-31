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


#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/SbBox.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/nodes/SoDMBufferBackground.h>

#include <Inventor/misc/checks.h>

SO_NODE_SOURCE(SoDMBufferBackground);

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructor
//
// Use: public

void
SoDMBufferBackground::initClass() {
    SO_NODE_INIT_CLASS(SoDMBufferBackground, SoShape, "SoShape");
}

SoDMBufferBackground::SoDMBufferBackground()
//
////////////////////////////////////////////////////////////////////////
{
    SO_NODE_CONSTRUCTOR(SoDMBufferBackground);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Destructor
//
// Use: private

SoDMBufferBackground::~SoDMBufferBackground()
//
////////////////////////////////////////////////////////////////////////
{
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Performs GL rendering on a texture node.
//
// Use: extender

void
SoDMBufferBackground::GLRender(SoGLRenderAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    SoState *	state = action->getState();

    glPushAttrib(GL_ENABLE_BIT);  
    glPixelTransferf(GL_RED_SCALE, 1.0);  // Default
    glPixelTransferf(GL_GREEN_SCALE, 1.0);  // Default
    glPixelTransferf(GL_BLUE_SCALE, 1.0);  // Default
    glPixelTransferf(GL_ALPHA_SCALE, 1.0);  // Default
    glPixelTransferf(GL_RED_BIAS, 0.0);  // Default
    glPixelTransferf(GL_GREEN_BIAS, 0.0);  // Default
    glPixelTransferf(GL_BLUE_BIAS, 0.0);  // Default
    glPixelTransferf(GL_ALPHA_BIAS, 0.0);  // Default

    glDisable(GL_LIGHTING);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    SbViewportRegion vpr = SoViewportRegionElement::get(state);
    const SbVec2s vprSize= vpr.getWindowSize();
      
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0,1,0,1);
      
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glViewport(0, 0, vprSize[0], vprSize[1]);

    glEnable(GL_TEXTURE_2D);
    glBegin(GL_POLYGON);
    glTexCoord2f( 0, 0); glVertex2i( 0, 0);
    glTexCoord2f( 0, 1); glVertex2i( 0, 1);
    glTexCoord2f( 1, 1); glVertex2i( 1, 1);
    glTexCoord2f( 1, 0); glVertex2i( 1, 0);
    glEnd();
    
    glPopAttrib();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void
SoDMBufferBackground::generatePrimitives(SoAction *){

}

void
SoDMBufferBackground::computeBBox(SoAction *, SbBox3f &, SbVec3f &){

}

