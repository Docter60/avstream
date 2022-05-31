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


// X
#include <X11/Xlib.h>

// Local
#include <Inventor/SbDMBuffer.h>
#include <Inventor/misc/checks.h>

// MVP
#include <vl/dev_mvp.h>

// DMIC
#include <dmedia/dm_params.h>
#include <dmedia/dm_imageconvert.h>

#define WIDTH  0
#define HEIGHT 1

SbDMBuffer::SbDMBuffer( const SbVec2s &size ){

    _size = size;
    _poolParams = NULL;
    _dmBufferPool = NULL;
    _imageFormat = NULL;
    _lockedBuffer = NULL;
    _GL_FORMAT = _GL_INTERNAL_FORMAT = GL_RGB8_EXT;
    _bufferCount = -1;

    _dmICDecoder = NULL;
    _dmICInputBufferPool = NULL;
    _dmICOutputBufferPool = NULL;
    _dmICInputFrameSize = 0;
    _dmICInputBuffer = NULL;

    _LOCK_STATE = UNLOCKED;

    //--- Semaphores -----------------------------------
    usconfig(CONF_ARENATYPE, US_SHAREDONLY);
    _arena = usinit(mktemp("/tmp/alXXXXXX"));
    if (_arena == NULL)
      perror("Trouble creating arena.\n");

    _workBufferSema = usnewsema(_arena, 1);
    if (_workBufferSema == NULL)
	perror("SbDMBuffer::Trouble creating mutex semaphore.\n");

    //--- Semaphores  -----------------------------------
}


SbDMBuffer::~SbDMBuffer( void ){
    glXDestroyGLXPbufferSGIX(_display, _glPBuffer);
    glXDestroyContext(_display, _glContext);

    lock();
    if ( _workBuffer != _lockedBuffer && _workBuffer != NULL){
	dmBufferFree( _workBuffer );
	_workBuffer = NULL;
    }

    if ( _lockedBuffer != NULL ){
	dmBufferFree( _lockedBuffer );
	_lockedBuffer = NULL;
    }

    if ( _dmBufferPool != NULL ){
	DMstatus status;
	status = dmBufferDestroyPool( _dmBufferPool );
	XCHECK_DM( status == DM_SUCCESS, "SbDMBuffer::Could not destroy dmbuffer!.\n" );
	_dmBufferPool = NULL;
    }
    release();

    if ( _workBufferSema != NULL ){
	usfreesema( _workBufferSema, _arena );
	_workBufferSema = NULL;
    }
}

void
SbDMBuffer::lock( void ) {
    uspsema( _workBufferSema );
    _LOCK_STATE = LOCKED;
}

void
SbDMBuffer::release( void ){
    usvsema( _workBufferSema );
    _LOCK_STATE = UNLOCKED;
}

unsigned char *
SbDMBuffer::getValue(SbVec2s &size, int &nc, unsigned char **pixels) {
    lock();
    size = _size;
    nc = 4;
    return *pixels = (unsigned char *) dmBufferMapData( _lockedBuffer );
}

unsigned char *
SbDMBuffer::getValue(unsigned char **pixels) {
    lock();
    return *pixels = (unsigned char *) dmBufferMapData( _lockedBuffer );
}

DMbuffer
SbDMBuffer::getValue(SbVec2s &size, int &nc)
{
    lock();
    size = _size;
    nc = 4;
    return _lockedBuffer;
};

DMbuffer
SbDMBuffer::getValue( void )
{ 
    lock();
    return _lockedBuffer;
};

SbBool
SbDMBuffer::constrain( DMparams *imageFormat ){

    XCHECK( _imageFormat == NULL, "SbDMBuffer::You only need specify a single pbuffer." );
    _imageFormat = imageFormat;

    switch ( dmParamsGetEnum( imageFormat, DM_IMAGE_PACKING )){
      case DM_IMAGE_PACKING_RGBA:
      case DM_IMAGE_PACKING_XRGB:
	_GL_FORMAT = GL_RGBA;
	_GL_INTERNAL_FORMAT = GL_RGBA8_EXT;
	break;
      case DM_IMAGE_PACKING_XRGB1555:
	_GL_FORMAT = GL_RGB5_A1_EXT;
	_GL_INTERNAL_FORMAT = GL_RGB5_A1_EXT;
	break;
    }

    // Allocate if necessary
    DMstatus status;
    if ( !_poolParams ){
	status = dmParamsCreate( &_poolParams );
	XCHECK_DM( status == DM_SUCCESS, "SbDMBuffer::Could not create dmbuffer pool parameter.\n" );

	status = dmBufferSetPoolDefaults( _poolParams, 0, 0, DM_FALSE, DM_FALSE);
	XCHECK_DM( status == DM_SUCCESS, "SbDMBuffer::Could not set pool defaults" );
    }

    // Constrain
    status = dmBufferGetGLPoolParams(_imageFormat, _poolParams);
    XCHECK_DM( status == DM_SUCCESS , "SbDMBuffer::Unable to constrain pool paramters to GL pbuffer.\n" );

    return status == DM_SUCCESS;
}

