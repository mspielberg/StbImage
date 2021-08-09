#include <memory.h>
#include "stb_dxt.h"
#include "stb_image.h"
#include "stb_image_resize.h"
#include "StbImage.h"

int GetImageInfo(char const* filename, int* x, int* y, int* comp)
{
  return stbi_info(filename, x, y, comp);
}

void GetRGBABlock(stbi_uc* img, int img_width, int img_height, unsigned char* dest, int blockX, int blockY)
{
  size_t pixelX = (size_t)blockX * 4;
  size_t pixelY = (size_t)blockY * 4;
  size_t stride = (size_t)img_width * 4;
  size_t row_size_bytes = pixelX + 4 < img_width ? 16 : (img_width - pixelX) * 4;
  size_t rows_to_copy = pixelY + 4 < img_height ? 4 : img_height - pixelY;
  memset(dest, 0, 64);
  for (size_t i = 0; i < rows_to_copy; i++)
    memcpy_s(dest + i * 16, 16, img + stride * (pixelY + i) + pixelX * 4, row_size_bytes);
}

void CompressToBCx(stbi_uc* img, int imgWidth, int imgHeight, int useAlpha, unsigned char* dest)
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
      stb_compress_dxt_block(dest + offset, rgbaBlock, useAlpha, STB_DXT_HIGHQUAL);
    }
  }
}

int CompressMipmapFullScale(stbi_uc* img, int imgWidth, int imgHeight, int useAlpha, unsigned char* scaleBuf, unsigned char* dest, int destSize, int mipmapLevel)
{
  int mipmapWidth = imgWidth >> mipmapLevel;
  int mipmapHeight = imgHeight >> mipmapLevel;
  if (mipmapWidth == 0 || mipmapHeight == 0)
    return 0;

  int mipmapBlockWidth = (mipmapWidth + 3) / 4;
  int mipmapBlockHeight = (mipmapHeight + 3) / 4;
  int mipmapCompressedSize = mipmapBlockHeight * mipmapBlockWidth * (useAlpha ? 16 : 8);
  if (mipmapCompressedSize > destSize)
    return 0;

  if (mipmapLevel == 0)
    scaleBuf = img;
  else
  {
    stbir_resize_uint8(
      img, imgWidth, imgHeight, 0,
      scaleBuf, mipmapWidth, mipmapHeight, 0, 4);
  }

  CompressToBCx(scaleBuf, mipmapWidth, mipmapHeight, useAlpha, dest);

  return mipmapCompressedSize;
}

int CompressMipmapRepeated(stbi_uc* img, int imgWidth, int imgHeight, int useAlpha, unsigned char* scaleBuf, unsigned char* dest, int destSize, int mipmapLevel)
{
  int sourceMipmapLevel = mipmapLevel - 1;
  int sourceWidth = imgWidth >> sourceMipmapLevel;
  int sourceHeight = imgHeight >> sourceMipmapLevel;
  int mipmapWidth = imgWidth >> mipmapLevel;
  int mipmapHeight = imgHeight >> mipmapLevel;
  if (mipmapWidth == 0 || mipmapHeight == 0)
    return 0;

  int mipmapBlockWidth = (mipmapWidth + 3) / 4;
  int mipmapBlockHeight = (mipmapHeight + 3) / 4;
  int mipmapCompressedSize = mipmapBlockHeight * mipmapBlockWidth * (useAlpha ? 16 : 8);
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
    stbir_resize_uint8(
      scaleSource, sourceWidth, sourceHeight, 0,
      scaleDest, mipmapWidth, mipmapHeight, 0, 4);
  }

  CompressToBCx(scaleDest, mipmapWidth, mipmapHeight, useAlpha, dest);

  return mipmapCompressedSize;
}

int ReadImageAsBC1(char const* filename, int flipVertically, unsigned char* dest, int destSize)
{
  stbi_set_flip_vertically_on_load(flipVertically);
  int imgWidth, imgHeight, channels_in_file;
  stbi_uc* img = stbi_load(filename, &imgWidth, &imgHeight, &channels_in_file, 4);
  if (!img)
    return 0;

  stbi_uc* scaleBuf =
    (stbi_uc*)malloc((size_t)imgWidth * (size_t)imgHeight / 2 /* 50% width */ / 2 /* 50% height */ * 4 /* channels */);

  int bytesWritten = 0;
  int mipmapLevel = 0;
  do
  {
    bytesWritten = CompressMipmapRepeated(img, imgWidth, imgHeight, 0, scaleBuf, dest, destSize, mipmapLevel);
    dest += bytesWritten;
    destSize -= bytesWritten;
    mipmapLevel++;
  }
  while (bytesWritten > 0);

  free(scaleBuf);
  stbi_image_free(img);

  return 1;
}

int ReadImageAsBC3(char const* filename, int flipVertically, unsigned char* dest, int destSize)
{
  stbi_set_flip_vertically_on_load(flipVertically);
  int imgWidth, imgHeight, channels_in_file;
  stbi_uc* img = stbi_load(filename, &imgWidth, &imgHeight, &channels_in_file, 4);
  if (!img)
    return 0;

  stbi_uc* scaleBuf =
    (stbi_uc*)malloc((size_t)imgWidth * (size_t)imgHeight / 2 /* 50% width */ / 2 /* 50% height */ * 4 /* channels */);

  int bytesWritten = 0;
  int mipmapLevel = 0;
  do
  {
    bytesWritten = CompressMipmapRepeated(img, imgWidth, imgHeight, 1, scaleBuf, dest, destSize, mipmapLevel);
    dest += bytesWritten;
    destSize -= bytesWritten;
    mipmapLevel++;
  }
  while (bytesWritten > 0);

  free(scaleBuf);
  stbi_image_free(img);

  return 1;
}

int ReadImageAsRGBA(char const* filename, unsigned char* dest, int destSize)
{
  int imgWidth, imgHeight, channels_in_file;
  stbi_uc* img = stbi_load(filename, &imgWidth, &imgHeight, &channels_in_file, 4);
  if (!img)
    return 0;

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

  return 1;
}