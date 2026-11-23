// JPEG and PNG images, decoded and encoded with the stb libraries
// (docs/decisions/0012-dependencies.md).

#ifndef TR_STB_H
#define TR_STB_H

// Decoded images are Z_Malloc'd and freed by the caller with Z_Free. When an image
// cannot be read, a warning is printed and *pic is NULL.

// These read the named file through the engine's file system; a file that does not
// exist is not an error. 32 bits per pixel, RGBA; alpha is 255 where the file has none.
void LoadJPG(const char *filename, unsigned char **pic, int *width, int *height);
void LoadPNG32(const char *filename, unsigned char **pic, int *width, int *height);

// 8 bits per pixel, as terrain height maps are stored.
void LoadPNG8(const char *filename, unsigned char **pic, int *width, int *height);

// Decodes JPEG data held in memory to 32-bit RGBA. The name is used in messages only.
void Decompress_JPG(const char *name, const unsigned char *data, int length, unsigned char **pic,
                    int *width, int *height);

// Encodes 32-bit RGBA pixels as JPEG. Returns Z_Malloc'd data, or NULL on failure, and
// its size in *outputSize. With invert, the rows are taken bottom first, as OpenGL
// returns them.
unsigned char *Compress_JPG(int *outputSize, int quality, int image_width, int image_height,
                            unsigned char *image_buffer, int invert);

// Writes pixels as returned by OpenGL (32-bit RGBA, bottom row first) to a JPEG file.
void SaveJPG(const char *filename, int quality, int image_width, int image_height,
             unsigned char *image_buffer);

#endif // TR_STB_H