SbBool
SbDMBuffer::constrain( VLServer server, VLPath path, VLNode node ){

    // Allocated if necessary
    DMstatus status;
    if ( !_poolParams ){
	status = dmParamsCreate( &_poolParams );
	XCHECK_DM( status == DM_SUCCESS, "SbDMBuffer::Could not create dmbuffer pool parameter.\n" );

	status = dmBufferSetPoolDefaults( _poolParams, 0, 0, DM_FALSE, DM_FALSE);
	XCHECK_DM( status == DM_SUCCESS, "SbDMBuffer::Could not set pool defaults" );
    }

    VLControlValue val;
    int vlstatus;

    vlstatus = vlGetControl( server, path, node, VL_PACKING, &val );
    XCHECK_VL( vlstatus == 0, "Could not set control" );

    switch ( val.intVal ){
      case VL_PACKING_RGBA_8:
	_GL_FORMAT = GL_ABGR_EXT;
	_GL_INTERNAL_FORMAT = GL_ABGR_EXT;
	break;
      case VL_PACKING_ABGR_8:
	_GL_FORMAT = GL_RGBA;
	_GL_INTERNAL_FORMAT = GL_RGBA8_EXT;
	break;
      case VL_PACKING_ARGB_1555:
	_GL_FORMAT = GL_RGB5_A1_EXT;
	_GL_INTERNAL_FORMAT = GL_RGB5_A1_EXT;
	break;
    }

    // Constrain
    int result;
    result = vlDMPoolGetParams( server, path, node, _poolParams );
    XCHECK_VL( result != -1, "SbDMBuffer::Unable to constrain pool parameters to video path.\n");

    return status == DM_SUCCESS;
}

SbBool
SbDMBuffer::constrain( int decoderDriver, DMparams *convParams, SbVec2s size){

    DMstatus status;

    // Build Decoder
    status = dmICCreate( decoderDriver, &_dmICDecoder );
    XCHECK_DM( status == DM_SUCCESS, "Could not instantiate image conversion." );

    status = dmICSetSrcParams( _dmICDecoder, convParams );
    XCHECK_DM( status == DM_SUCCESS, "Could not set source parameters for conversion." );
    _dmICInputFrameSize = dmImageFrameSize(convParams);

    // Destination Image
    status = dmParamsCreate(&_imageFormat);
    XCHECK_DM( status == DM_SUCCESS, "Could not create dmparams." );

    status = dmSetImageDefaults( _imageFormat, _size[ WIDTH ], _size[ HEIGHT ], DM_IMAGE_PACKING_RGBA );
    XCHECK_DM( status == DM_SUCCESS, "Could not set image defaults." );

    status = dmParamsSetEnum( _imageFormat, DM_IMAGE_LAYOUT, DM_IMAGE_LAYOUT_GRAPHICS);
    XCHECK_DM( status == DM_SUCCESS, "Could not set image layout." );

    status = dmICSetDstParams( _dmICDecoder, _imageFormat );
    XCHECK_DM( status == DM_SUCCESS, "Could not set destination parameters for conversion." );

    _GL_FORMAT = GL_RGBA;
    _GL_INTERNAL_FORMAT = GL_RGBA8_EXT;

    // Create Input Pool for Conversion
    DMparams *poolParams;
    status = dmParamsCreate(&poolParams);
    XCHECK_DM( status == DM_SUCCESS, "Could not create dmparams." );

#define	NOTCACHED	DM_FALSE
#define	NOTMAPPED	DM_FALSE
#define	IBUFS	3
    status = dmBufferSetPoolDefaults(poolParams, IBUFS, _dmICInputFrameSize, NOTCACHED, NOTMAPPED);
    XCHECK_DM( status == DM_SUCCESS, "Could not set src pool defaults" );

    status = dmICGetSrcPoolParams( _dmICDecoder, poolParams );
    XCHECK_DM( status == DM_SUCCESS, "Could not constrain pool parameters to converter source." );

    status = dmBufferCreatePool( poolParams, &_dmICInputBufferPool );
    XCHECK_DM( status == DM_SUCCESS, "Could not create input pool." );

    dmParamsDestroy( poolParams );

    // Create Output Pool for Conversion
    status = dmParamsCreate(&poolParams);
    XCHECK_DM( status == DM_SUCCESS, "Could not create dmparams." );

#define	OBUFS	4
#define MAPPED DM_TRUE
    status = dmBufferSetPoolDefaults(poolParams, OBUFS, _dmICInputFrameSize, NOTCACHED, MAPPED);
    XCHECK_DM( status == DM_SUCCESS, "Could not set dest pool defaults" );

    status = dmICGetDstPoolParams( _dmICDecoder, poolParams );
    XCHECK_DM( status == DM_SUCCESS, "Could not constrain pool parameters to converter destination." );

    status = dmBufferGetGLPoolParams( _imageFormat, poolParams);
    XCHECK_DM( status == DM_SUCCESS , "SbDMBuffer::Unable to constrain pool paramters to GL pbuffer.\n" );

    status = dmBufferCreatePool( poolParams, &_dmICOutputBufferPool );
    XCHECK_DM( status == DM_SUCCESS, "Could not create output pool." );
    dmParamsDestroy( poolParams );

    status = dmICSetDstPool( _dmICDecoder, _dmICOutputBufferPool );
    XCHECK_DM( status == DM_SUCCESS, "Could not establish pool as destination." );

    // Create Pool File Descriptor
    status = dmBufferSetPoolSelectSize(_dmICInputBufferPool, _dmICInputFrameSize);
    XCHECK_DM( status == DM_SUCCESS, "Could not set file descriptor reporting size for input pool." );

    return True;
}

