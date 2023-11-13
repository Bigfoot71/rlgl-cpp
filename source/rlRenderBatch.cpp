#include "rlRenderBatch.hpp"
#include "rlException.hpp"
#include "rlEnums.hpp"
#include "rlGLExt.hpp"
#include "rlgl.hpp"

#include <algorithm>

using namespace rlgl;

RenderBatch::RenderBatch(const Context* rlCtx, int numBuffers, int bufferElements)
: bufferCount(numBuffers), drawCounter(1), currentDepth(-1.0f)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)

    // Initialize CPU (RAM) vertex buffers (position, texcoord, color data and indexes)
    //--------------------------------------------------------------------------------------------
    vertexBuffer = new VertexBuffer[numBuffers];
    TRACELOG(TraceLogLevel::Info, "RLGL: Render batch vertex buffers loaded successfully in RAM (CPU)");
    //--------------------------------------------------------------------------------------------

    // Upload to GPU (VRAM) vertex data and initialize VAOs/VBOs
    //--------------------------------------------------------------------------------------------
    for (int i = 0; i < numBuffers; i++)
    {
        // Create vertex buffer
        vertexBuffer[i] = VertexBuffer(bufferElements);

        if (GetExtensions().vao)
        {
            // Initialize Quads VAO
            glGenVertexArrays(1, &vertexBuffer[i].vaoId);
            glBindVertexArray(vertexBuffer[i].vaoId);
        }

        // Quads - Vertex buffers binding and attributes enable
        // Vertex position buffer (shader-location = 0)
        glGenBuffers(1, &vertexBuffer[i].vboId[0]);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer[i].vboId[0]);
        glBufferData(GL_ARRAY_BUFFER, bufferElements*3*4*sizeof(float), vertexBuffer[i].vertices, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(rlCtx->State.currentShaderLocs[static_cast<int>(ShaderLocationIndex::VertexPosition)]);
        glVertexAttribPointer(rlCtx->State.currentShaderLocs[static_cast<int>(ShaderLocationIndex::VertexPosition)], 3, GL_FLOAT, 0, 0, 0);

        // Vertex texcoord buffer (shader-location = 1)
        glGenBuffers(1, &vertexBuffer[i].vboId[1]);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer[i].vboId[1]);
        glBufferData(GL_ARRAY_BUFFER, bufferElements*2*4*sizeof(float), vertexBuffer[i].texcoords, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(rlCtx->State.currentShaderLocs[static_cast<int>(ShaderLocationIndex::VertexTexCoord01)]);
        glVertexAttribPointer(rlCtx->State.currentShaderLocs[static_cast<int>(ShaderLocationIndex::VertexTexCoord01)], 2, GL_FLOAT, 0, 0, 0);

        // Vertex color buffer (shader-location = 3)
        glGenBuffers(1, &vertexBuffer[i].vboId[2]);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer[i].vboId[2]);
        glBufferData(GL_ARRAY_BUFFER, bufferElements*4*4*sizeof(unsigned char), vertexBuffer[i].colors, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(rlCtx->State.currentShaderLocs[static_cast<int>(ShaderLocationIndex::VertexColor)]);
        glVertexAttribPointer(rlCtx->State.currentShaderLocs[static_cast<int>(ShaderLocationIndex::VertexColor)], 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);

        // Fill index buffer
        glGenBuffers(1, &vertexBuffer[i].vboId[3]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexBuffer[i].vboId[3]);

#       if defined(GRAPHICS_API_OPENGL_33)
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, bufferElements*6*sizeof(int), vertexBuffer[i].indices, GL_STATIC_DRAW);
#       endif

