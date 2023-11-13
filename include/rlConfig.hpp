#ifndef RLGL_CONFIG_HPP
#define RLGL_CONFIG_HPP

#define RLGL_VERSION  "4.5"

// Support TRACELOG macros
#ifndef TRACELOG
    #define TRACELOG(level, ...) (void)0
    #define TRACELOGD(...) (void)0
#endif

// Security check in case no GRAPHICS_API_OPENGL_* defined
#if !defined(GRAPHICS_API_OPENGL_11) && \
    !defined(GRAPHICS_API_OPENGL_21) && \
    !defined(GRAPHICS_API_OPENGL_33) && \
    !defined(GRAPHICS_API_OPENGL_43) && \
    !defined(GRAPHICS_API_OPENGL_ES2) && \
    !defined(GRAPHICS_API_OPENGL_ES3)
        #define GRAPHICS_API_OPENGL_33
#endif

// Security check in case multiple GRAPHICS_API_OPENGL_* defined
#if defined(GRAPHICS_API_OPENGL_11)
    #if defined(GRAPHICS_API_OPENGL_21)
        #undef GRAPHICS_API_OPENGL_21
    #endif
    #if defined(GRAPHICS_API_OPENGL_33)
        #undef GRAPHICS_API_OPENGL_33
    #endif
    #if defined(GRAPHICS_API_OPENGL_43)
        #undef GRAPHICS_API_OPENGL_43
    #endif
    #if defined(GRAPHICS_API_OPENGL_ES2)
        #undef GRAPHICS_API_OPENGL_ES2
    #endif
#endif

// OpenGL 2.1 uses most of OpenGL 3.3 Core functionality
// WARNING: Specific parts are checked with #if defines
#if defined(GRAPHICS_API_OPENGL_21)
    #define GRAPHICS_API_OPENGL_33
#endif

// OpenGL 4.3 uses OpenGL 3.3 Core functionality
#if defined(GRAPHICS_API_OPENGL_43)
    #define GRAPHICS_API_OPENGL_33
#endif

// OpenGL ES 3.0 uses OpenGL ES 2.0 functionality (and more)
#if defined(GRAPHICS_API_OPENGL_ES3)
    #define GRAPHICS_API_OPENGL_ES2
#endif

// Support framebuffer objects by default
// NOTE: Some driver implementation do not support it, despite they should
#define RLGL_RENDER_TEXTURES_HINT

#if defined(GRAPHICS_API_OPENGL_11)
    #if defined(__APPLE__)
        #include <OpenGL/gl.h>          // OpenGL 1.1 library for OSX
        #include <OpenGL/glext.h>       // OpenGL extensions library
    #else
        // APIENTRY for OpenGL function pointer declarations is required
        #if !defined(APIENTRY)
            #if defined(_WIN32)
                #define APIENTRY __stdcall
            #else
                #define APIENTRY
            #endif
        #endif
        // WINGDIAPI definition. Some Windows OpenGL headers need it
        #if !defined(WINGDIAPI) && defined(_WIN32)
            #define WINGDIAPI __declspec(dllimport)
        #endif

        #include <GL/gl.h>              // OpenGL 1.1 library
    #endif
#endif

#if defined(GRAPHICS_API_OPENGL_33)
    #include "glad/gl.h"                // GLAD extensions loading library, includes OpenGL headers
#endif

#if defined(GRAPHICS_API_OPENGL_ES3)
    #include <GLES3/gl3.h>              // OpenGL ES 3.0 library
    #define GL_GLEXT_PROTOTYPES
    #include <GLES2/gl2ext.h>           // OpenGL ES 2.0 extensions library
#elif defined(GRAPHICS_API_OPENGL_ES2)
    // NOTE: OpenGL ES 2.0 can be enabled on PLATFORM_DESKTOP,
    // in that case, functions are loaded from a custom glad for OpenGL ES 2.0
    #if defined(PLATFORM_DESKTOP)
        #include "glad/gles2.h"
    #else
        #define GL_GLEXT_PROTOTYPES
        //#include <EGL/egl.h>          // EGL library -> not required, platform layer
        #include <GLES2/gl2.h>          // OpenGL ES 2.0 library
        #include <GLES2/gl2ext.h>       // OpenGL ES 2.0 extensions library
    #endif

    // It seems OpenGL ES 2.0 instancing entry points are not defined on Raspberry Pi
    // provided headers (despite being defined in official Khronos GLES2 headers)
    #if defined(PLATFORM_DRM)
    typedef void (GL_APIENTRYP PFNGLDRAWARRAYSINSTANCEDEXTPROC) (GLenum mode, GLint start, GLsizei count, GLsizei primcount);
    typedef void (GL_APIENTRYP PFNGLDRAWELEMENTSINSTANCEDEXTPROC) (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount);
    typedef void (GL_APIENTRYP PFNGLVERTEXATTRIBDIVISOREXTPROC) (GLuint index, GLuint divisor);
    #endif