int
SbDMBuffer::getDMICSourcePoolFD( void ){
    return dmBufferGetPoolFD(_dmICInputBufferPool);
}

int
SbDMBuffer::getDMICDestPoolFD( void ){
    return dmICGetDstQueueFD(_dmICDecoder);
}

SbBool
SbDMBuffer::setValueFromDecoder( void ){
    DMstatus status;

    status = dmICReceive(_dmICDecoder, &_workBuffer);
    XCHECK_DM( status == DM_SUCCESS, "Couldn't recieve image from converter." );

    swapBuffers();

    return status == DM_SUCCESS;
}

unsigned char *
SbDMBuffer::startEditingEncoder( void ){
    DMstatus status;

    status = dmBufferAllocate(_dmICInputBufferPool, &_dmICInputBuffer);
    XCHECK_DM( status == DM_SUCCESS, "Unable to allocated dmBuffer for source editing.\n");

    unsigned char *pixels = (unsigned char *) dmBufferMapData( _dmICInputBuffer );

    return pixels;
}

void
SbDMBuffer::finishEditingEncoder( int nbytes ){
    DMstatus status;

    status = dmBufferSetSize(_dmICInputBuffer, nbytes);
    XCHECK_DM( status == DM_SUCCESS, "Unable to establish buffer size.\n");

    if (dmICSend(_dmICDecoder, _dmICInputBuffer, 0, 0) == DM_FAILURE)
	printf("dmICSend dropped frame");

    dmBufferFree(_dmICInputBuffer);
}

SbBool
SbDMBuffer::setBufferCount( int bufferCount ){
    DMstatus status;
    XCHECK( _bufferCount == -1, "SbDMBuffer::Buffer count may only be allocated once.\n");
 
    _bufferCount = bufferCount;
    status = dmParamsSetInt( _poolParams, DM_BUFFER_COUNT, bufferCount );
    XCHECK_DM( status == DM_SUCCESS, "Unable to adjust buffer count.\n");

    return status == DM_SUCCESS;
}

SbBool
SbDMBuffer::createPool( void ){
    DMstatus status;
    XCHECK( _dmBufferPool == NULL, "SbDMBuffer::You may only create the buffer pool once.\n");
    XCHECK( _poolParams != NULL, "SbDMBuffer::Constrain GL and Video resource prior to buffer pool creation.\n");
    XCHECK( _bufferCount > 0, "SbDMBuffer::Reserve buffer space before creatng the buffer pool.\n");

    status = dmBufferCreatePool( _poolParams, &_dmBufferPool );
    XCHECK_DM( status == DM_SUCCESS , "SbDMBuffer::Unable to allocated constrained pool." );

    dmParamsDestroy( _poolParams );

    return status == DM_SUCCESS;
}

SbBool
SbDMBuffer::manage( Display *display, GLXFBConfigSGIX glConfig, GLXContext glContext, GLXPbufferSGIX glPBuffer){
    _display    = display;
    _glConfig   = glConfig;
    _glContext  = glContext;
    _glPBuffer  = glPBuffer;
    return True;
}

SbBool
SbDMBuffer::manage( VLServer server, VLPath path, VLNode drain){
    int result;
    result = vlDMPoolRegister( server, path, drain, _dmBufferPool );
    XCHECK_VL( result == 0, "SbDMBuffer::Could not register pool with input background path." );

    return result == 0;
}

void
SbDMBuffer::swapBuffers( void ) {
    lock();
    if ( _lockedBuffer != NULL )
	dmBufferFree( _lockedBuffer );
    _lockedBuffer = _workBuffer;
    release();
}
  
SbBool
SbDMBuffer::setValueFromVLEvent( VLEvent *event){
    int result;

    result = vlEventToDMBuffer( event, &_workBuffer );
    XCHECK_VL( result == 0, "Unable to convert video event to buffer.\n");

    swapBuffers();

    return result == 0;
}

unsigned char *
SbDMBuffer::startEditing( SbVec2s &size, int &nc){
    XCHECK( _dmBufferPool != NULL, "SbDMBuffer::DMBufferPool access prior to allocation.\n");

    DMstatus status;
    status = dmBufferAllocate(_dmBufferPool, &_workBuffer);
    XCHECK_DM( status == DM_SUCCESS, "SbDMBuffer::Unable to allocated dmBuffer.\n");

    size = _size;
    nc = 4;
    unsigned char *pixels = (unsigned char *) dmBufferMapData( _workBuffer );

    swapBuffers();

    return pixels;
}

DMbuffer
SbDMBuffer::startEditing( void ){
    XCHECK( _dmBufferPool != NULL, "SbDMBuffer::DMBufferPool access prior to allocation.\n");

    DMstatus status;
    status = dmBufferAllocate(_dmBufferPool, &_workBuffer);
    XCHECK_DM( status == DM_SUCCESS, "SbDMBuffer::Unable to allocated dmBuffer.\n");

    int result;
    result = glXAssociateDMPbufferSGIX( _display, _glPBuffer, _imageFormat, _workBuffer);
    XCHECK( result, "Unable to associate glPbuffer with allocated video dmBuffer.\n");

    Bool success;
    success = glXMakeCurrent( _display, _glPBuffer, _glContext );	
    XCHECK( success == True, "SbDMBuffer::Unable to make newly allocated buffer the current context!\n");

    return _workBuffer;
}

