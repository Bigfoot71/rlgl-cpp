#ifndef RLGL_ENUMS_HPP
#define RLGL_ENUMS_HPP

#include <cstdint>

namespace rlgl {

    enum class GlVersion
    {
        OpenGL_11,                          ///< OpenGL 1.1
        OpenGL_21,                          ///< OpenGL 2.1 (GLSL 120)
        OpenGL_33,                          ///< OpenGL 3.3 (GLSL 330)
        OpenGL_43,                          ///< OpenGL 4.3 (using GLSL 330)
        OpenGL_ES_20,                       ///< OpenGL ES 2.0 (GLSL 100)
        OpenGL_ES_30                        ///< OpenGL ES 3.0 (GLSL 300 es)
    };

    enum LogLevel
    {
        LogAll          = 0,                ///< Display all logs
        LogDebug        = 1,                ///< Debug logging, used for internal debugging, it should be disabled on release builds
        LogInfo         = 2,                ///< Info logging, used for program execution info
        LogWarning      = 3,                ///< Warning logging, used on recoverable failures
        LogError        = 4,                ///< Error logging, used on unrecoverable failures
        LogFatal        = 5,                ///< Fatal logging, used to abort program: exit(EXIT_FAILURE)
        LogNone         = 6                 ///< Disable logging
    };

    enum class PixelFormat
    {
        Grayscale,                          ///< 8 bit per pixel (no alpha)
        GrayAlpha,                          ///< 8*2 bpp (2 channels)
        R5G6B5,                             ///< 16 bpp
        R8G8B8,                             ///< 24 bpp
        R5G5B5A1,                           ///< 16 bpp (1 bit alpha)
        R4G4B4A4,                           ///< 16 bpp (4 bit alpha)
        R8G8B8A8,                           ///< 32 bpp
        R32,                                ///< 32 bpp (1 channel - float)
        R32G32B32,                          ///< 32*3 bpp (3 channels - float)
        R32G32B32A32,                       ///< 32*4 bpp (4 channels - float)
        R16,                                ///< 16 bpp (1 channel - half float)
        R16G16B16,                          ///< 16*3 bpp (3 channels - half float)
        R16G16B16A16,                       ///< 16*4 bpp (4 channels - half float)

        DXT1_RGB,                           ///< 4 bpp (no alpha)
        DXT1_RGBA,                          ///< 4 bpp (1 bit alpha)
        DXT3_RGBA,                          ///< 8 bpp
        DXT5_RGBA,                          ///< 8 bpp
        ETC1_RGB,                           ///< 4 bpp
        ETC2_RGB,                           ///< 4 bpp
        ETC2_EAC_RGBA,                      ///< 8 bpp
        PVRT_RGB,                           ///< 4 bpp
        PVRT_RGBA,                          ///< 4 bpp
        ASTC_4x4_RGBA,                      ///< 8 bpp
        ASTC_8x8_RGBA                       ///< 2 bpp
    };

    enum class TextureParam
    {
        Wrap_S                  = 0x2802,   ///< GL_TEXTURE_WRAP_S
        Wrap_T                  = 0x2803,   ///< GL_TEXTURE_WRAP_T
        MagFilter               = 0x2800,   ///< GL_TEXTURE_MAG_FILTER
        MinFilter               = 0x2801,   ///< GL_TEXTURE_MIN_FILTER
        Anisotropy              = 0x3000,   ///< Anisotropic filter (custom identifier)
        MipmapBiasRatio         = 0x4000    ///< Texture mipmap bias, percentage ratio (custom identifier)
    };

    enum class TextureWrap
    {
        Repeat                  = 0x2901,   ///< GL_REPEAT
        Clamp                   = 0x812F,   ///< GL_CLAMP_TO_EDGE
        MirrorRepeat            = 0x2901,   ///< GL_MIRRORED_REPEAT
        MirrorClamp             = 0x812F    ///< GL_MIRROR_CLAMP_EXT
    };

    enum class TextureFilter
    {
        Nearest                 = 0x2600,   ///< GL_NEAREST
        Linear                  = 0x2601,   ///< GL_LINEAR
        MipNearest              = 0x2700,   ///< GL_NEAREST_MIPMAP_NEAREST
        NearestMipLinear        = 0x2702,   ///< GL_NEAREST_MIPMAP_LINEAR
        LinearMipNearest        = 0x2701,   ///< GL_LINEAR_MIPMAP_NEAREST
        MipLinear               = 0x2703    ///< GL_LINEAR_MIPMAP_LINEAR
    };

