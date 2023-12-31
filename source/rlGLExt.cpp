#include "rlGLExt.hpp"
#include "rlConfig.hpp"

#if defined(GRAPHICS_API_OPENGL_ES2) && !defined(GRAPHICS_API_OPENGL_ES3)

// NOTE: VAO functionality is exposed through extensions (OES)
PFNGLGENVERTEXARRAYSOESPROC glGenVertexArrays = nullptr;
PFNGLBINDVERTEXARRAYOESPROC glBindVertexArray = nullptr;
PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArrays = nullptr;

// NOTE: Instancing functionality could also be available through extension
PFNGLDRAWARRAYSINSTANCEDEXTPROC glDrawArraysInstanced = nullptr;
PFNGLDRAWELEMENTSINSTANCEDEXTPROC glDrawElementsInstanced = nullptr;
PFNGLVERTEXATTRIBDIVISOREXTPROC glVertexAttribDivisor = nullptr;

#endif

namespace {
    static bool ExtLoaded = false;                  ///< Variable indicating whether OpenGL extensions have been loaded
    static rlgl::GlExtensions ExtSupported{};       ///< Extensions supported flags
}

bool rlgl::IsExtensionsLoaded()
{
    return ExtLoaded;
}

void rlgl::LoadExtensions(void *loader(const char *))
{
    if (ExtLoaded) return;  // If the extensions have already been loaded we stop here

#if defined(GRAPHICS_API_OPENGL_33)     // Also defined for GRAPHICS_API_OPENGL_21

    // NOTE: glad is generated and contains only required OpenGL 3.3 Core extensions (and lower versions)
    if (gladLoadGL(reinterpret_cast<GLADloadfunc>(loader)) == 0)
    {
        TRACELOG(TraceLogLevel::Warning, "GLAD: Cannot load OpenGL extensions");
    }
    else
    {
        TRACELOG(TraceLogLevel::Info, "GLAD: OpenGL extensions loaded successfully");
    }

    // Get number of supported extensions
    GLint numExt = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
    TRACELOG(TraceLogLevel::Info, "GL: Supported extensions count: %i", numExt);

#   if defined(RLGL_SHOW_GL_DETAILS_INFO)
        // Get supported extensions list
        // WARNING: glGetStringi() not available on OpenGL 2.1
        TRACELOG(TraceLogLevel::Info, "GL: OpenGL extensions:");
        for (int i = 0; i < numExt; i++) TRACELOG(TraceLogLevel::Info, "    %s", glGetStringi(GL_EXTENSIONS, i));
#   endif

#   if defined(GRAPHICS_API_OPENGL_21)
        // Register supported extensions flags
        // Optional OpenGL 2.1 extensions
        ExtSupported.vao = GLAD_GL_ARB_vertex_array_object;
        ExtSupported.instancing = (GLAD_GL_EXT_draw_instanced && GLAD_GL_ARB_instanced_arrays);
        ExtSupported.texNPOT = GLAD_GL_ARB_texture_non_power_of_two;
        ExtSupported.texFloat32 = GLAD_GL_ARB_texture_float;
        ExtSupported.texFloat16 = GLAD_GL_ARB_texture_float;
        ExtSupported.texDepth = GLAD_GL_ARB_depth_texture;
        ExtSupported.maxDepthBits = 32;
        ExtSupported.texAnisoFilter = GLAD_GL_EXT_texture_filter_anisotropic;
        ExtSupported.texMirrorClamp = GLAD_GL_EXT_texture_mirror_clamp;
#   else
        // Register supported extensions flags
        // OpenGL 3.3 extensions supported by default (core)
        ExtSupported.vao = true;
        ExtSupported.instancing = true;
        ExtSupported.texNPOT = true;
        ExtSupported.texFloat32 = true;
        ExtSupported.texFloat16 = true;
        ExtSupported.texDepth = true;
        ExtSupported.maxDepthBits = 32;
        ExtSupported.texAnisoFilter = true;
        ExtSupported.texMirrorClamp = true;
#   endif

    // Optional OpenGL 3.3 extensions
    ExtSupported.texCompASTC = GLAD_GL_KHR_texture_compression_astc_hdr && GLAD_GL_KHR_texture_compression_astc_ldr;
    ExtSupported.texCompDXT = GLAD_GL_EXT_texture_compression_s3tc;  // Texture compression: DXT
    ExtSupported.texCompETC2 = GLAD_GL_ARB_ES3_compatibility;        // Texture compression: ETC2/EAC

#   if defined(GRAPHICS_API_OPENGL_43)
        ExtSupported.computeShader = GLAD_GL_ARB_compute_shader;
        ExtSupported.ssbo = GLAD_GL_ARB_shader_storage_buffer_object;
#   endif

#endif  // GRAPHICS_API_OPENGL_33

#if defined(GRAPHICS_API_OPENGL_ES3)

    // Register supported extensions flags
    // OpenGL ES 3.0 extensions supported by default (or it should be)
    ExtSupported.vao = true;
    ExtSupported.instancing = true;
    ExtSupported.texNPOT = true;
    ExtSupported.texFloat32 = true;
    ExtSupported.texFloat16 = true;
    ExtSupported.texDepth = true;
    ExtSupported.texDepthWebGL = true;
    ExtSupported.maxDepthBits = 24;
    ExtSupported.texAnisoFilter = true;
    ExtSupported.texMirrorClamp = true;
    // TODO: Check for additional OpenGL ES 3.0 supported extensions:
    //ExtSupported.texCompDXT = true;
    //ExtSupported.texCompETC1 = true;
    //ExtSupported.texCompETC2 = true;
    //ExtSupported.texCompPVRT = true;
    //ExtSupported.texCompASTC = true;
    //ExtSupported.maxAnisotropyLevel = true;
    //ExtSupported.computeShader = true;
    //ExtSupported.ssbo = true;

#elif defined(GRAPHICS_API_OPENGL_ES2)

#   if defined(PLATFORM_DESKTOP)

        // TODO: Support GLAD loader for OpenGL ES 3.0
        if (gladLoadGLES2(reinterpret_cast<GLADloadfunc>(loader)) == 0)
        {
            TRACELOG(TraceLogLevel::Warning, "GLAD: Cannot load OpenGL ES2.0 functions");
        }
        else
        {
            TRACELOG(TraceLogLevel::Info, "GLAD: OpenGL ES 2.0 loaded successfully");
        }

#   endif

    // Get supported extensions list
    GLint numExt = 0;
    const char **extList = new const char*[512];                                            // Allocate 512 strings pointers (2 KB)
    std::string extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));     // One big const string

    extList[numExt] = extensions.c_str();
    for (int i = 0; i < extensions.size(); i++)
    {
        if (extensions[i] == ' ')
        {
            extensions[i] = '\0';
            numExt++;
            extList[numExt] = &extensions[i + 1];
        }
    }

    TRACELOG(TraceLogLevel::Info, "GL: Supported extensions count: %i", numExt);