void
SbDMBuffer::finishEditing( void ){
    swapBuffers();
}

void
SbDMBuffer::releaseValue( void ){
    release();
}

// Support/Convenience Routines -------------------------------------------------------------------

Bool openStandardVideo( SbDMBuffer *prepBuffer, const char* deviceName,
			SbVec2s origin, SbVec2s zoom, SbVec2s aspect,
			const VLServer &server, 
			VLPath &path, VLNode &drain, 
			VLControlValueType LinearOrGraphics )
{

    SbVec2s size = prepBuffer->getSize();

    //-- Begin Video Setup ------------------------------------

      int result;
      VLDevList devlist;
      VLNode    source;

      if ( deviceName != NULL ){
	  // Find out what video devices are supported
	  result = vlGetDeviceList(server, &devlist);
	  XCHECK_VL( result != -1, "Unable to determine available video devices.\n");

	  // Find requested input source
	  VLNodeInfo *videoInput = NULL;
	  for(int i=0;i<devlist.numDevices;i++)
	      for (int j=0;j<devlist.devices[i].numNodes;j++){
		  VLNodeInfo &node = devlist.devices[i].nodes[j];
		  if ( node.kind == VL_VIDEO && strstr(node.name,deviceName) != NULL )
		      videoInput = &node;
	      }

	  if ( videoInput == NULL )
	      perror("Unable to locate requested video source.\n");

	  source = vlGetNode(server, videoInput->type, videoInput->kind, videoInput->number);
      } else
	  source = vlGetNode(server, VL_SRC, VL_VIDEO, VL_ANY);
      XCHECK_VL( source != -1, "Unable to grab video source.\n");

      // Drain into memory
      drain = vlGetNode(server, VL_DRN, VL_MEM, VL_ANY);
      XCHECK_VL( drain != -1, "Unable to create memory drain for texturing.\n");

      // Create and Setup the Path
      path = vlCreatePath(server, VL_ANY, source, drain);
      XCHECK_VL( path != -1, "Unable to create path.\nVerify camera connection.\n");

      result = vlSetupPaths(server, &path, 1, VL_SHARE, VL_SHARE);
      XCHECK_VL( result == 0, "Unable to instantiate paths.\n");

      // Parameters

      setIntControl  ( server, path, source, VL_TIMING,       VL_TIMING_525_SQ_PIX );
      setIntControl  ( server, path, drain,  VL_PACKING,      VL_PACKING_ABGR_8 );
      setIntControl  ( server, path, drain,  VL_CAP_TYPE,     VL_CAPTURE_INTERLEAVED);

      if ( aspect[0] != -1 ) setFractControl( server, path, drain,  VL_ASPECT, aspect[0], aspect[1] );
      if ( origin[0] != -1 ) setFractControl( server, path, drain,  VL_ORIGIN, origin[0], origin[1] );
      if ( zoom[0] != -1 ) setFractControl( server, path, drain,  VL_ZOOM, zoom[0], zoom[1] );
      if ( size[WIDTH] != -1) setXYControl( server, path, drain,  VL_SIZE, size[ WIDTH ], size[ HEIGHT ] );

      setIntControl  ( server, path, drain,  VL_FORMAT,       VL_FORMAT_RGB );
      setIntControl  ( server, path, drain,  VL_LAYOUT,       LinearOrGraphics );
      setFractControl( server, path, drain,  VL_RATE,         30.0, 1);

      vlSelectEvents( server, path, VLTransferCompleteMask );
      //vlSelectEvents( server, path, VLAllEventsMask);

    //-- End Video Setup --------------------------------------

    return prepBuffer->constrain( server, path, drain );
}

//-- Support Routines -------------------------------------------------------------

#ifdef DEBUG
#define ARRAY_COUNT(a)    (sizeof(a)/sizeof(a[0]))

