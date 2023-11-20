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

        VertexBuffer(const int *shaderLocs, int bufferElements);
        ~VertexBuffer();

        VertexBuffer(const VertexBuffer&) = delete;
        VertexBuffer& operator=(const VertexBuffer&) = delete;

        VertexBuffer(VertexBuffer&& other) noexcept;
        VertexBuffer& operator=(VertexBuffer&& other) noexcept;

        /**
         * @brief Updates the vertex data in the Vertex Buffer Object (VBO).
         *
         * This function is responsible for updating the vertex data in the VBO associated
         * with the VertexBuffer instance. It assumes that the vertex data is organized
         * into three main buffers: vertex positions, texture coordinates, and colors.
         *
         * @param vertexCounter The number of vertices to update in the VBO.
         *
         * The function performs the following steps:
         * 1. Activates the Vertex Array Object (VAO) if supported.
         * 2. Updates the vertex positions buffer in the VBO.
         * 3. Updates the texture coordinates buffer in the VBO.
         * 4. Updates the colors buffer in the VBO.
         * 5. Unbinds the current VAO if supported.
         *
         * Note: This function assumes that OpenGL is being used, and it should be
         * called within a valid OpenGL rendering context.
         *
         * @see VertexBuffer
         * @see GetExtensions()
         * @see glBindVertexArray
         * @see glBindBuffer
         * @see glBufferSubData
         * @see glVertexAttribPointer
         * @see glEnableVertexAttribArray
         */
        void Update(int vertexCounter) const;

        /**
         * @brief Binds the Vertex Buffer Object (VBO) and Vertex Array Object (VAO) for rendering.
         *
         * This function is responsible for binding the VBO and VAO associated with the
         * VertexBuffer instance, setting up vertex attribute pointers for position, texture
         * coordinates, and colors. If VAO is supported, it directly binds the VAO; otherwise,
         * it manually configures the vertex attribute pointers and element array buffer.
         *
         * @param currentShaderLocs An array of shader locations for vertex attributes.
         *                          The order should match the shader's attribute layout.
         *
         * The function performs the following steps:
         * 1. Binds the VAO directly if supported.
         * 2. Manually configures and binds vertex attribute pointers for position, texcoord, and color.
         * 3. Binds the element array buffer for indexed rendering.
         *
         * Note: This function assumes that OpenGL is being used, and it should be called
         * within a valid OpenGL rendering context.
         *
         * @see VertexBuffer
         * @see GetExtensions()
         * @see glBindVertexArray
         * @see glBindBuffer
         * @see glVertexAttribPointer
         * @see glEnableVertexAttribArray
         */
        void Bind(const int *currentShaderLocs) const;
    };

}

#endif //RLGL_VERTEX_BUFFER_HPP
