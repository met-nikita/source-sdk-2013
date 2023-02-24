//=============================================================================//
//
// Purpose: Common JPEG image loader.
//
//=============================================================================//

#ifndef IMG_JPEG_LOADER_H
#define IMG_JPEG_LOADER_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlbuffer.h"
#include "filesystem.h"

bool JPEGtoRGBA( IFileSystem *filesystem, const char *filename, CUtlMemory< byte > &buffer, int &width, int &height );
bool JPEGtoRGBA( CUtlBuffer &fileBuffer, CUtlMemory< byte > &buffer, int &width, int &height );

bool JPEGSupported();

#endif // IMG_JPEG_LOADER_H
