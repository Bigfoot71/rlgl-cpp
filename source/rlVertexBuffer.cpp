#include "rlVertexBuffer.hpp"
#include "rlException.hpp"
#include "rlGLExt.hpp"
#include "rlgl.hpp"
#include <cstring>

using namespace rlgl;

VertexBuffer::VertexBuffer(const int* shaderLocs, int bufferElements) : elementCount(bufferElements)
{
    vertices = new float[bufferElements*3*4]{};     ///< 3 float by vertex, 4 vertex by quad
    texcoords = new float[bufferElements*2*4]{};    ///< 2 float by texcoord, 4 texcoord by quad
    colors = new uint8_t[bufferElements*4*4]{};     ///< 4 float by color, 4 colors by quad

#   if defined(GRAPHICS_API_OPENGL_33)
        indices = new uint32_t[bufferElements*6];   ///< 6 int by quad (indices)
#   endif

#   if defined(GRAPHICS_API_OPENGL_ES2)
        indices = new uint16_t[bufferElements*6];   ///< 6 int by quad (indices)
#   endif

    // Indices can be initialized right now
    for (int j = 0, k = 0; j < (6*bufferElements); j += 6, k++)
    {
        indices[j] = 4*k;
        indices[j + 1] = 4*k + 1;
        indices[j + 2] = 4*k + 2;
        indices[j + 3] = 4*k;
        indices[j + 4] = 4*k + 2;
        indices[j + 5] = 4*k + 3;
    }

    if (GetExtensions().vao)
    {
        // Initialize Quads VAO
        glGenVertexArrays(1, &vaoId);
        glBindVertexArray(vaoId);
    }

    // Quads - Vertex buffers binding and attributes enable
    // Vertex position buffer (shader-location = 0)
    glGenBuffers(1, &vboId[0]);
    glBindBuffer(GL_ARRAY_BUFFER, vboId[0]);
    glBufferData(GL_ARRAY_BUFFER, bufferElements*3*4*sizeof(float), vertices, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(shaderLocs[LocVertexPosition]);
    glVertexAttribPointer(shaderLocs[LocVertexPosition], 3, GL_FLOAT, 0, 0, 0);

    // Vertex texcoord buffer (shader-location = 1)
    glGenBuffers(1, &vboId[1]);
    glBindBuffer(GL_ARRAY_BUFFER, vboId[1]);
    glBufferData(GL_ARRAY_BUFFER, bufferElements*2*4*sizeof(float), texcoords, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(shaderLocs[LocVertexTexCoord01]);
    glVertexAttribPointer(shaderLocs[LocVertexTexCoord01], 2, GL_FLOAT, 0, 0, 0);

    // Vertex color buffer (shader-location = 3)
    glGenBuffers(1, &vboId[2]);
    glBindBuffer(GL_ARRAY_BUFFER, vboId[2]);
    glBufferData(GL_ARRAY_BUFFER, bufferElements*4*4*sizeof(unsigned char), colors, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(shaderLocs[LocVertexColor]);
    glVertexAttribPointer(shaderLocs[LocVertexColor], 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);

    // Fill index buffer
    glGenBuffers(1, &vboId[3]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboId[3]);

#   if defined(GRAPHICS_API_OPENGL_33)
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, bufferElements*6*sizeof(int), indices, GL_STATIC_DRAW);
#   endif

#   if defined(GRAPHICS_API_OPENGL_ES2)
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, bufferElements*6*sizeof(short), indices, GL_STATIC_DRAW);
#   endif
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
    other.vertices = nullptr;
    other.texcoords = nullptr;
    other.colors = nullptr;
    other.indices = nullptr;
    other.vaoId = 0;

    std::copy(other.vboId, other.vboId + 4, vboId);
    std::fill(other.vboId, other.vboId + 4, 0);
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

        other.vertices = nullptr;
        other.texcoords = nullptr;
        other.colors = nullptr;
        other.indices = nullptr;
        other.vaoId = 0;

        std::copy(other.vboId, other.vboId + 4, vboId);
        std::fill(other.vboId, other.vboId + 4, 0);
    }
    return *this;
}