void
printParams( const DMparams* p, int indent )
{
    int len = dmParamsGetNumElems( p );
    int i;
    int j;
    
    for ( i = 0;  i < len;  i++ ) {
	const char* name = dmParamsGetElem    ( p, i );
	DMparamtype type = dmParamsGetElemType( p, i );
	
	for ( j = 0;  j < indent;  j++ ) {
	    fprintf( stderr,  " " );
	}

	fprintf( stderr,  "%8s: ", name );
	switch( type ) 
	    {
	    case DM_TYPE_ENUM:
		fprintf( stderr,  "%d", dmParamsGetEnum( p, name ) );
		break;
	    case DM_TYPE_INT:
		fprintf( stderr,  "%d", dmParamsGetInt( p, name ) );
		break;
	    case DM_TYPE_STRING:
		fprintf( stderr,  "%s", dmParamsGetString( p, name ) );
		break;
	    case DM_TYPE_FLOAT:
		fprintf( stderr,  "%f", dmParamsGetFloat( p, name ) );
		break;
	    case DM_TYPE_FRACTION:
		{
		    DMfraction f;
		    f = dmParamsGetFract( p, name );
		    fprintf( stderr,  "%d/%d", f.numerator, f.denominator );
		}
		break;
	    case DM_TYPE_PARAMS:
		printParams( dmParamsGetParams( p, name ), indent + 4 );
		break;
	    case DM_TYPE_ENUM_ARRAY:
		{
		    int i;
		    const DMenumarray* array = dmParamsGetEnumArray( p, name );
		    for ( i = 0;  i < array->elemCount;  i++ ) {
			fprintf( stderr,  "%d ", array->elems[i] );
		    }
		}
		break;
	    case DM_TYPE_INT_ARRAY:
		{
		    int i;
		    const DMintarray* array = dmParamsGetIntArray( p, name );
		    for ( i = 0;  i < array->elemCount;  i++ ) {
			fprintf( stderr,  "%d ", array->elems[i] );
		    }
		}
		break;
	    case DM_TYPE_STRING_ARRAY:
		{
		    int i;
		    const DMstringarray* array = 
			dmParamsGetStringArray( p, name );
		    for ( i = 0;  i < array->elemCount;  i++ ) {
			fprintf( stderr,  "%s ", array->elems[i] );
		    }
		}
		break;
	    case DM_TYPE_FLOAT_ARRAY:
		{
		    int i;
		    const DMfloatarray* array = 
			dmParamsGetFloatArray( p, name );
		    for ( i = 0;  i < array->elemCount;  i++ ) {
			fprintf( stderr,  "%f ", array->elems[i] );
		    }
		}
		break;
	    case DM_TYPE_FRACTION_ARRAY:
		{
		    int i;
		    const DMfractionarray* array = 
			dmParamsGetFractArray( p, name );
		    for ( i = 0;  i < array->elemCount;  i++ ) {
			fprintf( stderr,  "%d/%d ", 
			        array->elems[i].numerator,
			        array->elems[i].denominator );
		    }
		}
		break;
	    case DM_TYPE_INT_RANGE:
		{
		    const DMintrange* range = dmParamsGetIntRange( p, name );
		    fprintf( stderr,  "%d ... %d", range->low, range->high );
		}
		break;
	    case DM_TYPE_FLOAT_RANGE:
		{
		    const DMfloatrange* range = 
			dmParamsGetFloatRange( p, name );
		    fprintf( stderr,  "%f ... %f", range->low, range->high );
		}
		break;
	    case DM_TYPE_FRACTION_RANGE:
		{
		    const DMfractionrange* range = 
			dmParamsGetFractRange( p, name );
		    fprintf( stderr,  "%d/%d ... %d/%d", 
			    range->low.numerator, 
			    range->low.denominator, 
			    range->high.numerator,
			    range->high.denominator );
		}
		break;

	    default:
		fprintf( stderr,  "UNKNOWN TYPE" );
	    }
	fprintf( stderr,  "\n" );
    }
}

void printFBConfig(Display *display,GLXFBConfigSGIX config){

    static struct { int attrib; const char* name; } attribs [] = {
	{ GLX_BUFFER_SIZE, "GLX_BUFFER_SIZE" },
	{ GLX_LEVEL, "GLX_LEVEL" },
	{ GLX_DOUBLEBUFFER, "GLX_DOUBLEBUFFER" },
	{ GLX_AUX_BUFFERS, "GLX_AUX_BUFFERS" },
	{ GLX_RED_SIZE, "GLX_RED_SIZE" },
	{ GLX_GREEN_SIZE, "GLX_GREEN_SIZE" },
	{ GLX_BLUE_SIZE, "GLX_BLUE_SIZE" },
	{ GLX_ALPHA_SIZE, "GLX_ALPHA_SIZE" },
	{ GLX_DEPTH_SIZE, "GLX_DEPTH_SIZE" },
	{ GLX_DRAWABLE_TYPE_SGIX, "GLX_DRAWABLE_TYPE_SGIX" },
	{ GLX_RENDER_TYPE_SGIX, "GLX_RENDER_TYPE_SGIX" },
	{ GLX_X_RENDERABLE_SGIX, "GLX_X_RENDERABLE_SGIX" },
	{ GLX_MAX_PBUFFER_WIDTH_SGIX, "GLX_MAX_PBUFFER_WIDTH_SGIX" },
	{ GLX_MAX_PBUFFER_HEIGHT_SGIX, "GLX_MAX_PBUFFER_HEIGHT_SGIX"},
    };
    int i;
    
    fprintf(stderr, "Frame Buffer Configuration:\n");
    for ( i = 0;  i < ARRAY_COUNT( attribs );  i++ )
    {
	int value;
	int errorCode = glXGetFBConfigAttribSGIX( display, config, attribs[i].attrib, &value );
	CHECK( errorCode == 0, "glXGetFBConfigAttribSGIX failed" );
	fprintf(stderr, "    %24s = %d\n", attribs[i].name, value );
    }
    fprintf (stderr, "=======\n\n");
}

void printPBufferStats(Display *display, GLXPbufferSGIX pbuffer){

    static struct { int attrib; const char* name; } attribs [] = {
	{ GLX_WIDTH_SGIX, "GLX_WIDTH_SGIX" },
	{ GLX_HEIGHT_SGIX, "GLX_HEIGHT_SGIX" },
	{ GLX_PRESERVED_CONTENTS_SGIX, "GLX_PRESERVED_CONTENTS_SGIX" },
	{ GLX_FBCONFIG_ID_SGIX, "GLX_FBCONFIG_ID_SGIX" },
	{ GLX_DIGITAL_MEDIA_PBUFFER_SGIX, "GLX_DIGITAL_MEDIA_PBUFFER_SGIX" }
    };
    int i;
    
    fprintf(stderr, "PBuffer Configuration:\n");
    for ( i = 0;  i < ARRAY_COUNT( attribs );  i++ )
    {
	unsigned int value;
	int errorCode = glXQueryGLXPbufferSGIX( display, pbuffer, attribs[i].attrib, &value );
	CHECK_GL( errorCode == 0, "glXQueryGLXPbufferSGIX failed" );
	if ( errorCode == GLX_BAD_ATTRIBUTE )
	    fprintf(stderr, "\tGLX_BAD_ATTRIBUTE\n");
	fprintf(stderr, "    %34s = %d\n", attribs[i].name, value );
    }
    fprintf (stderr, "=======\n\n");
}
#endif