#       if defined(GRAPHICS_API_OPENGL_ES2)
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, bufferElements*6*sizeof(short), vertexBuffer[i].indices, GL_STATIC_DRAW);
#       endif
    }

    TRACELOG(TraceLogLevel::Info, "RLGL: Render batch vertex buffers loaded successfully in VRAM (GPU)");

    // Unbind the current VAO
    if (GetExtensions().vao) glBindVertexArray(0);
    //--------------------------------------------------------------------------------------------

    // Init draw calls tracking system
    //--------------------------------------------------------------------------------------------
    draws = new DrawCall[RL_DEFAULT_BATCH_DRAWCALLS];

    for (int i = 0; i < RL_DEFAULT_BATCH_DRAWCALLS; i++)
    {
        draws[i].mode = DrawMode::Quads;
        draws[i].vertexCount = 0;
        draws[i].vertexAlignment = 0;
        //draws[i].vaoId = 0;
        //draws[i].shaderId = 0;
        draws[i].textureId = rlCtx->State.defaultTextureId;
        //draws[i].RLGL.State.projection = rlMatrixIdentity();
        //draws[i].RLGL.State.modelview = rlMatrixIdentity();
    }
    //--------------------------------------------------------------------------------------------
#endif
}

RenderBatch::~RenderBatch()
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)

    // Unbind everything
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    delete[] draws;
    delete[] vertexBuffer;

#endif
}

RenderBatch::RenderBatch(RenderBatch&& other) noexcept
  : bufferCount(other.bufferCount),
    currentBuffer(other.currentBuffer),
    vertexBuffer(other.vertexBuffer),
    draws(other.draws),
    drawCounter(other.drawCounter),
    currentDepth(other.currentDepth)
{
    other.vertexBuffer = nullptr;
    other.draws = nullptr;
}

RenderBatch& RenderBatch::operator=(RenderBatch&& other) noexcept
{
    if (this != &other)
    {
        bufferCount = other.bufferCount;
        currentBuffer = other.currentBuffer;
        vertexBuffer = other.vertexBuffer;
        draws = other.draws;
        drawCounter = other.drawCounter;
        currentDepth = other.currentDepth;

        other.vertexBuffer = nullptr;
        other.draws = nullptr;
    }
    return *this;
}

