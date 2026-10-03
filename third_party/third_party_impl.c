/* Single-header libraries, compiled once (public domain / MIT, see headers). */
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#include "stb_image.h"

#define STB_VORBIS_NO_PUSHDATA_API
#include "stb_vorbis.c"
