#ifndef RLGL_RENDER_BATCH_HPP
#define RLGL_RENDER_BATCH_HPP

#include "./rlVertexBuffer.hpp"
#include "./rlConfig.hpp"
#include "./rlEnums.hpp"
#include "rlUtils.hpp"
#include <cstdint>
#include <queue>

namespace rlgl {

    // Draw call type
    // NOTE: Only texture changes register a new draw, other state-change-related elements are not
    // used at this moment (vaoId, shaderId, matrices), raylib just forces a batch draw call if any
    // of those state-change happens (this is done in core module)

    struct DrawCall
    {
        DrawMode mode               = DrawMode::Quads;      ///< Drawing mode: LINES, TRIANGLES, QUADS
        int vertexCount             = 0;                    ///< Number of vertex of the draw
        int vertexAlignment         = 0;                    ///< Number of vertex required for index alignment (LINES, TRIANGLES)
        //uint32_t vaoId;           = 0;                    ///< Vertex array id to be used on the draw -> Using RLGL.currentBatch->vertexBuffer.vaoId
        //uint32_t shaderId         = 0;                    ///< Shader id to be used on the draw -> Using RLGL.currentShaderId
        uint32_t textureId          = 0;                    ///< Texture id to be used on the draw -> Use to create new draw call if changes

        //Matrix projection         = Matrix::Identity;     ///< Projection matrix for this draw -> Using RLGL.projection by default
        //Matrix modelview          = Matrix::Identity;     ///< Modelview matrix for this draw -> Using RLGL.modelview by default

        DrawCall() = default;

        DrawCall(uint32_t _textureId)
            : textureId(_textureId) { }

        void Render(int& vertexOffset);
    };

    // Render batch management
    // NOTE: rlgl provides a default render batch to behave like OpenGL 1.1 immediate mode
    // but this render batch API is exposed in case of custom batches are required

    struct RenderBatch
    {
      public:
        RenderBatch(const class Context& rlCtx,
            int numBuffers = RL_DEFAULT_BATCH_BUFFERS,
            int bufferElements = RL_DEFAULT_BATCH_BUFFER_ELEMENTS,
            int drawCallsLimit = RL_DEFAULT_BATCH_DRAWCALLS);

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
            return &drawQueue.back();
        }

        // WARNING: Always called 'Context::CheckRenderBatchLimit()' before calling this function
        // NOTE: This problem should change in the future
        inline DrawCall* NewDrawCall(uint32_t defaultTextureId)
        {
            drawQueue.emplace(defaultTextureId);
            return &drawQueue.back();
        }

        // NOTE: Temporary function
        inline std::size_t GetDrawCallCounter() const
        {
            return drawQueue.size();
        }

        // NOTE: Temporary function
        inline int GetDrawCallLimit() const
        {
            return drawQueueLimit;
        }

        // NOTE: Temporary function
        inline float GetCurrentDepth() const
        {
            return currentDepth;
        }

        // NOTE: Temporary function
        inline void IncrementCurrentDepth(float depth)
        {
            currentDepth += depth;
        }

        void Draw(struct Context& rlCtx);

      private:
        int bufferCount;                ///< Number of vertex buffers (multi-buffering support)
        int currentBuffer;              ///< Current buffer tracking in case of multi-buffering
        VertexBuffer *vertexBuffer;     ///< Dynamic buffer(s) for vertex data

        std::queue<DrawCall> drawQueue; ///< Draw calls queue, depends on textureId
        int drawQueueLimit;             ///< Limit draw calls to the queue
        float currentDepth;             ///< Current depth value for next draw
    };

}

#endif //RLGL_RENDER_BATCH_HPP