#   if defined(RLGL_SHOW_GL_DETAILS_INFO)
        TRACELOG(TraceLogLevel::Info, "GL: OpenGL extensions:");
        for (int i = 0; i < numExt; i++) TRACELOG(TraceLogLevel::Info, "    %s", extList[i]);
#   endif

    // Check required extensions
    for (int i = 0; i < numExt; i++)
    {
        // Check VAO support
        // NOTE: Only check on OpenGL ES, OpenGL 3.3 has VAO support as core feature
        if (std::strcmp(extList[i], (const char *)"GL_OES_vertex_array_object") == 0)
        {
            // The extension is supported by our hardware and driver, try to get related functions pointers
            // NOTE: emscripten does not support VAOs natively, it uses emulation and it reduces overall performance...
            glGenVertexArrays = (PFNGLGENVERTEXARRAYSOESPROC)((rlglLoadProc)loader)("glGenVertexArraysOES");
            glBindVertexArray = (PFNGLBINDVERTEXARRAYOESPROC)((rlglLoadProc)loader)("glBindVertexArrayOES");
            glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSOESPROC)((rlglLoadProc)loader)("glDeleteVertexArraysOES");
            //glIsVertexArray = (PFNGLISVERTEXARRAYOESPROC)loader("glIsVertexArrayOES");     // NOTE: Fails in WebGL, omitted

            if ((glGenVertexArrays != nullptr) && (glBindVertexArray != nullptr) && (glDeleteVertexArrays != nullptr)) ExtSupported.vao = true;
        }

        // Check instanced rendering support
        if (std::strcmp(extList[i], (const char *)"GL_ANGLE_instanced_arrays") == 0)         // Web ANGLE
        {
            glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDEXTPROC)((rlglLoadProc)loader)("glDrawArraysInstancedANGLE");
            glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDEXTPROC)((rlglLoadProc)loader)("glDrawElementsInstancedANGLE");
            glVertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISOREXTPROC)((rlglLoadProc)loader)("glVertexAttribDivisorANGLE");

            if ((glDrawArraysInstanced != nullptr) && (glDrawElementsInstanced != nullptr) && (glVertexAttribDivisor != nullptr)) ExtSupported.instancing = true;
        }
        else
        {
            if ((std::strcmp(extList[i], (const char *)"GL_EXT_draw_instanced") == 0) &&     // Standard EXT
                (std::strcmp(extList[i], (const char *)"GL_EXT_instanced_arrays") == 0))
            {
                glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDEXTPROC)((rlglLoadProc)loader)("glDrawArraysInstancedEXT");
                glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDEXTPROC)((rlglLoadProc)loader)("glDrawElementsInstancedEXT");
                glVertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISOREXTPROC)((rlglLoadProc)loader)("glVertexAttribDivisorEXT");

                if ((glDrawArraysInstanced != nullptr) && (glDrawElementsInstanced != nullptr) && (glVertexAttribDivisor != nullptr)) ExtSupported.instancing = true;
            }
        }

        // Check NPOT textures support
        // NOTE: Only check on OpenGL ES, OpenGL 3.3 has NPOT textures full support as core feature
        if (std::strcmp(extList[i], (const char *)"GL_OES_texture_npot") == 0) ExtSupported.texNPOT = true;

        // Check texture float support
        if (std::strcmp(extList[i], (const char *)"GL_OES_texture_float") == 0) ExtSupported.texFloat32 = true;
        if (std::strcmp(extList[i], (const char *)"GL_OES_texture_half_float") == 0) ExtSupported.texFloat16 = true;

        // Check depth texture support
        if (std::strcmp(extList[i], (const char *)"GL_OES_depth_texture") == 0) ExtSupported.texDepth = true;
        if (std::strcmp(extList[i], (const char *)"GL_WEBGL_depth_texture") == 0) ExtSupported.texDepthWebGL = true;    // WebGL requires unsized internal format
        if (ExtSupported.texDepthWebGL) ExtSupported.texDepth = true;

        if (std::strcmp(extList[i], (const char *)"GL_OES_depth24") == 0) ExtSupported.maxDepthBits = 24;   // Not available on WebGL
        if (std::strcmp(extList[i], (const char *)"GL_OES_depth32") == 0) ExtSupported.maxDepthBits = 32;   // Not available on WebGL

        // Check texture compression support: DXT
        if ((std::strcmp(extList[i], (const char *)"GL_EXT_texture_compression_s3tc") == 0) ||
            (std::strcmp(extList[i], (const char *)"GL_WEBGL_compressed_texture_s3tc") == 0) ||
            (std::strcmp(extList[i], (const char *)"GL_WEBKIT_WEBGL_compressed_texture_s3tc") == 0)) ExtSupported.texCompDXT = true;

        // Check texture compression support: ETC1
        if ((std::strcmp(extList[i], (const char *)"GL_OES_compressed_ETC1_RGB8_texture") == 0) ||
            (std::strcmp(extList[i], (const char *)"GL_WEBGL_compressed_texture_etc1") == 0)) ExtSupported.texCompETC1 = true;

        // Check texture compression support: ETC2/EAC
        if (std::strcmp(extList[i], (const char *)"GL_ARB_ES3_compatibility") == 0) ExtSupported.texCompETC2 = true;

        // Check texture compression support: PVR
        if (std::strcmp(extList[i], (const char *)"GL_IMG_texture_compression_pvrtc") == 0) ExtSupported.texCompPVRT = true;

        // Check texture compression support: ASTC
        if (std::strcmp(extList[i], (const char *)"GL_KHR_texture_compression_astc_hdr") == 0) ExtSupported.texCompASTC = true;

        // Check anisotropic texture filter support
        if (std::strcmp(extList[i], (const char *)"GL_EXT_texture_filter_anisotropic") == 0) ExtSupported.texAnisoFilter = true;

        // Check clamp mirror wrap mode support
        if (std::strcmp(extList[i], (const char *)"GL_EXT_texture_mirror_clamp") == 0) ExtSupported.texMirrorClamp = true;
    }

    // Free extensions pointers
    delete[] extList;

