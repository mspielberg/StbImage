#pragma once
#define DLLEXPORT __declspec(dllexport)
DLLEXPORT int GetImageInfo(char const* filename, int* x, int* y, int* comp);
DLLEXPORT int ReadImageAsBCx(char const* filename, int flipVertically, int useAlpha, unsigned char* dest, int destSize);
DLLEXPORT int ReadImageAsRGBA(char const* filename, int flipVertically, unsigned char* dest, int destSize);
