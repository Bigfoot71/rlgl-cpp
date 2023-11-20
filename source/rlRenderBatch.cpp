#include "rlRenderBatch.hpp"
#include "rlException.hpp"
#include "rlEnums.hpp"
#include "rlGLExt.hpp"
#include "rlgl.hpp"

using namespace rlgl;

/* DRAW CALL IMPLEMENTATION */

void DrawCall::Render(int& vertexOffset)
{
    // Bind current draw call texture, activated as GL_TEXTURE0 and Bound to sampler2D texture0 by default
    glBindTexture(GL_TEXTURE_2D, textureId);

    if (mode == DrawMode::Lines || mode == DrawMode::Triangles)
    {
        glDrawArrays(static_cast<int>(mode), vertexOffset, vertexCount);
    }
    else
    {
#       if defined(GRAPHICS_API_OPENGL_33)
            // We need to define the number of indices to be processed: elementCount*6
            // NOTE: The final parameter tells the GPU the offset in bytes from the
            // start of the index buffer to the location of the first index to process
            glDrawElements(GL_TRIANGLES, vertexCount/4*6, GL_UNSIGNED_INT,
                reinterpret_cast<const void*>(vertexOffset/4*6*sizeof(GLuint)));
#       endif

#       if defined(GRAPHICS_API_OPENGL_ES2)
            glDrawElements(GL_TRIANGLES, draws[i].vertexCount/4*6, GL_UNSIGNED_SHORT,
                reinterpret_cast<const void*>(vertexOffset/4*6*sizeof(GLushort)));
#       endif
    }

    vertexOffset += (vertexCount + vertexAlignment);
}

/* RENDER BATCH IMPLEMENTATION */

RenderBatch::RenderBatch(const Context& rlCtx, int numBuffers, int bufferElements, int drawCallsLimit)
: bufferCount(numBuffers), drawQueueLimit(drawCallsLimit), currentDepth(-1.0f)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)

    const Context::State &rlState = rlCtx.GetState();

    // Initialize CPU (RAM) vertex buffers (position, texcoord, color data and indexes)
    // And upload to GPU (VRAM) vertex data and initialize VAOs/VBOs
    //--------------------------------------------------------------------------------------------
    vertexBuffer = new VertexBuffer[numBuffers];

    for (int i = 0; i < numBuffers; i++)
    {
        vertexBuffer[i] = VertexBuffer(rlState.currentShaderLocs, bufferElements);
    }

    TRACELOG(TraceLogLevel::Info, "RLGL: Vertex buffers loaded successfully in RAM (CPU) and VRAM (GPU).");

    // Unbind the current VAO
    if (GetExtensions().vao) glBindVertexArray(0);
    //--------------------------------------------------------------------------------------------

    // Initializes the first DrawCall in the draw call queue
    drawQueue.push(DrawCall(rlCtx.GetTextureIdDefault()));

#endif
}

RenderBatch::~RenderBatch()
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)

    // Unbind everything
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    delete[] vertexBuffer;

#endif
}

RenderBatch::RenderBatch(RenderBatch&& other) noexcept
: bufferCount(other.bufferCount)
, currentBuffer(other.currentBuffer)
, vertexBuffer(other.vertexBuffer)
, drawQueue(std::move(other.drawQueue))
, currentDepth(other.currentDepth)
{
    other.vertexBuffer = nullptr;
}

RenderBatch& RenderBatch::operator=(RenderBatch&& other) noexcept
{
    if (this != &other)
    {
        bufferCount = other.bufferCount;
        currentBuffer = other.currentBuffer;
        vertexBuffer = other.vertexBuffer;
        drawQueue = std::move(other.drawQueue);
        currentDepth = other.currentDepth;

        other.vertexBuffer = nullptr;
    }
    return *this;
}