GLXFBConfigSGIX getFBConfig(Display *display, int params[]){

    /*
     * Find a frame buffer configuration that suits our needs.
     */

    GLXFBConfigSGIX config = NULL;

    int configCount;
    
    GLXFBConfigSGIX* configs = 
      glXChooseFBConfigSGIX(
			    display,
			    DefaultScreen(display),
			    params,
			    &configCount
			    );
    CHECK( configs != NULL || configCount != 0, "Could not get FBConfig" );

    #ifdef DEBUG
      fprintf (stderr, "configcount = %d\n",configCount);
      for (int i = 0; i < configCount; i++){
        fprintf (stderr, "config%d\n",i);
        printFBConfig( display, configs[i] );
        fprintf (stderr, "=======\n\n");
      }
    #endif

    // XXX HACK - we should really search the config list here and 
    //     make sure we find an fb config that's consistent 
    //     Came up with configs[1] just by printing out the list, but
    //     it's always the right one.. Fix it after press tour..
    config = configs[configCount - 1];

    XFree( configs );

    return config;
}

GLXFBConfigSGIX setupDrawable( Display *display, int params[], int w, int h, GLXPbufferSGIX* newDrawable){

    int attrib[] = {
	GLX_PRESERVED_CONTENTS_SGIX,    GL_TRUE,
        GLX_DIGITAL_MEDIA_PBUFFER_SGIX, GL_TRUE,
	(int) None};

    GLXFBConfigSGIX config = getFBConfig( display, params);
    
    *newDrawable = glXCreateGLXPbufferSGIX(display, config, w, h, attrib);
    CHECK( *newDrawable != None, "Failed to create pbuffer!\n");

    #ifdef DEBUG
      printPBufferStats(display, *newDrawable);
    #endif
    
    return config;
}

void setFractControl( VLServer server, VLPath path, VLNode node, VLControlType type, int numerator, int denominator )
{
    VLControlValue val;
    int vlstatus;
    
    val.fractVal.numerator   = numerator;
    val.fractVal.denominator = denominator;

    vlstatus = vlSetControl( server, path, node, type, &val );

    if ( vlstatus != 0 && vlGetErrno() == VLValueOutOfRange )
        vlstatus = vlGetControl( server, path, node, type, &val );

    CHECK_VL( vlstatus == 0, "Could not set control" );
}

void setIntControl(VLServer server, VLPath path, VLNode node, VLControlType type, int newValue)
{
    VLControlValue val;
    int vlstatus;
    
    val.intVal = newValue;
    vlstatus = vlSetControl( server, path, node, type, &val );
    XCHECK_VL( vlstatus == 0, "Could not set control" );
}

void setXYControl(VLServer server, VLPath path, VLNode node, VLControlType type, int x, int y )
{

    VLControlValue val;
    int vlstatus;
    
    val.xyVal.x = x;
    val.xyVal.y = y;
    vlstatus = vlSetControl( server, path, node, type, &val );
    CHECK_VL( vlstatus == 0, "Could not set control" );
}

void closeStandardVideo( VLServer server){
    XCHECK_VL( vlCloseVideo(server) != -1, "Unable to close video device\n");
}

Bool startVideo( VLServer server, VLPath path ){
    int result;

    result = vlBeginTransfer( server, path, 0, NULL );
    XCHECK_VL( result == 0, "Could not start video input transfer." );

    return result == 0;
}

Bool stopVideo( VLServer server, VLPath path ){
    int result;

    result = vlEndTransfer( server, path );
    XCHECK_VL( result == 0, "Could not stop video input transfer." );

    return result == 0;
}

// --- PBuffers ----------------------------------------------------------------------------

void closeStandardPBuffer(){
    // Simply for code balance
    // The DMBuffer class handles clean-up
}