#endif  // GRAPHICS_API_OPENGL_ES2

    // Check OpenGL information and capabilities
    //------------------------------------------------------------------------------
    // Show current OpenGL and GLSL version
    TRACELOG(TraceLogLevel::Info, "GL: OpenGL device information:");
    TRACELOG(TraceLogLevel::Info, "    > Vendor:   %s", glGetString(GL_VENDOR));
    TRACELOG(TraceLogLevel::Info, "    > Renderer: %s", glGetString(GL_RENDERER));
    TRACELOG(TraceLogLevel::Info, "    > Version:  %s", glGetString(GL_VERSION));
    TRACELOG(TraceLogLevel::Info, "    > GLSL:     %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)

    // NOTE: Anisotropy levels capability is an extension
#   ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#       define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#   endif

    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &ExtSupported.maxAnisotropyLevel);

#   if defined(RLGL_SHOW_GL_DETAILS_INFO)

        // Show some OpenGL GPU capabilities
        TRACELOG(TraceLogLevel::Info, "GL: OpenGL capabilities:");
        GLint capability = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &capability);
        TRACELOG(TraceLogLevel::Info, "    GL_MAX_TEXTURE_SIZE: %i", capability);
        glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &capability);
        TRACELOG(TraceLogLevel::Info, "    GL_MAX_CUBE_MAP_TEXTURE_SIZE: %i", capability);
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &capability);
        TRACELOG(TraceLogLevel::Info, "    GL_MAX_TEXTURE_IMAGE_UNITS: %i", capability);
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &capability);
        TRACELOG(TraceLogLevel::Info, "    GL_MAX_VERTEX_ATTRIBS: %i", capability);
        #if !defined(GRAPHICS_API_OPENGL_ES2)
        glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &capability);
        TRACELOG(TraceLogLevel::Info, "    GL_MAX_UNIFORM_BLOCK_SIZE: %i", capability);
        glGetIntegerv(GL_MAX_DRAW_BUFFERS, &capability);
        TRACELOG(TraceLogLevel::Info, "    GL_MAX_DRAW_BUFFERS: %i", capability);
        if (ExtSupported.texAnisoFilter) TRACELOG(TraceLogLevel::Info, "    GL_MAX_TEXTURE_MAX_ANISOTROPY: %.0f", ExtSupported.maxAnisotropyLevel);
        #endif
        glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &capability);
        TRACELOG(TraceLogLevel::Info, "    GL_NUM_COMPRESSED_TEXTURE_FORMATS: %i", capability);
        GLint *compFormats = reinterpret_cast<GLint*>(std::calloc(capability, sizeof(GLint)));
        glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, compFormats);
        for (int i = 0; i < capability; i++) TRACELOG(TraceLogLevel::Info, "        %s", GetCompressedFormatName(compFormats[i]));
        std::free(compFormats);