    enum class BlendingFactor
    {
        Zero                    = 0,        ///< GL_ZERO
        One                     = 1,        ///< GL_ONE
        SrcColor                = 0x0300,   ///< GL_SRC_COLOR
        OneMinusSrcColor        = 0x0301,   ///< GL_ONE_MINUS_SRC_COLOR
        SrcAlpha                = 0x0302,   ///< GL_SRC_ALPHA
        OneMinusSrcAlpha        = 0x0303,   ///< GL_ONE_MINUS_SRC_ALPHA
        DstAlpha                = 0x0304,   ///< GL_DST_ALPHA
        OneMinusDstAlpha        = 0x0305,   ///< GL_ONE_MINUS_DST_ALPHA
        DstColor                = 0x0306,   ///< GL_DST_COLOR
        OneMinusDstColor        = 0x0307,   ///< GL_ONE_MINUS_DST_COLOR
        SrcAlphaSaturate        = 0x0308,   ///< GL_SRC_ALPHA_SATURATE
        ConstantColor           = 0x8001,   ///< GL_CONSTANT_COLOR
        OneMinusConstantColor   = 0x8002,   ///< GL_ONE_MINUS_CONSTANT_COLOR
        ConstantAlpha           = 0x8003,   ///< GL_CONSTANT_ALPHA
        OneMinusConstantAlpha   = 0x8004    ///< GL_ONE_MINUS_CONSTANT_ALPHA
    };

    enum class BlendingFunction
    {
        FuncAdd                 = 0x8006,   ///< GL_FUNC_ADD
        Min                     = 0x8007,   ///< GL_MIN
        Max                     = 0x8008,   ///< GL_MAX
        FuncSubtract            = 0x800A,   ///< GL_FUNC_SUBTRACT
        FuncReverseSubtract     = 0x800B,   ///< GL_FUNC_REVERSE_SUBTRACT
        BlendEquation           = 0x8009,   ///< GL_BLEND_EQUATION
        BlendEquationRGB        = 0x8009,   ///< GL_BLEND_EQUATION_RGB (Same as BlendEquation)
        BlendEquationAlpha      = 0x883D,   ///< GL_BLEND_EQUATION_ALPHA
        BlendDstRGB             = 0x80C8,   ///< GL_BLEND_DST_RGB
        BlendSrcRGB             = 0x80C9,   ///< GL_BLEND_SRC_RGB
        BlendDstAlpha           = 0x80CA,   ///< GL_BLEND_DST_ALPHA
        BlendSrcAlpha           = 0x80CB,   ///< GL_BLEND_SRC_ALPHA
        BlendColor              = 0x8005    ///< GL_BLEND_COLOR
    }; 

    enum class BlendMode
    {
        Alpha,                              ///< Blend textures considering alpha (default)
        Additive,                           ///< Blend textures adding colors
        Multiplied,                         ///< Blend textures multiplying colors
        AddColors,                          ///< Blend textures adding colors (alternative)
        SubtractColors,                     ///< Blend textures subtracting colors (alternative)
        AlphaPremultiply,                   ///< Blend premultiplied textures considering alpha
        Custom,                             ///< Blend textures using custom src/dst factors (use rlSetBlendFactors())
        CustomSeparate                      ///< Blend textures using custom src/dst factors (use rlSetBlendFactorsSeparate())
    };

    enum class ShaderType
    {
        Fragment                = 0x8B30,   ///< GL_FRAGMENT_SHADER
        Vertex                  = 0x8B31,   ///< GL_VERTEX_SHADER
        Compute                 = 0x91B9    ///< GL_COMPUTE_SHADER
    };

    enum ShaderLocationIndex : uint8_t
    {
        LocVertexPosition       = 0,        ///< Shader location: vertex attribute: position
        LocVertexTexCoord01     = 1,        ///< Shader location: vertex attribute: texcoord01
        LocVertexTexCoord02     = 2,        ///< Shader location: vertex attribute: texcoord02
        LocVertexNormal         = 3,        ///< Shader location: vertex attribute: normal
        LocVertexTangent        = 4,        ///< Shader location: vertex attribute: tangent
        LocVertexColor          = 5,        ///< Shader location: vertex attribute: color
        LocMatrixMVP            = 6,        ///< Shader location: matrix uniform: model-view-projection
        LocMatrixView           = 7,        ///< Shader location: matrix uniform: view (camera transform)
        LocMatrixProjection     = 8,        ///< Shader location: matrix uniform: projection
        LocMatrixModel          = 9,        ///< Shader location: matrix uniform: model (transform)
        LocMatrixNormal         = 10,       ///< Shader location: matrix uniform: normal
        LocVectorView           = 11,       ///< Shader location: vector uniform: view
        LocColorDiffuse         = 12,       ///< Shader location: vector uniform: diffuse color
        LocColorSpecular        = 13,       ///< Shader location: vector uniform: specular color
        LocColorAmbient         = 14,       ///< Shader location: vector uniform: ambient color
        LocMapAlbedo            = 15,       ///< Shader location: sampler2d texture: albedo (same as: MapDiffuse)
        LocMapDiffuse           = 15,       ///< Shader location: sampler2d texture: diffuse (same as: MapAlbedo)
        LocMapMetalness         = 16,       ///< Shader location: sampler2d texture: metalness (same as: MapSpecular)
        LocMapSpecular          = 16,       ///< Shader location: sampler2d texture: specular (same as: MapMetalness)
        LocMapNormal            = 17,       ///< Shader location: sampler2d texture: normal
        LocMapRoughness         = 18,       ///< Shader location: sampler2d texture: roughness
        LocMapOcclusion         = 19,       ///< Shader location: sampler2d texture: occlusion
        LocMapEmission          = 20,       ///< Shader location: sampler2d texture: emission
        LocMapHeight            = 21,       ///< Shader location: sampler2d texture: height
        LocMapCubemap           = 22,       ///< Shader location: samplerCube texture: cubemap
        LocMapIrradiance        = 23,       ///< Shader location: samplerCube texture: irradiance
        LocMapPrefilter         = 24,       ///< Shader location: samplerCube texture: prefilter
        LocMapBRDF              = 25        ///< Shader location: sampler2d texture: brdf
    };

