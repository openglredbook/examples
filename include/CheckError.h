//////////////////////////////////////////////////////////////////////////////
//
//  --- CheckError.h ---
//
//////////////////////////////////////////////////////////////////////////////

#ifndef __CHECKERROR_H__
#define __CHECKERROR_H__

#include <stdio.h>
#include <GL/gl.h>

//----------------------------------------------------------------------------

static const char*
ErrorString( GLenum error )
{
    const char*  msg;
    switch( error ) {
#define Case( Token )  case Token: msg = #Token; break;
        Case( GL_NO_ERROR );
        Case( GL_INVALID_VALUE );
        Case( GL_INVALID_ENUM );
        Case( GL_INVALID_OPERATION );
        Case( GL_OUT_OF_MEMORY );
#undef Case        
    }

    return msg;
}

//----------------------------------------------------------------------------

static void
_CheckError( const char* file, int line )
{
    GLenum  error = glGetError();

    do {
        fprintf( stderr, "[%s:%d] %s\n", file, line, ErrorString(error) );
    } while ((error = glGetError()) != GL_NO_ERROR );

}

//----------------------------------------------------------------------------

#define CheckError()  _CheckError( __FILE__, __LINE__ )

//----------------------------------------------------------------------------

#endif // !__CHECKERROR_H__
