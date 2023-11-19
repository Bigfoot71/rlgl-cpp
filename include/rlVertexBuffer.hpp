#ifndef RLGL_VERTEX_BUFFER_HPP
#define RLGL_VERTEX_BUFFER_HPP

#include "./rlConfig.hpp"

namespace rlgl {

    // Dynamic vertex buffers (position + texcoords + colors + indices arrays)

    struct VertexBuffer
    {
        int elementCount        = 0;        ///< Number of elements in the buffer (QUADS)

        float *vertices         = nullptr;  ///< Vertex position (XYZ - 3 components per vertex) (shader-location = 0)
        float *texcoords        = nullptr;  ///< Vertex texture coordinates (UV - 2 components per vertex) (shader-location = 1)
        unsigned char *colors   = nullptr;  ///< Vertex colors (RGBA - 4 components per vertex) (shader-location = 3)

#       if defined(GRAPHICS_API_OPENGL_11) || defined(GRAPHICS_API_OPENGL_33)
            uint32_t *indices   = nullptr;  ///< Vertex indices (in case vertex data comes indexed) (6 indices per quad)
#       endif

#       if defined(GRAPHICS_API_OPENGL_ES2)
            uint16_t *indices   = nullptr;  ///< Vertex indices (in case vertex data comes indexed) (6 indices per quad)
#       endif

        uint32_t vaoId = 0;                 ///< OpenGL Vertex Array Object id
        uint32_t vboId[4]{};                ///< OpenGL Vertex Buffer Objects id (4 types of vertex data)

        VertexBuffer() = default;

        VertexBuffer(const int* shaderLocs, int bufferElements);
        ~VertexBuffer();

        VertexBuffer(const VertexBuffer&) = delete;
        VertexBuffer& operator=(const VertexBuffer&) = delete;

        VertexBuffer(VertexBuffer&& other) noexcept;
        VertexBuffer& operator=(VertexBuffer&& other) noexcept;
    };

}

#endif //RLGL_VERTEX_BUFFER_HPP
