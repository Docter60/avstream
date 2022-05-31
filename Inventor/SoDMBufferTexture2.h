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



#ifndef _SO_DM_BUFFER_TEXTURE2_H
#define _SO_DM_BUFFER_TEXTURE2_H

#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/fields/SoSFMatrix.h>
#include <Inventor/fields/SoSFDMBufferImage.h>

//////////////////////////////////////////////////////////////////////////////
//
//  Class: SoTexture
//
//  Texture node.
//
//////////////////////////////////////////////////////////////////////////////

class SoDMBufferTexture2 : public SoTexture2 {

    SO_NODE_HEADER(SoDMBufferTexture2);

  public:

    // Fields.
    SoSFDMBufferImage	image;		// The texture
    SoSFMatrix		transform;

    // Constructor
    SoDMBufferTexture2();
    
  SoEXTENDER public:
    virtual void	doAction(SoAction *action);
    virtual void	GLRender(SoGLRenderAction *action);

  SoINTERNAL public:
    static void		initClass();

  protected:
    virtual ~SoDMBufferTexture2();

  private:
    Bool doOnce;
    GLuint textureID;

    static GLuint DMBUFFER_TEXTURE_OBJECT_ID;
};

#endif 