void RenderBatch::Draw(Context* rlCtx)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    // Update batch vertex buffers
    //------------------------------------------------------------------------------------------------------------
    // NOTE: If there is not vertex data, buffers doesn't need to be updated (vertexCount > 0)
    // TODO: If no data changed on the CPU arrays --> No need to re-update GPU arrays (use a change detector flag?)
    if (rlCtx->State.vertexCounter > 0)
    {
        // Activate elements VAO
        if (GetExtensions().vao) glBindVertexArray(vertexBuffer[currentBuffer].vaoId);

        // Vertex positions buffer
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer[currentBuffer].vboId[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, rlCtx->State.vertexCounter*3*sizeof(float), vertexBuffer[currentBuffer].vertices);
        //glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*4*vertexBuffer[currentBuffer].elementCount, vertexBuffer[currentBuffer].vertices, GL_DYNAMIC_DRAW);  // Update all buffer

        // Texture coordinates buffer
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer[currentBuffer].vboId[1]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, rlCtx->State.vertexCounter*2*sizeof(float), vertexBuffer[currentBuffer].texcoords);
        //glBufferData(GL_ARRAY_BUFFER, sizeof(float)*2*4*vertexBuffer[currentBuffer].elementCount, vertexBuffer[currentBuffer].texcoords, GL_DYNAMIC_DRAW); // Update all buffer

        // Colors buffer
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer[currentBuffer].vboId[2]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, rlCtx->State.vertexCounter*4*sizeof(unsigned char), vertexBuffer[currentBuffer].colors);
        //glBufferData(GL_ARRAY_BUFFER, sizeof(float)*4*4*vertexBuffer[currentBuffer].elementCount, vertexBuffer[currentBuffer].colors, GL_DYNAMIC_DRAW);    // Update all buffer

        // NOTE: glMapBuffer() causes sync issue.
        // If GPU is working with this buffer, glMapBuffer() will wait(stall) until GPU to finish its job.
        // To avoid waiting (idle), you can call first glBufferData() with NULL pointer before glMapBuffer().
        // If you do that, the previous data in PBO will be discarded and glMapBuffer() returns a new
        // allocated pointer immediately even if GPU is still working with the previous data.

        // Another option: map the buffer object into client's memory
        // Probably this code could be moved somewhere else...
        // vertexBuffer[currentBuffer].vertices = (float *)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
        // if (vertexBuffer[currentBuffer].vertices)
        // {
            // Update vertex data
        // }
        // glUnmapBuffer(GL_ARRAY_BUFFER);

        // Unbind the current VAO
        if (GetExtensions().vao) glBindVertexArray(0);
    }
    //------------------------------------------------------------------------------------------------------------

    // Draw batch vertex buffers (considering VR stereo if required)
    //------------------------------------------------------------------------------------------------------------
    Matrix matProjection = rlCtx->GetMatrixProjection();
    Matrix matModelView = rlCtx->GetMatrixModelview();

    int eyeCount = 1;
    if (rlCtx->State.stereoRender) eyeCount = 2;

    for (int eye = 0; eye < eyeCount; eye++)
    {
        if (eyeCount == 2)
        {
            rlCtx->Viewport(eye*rlCtx->GetFramebufferWidth()/2, 0, rlCtx->GetFramebufferWidth()/2, rlCtx->GetFramebufferHeight());      // Setup current eye viewport (half screen width)
            rlCtx->SetMatrixModelview(matModelView * rlCtx->State.viewOffsetStereo[eye]);                                                         // Set current eye view offset to modelview matrix
            rlCtx->SetMatrixProjection(rlCtx->State.projectionStereo[eye]);                                                                       // Set current eye projection matrix
        }

        // Draw buffers
        if (rlCtx->State.vertexCounter > 0)
        {
            // Set current shader and upload current MVP matrix
            glUseProgram(rlCtx->State.currentShaderId);

            // Create modelview-projection matrix and upload to shader
            Matrix matMVP = rlCtx->State.modelview * rlCtx->State.projection;

            float matMVPfloat[16] = {
                matMVP.m0, matMVP.m1, matMVP.m2, matMVP.m3,
                matMVP.m4, matMVP.m5, matMVP.m6, matMVP.m7,
                matMVP.m8, matMVP.m9, matMVP.m10, matMVP.m11,
                matMVP.m12, matMVP.m13, matMVP.m14, matMVP.m15
            };

            glUniformMatrix4fv(rlCtx->State.currentShaderLocs[static_cast<int>(ShaderLocationIndex::MatrixMVP)], 1, false, matMVPfloat);

            if (GetExtensions().vao)
            {
                glBindVertexArray(vertexBuffer[currentBuffer].vaoId);
            }
            else
            {
                // Bind vertex attrib: position (shader-location = 0)
                glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer[currentBuffer].vboId[0]);
                glVertexAttribPointer(rlCtx->State.currentShaderLocs[static_cast<int>(ShaderLocationIndex::VertexPosition)], 3, GL_FLOAT, 0, 0, 0);
                glEnableVertexAttribArray(rlCtx->State.currentShaderLocs[static_cast<int>(ShaderLocationIndex::VertexPosition)]);

                // Bind vertex attrib: texcoord (shader-location = 1)
                glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer[currentBuffer].vboId[1]);
                glVertexAttribPointer(rlCtx->State.currentShaderLocs[static_cast<int>(ShaderLocationIndex::VertexTexCoord01)], 2, GL_FLOAT, 0, 0, 0);
                glEnableVertexAttribArray(rlCtx->State.currentShaderLocs[static_cast<int>(ShaderLocationIndex::VertexTexCoord01)]);

                // Bind vertex attrib: color (shader-location = 3)
                glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer[currentBuffer].vboId[2]);
                glVertexAttribPointer(rlCtx->State.currentShaderLocs[static_cast<int>(ShaderLocationIndex::VertexColor)], 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);
                glEnableVertexAttribArray(rlCtx->State.currentShaderLocs[static_cast<int>(ShaderLocationIndex::VertexColor)]);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexBuffer[currentBuffer].vboId[3]);
            }

            // Setup some default shader values
            glUniform4f(rlCtx->State.currentShaderLocs[static_cast<int>(ShaderLocationIndex::ColorDiffuse)], 1.0f, 1.0f, 1.0f, 1.0f);
            glUniform1i(rlCtx->State.currentShaderLocs[static_cast<int>(ShaderLocationIndex::MapDiffuse)], 0);  // Active default sampler2D: texture0

            // Activate additional sampler textures
            // Those additional textures will be common for all draw calls of the batch
            for (int i = 0; i < RL_DEFAULT_BATCH_MAX_TEXTURE_UNITS; i++)
            {
                if (rlCtx->State.activeTextureId[i] > 0)
                {
                    glActiveTexture(GL_TEXTURE0 + 1 + i);
                    glBindTexture(GL_TEXTURE_2D, rlCtx->State.activeTextureId[i]);
                }
            }

            // Activate default sampler2D texture0 (one texture is always active for default batch shader)
            // NOTE: Batch system accumulates calls by texture0 changes, additional textures are enabled for all the draw calls
            glActiveTexture(GL_TEXTURE0);

            for (int i = 0, vertexOffset = 0; i < drawCounter; i++)
            {
                // Bind current draw call texture, activated as GL_TEXTURE0 and Bound to sampler2D texture0 by default
                glBindTexture(GL_TEXTURE_2D, draws[i].textureId);

                if ((draws[i].mode == DrawMode::Lines) || (draws[i].mode == DrawMode::Triangles))
                {
                    glDrawArrays(static_cast<int>(draws[i].mode), vertexOffset, draws[i].vertexCount);
                }
                else
                {
#                   if defined(GRAPHICS_API_OPENGL_33)
                        // We need to define the number of indices to be processed: elementCount*6
                        // NOTE: The final parameter tells the GPU the offset in bytes from the
                        // start of the index buffer to the location of the first index to process
                        glDrawElements(GL_TRIANGLES, draws[i].vertexCount/4*6, GL_UNSIGNED_INT, (GLvoid *)(vertexOffset/4*6*sizeof(GLuint)));
#                   endif

#                   if defined(GRAPHICS_API_OPENGL_ES2)
                        glDrawElements(GL_TRIANGLES, draws[i].vertexCount/4*6, GL_UNSIGNED_SHORT, (GLvoid *)(vertexOffset/4*6*sizeof(GLushort)));
#                   endif
                }

                vertexOffset += (draws[i].vertexCount + draws[i].vertexAlignment);
            }

            if (!GetExtensions().vao)
            {
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            }

            glBindTexture(GL_TEXTURE_2D, 0);    // Unbind textures
        }

        if (GetExtensions().vao) glBindVertexArray(0); // Unbind VAO

        glUseProgram(0);    // Unbind shader program
    }

    // Restore viewport to default measures
    if (eyeCount == 2) rlCtx->Viewport(0, 0, rlCtx->State.framebufferWidth, rlCtx->State.framebufferHeight);
    //------------------------------------------------------------------------------------------------------------

    // Reset batch buffers
    //------------------------------------------------------------------------------------------------------------
    // Reset depth for next draw
    currentDepth = -1.0f;

    // Restore projection/modelview matrices
    rlCtx->SetMatrixProjection(matProjection);
    rlCtx->SetMatrixModelview(matModelView);

    // Reset rlCtx->currentBatch->draws array
    for (int i = 0; i < RL_DEFAULT_BATCH_DRAWCALLS; i++)
    {
        draws[i].mode = DrawMode::Quads;
        draws[i].vertexCount = 0;
        draws[i].textureId = rlCtx->GetTextureIdDefault();
    }

    // Reset draws counter to one draw for the batch
    drawCounter = 1;
    //------------------------------------------------------------------------------------------------------------

    // Change to next buffer in the list (in case of multi-buffering)
    currentBuffer++;
    if (currentBuffer >= bufferCount) currentBuffer = 0;
#endif
}