    enum class ShaderUniformType
    {
        Float,                              ///< Shader uniform type: float
        Vec2,                               ///< Shader uniform type: vec2 (2 float)
        Vec3,                               ///< Shader uniform type: vec3 (3 float)
        Vec4,                               ///< Shader uniform type: vec4 (4 float)
        Int,                                ///< Shader uniform type: int
        IVec2,                              ///< Shader uniform type: ivec2 (2 int)
        IVec3,                              ///< Shader uniform type: ivec3 (3 int)
        IVec4,                              ///< Shader uniform type: ivec4 (4 int)
        Sampler2D                           ///< Shader uniform type: sampler2d
    };

    enum class ShaderAttributeType
    {
        Float,                              ///< Shader attribute type: float
        Vec2,                               ///< Shader attribute type: vec2 (2 float)
        Vec3,                               ///< Shader attribute type: vec3 (3 float)
        Vec4                                ///< Shader attribute type: vec4 (4 float)
    };

    // NOTE: By default up to 8 color channels defined, but it can be more
    enum class FramebufferAttachType : uint8_t
    {
        ColorChannel0           = 0,        ///< Framebuffer attachment type: color 0
        ColorChannel1           = 1,        ///< Framebuffer attachment type: color 1
        ColorChannel2           = 2,        ///< Framebuffer attachment type: color 2
        ColorChannel3           = 3,        ///< Framebuffer attachment type: color 3
        ColorChannel4           = 4,        ///< Framebuffer attachment type: color 4
        ColorChannel5           = 5,        ///< Framebuffer attachment type: color 5
        ColorChannel6           = 6,        ///< Framebuffer attachment type: color 6
        ColorChannel7           = 7,        ///< Framebuffer attachment type: color 7
        Depth                   = 100,      ///< Framebuffer attachment type: depth
        Stencil                 = 200,      ///< Framebuffer attachment type: stencil
    } ;

    enum class FramebufferAttachTextureType
    {
        CubemapPositiveX        = 0,        ///< Framebuffer texture attachment type: cubemap, +X side
        CubemapNegativeX        = 1,        ///< Framebuffer texture attachment type: cubemap, -X side
        CubemapPositiveY        = 2,        ///< Framebuffer texture attachment type: cubemap, +Y side
        CubemapNegativeY        = 3,        ///< Framebuffer texture attachment type: cubemap, -Y side
        CubemapPositiveZ        = 4,        ///< Framebuffer texture attachment type: cubemap, +Z side
        CubemapNegativeZ        = 5,        ///< Framebuffer texture attachment type: cubemap, -Z side
        Texture2D               = 100,      ///< Framebuffer texture attachment type: texture2d
        RenderBuffer            = 200,      ///< Framebuffer texture attachment type: renderbuffer
    };

    enum class CullMode
    {
        FaceFront,
        FaceBack
    };

    enum class MatrixMode
    {
        ModelView               = 0x1700,   ///< GL_MODELVIEW
        Projection              = 0x1701,   ///< GL_PROJECTION
        Texture                 = 0x1702    ///< GL_TEXTURE
    };

    enum class DrawMode
    {
        Lines                   = 0x0001,   ///< GL_LINES
        Triangles               = 0x0004,   ///< GL_TRIANGLES
        Quads                   = 0x0007    ///< GL_QUADS
    };

    enum class DataType
    {
        UnsignedByte            = 0x1401,   ///< GL_UNSIGNED_BYTE
        Float                   = 0x1406    ///< GL_FLOAT
    };

    enum class BufferUsage
    {
        StreamDraw              = 0x88E0,   ///< GL_STREAM_DRAW
        StreamRead              = 0x88E1,   ///< GL_STREAM_READ
        StreamCopy              = 0x88E2,   ///< GL_STREAM_COPY
        StaticDraw              = 0x88E4,   ///< GL_STATIC_DRAW
        StaticRead              = 0x88E5,   ///< GL_STATIC_READ
        StaticCopy              = 0x88E6,   ///< GL_STATIC_COPY
        DynamicDraw             = 0x88E8,   ///< GL_DYNAMIC_DRAW
        DynamicRead             = 0x88E9,   ///< GL_DYNAMIC_READ
        DynamicCopy             = 0x88EA    ///< GL_DYNAMIC_COPY
    };  

}

#endif //RLGL_ENUMS_HPP
