#pragma once
#include <windows.h>
#include <zlib.h>



// gzCompress: do the compressing
int gzCompress(const char *src, int srcLen, char *dest, int destLen);

// gzDecompress: do the decompressing
int gzDecompress(const char *src, int srcLen, const char *dst, int dstLen);