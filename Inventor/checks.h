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



#ifndef _CHECKS_H
#define _CHECKS_H

#include <assert.h>

short __debugEnv( void );

#define CHECK(condition,message)					\
    {									\
	if ( ! ( condition ) )						\
	{								\
	    fprintf( stderr, "Error at line %d of %s:\n",		\
		     __LINE__, __FILE__ );				\
	    fprintf( stderr, "    %s\n", message );			\
	}								\
    }

#define XCHECK(condition,message)					\
    {									\
	if ( ! ( condition ) )						\
	{								\
	    fprintf( stderr, "Error at line %d of %s:\n",		\
		     __LINE__, __FILE__ );				\
	    fprintf( stderr, "    %s\n", message );			\
	    assert(0);							\
            exit(1);							\
	}								\
    }

#define CHECK_DM(condition,message)					\
    {									\
	if ( ! ( condition ) )						\
	{								\
	    const char* msg;						\
	    int errornum;						\
	    char detail[DM_MAX_ERROR_DETAIL];				\
	    fprintf( stderr, "Error at line %d of %s:\n",		\
		     __LINE__, __FILE__ );				\
	    fprintf( stderr, "    %s\n", message ); 		        \
	    msg = dmGetError( &errornum, detail );			\
	       fprintf( stderr, "    DM error message: %s\t(%s)\n",	\
			detail, msg );					\
	}								\
    }

#define XCHECK_DM(condition,message)					\
    {									\
	if ( ! ( condition ) )						\
	{								\
	    const char* msg;						\
	    int errornum;						\
	    char detail[DM_MAX_ERROR_DETAIL];				\
	    fprintf( stderr, "Error at line %d of %s:\n",		\
		     __LINE__, __FILE__ );				\
	    fprintf( stderr, "    %s\n", message ); 		        \
	    msg = dmGetError( &errornum, detail );			\
	    fprintf( stderr, "    DM error message: %s\t(%s)\n",	\
		     detail, msg );					\
	    assert(0);							\
            exit( errornum );						\
	}								\
    }

#define CHECK_GL(condition,message)					\
    {									\
	if ( ! ( condition ) )						\
	{								\
	    const GLubyte* msg;						\
	    fprintf( stderr, "Error at line %d of %s:\n",		\
		     __LINE__, __FILE__ );				\
	    fprintf( stderr, "    %s\n", message );			\
	    msg = gluErrorString( glGetError() );			\
	    fprintf( stderr, "    GL error message: %s\n", msg );	\
	}								\
    }

#define CHECK_MV(condition,message)					\
    {									\
	if ( ! ( condition ) )						\
	{								\
	    const char* msg;						\
	    fprintf( stderr, "Error at line %d of %s:\n",		\
		     __LINE__, __FILE__ );				\
	    fprintf( stderr, "    %s\n", message );			\
	    msg = mvGetErrorStr( mvGetErrno() );			\
	    fprintf( stderr, "    MV error message: %s\n", msg );	\
	}								\
    }

#define CHECK_VL(condition,message)					\
    {									\
	if ( ! ( condition ) )						\
	{								\
	    fprintf( stderr, "Error at line %d of %s:\n",		\
		     __LINE__, __FILE__ );				\
	    vlPerror( message );					\
	}								\
    }


#define XCHECK_VL(condition,message)					\
    {									\
	if ( ! ( condition ) )						\
	{								\
	    fprintf( stderr, "Error at line %d of %s:\n",		\
		     __LINE__, __FILE__ );				\
	    vlPerror( message );					\
	    assert(0);							\
            exit( vlErrno );						\
	}								\
    }

#define dprintf								\
       if (__debugEnv())						\
           fprintf

#endif