#endif

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------

// Default internal render batch elements limits
#ifndef RL_DEFAULT_BATCH_BUFFER_ELEMENTS
    #if defined(GRAPHICS_API_OPENGL_11) || defined(GRAPHICS_API_OPENGL_33)
        // This is the maximum amount of elements (quads) per batch
        // NOTE: Be careful with text, every letter maps to a quad
        #define RL_DEFAULT_BATCH_BUFFER_ELEMENTS  8192
    #endif
    #if defined(GRAPHICS_API_OPENGL_ES2)
        // We reduce memory sizes for embedded systems (RPI and HTML5)
        // NOTE: On HTML5 (emscripten) this is allocated on heap,
        // by default it's only 16MB!...just take care...
        #define RL_DEFAULT_BATCH_BUFFER_ELEMENTS  2048
    #endif
#endif
#ifndef RL_DEFAULT_BATCH_BUFFERS
    #define RL_DEFAULT_BATCH_BUFFERS                 1      // Default number of batch buffers (multi-buffering)
#endif
#ifndef RL_DEFAULT_BATCH_DRAWCALLS
    #define RL_DEFAULT_BATCH_DRAWCALLS             256      // Default number of batch draw calls (by state changes: mode, texture)
#endif
#ifndef RL_DEFAULT_BATCH_MAX_TEXTURE_UNITS
    #define RL_DEFAULT_BATCH_MAX_TEXTURE_UNITS       4      // Maximum number of textures units that can be activated on batch drawing (SetShaderValueTexture())
#endif

// Internal Matrix stack
#ifndef RL_MAX_MATRIX_STACK_SIZE
    #define RL_MAX_MATRIX_STACK_SIZE                32      // Maximum size of Matrix stack
#endif

// Shader limits
#ifndef RL_MAX_SHADER_LOCATIONS
    #define RL_MAX_SHADER_LOCATIONS                 32      // Maximum number of shader locations supported
#endif

#ifndef GL_SHADING_LANGUAGE_VERSION
    #define GL_SHADING_LANGUAGE_VERSION         0x8B8C
#endif

#ifndef GL_COMPRESSED_RGB_S3TC_DXT1_EXT
    #define GL_COMPRESSED_RGB_S3TC_DXT1_EXT     0x83F0
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
    #define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT    0x83F1
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
    #define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT    0x83F2
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
    #define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT    0x83F3
#endif
#ifndef GL_ETC1_RGB8_OES
    #define GL_ETC1_RGB8_OES                    0x8D64
#endif
#ifndef GL_COMPRESSED_RGB8_ETC2
    #define GL_COMPRESSED_RGB8_ETC2             0x9274
#endif
#ifndef GL_COMPRESSED_RGBA8_ETC2_EAC
    #define GL_COMPRESSED_RGBA8_ETC2_EAC        0x9278
#endif
#ifndef GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG
    #define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG  0x8C00
#endif
#ifndef GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG
    #define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG 0x8C02
#endif
#ifndef GL_COMPRESSED_RGBA_ASTC_4x4_KHR
    #define GL_COMPRESSED_RGBA_ASTC_4x4_KHR     0x93b0
#endif
#ifndef GL_COMPRESSED_RGBA_ASTC_8x8_KHR
    #define GL_COMPRESSED_RGBA_ASTC_8x8_KHR     0x93b7
#endif

#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
    #define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT   0x84FF
#endif
#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
    #define GL_TEXTURE_MAX_ANISOTROPY_EXT       0x84FE
#endif

#if defined(GRAPHICS_API_OPENGL_11)
    #define GL_UNSIGNED_SHORT_5_6_5             0x8363
    #define GL_UNSIGNED_SHORT_5_5_5_1           0x8034
    #define GL_UNSIGNED_SHORT_4_4_4_4           0x8033
#endif

