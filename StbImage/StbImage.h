#pragma once
#define DLLEXPORT __declspec(dllexport)
DLLEXPORT int GetImageInfo(char const* filename, int* x, int* y, int* comp);
DLLEXPORT int ReadImageAsBC1(char const* filename, int flipVertically, unsigned char* dest, int destSize);
DLLEXPORT int ReadImageAsBC3(char const* filename, int flipVertically, unsigned char* dest, int destSize);
DLLEXPORT int ReadImageAsRGBA(char const* filename, unsigned char* dest, int destSize);
