#pragma once

#define STBIMAGE_FORMAT_BC1 1
#define STBIMAGE_FORMAT_BC3 3
#define STBIMAGE_FORMAT_BC5 5

#define DLLEXPORT __declspec(dllexport)
DLLEXPORT int GetImageInfo(char const* filename, int* width, int* height, int* numComponents);
DLLEXPORT int ReadImageAsBCx(char const* filename, int flipVertically, int format, unsigned char* dest, int destSize);
DLLEXPORT int ReadImageAsRGBA(char const* filename, int flipVertically, unsigned char* dest, int destSize);
