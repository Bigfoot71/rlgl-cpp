#ifndef RLGL_UTILS_HPP
#define RLGL_UTILS_HPP

#include "./rlEnums.hpp"

namespace rlgl {

    GlVersion GetVersion();                                                                                          // Get current OpenGL version
    int GetPixelDataSize(int width, int height, PixelFormat format);                                                 // Get pixel data size in bytes (image or texture)
    void GetGlTextureFormats(PixelFormat format, uint32_t *glInternalFormat, uint32_t *glFormat, uint32_t *glType);  // Get OpenGL internal formats

#   if (defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)) && defined(RLGL_SHOW_GL_DETAILS_INFO)
    const char *GetCompressedFormatName(int format); // Get compressed format official GL identifier name
#   endif  // RLGL_SHOW_GL_DETAILS_INFO

}

#endif //RLGL_UTILS_HPP