Bool openStandardDMICOutputPBuffer(SbDMBuffer *sourceBuffer, SbVec2s size){

    DMstatus status;

    //#define DMIC_EASY_SEARCH
#define DEBUG
#ifdef DMIC_EASY_SEARCH
    DMparams *sourceParams;
    DMparams *destParams;
    DMparams *convParams;

    // Source Image
    status = dmParamsCreate( &sourceParams );
    XCHECK_DM( status == DM_SUCCESS, "Could not create dmparams." );
    status = dmSetImageDefaults( sourceParams, size[ WIDTH ], size[ HEIGHT ], DM_IMAGE_PACKING_CbYCrY );
    XCHECK_DM( status == DM_SUCCESS, "Could not set image defaults." );
    status = dmParamsSetEnum( sourceParams, DM_IMAGE_ORIENTATION, DM_IMAGE_TOP_TO_BOTTOM);
    XCHECK_DM( status == DM_SUCCESS, "Could not set image orientation." );
    status = dmParamsSetString( sourceParams, DM_IMAGE_COMPRESSION, DM_IMAGE_JPEG);
    XCHECK_DM( status == DM_SUCCESS, "Could not set image compression." );

    // Destination Image
    status = dmParamsCreate( &destParams );
    XCHECK_DM( status == DM_SUCCESS, "Could not create dmparams." );
    status = dmSetImageDefaults( destParams, size[ WIDTH ], size[ HEIGHT ], DM_IMAGE_PACKING_RGBA );
    XCHECK_DM( status == DM_SUCCESS, "Could not set image defaults." );
    status = dmParamsSetEnum( destParams, DM_IMAGE_ORIENTATION, DM_IMAGE_TOP_TO_BOTTOM);
    XCHECK_DM( status == DM_SUCCESS, "Could not set image orientation." );
    status = dmParamsSetString( destParams, DM_IMAGE_COMPRESSION, DM_IMAGE_UNCOMPRESSED);
    XCHECK_DM( status == DM_SUCCESS, "Could not set image compression." );

    // Conversion Parameters
    status = dmParamsCreate( &convParams );
    XCHECK_DM( status == DM_SUCCESS, "Could not create dmparams." );
    status = dmParamsSetInt( convParams, DM_IC_ID, *((unsigned *) DM_IMAGE_JPEG));
    XCHECK_DM( status == DM_SUCCESS, "Could not set image compression." );
    status = dmParamsSetEnum( convParams, DM_IC_SPEED, DM_IC_SPEED_REALTIME);
    XCHECK_DM( status == DM_SUCCESS, "Could not set conversion speed." );
    status = dmParamsSetEnum( convParams, DM_IC_CODE_DIRECTION, DM_IC_CODE_DIRECTION_DECODE);
    XCHECK_DM( status == DM_SUCCESS, "Could not set coding direction." );

    fprintf(stderr, "%d\n", dmICChooseConverter( sourceParams, destParams, convParams ));

    dmParamsDestroy(sourceParams);
    dmParamsDestroy(destParams);
    dmParamsDestroy(convParams);

    return True;
#else // DMIC_EASY_SEARCH
    int converterCount = dmICGetNum();
    int decoderDriver;

    DMparams *convParams;
    status = dmParamsCreate(&convParams);
    XCHECK_DM( status == DM_SUCCESS, "Could not create dmparams." );

    SbBool found = False;
    for (int i=0;i<converterCount;i++){	
	dmICGetDescription(i,convParams);

#ifdef DEBUG	
	fprintf(stderr, "\nConverter: ");
	putw(dmParamsGetInt( convParams, DM_IC_ID ), stderr);
	fprintf(stderr, "\n\tEngine: %s\n", dmParamsGetString( convParams, DM_IC_ENGINE ));
	fprintf(stderr, "\tSpeed: ");
	switch ( dmParamsGetEnum( convParams, DM_IC_SPEED )){
	  case DM_IC_SPEED_REALTIME:
	    fprintf(stderr, "DM_IC_SPEED_REALTIME\n");
	    break;
	  case DM_IC_SPEED_NONREALTIME:
	    fprintf(stderr, "DM_IC_SPEED_NONREALTIME\n");
	    break;
	  case DM_IC_SPEED_UNDEFINED:
	    fprintf(stderr, "DM_IC_SPEED_UNDEFINED\n");
	    break;
	}
	fprintf(stderr, "\tDirection: ");
	switch ( dmParamsGetEnum( convParams, DM_IC_CODE_DIRECTION )){
	  case DM_IC_CODE_DIRECTION_ENCODE:
	    fprintf(stderr, "DM_IC_CODE_DIRECTION_ENCODE\n");
	    break;
	  case DM_IC_CODE_DIRECTION_DECODE:
	    fprintf(stderr, "DM_IC_CODE_DIRECTION_DECODE\n");
	    break;
	  case DM_IC_CODE_DIRECTION_UNDEFINED:
	    fprintf(stderr, "DM_IC_DM_IC_CODE_DIRECTION_UNDEFINED\n");
	    break;
	}
	fprintf(stderr, "\tVersion: %d\n", dmParamsGetInt( convParams, DM_IC_VERSION ));
	fprintf(stderr, "\tRevision: %d\n", dmParamsGetInt( convParams, DM_IC_REVISION ));
#endif // DEBUG

	if ( dmParamsGetInt( convParams, DM_IC_ID ) != 'jpeg' )
	    continue;
	if ( dmParamsGetEnum( convParams, DM_IC_SPEED ) != DM_IC_SPEED_REALTIME )
	    continue;
	if ( dmParamsGetEnum( convParams, DM_IC_CODE_DIRECTION ) != DM_IC_CODE_DIRECTION_DECODE )
	    continue;
	decoderDriver = i;
	found = True;
	
    }
    dmParamsDestroy(convParams);

    if ( !found )
	return False;

    // Describe Conversion
    // Source Image
    status = dmParamsCreate( &convParams );
    XCHECK_DM( status == DM_SUCCESS, "Could not create dmparams." );
    status = dmSetImageDefaults( convParams, size[ WIDTH ], size[ HEIGHT ], DM_IMAGE_PACKING_CbYCrY );
    XCHECK_DM( status == DM_SUCCESS, "Could not set image defaults." );
    status = dmParamsSetEnum( convParams, DM_IMAGE_ORIENTATION, DM_IMAGE_TOP_TO_BOTTOM);
    XCHECK_DM( status == DM_SUCCESS, "Could not set image orientation." );
    status = dmParamsSetString( convParams, DM_IMAGE_COMPRESSION, DM_IMAGE_JPEG);
    XCHECK_DM( status == DM_SUCCESS, "Could not set image compression." );

    sourceBuffer->constrain( decoderDriver, convParams, size );
    dmParamsDestroy(convParams);

    GLXFBConfigSGIX glConfig;
    GLXContext glContext;
    GLXPbufferSGIX glPBuffer;

    Display *display = XOpenDisplay( ":0" );
    XCHECK( display != NULL, "Unable to open display.\n");

    //-- Begin GL Pbuffer Setup ---------------------
    {
      int params[] = { 
	  GLX_RENDER_TYPE_SGIX, 	GLX_RGBA_BIT_SGIX,
	  GLX_DRAWABLE_TYPE_SGIX,	GLX_PBUFFER_BIT_SGIX,
	  GLX_DEPTH_SIZE, 		1,
	  GLX_ALPHA_SIZE,		8,
	  GLX_RED_SIZE, 	        8,
	  GLX_GREEN_SIZE, 		8,
	  GLX_BLUE_SIZE, 		8,
	  GLX_X_RENDERABLE_SGIX,	FALSE,
	  (int)None,
      };

      SbVec2s size = sourceBuffer->getSize();

      glConfig = setupDrawable( display, params, size[ WIDTH ], size[ HEIGHT ], &glPBuffer );
      glContext = glXCreateContextWithConfigSGIX( display, glConfig, 
						  GLX_RGBA_TYPE_SGIX,     // RGBA (Not Indexed)
						  NULL,                   // Context with which to share
						  GL_TRUE);	          // Use GL hardware (Not X)
      XCHECK( glContext != NULL, "Could not allocate GL context.\n" );
    }
    //-- End GL pbuffer Setup --------------------------------

    return sourceBuffer->manage( display, glConfig, glContext, glPBuffer);

#endif // DMIC_EASY_SEARCH

}

