#ifndef RLGL_RENDER_BATCH_HPP
#define RLGL_RENDER_BATCH_HPP

#include "./rlEnums.hpp"
#include "./rlConfig.hpp"
#include "./rlVertexBuffer.hpp"

namespace rlgl {

    // Draw call type
    // NOTE: Only texture changes register a new draw, other state-change-related elements are not
    // used at this moment (vaoId, shaderId, matrices), raylib just forces a batch draw call if any
    // of those state-change happens (this is done in core module)

    struct DrawCall
    {
        DrawMode mode;              ///< Drawing mode: LINES, TRIANGLES, QUADS
        int vertexCount;            ///< Number of vertex of the draw
        int vertexAlignment;        ///< Number of vertex required for index alignment (LINES, TRIANGLES)
        //unsigned int vaoId;       ///< Vertex array id to be used on the draw -> Using RLGL.currentBatch->vertexBuffer.vaoId
        //unsigned int shaderId;    ///< Shader id to be used on the draw -> Using RLGL.currentShaderId
        uint32_t textureId;         ///< Texture id to be used on the draw -> Use to create new draw call if changes

        //Matrix projection;        ///< Projection matrix for this draw -> Using RLGL.projection by default
        //Matrix modelview;         ///< Modelview matrix for this draw -> Using RLGL.modelview by default
    };

    // Render batch management
    // NOTE: rlgl provides a default render batch to behave like OpenGL 1.1 immediate mode
    // but this render batch API is exposed in case of custom batches are required

    struct RenderBatch
    {
      private:
        friend class Context;

      public:
        RenderBatch(const class Context* rlCtx, int numBuffers, int bufferElements);
        ~RenderBatch();

        RenderBatch(const RenderBatch&) = delete;
        RenderBatch& operator=(const RenderBatch&) = delete;

        RenderBatch(RenderBatch&& other) noexcept;
        RenderBatch& operator=(RenderBatch&& other) noexcept;

        inline VertexBuffer* GetCurrentBuffer()
        {
            return &vertexBuffer[currentBuffer];
        }

        inline DrawCall* GetLastDrawCall()
        {
            return &draws[drawCounter - 1];
        }

      private:
        void Draw(class Context* rlCtx);

      private:
        int bufferCount;                ///< Number of vertex buffers (multi-buffering support)
        int currentBuffer;              ///< Current buffer tracking in case of multi-buffering
        VertexBuffer *vertexBuffer;     ///< Dynamic buffer(s) for vertex data

        DrawCall *draws;                ///< Draw calls array, depends on textureId
        int drawCounter;                ///< Draw calls counter
        float currentDepth;             ///< Current depth value for next draw
    };

}

#endif //RLGL_RENDER_BATCH_HPP
