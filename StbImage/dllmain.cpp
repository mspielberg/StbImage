#include <memory.h>
#include "stb_dxt.h"
#include "stb_image.h"
#include "stb_image_resize.h"

extern "C" __declspec(dllexport) int GetImageInfo(char const* filename, int* x, int* y, int* comp);
int GetImageInfo(char const* filename, int* x, int* y, int* comp)
{
  return stbi_info(filename, x, y, comp);
}

void DumpBlock(unsigned char* block, int size)
{
  for (int i = 0; i < size; i++)
  {
    fprintf(stderr, "%d,", block[i]);
    //if (i % 8 == 0)
    //  fprintf(stderr, "\n");
  }
  fprintf(stderr, "\n");
}

void GetRGBABlock(stbi_uc* img, int img_width, int img_height, unsigned char* dest, int blockX, int blockY)
{
  int pixelX = blockX * 4;
  int pixelY = blockY * 4;
  int stride = img_width * 4;
  int row_size_bytes = pixelX + 4 < img_width ? 16 : (img_width - pixelX) * 4;
  int rows_to_copy = pixelY + 4 < img_height ? 4 : img_height - pixelY;
  memset(dest, 0, 64);
  for (int i = 0; i < rows_to_copy; i++)
    memcpy_s(dest + i * 16, 16, img + stride * (pixelY + i) + pixelX * 4, row_size_bytes);
  //fprintf(stderr, "Block at %d,%d:\n", blockX, blockY);
  //DumpBlock(dest, 64);
}

void CompressToBC1(stbi_uc* img, int imgWidth, int imgHeight, unsigned char* dest)
{
  int blockWidth = (imgWidth + 3) / 4;
  int blockHeight = (imgHeight + 3) / 4;
  unsigned char rgbaBlock[64];

  for (int blockY = 0; blockY < blockHeight; blockY++)
  {
    for (int blockX = 0; blockX < blockWidth; blockX++)
    {
      GetRGBABlock(img, imgWidth, imgHeight, rgbaBlock, blockX, blockY);
      int offset = ((blockWidth * blockY) + blockX) * 8;
      stb_compress_dxt_block(dest + offset, rgbaBlock, 0, STB_DXT_HIGHQUAL);
    }
  }
}

void CompressToBC3(stbi_uc* img, int imgWidth, int imgHeight, unsigned char* dest)
{
  int blockWidth = (imgWidth + 3) / 4;
  int blockHeight = (imgHeight + 3) / 4;
  unsigned char rgbaBlock[64];

  for (int blockY = 0; blockY < blockHeight; blockY++)
  {
    for (int blockX = 0; blockX < blockWidth; blockX++)
    {
      GetRGBABlock(img, imgWidth, imgHeight, rgbaBlock, blockX, blockY);
      int offset = ((blockWidth * blockY) + blockX) * 16;
      stb_compress_dxt_block(dest + offset, rgbaBlock, 1, STB_DXT_HIGHQUAL);
    }
  }
}

int CompressMipmapFullScale(stbi_uc* img, int imgWidth, int imgHeight, int useAlpha, unsigned char* scaleBuf, unsigned char* dest, int destSize, int mipmapLevel)
{
  int mipmapWidth = imgWidth >> mipmapLevel;
  int mipmapHeight = imgHeight >> mipmapLevel;
  //fprintf(stderr, "Mipmap dimensions %dx%d\n", mipmapWidth, mipmapHeight);
  if (mipmapWidth == 0 || mipmapHeight == 0)
    return 0;

  int mipmapBlockWidth = (mipmapWidth + 3) / 4;
  int mipmapBlockHeight = (mipmapHeight + 3) / 4;
  int mipmapCompressedSize = mipmapBlockHeight * mipmapBlockWidth * (useAlpha ? 16 : 8);
  //fprintf(stderr, "Mipmap compressed size=%d\n", mipmapCompressedSize);
  if (mipmapCompressedSize > destSize)
    return 0;

  if (mipmapLevel == 0)
    scaleBuf = img;
  else
  {
    //fprintf(stderr, "Scaling to %dx%d\n", mipmapWidth, mipmapHeight);
    stbir_resize_uint8(
      img, imgWidth, imgHeight, 0,
      scaleBuf, mipmapWidth, mipmapHeight, 0, 4);
  }

  //fprintf(stderr, "Compressing to BC1\n");
  if (useAlpha)
    CompressToBC3(scaleBuf, mipmapWidth, mipmapHeight, dest);
  else
    CompressToBC1(scaleBuf, mipmapWidth, mipmapHeight, dest);

  return mipmapCompressedSize;
}