#       if defined(GRAPHICS_API_OPENGL_43)
            glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &capability);
            TRACELOG(TraceLogLevel::Info, "    GL_MAX_VERTEX_ATTRIB_BINDINGS: %i", capability);
            glGetIntegerv(GL_MAX_UNIFORM_LOCATIONS, &capability);
            TRACELOG(TraceLogLevel::Info, "    GL_MAX_UNIFORM_LOCATIONS: %i", capability);
#       endif  // GRAPHICS_API_OPENGL_43

#   else   // RLGL_SHOW_GL_DETAILS_INFO

        // Show some basic info about GL supported features
        if (ExtSupported.vao) TRACELOG(TraceLogLevel::Info, "GL: VAO extension detected, VAO functions loaded successfully");
        else TRACELOG(TraceLogLevel::Warning, "GL: VAO extension not found, VAO not supported");
        if (ExtSupported.texNPOT) TRACELOG(TraceLogLevel::Info, "GL: NPOT textures extension detected, full NPOT textures supported");
        else TRACELOG(TraceLogLevel::Warning, "GL: NPOT textures extension not found, limited NPOT support (no-mipmaps, no-repeat)");
        if (ExtSupported.texCompDXT) TRACELOG(TraceLogLevel::Info, "GL: DXT compressed textures supported");
        if (ExtSupported.texCompETC1) TRACELOG(TraceLogLevel::Info, "GL: ETC1 compressed textures supported");
        if (ExtSupported.texCompETC2) TRACELOG(TraceLogLevel::Info, "GL: ETC2/EAC compressed textures supported");
        if (ExtSupported.texCompPVRT) TRACELOG(TraceLogLevel::Info, "GL: PVRT compressed textures supported");
        if (ExtSupported.texCompASTC) TRACELOG(TraceLogLevel::Info, "GL: ASTC compressed textures supported");
        if (ExtSupported.computeShader) TRACELOG(TraceLogLevel::Info, "GL: Compute shaders supported");
        if (ExtSupported.ssbo) TRACELOG(TraceLogLevel::Info, "GL: Shader storage buffer objects supported");

#   endif  // RLGL_SHOW_GL_DETAILS_INFO

#endif  // GRAPHICS_API_OPENGL_33 || GRAPHICS_API_OPENGL_ES2

    ExtLoaded = true; // Here we mark the extensions as loaded
}

const rlgl::GlExtensions& rlgl::GetExtensions()
{
    return ExtSupported;
}