Bool openStandardPBuffer( Display *display, SbDMBuffer *prepBuffer){
    
    DMstatus status;
    DMparams *imageFormat;
    SbVec2s size = prepBuffer->getSize();
    GLXFBConfigSGIX glConfig;
    GLXContext glContext;
    GLXPbufferSGIX glPBuffer;

    //-- Begin GL Pbuffer Setup ---------------------
    {
      status = dmParamsCreate(&imageFormat);
      XCHECK_DM( status == DM_SUCCESS, "Could not create dmparams." );
    
      status = dmSetImageDefaults( imageFormat, size[ WIDTH ], size[ HEIGHT ], DM_IMAGE_PACKING_RGBA );
      XCHECK_DM( status == DM_SUCCESS, "Could not set image defaults." );

      status = dmParamsSetEnum( imageFormat, DM_IMAGE_LAYOUT, DM_IMAGE_LAYOUT_GRAPHICS);
      XCHECK_DM( status == DM_SUCCESS, "Could not set image layout." );

      display = XOpenDisplay( ":0" );
      XCHECK( display != NULL, "Unable to open display.\n");

      int params[] = { 
	  GLX_RENDER_TYPE_SGIX, 	GLX_RGBA_BIT_SGIX,
	  GLX_DRAWABLE_TYPE_SGIX,	GLX_PBUFFER_BIT_SGIX,
	  GLX_DEPTH_SIZE, 		1,
	  GLX_ALPHA_SIZE,		8,
	  GLX_RED_SIZE, 	        8,
	  GLX_GREEN_SIZE, 		8,
	  GLX_BLUE_SIZE, 		8,
	  GLX_X_RENDERABLE_SGIX,	FALSE,
	  (int)None,
      };

      glConfig = setupDrawable( display, params, size[ WIDTH ], size[ HEIGHT ], &glPBuffer );
      glContext = glXCreateContextWithConfigSGIX( display, glConfig, 
						  GLX_RGBA_TYPE_SGIX,     // RGBA (Not Indexed)
						  NULL,                   // Context with which to share
						  GL_TRUE);	          // Use GL hardware (Not X)
      XCHECK( glContext != NULL, "Could not allocate GL context.\n" );
    }
    //-- End GL pbuffer Setup --------------------------------

    prepBuffer->constrain( imageFormat );
    return prepBuffer->manage( display, glConfig, glContext, glPBuffer);

}

// DMbuffers

unsigned char *map( DMbuffer &buffer ){
    return (unsigned char *) dmBufferMapData( buffer );
}

Bool makeReadable( SbDMBuffer &buffer ){
    int result;
    DMbuffer contents = buffer.getValue();

    if ( contents == NULL ){
	buffer.releaseValue();
	return False;
    }

    result = glXAssociateDMPbufferSGIX( buffer.getDisplay(), buffer.getPBuffer(), buffer.getFormat(),  contents );
    XCHECK( result, "Unable to associate glPbuffer with allocated dmBuffer.\n");
    
    buffer.releaseValue();

    Bool success;
    success = glXMakeCurrentReadSGI( buffer.getDisplay(), glXGetCurrentDrawable(), buffer.getPBuffer(), glXGetCurrentContext());
    XCHECK( success == True, "Unable to make glPbuffer the current readable!\n");

    return success;
}