int CompressMipmapRepeated(stbi_uc* img, int imgWidth, int imgHeight, int useAlpha, unsigned char* scaleBuf, unsigned char* dest, int destSize, int mipmapLevel)
{
  int sourceMipmapLevel = mipmapLevel - 1;
  int sourceWidth = imgWidth >> sourceMipmapLevel;
  int sourceHeight = imgHeight >> sourceMipmapLevel;
  int mipmapWidth = imgWidth >> mipmapLevel;
  int mipmapHeight = imgHeight >> mipmapLevel;
  //fprintf(stderr, "Mipmap dimensions %dx%d\n", mipmapWidth, mipmapHeight);
  if (mipmapWidth == 0 || mipmapHeight == 0)
    return 0;

  int mipmapBlockWidth = (mipmapWidth + 3) / 4;
  int mipmapBlockHeight = (mipmapHeight + 3) / 4;
  int mipmapCompressedSize = mipmapBlockHeight * mipmapBlockWidth * (useAlpha ? 16 : 8);
  //fprintf(stderr, "Mipmap compressed size=%d\n", mipmapCompressedSize);
  if (mipmapCompressedSize > destSize)
    return 0;

  stbi_uc* scaleSource = mipmapLevel % 2 == 1 ? img : scaleBuf;
  stbi_uc* scaleDest = mipmapLevel % 2 == 1 ? scaleBuf : img;
  if (mipmapLevel == 0)
  {
    scaleDest = img;
  }
  else
  {
    //fprintf(stderr, "Scaling from %dx%d to %dx%d\n", sourceWidth, sourceHeight, mipmapWidth, mipmapHeight);
    stbir_resize_uint8(
      scaleSource, sourceWidth, sourceHeight, 0,
      scaleDest, mipmapWidth, mipmapHeight, 0, 4);
  }

  //fprintf(stderr, "Compressing to BC1\n");
  if (useAlpha)
    CompressToBC3(scaleDest, mipmapWidth, mipmapHeight, dest);
  else
    CompressToBC1(scaleDest, mipmapWidth, mipmapHeight, dest);

  return mipmapCompressedSize;
}

extern "C" __declspec(dllexport) int ReadImageAsBC1(char const* filename, int flipVertically, unsigned char* dest, int destSize);
int ReadImageAsBC1(char const* filename, int flipVertically, unsigned char* dest, int destSize)
{
  stbi_set_flip_vertically_on_load(flipVertically);
  int imgWidth, imgHeight, channels_in_file;
  stbi_uc* img = stbi_load(filename, &imgWidth, &imgHeight, &channels_in_file, 4);
  if (!img)
    return false;

  //fprintf(stderr, "destSize=%d\n", destSize);

  stbi_uc* scaleBuf =
    (stbi_uc*)malloc(imgWidth * imgHeight / 2 /* 50% width */ / 2 /* 50% height */ * 4 /* channels */);

  int bytesWritten = 0;
  int mipmapLevel = 0;
  do
  {
    //fprintf(stderr, "Compressing mipmap level %d\n", mipmapLevel);
    bytesWritten = CompressMipmapRepeated(img, imgWidth, imgHeight, 0, scaleBuf, dest, destSize, mipmapLevel);
    //fprintf(stderr, "mipmap size: %d\n", bytesWritten);
    dest += bytesWritten;
    destSize -= bytesWritten;
    mipmapLevel++;
  }
  while (bytesWritten > 0);

  free(scaleBuf);
  stbi_image_free(img);

  return true;
}

extern "C" __declspec(dllexport) int ReadImageAsBC3(char const* filename, int flipVertically, unsigned char* dest, int destSize);
int ReadImageAsBC3(char const* filename, int flipVertically, unsigned char* dest, int destSize)
{
  stbi_set_flip_vertically_on_load(flipVertically);
  int imgWidth, imgHeight, channels_in_file;
  stbi_uc* img = stbi_load(filename, &imgWidth, &imgHeight, &channels_in_file, 4);
  if (!img)
    return false;

  //fprintf(stderr, "destSize=%d\n", destSize);

  stbi_uc* scaleBuf =
    (stbi_uc*)malloc(imgWidth * imgHeight / 2 /* 50% width */ / 2 /* 50% height */ * 4 /* channels */);

  int bytesWritten = 0;
  int mipmapLevel = 0;
  do
  {
    //fprintf(stderr, "Compressing mipmap level %d\n", mipmapLevel);
    bytesWritten = CompressMipmapRepeated(img, imgWidth, imgHeight, 1, scaleBuf, dest, destSize, mipmapLevel);
    //fprintf(stderr, "mipmap size: %d\n", bytesWritten);
    dest += bytesWritten;
    destSize -= bytesWritten;
    mipmapLevel++;
  }
  while (bytesWritten > 0);

  free(scaleBuf);
  stbi_image_free(img);

  return true;
}

extern "C" __declspec(dllexport) int ReadImageAsRGBA(char const* filename, unsigned char* dest, int destSize);
int ReadImageAsRGBA(char const* filename, unsigned char* dest, int destSize)
{
  int imgWidth, imgHeight, channels_in_file;
  stbi_uc* img = stbi_load(filename, &imgWidth, &imgHeight, &channels_in_file, 4);
  if (!img)
    return false;

  int imgSize = imgWidth * imgHeight * 4;
  memcpy_s(dest, destSize, img, imgSize);
  stbi_image_free(img);

  stbi_uc* source = dest;
  int sourceWidth = imgWidth;
  int sourceHeight = imgHeight;

  dest += imgSize;
  destSize -= imgSize;

  int mipmapWidth = (imgWidth > 1) ? imgWidth >> 1 : 1;
  int mipmapHeight = (imgHeight > 1) ? imgHeight >> 1 : 1;
  int mipmapSize = mipmapWidth * mipmapHeight * 4;

  while (destSize >= mipmapSize)
  {
    stbir_resize_uint8(
      source, sourceWidth, sourceHeight, 0,
      dest, mipmapWidth, mipmapHeight, 0, 4);

    // use dest as next source
    source = dest;
    sourceWidth = mipmapWidth;
    sourceHeight = mipmapHeight;

    // compute destination for next level
    dest += mipmapSize;
    destSize -= mipmapSize;

    // compute size for next level
    mipmapWidth = (mipmapWidth > 1) ? mipmapWidth >> 1 : 1;
    mipmapHeight = (mipmapHeight > 1) ? mipmapHeight >> 1 : 1;
    mipmapSize = mipmapWidth * mipmapHeight * 4;
  }

  return true;
}