void RenderBatch::Draw(Context& rlCtx)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)

    const VertexBuffer &curBuffer = vertexBuffer[currentBuffer];
    const Context::State &rlState = rlCtx.GetState();

    // Update batch vertex buffers
    //------------------------------------------------------------------------------------------------------------
    // NOTE: If there is not vertex data, buffers doesn't need to be updated (vertexCount > 0)
    // TODO: If no data changed on the CPU arrays --> No need to re-update GPU arrays (use a change detector flag?)
    if (rlState.vertexCounter > 0)
    {
        // Activate elements VAO
        if (GetExtensions().vao) glBindVertexArray(curBuffer.vaoId);

        // Vertex positions buffer
        glBindBuffer(GL_ARRAY_BUFFER, curBuffer.vboId[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, rlState.vertexCounter*3*sizeof(float), curBuffer.vertices);

        // Texture coordinates buffer
        glBindBuffer(GL_ARRAY_BUFFER, curBuffer.vboId[1]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, rlState.vertexCounter*2*sizeof(float), curBuffer.texcoords);

        // Colors buffer
        glBindBuffer(GL_ARRAY_BUFFER, curBuffer.vboId[2]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, rlState.vertexCounter*4*sizeof(unsigned char), curBuffer.colors);

        // Unbind the current VAO
        if (GetExtensions().vao) glBindVertexArray(0);
    }
    //------------------------------------------------------------------------------------------------------------

    // Draw batch vertex buffers (considering VR stereo if required)
    //------------------------------------------------------------------------------------------------------------
    Matrix matProjection = rlCtx.GetMatrixProjection();
    Matrix matModelView = rlCtx.GetMatrixModelview();

    int eyeCount = 1;
    if (rlState.stereoRender) eyeCount = 2;

    for (int eye = 0; eye < eyeCount; eye++)
    {
        if (eyeCount == 2)
        {
            rlCtx.Viewport(eye*rlCtx.GetFramebufferWidth()/2, 0, rlCtx.GetFramebufferWidth()/2, rlCtx.GetFramebufferHeight());      // Setup current eye viewport (half screen width)
            rlCtx.SetMatrixModelview(matModelView * rlState.viewOffsetStereo[eye]);                                                         // Set current eye view offset to modelview matrix
            rlCtx.SetMatrixProjection(rlState.projectionStereo[eye]);                                                                       // Set current eye projection matrix
        }

        // Draw buffers
        if (rlState.vertexCounter > 0)
        {
            // Set current shader and upload current MVP matrix
            glUseProgram(rlState.currentShaderId);

            // Create modelview-projection matrix and upload to shader
            glUniformMatrix4fv(rlState.currentShaderLocs[LocMatrixMVP], 1, false,
                (rlState.modelview * rlState.projection).m); // MVP

            if (GetExtensions().vao)
            {
                glBindVertexArray(curBuffer.vaoId);
            }
            else
            {
                // Bind vertex attrib: position (shader-location = 0)
                glBindBuffer(GL_ARRAY_BUFFER, curBuffer.vboId[0]);
                glVertexAttribPointer(rlState.currentShaderLocs[LocVertexPosition], 3, GL_FLOAT, 0, 0, 0);
                glEnableVertexAttribArray(rlState.currentShaderLocs[LocVertexPosition]);

                // Bind vertex attrib: texcoord (shader-location = 1)
                glBindBuffer(GL_ARRAY_BUFFER, curBuffer.vboId[1]);
                glVertexAttribPointer(rlState.currentShaderLocs[LocVertexTexCoord01], 2, GL_FLOAT, 0, 0, 0);
                glEnableVertexAttribArray(rlState.currentShaderLocs[LocVertexTexCoord01]);

                // Bind vertex attrib: color (shader-location = 3)
                glBindBuffer(GL_ARRAY_BUFFER, curBuffer.vboId[2]);
                glVertexAttribPointer(rlState.currentShaderLocs[LocVertexColor], 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);
                glEnableVertexAttribArray(rlState.currentShaderLocs[LocVertexColor]);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, curBuffer.vboId[3]);
            }

            // Setup some default shader values
            glUniform4f(rlState.currentShaderLocs[LocColorDiffuse], 1.0f, 1.0f, 1.0f, 1.0f);
            glUniform1i(rlState.currentShaderLocs[LocMapDiffuse], 0);  // Active default sampler2D: texture0

            // Activate additional sampler textures
            // Those additional textures will be common for all draw calls of the batch
            for (int i = 0; i < RL_DEFAULT_BATCH_MAX_TEXTURE_UNITS; i++)
            {
                if (rlState.activeTextureId[i] > 0)
                {
                    glActiveTexture(GL_TEXTURE0 + 1 + i);
                    glBindTexture(GL_TEXTURE_2D, rlState.activeTextureId[i]);
                }
            }

            // Activate default sampler2D texture0 (one texture is always active for default batch shader)
            // NOTE: Batch system accumulates calls by texture0 changes, additional textures are enabled for all the draw calls
            glActiveTexture(GL_TEXTURE0);

            for (int vertexOffset = 0; !drawQueue.empty();)
            {
                drawQueue.front().Render(vertexOffset);
                if (eye == eyeCount - 1) drawQueue.pop();
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
    if (eyeCount == 2)
    {
        rlCtx.Viewport(0, 0, rlState.framebufferWidth, rlState.framebufferHeight);
    }
    //------------------------------------------------------------------------------------------------------------

    // Reset depth for next draw
    currentDepth = -1.0f;

    // Restore projection/modelview matrices
    rlCtx.SetMatrixProjection(matProjection);
    rlCtx.SetMatrixModelview(matModelView);

    // If all drawCalls have been dequeued, we are resetting one
    if (drawQueue.size() == 0)
    {
        drawQueue.push(DrawCall(rlCtx.GetTextureIdDefault()));
    }

    // Change to next buffer in the list (in case of multi-buffering)
    if ((++currentBuffer) >= bufferCount) currentBuffer = 0;

#endif
}
