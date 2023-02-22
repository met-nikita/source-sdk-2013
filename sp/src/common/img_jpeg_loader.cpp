//=============================================================================//
//
// Purpose: Common JPEG image loader.
//
//=============================================================================//

#include "img_jpeg_loader.h"
#if defined(LINUX)
#include <setjmp.h>
#endif
#include "jpeglib/jpeglib.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef JPEG_LIB_VERSION
struct JpegErrorHandler_t
{
	struct jpeg_error_mgr mgr;
	jmp_buf jmp;
};

void JpegErrorExit( j_common_ptr cinfo )
{
	char msg[ JMSG_LENGTH_MAX ];
	(cinfo->err->format_message)( cinfo, msg );
	Warning( "%s\n", msg );

	longjmp( ((JpegErrorHandler_t*)(cinfo->err))->jmp, 1 );
}

// Read JPEG from CUtlBuffer
struct JpegSourceMgr_t : jpeg_source_mgr
{
	// See libjpeg.txt, jdatasrc.c

	static void _init_source(j_decompress_ptr cinfo) {}
	static void _term_source(j_decompress_ptr cinfo) {}
	static boolean _fill_input_buffer(j_decompress_ptr cinfo) { return 0; }
	static void _skip_input_data( j_decompress_ptr cinfo, long num_bytes )
	{
		JpegSourceMgr_t *src = (JpegSourceMgr_t*)cinfo->src;
		src->next_input_byte += (size_t)num_bytes;
		src->bytes_in_buffer -= (size_t)num_bytes;
	}

	void SetSourceMgr( j_decompress_ptr cinfo, CUtlBuffer *pBuffer )
	{
		cinfo->src = this;
		init_source = _init_source;
		fill_input_buffer = _fill_input_buffer;
		skip_input_data = _skip_input_data;
		resync_to_restart = jpeg_resync_to_restart;
		term_source = _term_source;

		bytes_in_buffer = pBuffer->TellMaxPut();
		next_input_byte = (const JOCTET*)pBuffer->Base();
	}
};

//
// Read a JPEG image from file into buffer as RGBA.
//
bool JPEGtoRGBA( IFileSystem *filesystem, const char *filename, CUtlMemory< byte > &buffer, int &width, int &height )
{
	// Read the whole image to memory
	CUtlBuffer fileBuffer;

	if ( !filesystem->ReadFile( filename, NULL, fileBuffer ) )
	{
		Warning( "Failed to read JPEG file (%s)\n", filename );
		return false;
	}

	return JPEGtoRGBA( fileBuffer, buffer, width, height );
}

//
// TODO: Error codes
//
bool JPEGtoRGBA( CUtlBuffer &fileBuffer, CUtlMemory< byte > &buffer, int &width, int &height )
{
	Assert( sizeof(JSAMPLE) == sizeof(byte) );
	Assert( !buffer.IsReadOnly() );

	struct jpeg_decompress_struct cinfo;
	struct JpegErrorHandler_t jerr;
	JpegSourceMgr_t src_mgr;

	cinfo.err = jpeg_std_error( &jerr.mgr );
	jerr.mgr.error_exit = JpegErrorExit;

	if ( setjmp( jerr.jmp ) )
	{
		jpeg_destroy_decompress( &cinfo );
		return false;
	}

	jpeg_create_decompress( &cinfo );
	src_mgr.SetSourceMgr( &cinfo, &fileBuffer );

	if ( jpeg_read_header( &cinfo, TRUE ) != JPEG_HEADER_OK )
	{
		Warning( "Bad JPEG signature\n" );
		jpeg_destroy_decompress( &cinfo );
		return false;
	}

	if ( !jpeg_start_decompress( &cinfo ) )
	{
		jpeg_destroy_decompress( &cinfo );
		return false;
	}

	int image_width = width = cinfo.output_width;
	int image_height = height = cinfo.output_height;
	int image_components = cinfo.output_components + 1; // RGB + A

	if ( image_components != 4 )
	{
		Warning( "JPEG is not RGB\n" );
		jpeg_destroy_decompress( &cinfo );
		return false;
	}

	buffer.Init( 0, image_components * image_width * image_height );

	while ( cinfo.output_scanline < cinfo.output_height )
	{
		JSAMPROW pRow = buffer.Base() + image_components * image_width * cinfo.output_scanline;
		jpeg_read_scanlines( &cinfo, &pRow, 1 );

		// Expand RGB to RGBA
		for ( int i = image_width; i--; )
		{
			pRow[i*4+0] = pRow[i*3+0];
			pRow[i*4+1] = pRow[i*3+1];
			pRow[i*4+2] = pRow[i*3+2];
			pRow[i*4+3] = 0xFF;
		}
	}

	jpeg_finish_decompress( &cinfo );
	jpeg_destroy_decompress( &cinfo );

	Assert( jerr.mgr.num_warnings == 0 );
	return true;
}

bool JPEGSupported()
{
	return true;
}
#else
bool JPEGtoRGBA( IFileSystem *filesystem, const char *filename, CUtlMemory< byte > &buffer, int &width, int &height )
{
	Warning( "JPEG library unsupported\n" );
	return false;
}

bool JPEGtoRGBA( CUtlBuffer &fileBuffer, CUtlMemory< byte > &buffer, int &width, int &height )
{
	Warning( "JPEG library unsupported\n" );
	return false;
}

bool JPEGSupported()
{
	return false;
}
#endif
