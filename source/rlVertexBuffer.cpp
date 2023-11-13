#include "rlVertexBuffer.hpp"
#include "rlException.hpp"
#include "rlGLExt.hpp"
#include "rlgl.hpp"
#include <cstring>

using namespace rlgl;

VertexBuffer::VertexBuffer()
: vertices(nullptr), texcoords(nullptr)
, colors(nullptr), indices(nullptr) { }

VertexBuffer::VertexBuffer(int bufferElements) : elementCount(bufferElements)
{
    vertices = new float[bufferElements*3*4];       ///< 3 float by vertex, 4 vertex by quad
    texcoords = new float[bufferElements*2*4];      ///< 2 float by texcoord, 4 texcoord by quad
    colors = new uint8_t[bufferElements*4*4];       ///< 4 float by color, 4 colors by quad

#   if defined(GRAPHICS_API_OPENGL_33)
        indices = new uint32_t[bufferElements*6];   ///< 6 int by quad (indices)
#   endif

#   if defined(GRAPHICS_API_OPENGL_ES2)
        indices = new uint16_t[bufferElements*6];   ///< 6 int by quad (indices)
#   endif

    std::fill(vertices, vertices + 3*4*bufferElements, 0.0f);
    std::fill(texcoords, texcoords + 2*4*bufferElements, 0.0f);
    std::fill(colors, colors + 4*4*bufferElements, 0);

    int k = 0;

    // Indices can be initialized right now
    for (int j = 0; j < (6*bufferElements); j += 6)
    {
        indices[j] = 4*k;
        indices[j + 1] = 4*k + 1;
        indices[j + 2] = 4*k + 2;
        indices[j + 3] = 4*k;
        indices[j + 4] = 4*k + 2;
        indices[j + 5] = 4*k + 3;

        k++;
    }
}

VertexBuffer::~VertexBuffer()
{
    // Unbind VAO attribs data
    if (GetExtensions().vao)
    {
        glBindVertexArray(vaoId);
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
        glDisableVertexAttribArray(3);
        glBindVertexArray(0);
    }

    // Delete VBOs from GPU (VRAM)
    glDeleteBuffers(1, &vboId[0]);
    glDeleteBuffers(1, &vboId[1]);
    glDeleteBuffers(1, &vboId[2]);
    glDeleteBuffers(1, &vboId[3]);

    // Delete VAOs from GPU (VRAM)
    if (GetExtensions().vao)
    {
        glDeleteVertexArrays(1, &vaoId);
    }

    // Free allocated memory CPU (RAM)
    delete[] indices;
    delete[] colors;
    delete[] texcoords;
    delete[] vertices;

    // Set pointers to null
    vertices = nullptr;
    texcoords = nullptr;
    colors = nullptr;
    indices = nullptr;
}

VertexBuffer::VertexBuffer(VertexBuffer&& other) noexcept
: elementCount(other.elementCount)
, vertices(other.vertices)
, texcoords(other.texcoords)
, colors(other.colors)
, indices(other.indices)
, vaoId(other.vaoId)
{
    std::memcpy(vboId, other.vboId, sizeof(vboId));

    other.vertices = nullptr;
    other.texcoords = nullptr;
    other.colors = nullptr;
    other.indices = nullptr;
}

VertexBuffer& VertexBuffer::operator=(VertexBuffer&& other) noexcept
{
    if (this != &other)
    {
        elementCount = other.elementCount;

        vertices = other.vertices;
        texcoords = other.texcoords;
        colors = other.colors;
        indices = other.indices;

        vaoId = other.vaoId;
        std::memcpy(vboId, other.vboId, sizeof(vboId));

        other.vertices = nullptr;
        other.texcoords = nullptr;
        other.colors = nullptr;
        other.indices = nullptr;
    }
    return *this;
}