#if defined(GRAPHICS_API_OPENGL_21)
    #define GL_LUMINANCE                        0x1909
    #define GL_LUMINANCE_ALPHA                  0x190A
#endif

#if defined(GRAPHICS_API_OPENGL_ES2)
    #define glClearDepth                 glClearDepthf
    #if !defined(GRAPHICS_API_OPENGL_ES3)
        #define GL_READ_FRAMEBUFFER         GL_FRAMEBUFFER
        #define GL_DRAW_FRAMEBUFFER         GL_FRAMEBUFFER
    #endif
#endif

// Default shader vertex attribute names to set location points
#ifndef RL_DEFAULT_SHADER_ATTRIB_NAME_POSITION
    #define RL_DEFAULT_SHADER_ATTRIB_NAME_POSITION     "vertexPosition"    // Bound by default to shader location: 0
#endif
#ifndef RL_DEFAULT_SHADER_ATTRIB_NAME_TEXCOORD
    #define RL_DEFAULT_SHADER_ATTRIB_NAME_TEXCOORD     "vertexTexCoord"    // Bound by default to shader location: 1
#endif
#ifndef RL_DEFAULT_SHADER_ATTRIB_NAME_NORMAL
    #define RL_DEFAULT_SHADER_ATTRIB_NAME_NORMAL       "vertexNormal"      // Bound by default to shader location: 2
#endif
#ifndef RL_DEFAULT_SHADER_ATTRIB_NAME_COLOR
    #define RL_DEFAULT_SHADER_ATTRIB_NAME_COLOR        "vertexColor"       // Bound by default to shader location: 3
#endif
#ifndef RL_DEFAULT_SHADER_ATTRIB_NAME_TANGENT
    #define RL_DEFAULT_SHADER_ATTRIB_NAME_TANGENT      "vertexTangent"     // Bound by default to shader location: 4
#endif
#ifndef RL_DEFAULT_SHADER_ATTRIB_NAME_TEXCOORD2
    #define RL_DEFAULT_SHADER_ATTRIB_NAME_TEXCOORD2    "vertexTexCoord2"   // Bound by default to shader location: 5
#endif

#ifndef RL_DEFAULT_SHADER_UNIFORM_NAME_MVP
    #define RL_DEFAULT_SHADER_UNIFORM_NAME_MVP         "mvp"               // model-view-projection matrix
#endif
#ifndef RL_DEFAULT_SHADER_UNIFORM_NAME_VIEW
    #define RL_DEFAULT_SHADER_UNIFORM_NAME_VIEW        "matView"           // view matrix
#endif
#ifndef RL_DEFAULT_SHADER_UNIFORM_NAME_PROJECTION
    #define RL_DEFAULT_SHADER_UNIFORM_NAME_PROJECTION  "matProjection"     // projection matrix
#endif
#ifndef RL_DEFAULT_SHADER_UNIFORM_NAME_MODEL
    #define RL_DEFAULT_SHADER_UNIFORM_NAME_MODEL       "matModel"          // model matrix
#endif
#ifndef RL_DEFAULT_SHADER_UNIFORM_NAME_NORMAL
    #define RL_DEFAULT_SHADER_UNIFORM_NAME_NORMAL      "matNormal"         // normal matrix (transpose(inverse(matModelView))
#endif
#ifndef RL_DEFAULT_SHADER_UNIFORM_NAME_COLOR
    #define RL_DEFAULT_SHADER_UNIFORM_NAME_COLOR       "colDiffuse"        // color diffuse (base tint color, multiplied by texture color)
#endif
#ifndef RL_DEFAULT_SHADER_SAMPLER2D_NAME_TEXTURE0
    #define RL_DEFAULT_SHADER_SAMPLER2D_NAME_TEXTURE0  "texture0"          // texture0 (texture slot active 0)
#endif
#ifndef RL_DEFAULT_SHADER_SAMPLER2D_NAME_TEXTURE1
    #define RL_DEFAULT_SHADER_SAMPLER2D_NAME_TEXTURE1  "texture1"          // texture1 (texture slot active 1)
#endif
#ifndef RL_DEFAULT_SHADER_SAMPLER2D_NAME_TEXTURE2
    #define RL_DEFAULT_SHADER_SAMPLER2D_NAME_TEXTURE2  "texture2"          // texture2 (texture slot active 2)
#endif

#endif //RLGL_CONFIG_HPP
