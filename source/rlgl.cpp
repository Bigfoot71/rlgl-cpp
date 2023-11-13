#include "rlgl.hpp"

#include "rlEnums.hpp"
#include "rlUtils.hpp"
#include "rlGLExt.hpp"
#include "rlException.hpp"
#include "rlRenderBatch.hpp"

#include <cmath>
#include <string>
#include <cstring>
#include <cstdint>
#include <algorithm>

using namespace rlgl;

Context::Context(int width, int height)
{
    // Enable OpenGL debug context if required
#   if defined(RLGL_ENABLE_OPENGL_DEBUG_CONTEXT) && defined(GRAPHICS_API_OPENGL_43)

        if ((glDebugMessageCallback != nullptr) && (glDebugMessageControl != nullptr))
        {
            glDebugMessageCallback(rlDebugMessageCallback, 0);
            // glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_ERROR, GL_DEBUG_SEVERITY_HIGH, 0, 0, GL_TRUE);

            // Debug context options:
            //  - GL_DEBUG_OUTPUT - Faster version but not useful for breakpoints
            //  - GL_DEBUG_OUTPUT_SYNCHRONUS - Callback is in sync with errors, so a breakpoint can be placed on the callback in order to get a stacktrace for the GL error
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        }

#   endif

#   if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)

        // Init default white texture
        uint8_t pixels[4] = { 255, 255, 255, 255 };   // 1 pixel RGBA (4 bytes)
        State.defaultTextureId = LoadTexture(pixels, 1, 1, PixelFormat::R8G8B8A8, 1);

        if (State.defaultTextureId != 0)
        {
            TRACELOG(TraceLogLevel::Info, "TEXTURE: [ID %i] Default texture loaded successfully", State.defaultTextureId);
        }
        else
        {
            TRACELOG(TraceLogLevel::Warning, "TEXTURE: Failed to load default texture");
        }

        // Init default Shader (customized for GL 3.3 and ES2)
        // Loaded: State.defaultShaderId + State.defaultShaderLocs
        LoadShaderDefault();
        State.currentShaderId = State.defaultShaderId;
        State.currentShaderLocs = State.defaultShaderLocs;

        // Init default vertex arrays buffers
        defaultBatch = std::make_unique<RenderBatch>(this, RL_DEFAULT_BATCH_BUFFERS, RL_DEFAULT_BATCH_BUFFER_ELEMENTS);
        currentBatch = defaultBatch.get();

        // Init vertexCounter
        State.vertexCounter = 0;

        // Init stack matrices (emulating OpenGL 1.1)
        for (int i = 0; i < RL_MAX_MATRIX_STACK_SIZE; i++)
        {
            State.stack[i] = Matrix::Identity();
        }

        // Init internal matrices
        State.transform = Matrix::Identity();
        State.projection = Matrix::Identity();
        State.modelview = Matrix::Identity();
        State.currentMatrix = &State.modelview;

#   endif  // GRAPHICS_API_OPENGL_33 || GRAPHICS_API_OPENGL_ES2

    // Initialize OpenGL default states
    //----------------------------------------------------------
    // Init state: Depth test
    glDepthFunc(GL_LEQUAL);                                 // Type of depth testing to apply
    glDisable(GL_DEPTH_TEST);                               // Disable depth testing for 2D (only used for 3D)

    // Init state: Blending mode
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);      // Color blending function (how colors are mixed)
    glEnable(GL_BLEND);                                     // Enable color blending (required to work with transparencies)

    // Init state: Culling
    // NOTE: All shapes/models triangles are drawn CCW
    glCullFace(GL_BACK);                                    // Cull the back face (default)
    glFrontFace(GL_CCW);                                    // Front face are defined counter clockwise (default)
    glEnable(GL_CULL_FACE);                                 // Enable backface culling

    // Init state: Cubemap seamless
#   if defined(GRAPHICS_API_OPENGL_33)
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);             // Seamless cubemaps (not supported on OpenGL ES 2.0)
#   endif

#   if defined(GRAPHICS_API_OPENGL_11)
        // Init state: Color hints (deprecated in OpenGL 3.0+)
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);      // Improve quality of color and texture coordinate interpolation
        glShadeModel(GL_SMOOTH);                                // Smooth shading between vertex (vertex colors interpolation)
#   endif

#   if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
        // Store screen size into global variables
        State.framebufferWidth = width;
        State.framebufferHeight = height;

        TRACELOG(TraceLogLevel::Info, "RLGL: Default OpenGL state initialized successfully");
        //----------------------------------------------------------
#   endif

    // Init state: Color/Depth buffers clear
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);                   // Set clear color (black)
    glClearDepth(1.0f);                                     // Set clear depth value (default)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);     // Clear color and depth buffers (depth buffer required for 3D)
}

Context::~Context()
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)

    UnloadShaderDefault();                          // Unload default shader
    glDeleteTextures(1, &State.defaultTextureId);   // Unload default texture

    TRACELOG(TraceLogLevel::Info, "TEXTURE: [ID %i] Default texture unloaded successfully", State.defaultTextureId);

#endif
}

#if defined(GRAPHICS_API_OPENGL_11)
// Fallback to OpenGL 1.1 function calls
//---------------------------------------

void Context::MatrixMode(enum MatrixMode mode)
{
    switch (mode)
    {
        case MatrixMode::Projection: glMatrixMode(GL_PROJECTION);   break;
        case MatrixMode::ModelView:  glMatrixMode(GL_MODELVIEW);    break;
        case MatrixMode::Texture:    glMatrixMode(GL_TEXTURE);      break;
        default:                                                    break;
    }
}

void Context::Frustum(double left, double right, double bottom, double top, double znear, double zfar)
{
    glFrustum(left, right, bottom, top, znear, zfar);
}

void Context::Ortho(double left, double right, double bottom, double top, double znear, double zfar)
{
    glOrtho(left, right, bottom, top, znear, zfar);
}

void Context::PushMatrix()
{
    glPushMatrix();
}

void Context::PopMatrix()
{
    glPopMatrix();
}

void Context::LoadIdentity()
{
    glLoadIdentity();
}

void Context::Translate(float x, float y, float z)
{
    glTranslatef(x, y, z);
}

void Context::Rotate(float angle, float x, float y, float z)
{
    glRotatef(angle, x, y, z);
}

void Context::Scale(float x, float y, float z)
{
    glScalef(x, y, z);
}

void Context::MultMatrix(const float *matf)
{
    glMultMatrixf(matf);
}

#endif

#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)

// Choose the current matrix to be transformed
void Context::MatrixMode(enum MatrixMode mode)
{
    switch (mode)
    {
        case MatrixMode::Projection:
            State.currentMatrix = &State.projection;
            break;

        case MatrixMode::ModelView:
            State.currentMatrix = &State.modelview;
            break;

        case MatrixMode::Texture:
            // Not supported
            break;
    }

    State.currentMatrixMode = mode;
}

// Push the current matrix into State.stack
void Context::PushMatrix()
{
    if (State.stackCounter >= RL_MAX_MATRIX_STACK_SIZE)
    {
        TRACELOG(TraceLogLevel::Error, "RLGL: Matrix stack overflow (RL_MAX_MATRIX_STACK_SIZE)");
    }

    if (State.currentMatrixMode == MatrixMode::ModelView)
    {
        State.transformRequired = true;
        State.currentMatrix = &State.transform;
    }

    State.stack[State.stackCounter] = *State.currentMatrix;
    State.stackCounter++;
}

// Pop lattest inserted matrix from State.stack
void Context::PopMatrix()
{
    if (State.stackCounter > 0)
    {
        Matrix mat = State.stack[State.stackCounter - 1];
        *State.currentMatrix = mat;
        State.stackCounter--;
    }

    if ((State.stackCounter == 0) && (State.currentMatrixMode == MatrixMode::ModelView))
    {
        State.currentMatrix = &State.modelview;
        State.transformRequired = false;
    }
}

// Reset current matrix to identity matrix
void Context::LoadIdentity()
{
    *State.currentMatrix = Matrix::Identity();
}

// Multiply the current matrix by a translation matrix
void Context::Translate(float x, float y, float z)
{
    Matrix matTranslation = {
        1.0f, 0.0f, 0.0f, x,
        0.0f, 1.0f, 0.0f, y,
        0.0f, 0.0f, 1.0f, z,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    // NOTE: We transpose matrix with multiplication order
    *State.currentMatrix = matTranslation * (*State.currentMatrix);
}

// Multiply the current matrix by a rotation matrix
// NOTE: The provided angle must be in degrees
void Context::Rotate(float angle, float x, float y, float z)
{
    Matrix matRotation = Matrix::Identity();

    // Axis vector (x, y, z) normalization
    float lengthSquared = x*x + y*y + z*z;
    if ((lengthSquared != 1.0f) && (lengthSquared != 0.0f))
    {
        float inverseLength = 1.0f/std::sqrt(lengthSquared);
        x *= inverseLength;
        y *= inverseLength;
        z *= inverseLength;
    }

    // Rotation matrix generation
    float sinres = std::sin(DEG2RAD*angle);
    float cosres = std::cos(DEG2RAD*angle);
    float t = 1.0f - cosres;

    matRotation.m0 = x*x*t + cosres;
    matRotation.m1 = y*x*t + z*sinres;
    matRotation.m2 = z*x*t - y*sinres;
    matRotation.m3 = 0.0f;

    matRotation.m4 = x*y*t - z*sinres;
    matRotation.m5 = y*y*t + cosres;
    matRotation.m6 = z*y*t + x*sinres;
    matRotation.m7 = 0.0f;

    matRotation.m8 = x*z*t + y*sinres;
    matRotation.m9 = y*z*t - x*sinres;
    matRotation.m10 = z*z*t + cosres;
    matRotation.m11 = 0.0f;

    matRotation.m12 = 0.0f;
    matRotation.m13 = 0.0f;
    matRotation.m14 = 0.0f;
    matRotation.m15 = 1.0f;

    // NOTE: We transpose matrix with multiplication order
    *State.currentMatrix = matRotation * (*State.currentMatrix);
}

// Multiply the current matrix by a scaling matrix
void Context::Scale(float x, float y, float z)
{
    Matrix matScale = {
        x, 0.0f, 0.0f, 0.0f,
        0.0f, y, 0.0f, 0.0f,
        0.0f, 0.0f, z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    // NOTE: We transpose matrix with multiplication order
    *State.currentMatrix = matScale * (*State.currentMatrix);
}

// Multiply the current matrix by another matrix
void Context::MultMatrix(const float *matf)
{
    // Matrix creation from array
    Matrix mat = { matf[0], matf[4], matf[8], matf[12],
                   matf[1], matf[5], matf[9], matf[13],
                   matf[2], matf[6], matf[10], matf[14],
                   matf[3], matf[7], matf[11], matf[15] };

    *State.currentMatrix = (*State.currentMatrix) * mat;
}

// Multiply the current matrix by a perspective matrix generated by parameters
void Context::Frustum(double left, double right, double bottom, double top, double znear, double zfar)
{
    Matrix matFrustum;

    float rl = static_cast<float>(right - left);
    float tb = static_cast<float>(top - bottom);
    float fn = static_cast<float>(zfar - znear);

    matFrustum.m0   = (znear*2.0f)/rl;
    matFrustum.m1   = 0.0f;
    matFrustum.m2   = 0.0f;
    matFrustum.m3   = 0.0f;

    matFrustum.m4   = 0.0f;
    matFrustum.m5   = (znear*2.0f)/tb;
    matFrustum.m6   = 0.0f;
    matFrustum.m7   = 0.0f;

    matFrustum.m8   = (right + left)/rl;
    matFrustum.m9   = (top + bottom)/tb;
    matFrustum.m10  = -(zfar + znear)/fn;
    matFrustum.m11  = -1.0f;

    matFrustum.m12  = 0.0f;
    matFrustum.m13  = 0.0f;
    matFrustum.m14  = -(zfar*znear*2.0f)/fn;
    matFrustum.m15  = 0.0f;

    *State.currentMatrix = (*State.currentMatrix) * matFrustum;
}

// Multiply the current matrix by an orthographic matrix generated by parameters
void Context::Ortho(double left, double right, double bottom, double top, double znear, double zfar)
{
    // NOTE: If left-right and top-botton values are equal it could create a division by zero,
    // response to it is platform/compiler dependant
    Matrix matOrtho;

    float rl = static_cast<float>(right - left);
    float tb = static_cast<float>(top - bottom);
    float fn = static_cast<float>(zfar - znear);

    matOrtho.m0 = 2.0f/rl;
    matOrtho.m1 = 0.0f;
    matOrtho.m2 = 0.0f;
    matOrtho.m3 = 0.0f;
    matOrtho.m4 = 0.0f;
    matOrtho.m5 = 2.0f/tb;
    matOrtho.m6 = 0.0f;
    matOrtho.m7 = 0.0f;
    matOrtho.m8 = 0.0f;
    matOrtho.m9 = 0.0f;
    matOrtho.m10 = -2.0f/fn;
    matOrtho.m11 = 0.0f;
    matOrtho.m12 = -(left + right)/rl;
    matOrtho.m13 = -(top + bottom)/tb;
    matOrtho.m14 = -(zfar + znear)/fn;
    matOrtho.m15 = 1.0f;

    *State.currentMatrix = (*State.currentMatrix) * matOrtho;
}

#endif

// Set the viewport area (transformation from normalized device coordinates to window coordinates)
// NOTE: We store current viewport dimensions
void Context::Viewport(int x, int y, int width, int height)
{
    glViewport(x, y, width, height);
}

//----------------------------------------------------------------------------------
// Module Functions Definition - Vertex level operations
//----------------------------------------------------------------------------------

#if defined(GRAPHICS_API_OPENGL_11)
// Fallback to OpenGL 1.1 function calls
//---------------------------------------

void Context::Begin(DrawMode mode)
{
    switch (mode)
    {
        case DrawMode::Lines:       glBegin(GL_LINES);          break;
        case DrawMode::Triangles:   glBegin(GL_TRIANGLES);      break;
        case DrawMode::Quads:       glBegin(GL_QUADS);          break;
        default:                                                break;
    }
}

void Context::End()
{
    glEnd();
}

void Context::Vertex(int x, int y)
{
    glVertex2i(x, y);
}

void Context::Vertex(float x, float y)
{
    glVertex2f(x, y);
}

void Context::Vertex(float x, float y, float z)
{
    glVertex3f(x, y, z);
}

void Context::TexCoord(float x, float y)
{
    glTexCoord2f(x, y);
}

void Context::Normal(float x, float y, float z)
{
    glNormal3f(x, y, z);
}

void Context::Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    glColor4ub(r, g, b, a);
}

void Context::Color(float x, float y, float z)
{
    glColor3f(x, y, z);
}

void Context::Color(float x, float y, float z, float w)
{
    glColor4f(x, y, z, w);
}

#endif

#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)

// Initialize drawing mode (how to organize vertex)
void Context::Begin(DrawMode mode)
{
    // Draw mode can be RL_LINES, RL_TRIANGLES and RL_QUADS
    // NOTE: In all three cases, vertex are accumulated over default internal vertex buffer
    if (currentBatch->draws[currentBatch->drawCounter - 1].mode != mode)
    {
        if (currentBatch->draws[currentBatch->drawCounter - 1].vertexCount > 0)
        {
            // Make sure current currentBatch->draws[i].vertexCount is aligned a multiple of 4,
            // that way, following QUADS drawing will keep aligned with index processing
            // It implies adding some extra alignment vertex at the end of the draw,
            // those vertex are not processed but they are considered as an additional offset
            // for the next set of vertex to be drawn
            if (currentBatch->draws[currentBatch->drawCounter - 1].mode == DrawMode::Lines)
            {
                currentBatch->draws[currentBatch->drawCounter - 1].vertexAlignment = ((currentBatch->draws[currentBatch->drawCounter - 1].vertexCount < 4)
                    ? currentBatch->draws[currentBatch->drawCounter - 1].vertexCount : currentBatch->draws[currentBatch->drawCounter - 1].vertexCount%4);
            }
            else if (currentBatch->draws[currentBatch->drawCounter - 1].mode == DrawMode::Triangles)
            {
                currentBatch->draws[currentBatch->drawCounter - 1].vertexAlignment = ((currentBatch->draws[currentBatch->drawCounter - 1].vertexCount < 4)
                    ? 1 : (4 - (currentBatch->draws[currentBatch->drawCounter - 1].vertexCount%4)));
            }
            else
            {
                currentBatch->draws[currentBatch->drawCounter - 1].vertexAlignment = 0;
            }

            if (!CheckRenderBatchLimit(currentBatch->draws[currentBatch->drawCounter - 1].vertexAlignment))
            {
                State.vertexCounter += currentBatch->draws[currentBatch->drawCounter - 1].vertexAlignment;
                currentBatch->drawCounter++;
            }
        }

        if (currentBatch->drawCounter >= RL_DEFAULT_BATCH_DRAWCALLS) DrawRenderBatch(currentBatch);

        currentBatch->draws[currentBatch->drawCounter - 1].mode = mode;
        currentBatch->draws[currentBatch->drawCounter - 1].vertexCount = 0;
        currentBatch->draws[currentBatch->drawCounter - 1].textureId = State.defaultTextureId;
    }
}

// Finish vertex providing
void Context::End()
{
    // NOTE: Depth increment is dependant on Context::Ortho(): z-near and z-far values,
    // as well as depth buffer bit-depth (16bit or 24bit or 32bit)
    // Correct increment formula would be: depthInc = (zfar - znear)/pow(2, bits)
    currentBatch->currentDepth += (1.0f/20000.0f);
}

// Define one vertex (position)
// NOTE: Vertex position data is the basic information required for drawing
void Context::Vertex(float x, float y, float z)
{
    float tx = x;
    float ty = y;
    float tz = z;

    // Transform provided vector if required
    if (State.transformRequired)
    {
        tx = State.transform.m0*x + State.transform.m4*y + State.transform.m8*z + State.transform.m12;
        ty = State.transform.m1*x + State.transform.m5*y + State.transform.m9*z + State.transform.m13;
        tz = State.transform.m2*x + State.transform.m6*y + State.transform.m10*z + State.transform.m14;
    }

    // WARNING: We can't break primitives when launching a new batch.
    // RL_LINES comes in pairs, RL_TRIANGLES come in groups of 3 vertices and RL_QUADS come in groups of 4 vertices.
    // We must check current draw.mode when a new vertex is required and finish the batch only if the draw.mode draw.vertexCount is %2, %3 or %4
    if (State.vertexCounter > (currentBatch->vertexBuffer[currentBatch->currentBuffer].elementCount*4 - 4))
    {
        if ((currentBatch->draws[currentBatch->drawCounter - 1].mode == DrawMode::Lines) &&
            (currentBatch->draws[currentBatch->drawCounter - 1].vertexCount%2 == 0))
        {
            // Reached the maximum number of vertices for RL_LINES drawing
            // Launch a draw call but keep current state for next vertices comming
            // NOTE: We add +1 vertex to the check for security
            CheckRenderBatchLimit(2 + 1);
        }
        else if ((currentBatch->draws[currentBatch->drawCounter - 1].mode == DrawMode::Triangles) &&
            (currentBatch->draws[currentBatch->drawCounter - 1].vertexCount%3 == 0))
        {
            CheckRenderBatchLimit(3 + 1);
        }
        else if ((currentBatch->draws[currentBatch->drawCounter - 1].mode == DrawMode::Quads) &&
            (currentBatch->draws[currentBatch->drawCounter - 1].vertexCount%4 == 0))
        {
            CheckRenderBatchLimit(4 + 1);
        }
    }

    // Add vertices
    currentBatch->vertexBuffer[currentBatch->currentBuffer].vertices[3*State.vertexCounter] = tx;
    currentBatch->vertexBuffer[currentBatch->currentBuffer].vertices[3*State.vertexCounter + 1] = ty;
    currentBatch->vertexBuffer[currentBatch->currentBuffer].vertices[3*State.vertexCounter + 2] = tz;

    // Add current texcoord
    currentBatch->vertexBuffer[currentBatch->currentBuffer].texcoords[2*State.vertexCounter] = State.texcoordx;
    currentBatch->vertexBuffer[currentBatch->currentBuffer].texcoords[2*State.vertexCounter + 1] = State.texcoordy;

    // WARNING: By default rlVertexBuffer struct does not store normals

    // Add current color
    currentBatch->vertexBuffer[currentBatch->currentBuffer].colors[4*State.vertexCounter] = State.colorr;
    currentBatch->vertexBuffer[currentBatch->currentBuffer].colors[4*State.vertexCounter + 1] = State.colorg;
    currentBatch->vertexBuffer[currentBatch->currentBuffer].colors[4*State.vertexCounter + 2] = State.colorb;
    currentBatch->vertexBuffer[currentBatch->currentBuffer].colors[4*State.vertexCounter + 3] = State.colora;

    State.vertexCounter++;
    currentBatch->draws[currentBatch->drawCounter - 1].vertexCount++;
}

// Define one vertex (position)
void Context::Vertex(float x, float y)
{
    Vertex(x, y, currentBatch->currentDepth);
}

// Define one vertex (position)
void Context::Vertex(int x, int y)
{
    Vertex(static_cast<float>(x), static_cast<float>(y), currentBatch->currentDepth);
}

// Define one vertex (texture coordinate)
// NOTE: Texture coordinates are limited to QUADS only
void Context::TexCoord(float x, float y)
{
    State.texcoordx = x;
    State.texcoordy = y;
}

// Define one vertex (normal)
// NOTE: Normals limited to TRIANGLES only?
void Context::Normal(float x, float y, float z)
{
    State.normalx = x;
    State.normaly = y;
    State.normalz = z;
}

// Define one vertex (color)
void Context::Color(uint8_t x, uint8_t y, uint8_t z, uint8_t w)
{
    State.colorr = x;
    State.colorg = y;
    State.colorb = z;
    State.colora = w;
}

// Define one vertex (color)
void Context::Color(float r, float g, float b, float a)
{
    Color(static_cast<uint8_t>(r*255),
          static_cast<uint8_t>(g*255),
          static_cast<uint8_t>(b*255),
          static_cast<uint8_t>(a*255));
}

// Define one vertex (color)
void Context::Color(float x, float y, float z)
{
    Color(static_cast<uint8_t>(x*255),
          static_cast<uint8_t>(y*255),
          static_cast<uint8_t>(z*255),
          255);
}

#endif

//--------------------------------------------------------------------------------------
// Module Functions Definition - OpenGL style functions (common to 1.1, 3.3+, ES2)
//--------------------------------------------------------------------------------------

// Set current texture to use
void Context::SetTexture(uint32_t id)
{
    if (id == 0)
    {
#       if defined(GRAPHICS_API_OPENGL_11)
            DisableTexture();
#       else
            // NOTE: If quads batch limit is reached, we force a draw call and next batch starts
            if (State.vertexCounter >= currentBatch->vertexBuffer[currentBatch->currentBuffer].elementCount*4)
            {
                DrawRenderBatch(currentBatch);
            }
#       endif
    }
    else
    {
#       if defined(GRAPHICS_API_OPENGL_11)
            EnableTexture(id);
#       else
            if (currentBatch->draws[currentBatch->drawCounter - 1].textureId != id)
            {
                if (currentBatch->draws[currentBatch->drawCounter - 1].vertexCount > 0)
                {
                    // Make sure current currentBatch->draws[i].vertexCount is aligned a multiple of 4,
                    // that way, following QUADS drawing will keep aligned with index processing
                    // It implies adding some extra alignment vertex at the end of the draw,
                    // those vertex are not processed but they are considered as an additional offset
                    // for the next set of vertex to be drawn
                    if (currentBatch->draws[currentBatch->drawCounter - 1].mode == DrawMode::Lines)
                    {
                        currentBatch->draws[currentBatch->drawCounter - 1].vertexAlignment = ((currentBatch->draws[currentBatch->drawCounter - 1].vertexCount < 4)
                            ? currentBatch->draws[currentBatch->drawCounter - 1].vertexCount : currentBatch->draws[currentBatch->drawCounter - 1].vertexCount%4);
                    }
                    else if (currentBatch->draws[currentBatch->drawCounter - 1].mode == DrawMode::Triangles)
                    {
                        currentBatch->draws[currentBatch->drawCounter - 1].vertexAlignment = ((currentBatch->draws[currentBatch->drawCounter - 1].vertexCount < 4)
                            ? 1 : (4 - (currentBatch->draws[currentBatch->drawCounter - 1].vertexCount%4)));
                    }
                    else
                    {
                        currentBatch->draws[currentBatch->drawCounter - 1].vertexAlignment = 0;
                    }

                    if (!CheckRenderBatchLimit(currentBatch->draws[currentBatch->drawCounter - 1].vertexAlignment))
                    {
                        State.vertexCounter += currentBatch->draws[currentBatch->drawCounter - 1].vertexAlignment;
                        currentBatch->drawCounter++;
                    }
                }

                if (currentBatch->drawCounter >= RL_DEFAULT_BATCH_DRAWCALLS)
                {
                    DrawRenderBatch(currentBatch);
                }

                currentBatch->draws[currentBatch->drawCounter - 1].textureId = id;
                currentBatch->draws[currentBatch->drawCounter - 1].vertexCount = 0;
            }
#       endif
    }
}

// Select and active a texture slot
void Context::ActiveTextureSlot(int slot)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    glActiveTexture(GL_TEXTURE0 + slot);
#endif
}

// Enable texture
void Context::EnableTexture(uint32_t id)
{
#   if defined(GRAPHICS_API_OPENGL_11)
        glEnable(GL_TEXTURE_2D);
#   endif

    glBindTexture(GL_TEXTURE_2D, id);
}

// Disable texture
void Context::DisableTexture()
{
#   if defined(GRAPHICS_API_OPENGL_11)
        glDisable(GL_TEXTURE_2D);
#   endif

    glBindTexture(GL_TEXTURE_2D, 0);
}

// Enable texture cubemap
void Context::EnableTextureCubemap(uint32_t id)
{
#   if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
        glBindTexture(GL_TEXTURE_CUBE_MAP, id);
#   endif
}

// Disable texture cubemap
void Context::DisableTextureCubemap()
{
#   if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
#   endif
}

// Set texture parameters (wrap mode)
void Context::TextureParameters(uint32_t id, TextureParam param, TextureWrap wrap)
{
    if (param == TextureParam::Wrap_S || param == TextureParam::Wrap_T)
    {
        glBindTexture(GL_TEXTURE_2D, id);

            if (wrap == TextureWrap::MirrorClamp)
            {
#               if !defined(GRAPHICS_API_OPENGL_11)
                    if (GetExtensions().texMirrorClamp) glTexParameteri(GL_TEXTURE_2D, static_cast<int>(param), static_cast<int>(wrap));
                    else TRACELOG(TraceLogLevel::Warning, "GL: Clamp mirror wrap mode not supported (GL_MIRROR_CLAMP_EXT)");
#               endif
            }
            else
            {
                glTexParameteri(GL_TEXTURE_2D, static_cast<int>(param), static_cast<int>(wrap));
            }

        glBindTexture(GL_TEXTURE_2D, 0);
    }
    else TRACELOG(TraceLogLevel::Warning, "Invalid texture parameter given to 'TextureParameters(uint32_t, TextureParam, TextureWrap)'");
}

// Set texture parameters (filter mode)
void Context::TextureParameters(uint32_t id, TextureParam param, TextureFilter filter)
{
    if (param == TextureParam::MagFilter || param == TextureParam::MinFilter)
    {
        glBindTexture(GL_TEXTURE_2D, id);
            glTexParameteri(GL_TEXTURE_2D, static_cast<int>(param), static_cast<int>(filter));
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    else TRACELOG(TraceLogLevel::Warning, "Invalid texture parameter given to 'TextureParameters(uint32_t, TextureParam, TextureFilter)'");
}

// Set texture parameters
void Context::TextureParameters(uint32_t id, TextureParam param, float value)
{
    if (param == TextureParam::Anisotropy)
    {
#       if !defined(GRAPHICS_API_OPENGL_11)

            glBindTexture(GL_TEXTURE_2D, id);

                if (value <= GetExtensions().maxAnisotropyLevel)
                {
                    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, static_cast<float>(value));
                }
                else if (GetExtensions().maxAnisotropyLevel > 0.0f)
                {
                    TRACELOG(TraceLogLevel::Warning, "GL: Maximum anisotropic filter level supported is %iX", id, static_cast<int>(GetExtensions().maxAnisotropyLevel));
                    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, static_cast<float>(value));
                }
                else
                {
                    TRACELOG(TraceLogLevel::Warning, "GL: Anisotropic filtering not supported");
                }

            glBindTexture(GL_TEXTURE_2D, 0);

#       endif
    }
    else if (param == TextureParam::MipmapBiasRatio)
    {
#       if defined(GRAPHICS_API_OPENGL_33)
            glBindTexture(GL_TEXTURE_2D, id);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, value);
            glBindTexture(GL_TEXTURE_2D, 0);
#       endif
    }
    else TRACELOG(TraceLogLevel::Warning, "Invalid texture parameter given to 'TextureParameters(uint32_t, TextureParam, float)'");
}

// Set cubemap parameters (wrap mode)
void Context::CubemapParameters(uint32_t id, TextureParam param, TextureWrap wrap)
{
#if !defined(GRAPHICS_API_OPENGL_11)

    if (param == TextureParam::Wrap_S || param == TextureParam::Wrap_T)
    {
        glBindTexture(GL_TEXTURE_CUBE_MAP, id);

            if (wrap == TextureWrap::MirrorClamp)
            {
#               if !defined(GRAPHICS_API_OPENGL_11)
                    if (GetExtensions().texMirrorClamp) glTexParameteri(GL_TEXTURE_CUBE_MAP, static_cast<int>(param), static_cast<int>(wrap));
                    else TRACELOG(TraceLogLevel::Warning, "GL: Clamp mirror wrap mode not supported (GL_MIRROR_CLAMP_EXT)");
#               endif
            }
            else
            {
                glTexParameteri(GL_TEXTURE_CUBE_MAP, static_cast<int>(param), static_cast<int>(wrap));
            }

        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }
    else TRACELOG(TraceLogLevel::Warning, "Invalid texture parameter given to 'TextureParameters(uint32_t, TextureParam, TextureWrap)'");

#endif
}

// Set cubemap parameters (filter mode)
void Context::CubemapParameters(uint32_t id, TextureParam param, TextureFilter filter)
{
#if !defined(GRAPHICS_API_OPENGL_11)

    if (param == TextureParam::MagFilter || param == TextureParam::MinFilter)
    {
        glBindTexture(GL_TEXTURE_CUBE_MAP, id);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, static_cast<int>(param), static_cast<int>(filter));
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }
    else TRACELOG(TraceLogLevel::Warning, "Invalid texture parameter given to 'TextureParameters(uint32_t, TextureParam, TextureFilter)'");

#endif
}

// Set cubemap parameters
void Context::CubemapParameters(uint32_t id, TextureParam param, float value)
{
#if !defined(GRAPHICS_API_OPENGL_11)

    if (param == TextureParam::Anisotropy)
    {
        glBindTexture(GL_TEXTURE_CUBE_MAP, id);

            if (value <= GetExtensions().maxAnisotropyLevel)
            {
                glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, static_cast<float>(value));
            }
            else if (GetExtensions().maxAnisotropyLevel > 0.0f)
            {
                TRACELOG(TraceLogLevel::Warning, "GL: Maximum anisotropic filter level supported is %iX", id, static_cast<int>(GetExtensions().maxAnisotropyLevel));
                glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, static_cast<float>(value));
            }
            else
            {
                TRACELOG(TraceLogLevel::Warning, "GL: Anisotropic filtering not supported");
            }

        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }
    else if (param == TextureParam::MipmapBiasRatio)
    {
#       if defined(GRAPHICS_API_OPENGL_33)
            glBindTexture(GL_TEXTURE_CUBE_MAP, id);
                glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_LOD_BIAS, value);
            glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
#       endif
    }
    else TRACELOG(TraceLogLevel::Warning, "Invalid texture parameter given to 'TextureParameters(uint32_t, TextureParam, float)'");

#endif
}

// Enable shader program
void Context::EnableShader(uint32_t id)
{
#if (defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2))
    glUseProgram(id);
#endif
}

// Disable shader program
void Context::DisableShader()
{
#if (defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2))
    glUseProgram(0);
#endif
}

// Enable rendering to texture (fbo)
void Context::EnableFramebuffer(uint32_t id)
{
#if (defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)) && defined(RLGL_RENDER_TEXTURES_HINT)
    glBindFramebuffer(GL_FRAMEBUFFER, id);
#endif
}

// Disable rendering to texture
void Context::DisableFramebuffer()
{
#if (defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)) && defined(RLGL_RENDER_TEXTURES_HINT)
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
}

// Blit active framebuffer to main framebuffer
void Context::BlitFramebuffer(int srcX, int srcY, int srcWidth, int srcHeight, int dstX, int dstY, int dstWidth, int dstHeight, int bufferMask)
{
#if (defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES3)) && defined(RLGL_RENDER_TEXTURES_HINT)
    glBlitFramebuffer(srcX, srcY, srcWidth, srcHeight, dstX, dstY, dstWidth, dstHeight, bufferMask, GL_NEAREST);
#endif
}

// Activate multiple draw color buffers
// NOTE: One color buffer is always active by default
void Context::ActiveDrawBuffers(int count)
{
#if ((defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES3)) && defined(RLGL_RENDER_TEXTURES_HINT))

    // NOTE: Maximum number of draw buffers supported is implementation dependant,
    // it can be queried with glGet*() but it must be at least 8
    //GLint maxDrawBuffers = 0;
    //glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);

    if (count > 0)
    {
        if (count > 8)
        {
            TRACELOG(TraceLogLevel::Warning, "GL: Max color buffers limited to 8");
        }
        else
        {
            uint32_t buffers[8] = {
#           if defined(GRAPHICS_API_OPENGL_ES3)
                GL_COLOR_ATTACHMENT0_EXT,
                GL_COLOR_ATTACHMENT1_EXT,
                GL_COLOR_ATTACHMENT2_EXT,
                GL_COLOR_ATTACHMENT3_EXT,
                GL_COLOR_ATTACHMENT4_EXT,
                GL_COLOR_ATTACHMENT5_EXT,
                GL_COLOR_ATTACHMENT6_EXT,
                GL_COLOR_ATTACHMENT7_EXT,
#           else
                GL_COLOR_ATTACHMENT0,
                GL_COLOR_ATTACHMENT1,
                GL_COLOR_ATTACHMENT2,
                GL_COLOR_ATTACHMENT3,
                GL_COLOR_ATTACHMENT4,
                GL_COLOR_ATTACHMENT5,
                GL_COLOR_ATTACHMENT6,
                GL_COLOR_ATTACHMENT7,
#           endif
            };

#           if defined(GRAPHICS_API_OPENGL_ES3)
                glDrawBuffersEXT(count, buffers);
#           else
                glDrawBuffers(count, buffers);
#           endif
        }
    }
    else
    {
        TRACELOG(TraceLogLevel::Warning, "GL: One color buffer active by default");
    }
#endif
}

//----------------------------------------------------------------------------------
// General render state configuration
//----------------------------------------------------------------------------------

// Enable color blending
void Context::EnableColorBlend()
{
    glEnable(GL_BLEND);
}

// Disable color blending
void Context::DisableColorBlend()
{
    glDisable(GL_BLEND);
}

// Enable depth test
void Context::EnableDepthTest()
{
    glEnable(GL_DEPTH_TEST);
}

// Disable depth test
void Context::DisableDepthTest()
{
    glDisable(GL_DEPTH_TEST);
}

// Enable depth write
void Context::EnableDepthMask()
{
    glDepthMask(GL_TRUE);
}

// Disable depth write
void Context::DisableDepthMask()
{
    glDepthMask(GL_FALSE);
}

// Enable backface culling
void Context::EnableBackfaceCulling()
{
    glEnable(GL_CULL_FACE);
}

// Disable backface culling
void Context::DisableBackfaceCulling()
{
    glDisable(GL_CULL_FACE);
}

// Set face culling mode
void Context::SetCullFace(CullMode mode)
{
    switch (mode)
    {
        case CullMode::FaceBack: glCullFace(GL_BACK); break;
        case CullMode::FaceFront: glCullFace(GL_FRONT); break;
        default: break;
    }
}

// Enable scissor test
void Context::EnableScissorTest()
{
    glEnable(GL_SCISSOR_TEST);
}

// Disable scissor test
void Context::DisableScissorTest()
{
    glDisable(GL_SCISSOR_TEST);
}

// Scissor test
void Context::Scissor(int x, int y, int width, int height)
{
    glScissor(x, y, width, height);
}

// Enable wire mode
void Context::EnableWireMode()
{
#if defined(GRAPHICS_API_OPENGL_11) || defined(GRAPHICS_API_OPENGL_33)
    // NOTE: glPolygonMode() not available on OpenGL ES
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#endif
}

void Context::EnablePointMode()
{
#if defined(GRAPHICS_API_OPENGL_11) || defined(GRAPHICS_API_OPENGL_33)
    // NOTE: glPolygonMode() not available on OpenGL ES
    glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    glEnable(GL_PROGRAM_POINT_SIZE);
#endif
}

// Disable wire mode
void Context::DisableWireMode()
{
#if defined(GRAPHICS_API_OPENGL_11) || defined(GRAPHICS_API_OPENGL_33)
    // NOTE: glPolygonMode() not available on OpenGL ES
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
}

// Set the line drawing width
void Context::SetLineWidth(float width)
{
    glLineWidth(width);
}

// Get the line drawing width
float Context::GetLineWidth()
{
    float width = 0;
    glGetFloatv(GL_LINE_WIDTH, &width);
    return width;
}

// Enable line aliasing
void Context::EnableSmoothLines()
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_11)
    glEnable(GL_LINE_SMOOTH);
#endif
}

// Disable line aliasing
void Context::DisableSmoothLines()
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_11)
    glDisable(GL_LINE_SMOOTH);
#endif
}

// Enable stereo rendering
void Context::EnableStereoRender()
{
#if (defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2))
    State.stereoRender = true;
#endif
}

// Disable stereo rendering
void Context::DisableStereoRender()
{
#if (defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2))
    State.stereoRender = false;
#endif
}

// Check if stereo render is enabled
bool Context::IsStereoRenderEnabled()
{
#if (defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2))
    return State.stereoRender;
#else
    return false;
#endif
}

// Clear color buffer with color
void Context::ClearColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    // Color values clamp to 0.0f(0) and 1.0f(255)
    glClearColor(r/255.0f, g/255.0f, b/255.0f, a/255.0f);
}

// Clear used screen buffers (color and depth)
void Context::ClearScreenBuffers()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);     // Clear used buffers: Color and Depth (Depth is used for 3D)
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);     // Stencil buffer not used...
}

// Check and log OpenGL error codes
void Context::CheckErrors()
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    int check = 1;
    while (check)
    {
        const GLenum err = glGetError();
        switch (err)
        {
            case GL_NO_ERROR: check = 0; break;
            case 0x0500: TRACELOG(TraceLogLevel::Warning, "GL: Error detected: GL_INVALID_ENUM"); break;
            case 0x0501: TRACELOG(TraceLogLevel::Warning, "GL: Error detected: GL_INVALID_VALUE"); break;
            case 0x0502: TRACELOG(TraceLogLevel::Warning, "GL: Error detected: GL_INVALID_OPERATION"); break;
            case 0x0503: TRACELOG(TraceLogLevel::Warning, "GL: Error detected: GL_STACK_OVERFLOW"); break;
            case 0x0504: TRACELOG(TraceLogLevel::Warning, "GL: Error detected: GL_STACK_UNDERFLOW"); break;
            case 0x0505: TRACELOG(TraceLogLevel::Warning, "GL: Error detected: GL_OUT_OF_MEMORY"); break;
            case 0x0506: TRACELOG(TraceLogLevel::Warning, "GL: Error detected: GL_INVALID_FRAMEBUFFER_OPERATION"); break;
            default: TRACELOG(TraceLogLevel::Warning, "GL: Error detected: Unknown error code: %x", err); break;
        }
    }
#endif
}

// Set blend mode
void Context::SetBlendMode(BlendMode mode)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    if ((State.currentBlendMode != mode) || ((mode == BlendMode::Custom || mode == BlendMode::CustomSeparate) && State.glCustomBlendModeModified))
    {
        DrawRenderBatch(currentBatch);

        switch (mode)
        {
            case BlendMode::Alpha: glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glBlendEquation(GL_FUNC_ADD); break;
            case BlendMode::Additive: glBlendFunc(GL_SRC_ALPHA, GL_ONE); glBlendEquation(GL_FUNC_ADD); break;
            case BlendMode::Multiplied: glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA); glBlendEquation(GL_FUNC_ADD); break;
            case BlendMode::AddColors: glBlendFunc(GL_ONE, GL_ONE); glBlendEquation(GL_FUNC_ADD); break;
            case BlendMode::SubtractColors: glBlendFunc(GL_ONE, GL_ONE); glBlendEquation(GL_FUNC_SUBTRACT); break;
            case BlendMode::AlphaPremultiply: glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); glBlendEquation(GL_FUNC_ADD); break;
            case BlendMode::Custom:
            {
                // NOTE: Using GL blend src/dst factors and GL equation configured with Context::SetBlendFactors()
                glBlendFunc(State.glBlendSrcFactor, State.glBlendDstFactor); glBlendEquation(State.glBlendEquation);

            } break;
            case BlendMode::CustomSeparate:
            {
                // NOTE: Using GL blend src/dst factors and GL equation configured with Context::SetBlendFactorsSeparate()
                glBlendFuncSeparate(State.glBlendSrcFactorRGB, State.glBlendDestFactorRGB, State.glBlendSrcFactorAlpha, State.glBlendDestFactorAlpha);
                glBlendEquationSeparate(State.glBlendEquationRGB, State.glBlendEquationAlpha);

            } break;
            default: break;
        }

        State.currentBlendMode = mode;
        State.glCustomBlendModeModified = false;
    }
#endif
}

// Set blending mode factor and equation
void Context::SetBlendFactors(int glSrcFactor, int glDstFactor, int glEquation)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    if ((State.glBlendSrcFactor != glSrcFactor) ||
        (State.glBlendDstFactor != glDstFactor) ||
        (State.glBlendEquation != glEquation))
    {
        State.glBlendSrcFactor = glSrcFactor;
        State.glBlendDstFactor = glDstFactor;
        State.glBlendEquation = glEquation;

        State.glCustomBlendModeModified = true;
    }
#endif
}

// Set blending mode factor and equation separately for RGB and alpha
void Context::SetBlendFactorsSeparate(int glSrcRGB, int glDstRGB, int glSrcAlpha, int glDstAlpha, int glEqRGB, int glEqAlpha)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    if ((State.glBlendSrcFactorRGB != glSrcRGB) ||
        (State.glBlendDestFactorRGB != glDstRGB) ||
        (State.glBlendSrcFactorAlpha != glSrcAlpha) ||
        (State.glBlendDestFactorAlpha != glDstAlpha) ||
        (State.glBlendEquationRGB != glEqRGB) ||
        (State.glBlendEquationAlpha != glEqAlpha))
    {
        State.glBlendSrcFactorRGB = glSrcRGB;
        State.glBlendDestFactorRGB = glDstRGB;
        State.glBlendSrcFactorAlpha = glSrcAlpha;
        State.glBlendDestFactorAlpha = glDstAlpha;
        State.glBlendEquationRGB = glEqRGB;
        State.glBlendEquationAlpha = glEqAlpha;

        State.glCustomBlendModeModified = true;
    }
#endif
}

//----------------------------------------------------------------------------------
// Module Functions Definition - OpenGL Debug
//----------------------------------------------------------------------------------
#if defined(RLGL_ENABLE_OPENGL_DEBUG_CONTEXT) && defined(GRAPHICS_API_OPENGL_43)
static void GLAPIENTRY DebugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
    // Ignore non-significant error/warning codes (NVidia drivers)
    // NOTE: Here there are the details with a sample output:
    // - #131169 - Framebuffer detailed info: The driver allocated storage for renderbuffer 2. (severity: low)
    // - #131185 - Buffer detailed info: Buffer object 1 (bound to GL_ELEMENT_ARRAY_BUFFER_ARB, usage hint is GL_ENUM_88e4)
    //             will use VIDEO memory as the source for buffer object operations. (severity: low)
    // - #131218 - Program/shader state performance warning: Vertex shader in program 7 is being recompiled based on GL state. (severity: medium)
    // - #131204 - Texture state usage warning: The texture object (0) bound to texture image unit 0 does not have
    //             a defined base level and cannot be used for texture mapping. (severity: low)
    if ((id == 131169) || (id == 131185) || (id == 131218) || (id == 131204)) return;

    const char *msgSource = nullptr;
    switch (source)
    {
        case GL_DEBUG_SOURCE_API: msgSource = "API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: msgSource = "WINDOW_SYSTEM"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: msgSource = "SHADER_COMPILER"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY: msgSource = "THIRD_PARTY"; break;
        case GL_DEBUG_SOURCE_APPLICATION: msgSource = "APPLICATION"; break;
        case GL_DEBUG_SOURCE_OTHER: msgSource = "OTHER"; break;
        default: break;
    }

    const char *msgType = nullptr;
    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR: msgType = "ERROR"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: msgType = "DEPRECATED_BEHAVIOR"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: msgType = "UNDEFINED_BEHAVIOR"; break;
        case GL_DEBUG_TYPE_PORTABILITY: msgType = "PORTABILITY"; break;
        case GL_DEBUG_TYPE_PERFORMANCE: msgType = "PERFORMANCE"; break;
        case GL_DEBUG_TYPE_MARKER: msgType = "MARKER"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP: msgType = "PUSH_GROUP"; break;
        case GL_DEBUG_TYPE_POP_GROUP: msgType = "POP_GROUP"; break;
        case GL_DEBUG_TYPE_OTHER: msgType = "OTHER"; break;
        default: break;
    }

    const char *msgSeverity = "DEFAULT";
    switch (severity)
    {
        case GL_DEBUG_SEVERITY_LOW: msgSeverity = "LOW"; break;
        case GL_DEBUG_SEVERITY_MEDIUM: msgSeverity = "MEDIUM"; break;
        case GL_DEBUG_SEVERITY_HIGH: msgSeverity = "HIGH"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: msgSeverity = "NOTIFICATION"; break;
        default: break;
    }

    TRACELOG(TraceLogLevel::Warning, "GL: OpenGL debug message: %s", message);
    TRACELOG(TraceLogLevel::Warning, "    > Type: %s", msgType);
    TRACELOG(TraceLogLevel::Warning, "    > Source = %s", msgSource);
    TRACELOG(TraceLogLevel::Warning, "    > Severity = %s", msgSeverity);
}
#endif

//----------------------------------------------------------------------------------
// Module Functions Definition - rlgl functionality
//----------------------------------------------------------------------------------

// Set current framebuffer width
void Context::SetFramebufferWidth(int width)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    State.framebufferWidth = width;
#endif
}

// Set current framebuffer height
void Context::SetFramebufferHeight(int height)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    State.framebufferHeight = height;
#endif
}

// Get default framebuffer width
int Context::GetFramebufferWidth()
{
    int width = 0;
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    width = State.framebufferWidth;
#endif
    return width;
}

// Get default framebuffer height
int Context::GetFramebufferHeight()
{
    int height = 0;
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    height = State.framebufferHeight;
#endif
    return height;
}

// Get default internal texture (white texture)
// NOTE: Default texture is a 1x1 pixel UNCOMPRESSED_R8G8B8A8
uint32_t Context::GetTextureIdDefault()
{
    uint32_t id = 0;
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    id = State.defaultTextureId;
#endif
    return id;
}

// Get default shader id
uint32_t Context::GetShaderIdDefault()
{
    uint32_t id = 0;
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    id = State.defaultShaderId;
#endif
    return id;
}

// Get default shader locs
int *Context::GetShaderLocsDefault()
{
    int *locs = nullptr;
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    locs = State.defaultShaderLocs;
#endif
    return locs;
}

// Render batch management
//------------------------------------------------------------------------------------------------

// Draw render batch
// NOTE: We require a pointer to reset batch and increase current buffer (multi-buffer)
void Context::DrawRenderBatch(RenderBatch* batch)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)

    batch->Draw(this);

    // Reset vertex counter for next frame
    State.vertexCounter = 0;

    // Reset active texture units for next batch
    for (int i = 0; i < RL_DEFAULT_BATCH_MAX_TEXTURE_UNITS; i++)
    {
        State.activeTextureId[i] = 0;
    }

#endif
}

// Set the active render batch for rlgl
void Context::SetRenderBatchActive(RenderBatch* batch)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)

    DrawRenderBatch(currentBatch);
    currentBatch = defaultBatch.get();

    if (batch == nullptr)
    {
        throw RLGLException("[Context::SetRenderBatchActive] Pointer to given batch is null");
    }

    //if (batch->rlCtx != this)
    //{
    //    throw RLGLException("[Context::SetRenderBatchActive] The context of the RenderBatch must be the same as the one it is passed to");    
    //}

    currentBatch = batch;

#endif
}

// Update and draw internal render batch
void Context::DrawRenderBatchActive()
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    DrawRenderBatch(currentBatch);    // NOTE: Stereo rendering is checked inside
#endif
}

// Check internal buffer overflow for a given number of vertex
// and force a Context::RenderBatch draw call if required
bool Context::CheckRenderBatchLimit(int vCount)
{
    bool overflow = false;

#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)

    if ((State.vertexCounter + vCount) >= (currentBatch->vertexBuffer[currentBatch->currentBuffer].elementCount*4))
    {
        overflow = true;

        // Store current primitive drawing mode and texture id
        DrawMode currentMode = currentBatch->draws[currentBatch->drawCounter - 1].mode;
        int currentTexture = currentBatch->draws[currentBatch->drawCounter - 1].textureId;

        DrawRenderBatch(currentBatch);    // NOTE: Stereo rendering is checked inside

        // Restore state of last batch so we can continue adding vertices
        currentBatch->draws[currentBatch->drawCounter - 1].mode = currentMode;
        currentBatch->draws[currentBatch->drawCounter - 1].textureId = currentTexture;
    }

#endif

    return overflow;
}

// Textures data management
//-----------------------------------------------------------------------------------------
// Convert image data to OpenGL texture (returns OpenGL valid Id)
uint32_t Context::LoadTexture(const void *data, int width, int height, PixelFormat format, int mipmapCount)
{
    uint32_t id = 0;

    glBindTexture(GL_TEXTURE_2D, 0);    // Free any old binding

    // Check texture format support by OpenGL 1.1 (compressed textures not supported)
#   if defined(GRAPHICS_API_OPENGL_11)

        if (format >= PixelFormat::DXT1_RGB)
        {
            TRACELOG(TraceLogLevel::Warning, "GL: OpenGL 1.1 does not support GPU compressed texture formats");
            return id;
        }

#   else

        if ((!GetExtensions().texCompDXT) && ((format == PixelFormat::DXT1_RGB) || (format == PixelFormat::DXT1_RGBA) ||
            (format == PixelFormat::DXT3_RGBA) || (format == PixelFormat::DXT5_RGBA)))
        {
            TRACELOG(TraceLogLevel::Warning, "GL: DXT compressed texture format not supported");
            return id;
        }

#       if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)

            if ((!GetExtensions().texCompETC1) && (format == PixelFormat::ETC1_RGB))
            {
                TRACELOG(TraceLogLevel::Warning, "GL: ETC1 compressed texture format not supported");
                return id;
            }

            if ((!GetExtensions().texCompETC2) && ((format == PixelFormat::ETC2_RGB) || (format == PixelFormat::ETC2_EAC_RGBA)))
            {
                TRACELOG(TraceLogLevel::Warning, "GL: ETC2 compressed texture format not supported");
                return id;
            }

            if ((!GetExtensions().texCompPVRT) && ((format == PixelFormat::PVRT_RGB) || (format == PixelFormat::PVRT_RGBA)))
            {
                TRACELOG(TraceLogLevel::Warning, "GL: PVRT compressed texture format not supported");
                return id;
            }

            if ((!GetExtensions().texCompASTC) && ((format == PixelFormat::ASTC_4x4_RGBA) || (format == PixelFormat::ASTC_8x8_RGBA)))
            {
                TRACELOG(TraceLogLevel::Warning, "GL: ASTC compressed texture format not supported");
                return id;
            }

#       endif

#   endif  // GRAPHICS_API_OPENGL_11

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenTextures(1, &id); // Generate texture id
    glBindTexture(GL_TEXTURE_2D, id);

    int mipWidth = width;
    int mipHeight = height;
    int mipOffset = 0;          // Mipmap data offset, only used for tracelog

    // NOTE: Added pointer math separately from function to avoid UBSAN complaining
    uint8_t *dataPtr = nullptr;
    if (data != nullptr) dataPtr = (uint8_t *)data;

    // Load the different mipmap levels
    for (int i = 0; i < mipmapCount; i++)
    {
        uint32_t mipSize = GetPixelDataSize(mipWidth, mipHeight, format);

        uint32_t glInternalFormat, glFormat, glType;
        GetGlTextureFormats(format, &glInternalFormat, &glFormat, &glType);

        TRACELOGD("TEXTURE: Load mipmap level %i (%i x %i), size: %i, offset: %i", i, mipWidth, mipHeight, mipSize, mipOffset);

        if (glInternalFormat != 0)
        {
            if (format < PixelFormat::DXT1_RGB)
            {
                glTexImage2D(GL_TEXTURE_2D, i, glInternalFormat, mipWidth, mipHeight, 0, glFormat, glType, dataPtr);
            }
#           if !defined(GRAPHICS_API_OPENGL_11)
            else
            {
                glCompressedTexImage2D(GL_TEXTURE_2D, i, glInternalFormat, mipWidth, mipHeight, 0, mipSize, dataPtr);
            }
#           endif

#           if defined(GRAPHICS_API_OPENGL_33)
            if (format == PixelFormat::Grayscale)
            {
                GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
                glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
            }
            else if (format == PixelFormat::GrayAlpha)
            {
#               if defined(GRAPHICS_API_OPENGL_21)
                    GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_ALPHA };
#               elif defined(GRAPHICS_API_OPENGL_33)
                    GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_GREEN };
#               endif

                glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
            }
#           endif
        }

        mipWidth /= 2;
        mipHeight /= 2;
        mipOffset += mipSize;       // Increment offset position to next mipmap
        if (data != nullptr) dataPtr += mipSize;         // Increment data pointer to next mipmap

        // Security check for NPOT textures
        if (mipWidth < 1) mipWidth = 1;
        if (mipHeight < 1) mipHeight = 1;
    }

    // Texture parameters configuration
    // NOTE: glTexParameteri does NOT affect texture uploading, just the way it's used
#   if defined(GRAPHICS_API_OPENGL_ES2)

        // NOTE: OpenGL ES 2.0 with no GL_OES_texture_npot support (i.e. WebGL) has limited NPOT support, so CLAMP_TO_EDGE must be used
        if (GetExtensions().texNPOT)
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);       // Set texture to repeat on x-axis
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);       // Set texture to repeat on y-axis
        }
        else
        {
            // NOTE: If using negative texture coordinates (LoadOBJ()), it does not work!
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);       // Set texture to clamp on x-axis
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);       // Set texture to clamp on y-axis
        }

#   else
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);       // Set texture to repeat on x-axis
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);       // Set texture to repeat on y-axis
#   endif

    // Magnification and minification filters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  // Alternative: GL_LINEAR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);  // Alternative: GL_LINEAR

#   if defined(GRAPHICS_API_OPENGL_33)

        if (mipmapCount > 1)
        {
            // Activate Trilinear filtering if mipmaps are available
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        }

#   endif

    // At this point we have the texture loaded in GPU and texture parameters configured
    // NOTE: If mipmaps were not in data, they are not generated automatically

    // Unbind current texture
    glBindTexture(GL_TEXTURE_2D, 0);

    if (id > 0) TRACELOG(TraceLogLevel::Info, "TEXTURE: [ID %i] Texture loaded successfully (%ix%i | %s | %i mipmaps)", id, width, height, GetPixelFormatName(format), mipmapCount);
    else TRACELOG(TraceLogLevel::Warning, "TEXTURE: Failed to load texture");

    return id;
}

// Load depth texture/renderbuffer (to be attached to fbo)
// WARNING: OpenGL ES 2.0 requires GL_OES_depth_texture and WebGL requires WEBGL_depth_texture extensions
uint32_t Context::LoadTextureDepth(int width, int height, bool useRenderBuffer)
{
    uint32_t id = 0;

#   if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)

        // In case depth textures not supported, we force renderbuffer usage
        if (!GetExtensions().texDepth) useRenderBuffer = true;

        // NOTE: We let the implementation to choose the best bit-depth
        // Possible formats: GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT32 and GL_DEPTH_COMPONENT32F
        uint32_t glInternalFormat = GL_DEPTH_COMPONENT;

#   if (defined(GRAPHICS_API_OPENGL_ES2) || defined(GRAPHICS_API_OPENGL_ES3))

        // WARNING: WebGL platform requires unsized internal format definition (GL_DEPTH_COMPONENT)
        // while other platforms using OpenGL ES 2.0 require/support sized internal formats depending on the GPU capabilities
        if (!GetExtensions().texDepthWebGL || useRenderBuffer)
        {
            if (GetExtensions().maxDepthBits == 32) glInternalFormat = GL_DEPTH_COMPONENT32_OES;
            else if (GetExtensions().maxDepthBits == 24) glInternalFormat = GL_DEPTH_COMPONENT24_OES;
            else glInternalFormat = GL_DEPTH_COMPONENT16;
        }

#   endif

    if (!useRenderBuffer && GetExtensions().texDepth)
    {
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, glInternalFormat, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_2D, 0);

        TRACELOG(TraceLogLevel::Info, "TEXTURE: Depth texture loaded successfully");
    }
    else
    {
        // Create the renderbuffer that will serve as the depth attachment for the framebuffer
        // NOTE: A renderbuffer is simpler than a texture and could offer better performance on embedded devices
        glGenRenderbuffers(1, &id);
        glBindRenderbuffer(GL_RENDERBUFFER, id);
        glRenderbufferStorage(GL_RENDERBUFFER, glInternalFormat, width, height);

        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        TRACELOG(TraceLogLevel::Info, "TEXTURE: [ID %i] Depth renderbuffer loaded successfully (%i bits)", id, (GetExtensions().maxDepthBits >= 24)? GetExtensions().maxDepthBits : 16);
    }
#endif

    return id;
}

// Load texture cubemap
// NOTE: Cubemap data is expected to be 6 images in a single data array (one after the other),
// expected the following convention: +X, -X, +Y, -Y, +Z, -Z
uint32_t Context::LoadTextureCubemap(const void *data, int size, PixelFormat format)
{
    uint32_t id = 0;

#   if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)

        uint32_t dataSize = GetPixelDataSize(size, size, format);

        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_CUBE_MAP, id);

        uint32_t glInternalFormat, glFormat, glType;
        GetGlTextureFormats(format, &glInternalFormat, &glFormat, &glType);

        if (glInternalFormat != 0)
        {
            // Load cubemap faces
            for (uint32_t i = 0; i < 6; i++)
            {
                if (data == nullptr)
                {
                    if (format < PixelFormat::DXT1_RGB)
                    {
                        if ((format == PixelFormat::R32) || (format == PixelFormat::R32G32B32A32)
                                || (format == PixelFormat::R16) || (format == PixelFormat::R16G16B16A16))
                            TRACELOG(TraceLogLevel::Warning, "TEXTURES: Cubemap requested format not supported");
                        else glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, glInternalFormat, size, size, 0, glFormat, glType, nullptr);
                    }
                    else TRACELOG(TraceLogLevel::Warning, "TEXTURES: Empty cubemap creation does not support compressed format");
                }
                else
                {
                    if (format < PixelFormat::DXT1_RGB) glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, glInternalFormat, size, size, 0, glFormat, glType, (uint8_t *)data + i*dataSize);
                    else glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, glInternalFormat, size, size, 0, dataSize, (uint8_t *)data + i*dataSize);
                }

#           if defined(GRAPHICS_API_OPENGL_33)

                if (format == PixelFormat::Grayscale)
                {
                    GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
                    glTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
                }
                else if (format == PixelFormat::GrayAlpha)
                {
#                   if defined(GRAPHICS_API_OPENGL_21)
                        GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_ALPHA };
#                   elif defined(GRAPHICS_API_OPENGL_33)
                        GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_GREEN };
#                   endif

                    glTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
                }
#           endif
        }
    }

    // Set cubemap texture sampling parameters
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

#   if defined(GRAPHICS_API_OPENGL_33)
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);  // Flag not supported on OpenGL ES 2.0
#   endif

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
#endif

    if (id > 0) TRACELOG(TraceLogLevel::Info, "TEXTURE: [ID %i] Cubemap texture loaded successfully (%ix%i)", id, size, size);
    else TRACELOG(TraceLogLevel::Warning, "TEXTURE: Failed to load cubemap texture");

    return id;
}

// Update already loaded texture in GPU with new data
// NOTE: We don't know safely if internal texture format is the expected one...
void Context::UpdateTexture(uint32_t id, int offsetX, int offsetY, int width, int height, PixelFormat format, const void *data)
{
    glBindTexture(GL_TEXTURE_2D, id);

    uint32_t glInternalFormat, glFormat, glType;
    GetGlTextureFormats(format, &glInternalFormat, &glFormat, &glType);

    if ((glInternalFormat != 0) && (format < PixelFormat::DXT1_RGB))
    {
        glTexSubImage2D(GL_TEXTURE_2D, 0, offsetX, offsetY, width, height, glFormat, glType, data);
    }
    else TRACELOG(TraceLogLevel::Warning, "TEXTURE: [ID %i] Failed to update for current texture format (%i)", id, format);
}

// Unload texture from GPU memory
void Context::UnloadTexture(uint32_t id)
{
    glDeleteTextures(1, &id);
}

// Generate mipmap data for selected texture
// NOTE: Only supports GPU mipmap generation
void Context::GenTextureMipmaps(uint32_t id, int width, int height, PixelFormat format, int *mipmaps)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    glBindTexture(GL_TEXTURE_2D, id);

    // Check if texture is power-of-two (POT)
    bool texIsPOT = false;

    if (((width > 0) && ((width & (width - 1)) == 0)) &&
        ((height > 0) && ((height & (height - 1)) == 0))) texIsPOT = true;

    if ((texIsPOT) || (GetExtensions().texNPOT))
    {
        //glHint(GL_GENERATE_MIPMAP_HINT, GL_DONT_CARE);   // Hint for mipmaps generation algorithm: GL_FASTEST, GL_NICEST, GL_DONT_CARE
        glGenerateMipmap(GL_TEXTURE_2D);    // Generate mipmaps automatically

        *mipmaps = 1 + std::floor(std::log(std::max(width, height))/std::log(2));
        TRACELOG(TraceLogLevel::Info, "TEXTURE: [ID %i] Mipmaps generated automatically, total: %i", id, *mipmaps);
    }
    else TRACELOG(TraceLogLevel::Warning, "TEXTURE: [ID %i] Failed to generate mipmaps", id);

    glBindTexture(GL_TEXTURE_2D, 0);
#else
    TRACELOG(TraceLogLevel::Warning, "TEXTURE: [ID %i] GPU mipmap generation not supported", id);
#endif
}


// Read texture pixel data
std::vector<uint8_t> Context::ReadTexturePixels(uint32_t id, int width, int height, PixelFormat format)
{
    std::vector<uint8_t> pixels;

#   if defined(GRAPHICS_API_OPENGL_11) || defined(GRAPHICS_API_OPENGL_33)

        glBindTexture(GL_TEXTURE_2D, id);

        // NOTE: Using texture id, we can retrieve some texture info (but not on OpenGL ES 2.0)
        // Possible texture info: GL_TEXTURE_RED_SIZE, GL_TEXTURE_GREEN_SIZE, GL_TEXTURE_BLUE_SIZE, GL_TEXTURE_ALPHA_SIZE
        //int width, height, format;
        //glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
        //glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
        //glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);

        // NOTE: Each row written to or read from by OpenGL pixel operations like glGetTexImage are aligned to a 4 byte boundary by default, which may add some padding.
        // Use glPixelStorei to modify padding with the GL_[UN]PACK_ALIGNMENT setting.
        // GL_PACK_ALIGNMENT affects operations that read from OpenGL memory (glReadPixels, glGetTexImage, etc.)
        // GL_UNPACK_ALIGNMENT affects operations that write to OpenGL memory (glTexImage, etc.)
        glPixelStorei(GL_PACK_ALIGNMENT, 1);

        uint32_t glInternalFormat, glFormat, glType;
        GetGlTextureFormats(format, &glInternalFormat, &glFormat, &glType);
        uint32_t size = GetPixelDataSize(width, height, format);

        if ((glInternalFormat != 0) && (format < PixelFormat::DXT1_RGB))
        {
            pixels.resize(size);
            glGetTexImage(GL_TEXTURE_2D, 0, glFormat, glType, pixels.data());
        }
        else TRACELOG(TraceLogLevel::Warning, "TEXTURE: [ID %i] Data retrieval not suported for pixel format (%i)", id, format);

        glBindTexture(GL_TEXTURE_2D, 0);

#   endif

#   if defined(GRAPHICS_API_OPENGL_ES2) || defined(PLATFORM_DESKTOP)

        // glGetTexImage() is not available on OpenGL ES 2.0
        // Texture width and height are required on OpenGL ES 2.0. There is no way to get it from texture id.
        // Two possible Options:
        // 1 - Bind texture to color fbo attachment and glReadPixels()
        // 2 - Create an fbo, activate it, render quad with texture, glReadPixels()
        // We are using Option 1, just need to care for texture format on retrieval
        // NOTE: This behaviour could be conditioned by graphic driver...
        uint32_t fboId = LoadFramebuffer(width, height);

        glBindFramebuffer(GL_FRAMEBUFFER, fboId);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Attach our texture to FBO
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, id, 0);

        // We read data as RGBA because FBO texture is configured as RGBA, despite binding another texture format
        pixels.resize(GetPixelDataSize(width, height, PixelFormat::R8G8B8A8));
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Clean up temporal fbo
        UnloadFramebuffer(fboId);

#   endif

    return pixels;
}

// Read screen pixel data (color buffer)
std::vector<uint8_t> Context::ReadScreenPixels(int width, int height)
{
    std::vector<uint8_t> screenData(width*height*4, 0);

    // NOTE 1: glReadPixels returns image flipped vertically -> (0,0) is the bottom left corner of the framebuffer
    // NOTE 2: We are getting alpha channel! Be careful, it can be transparent if not cleared properly!
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, screenData.data());

    // Flip image vertically!
    std::vector<uint8_t> imgData(screenData.size());

    for (int y = height - 1; y >= 0; y--)
    {
        for (int x = 0; x < (width*4); x++)
        {
            imgData[((height - 1) - y)*width*4 + x] = screenData[(y*width*4) + x];  // Flip line

            // Set alpha component value to 255 (no trasparent image retrieval)
            // NOTE: Alpha value has already been applied to RGB in framebuffer, we don't need it!
            if (((x + 1)%4) == 0) imgData[((height - 1) - y)*width*4 + x] = 255;
        }
    }

    return imgData;     // NOTE: image data should be freed
}

// Framebuffer management (fbo)
//-----------------------------------------------------------------------------------------
// Load a framebuffer to be used for rendering
// NOTE: No textures attached
uint32_t Context::LoadFramebuffer(int width, int height)
{
    uint32_t fboId = 0;

#   if (defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)) && defined(RLGL_RENDER_TEXTURES_HINT)
        glGenFramebuffers(1, &fboId);       // Create the framebuffer object
        glBindFramebuffer(GL_FRAMEBUFFER, 0);   // Unbind any framebuffer
#   endif

    return fboId;
}

// Attach color buffer texture to an fbo (unloads previous attachment)
// NOTE: Attach type: 0-Color, 1-Depth renderbuffer, 2-Depth texture
void Context::FramebufferAttach(uint32_t fboId, uint32_t texId, FramebufferAttachType attachType, FramebufferAttachTextureType texType, int mipLevel)
{
#if (defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)) && defined(RLGL_RENDER_TEXTURES_HINT)
    glBindFramebuffer(GL_FRAMEBUFFER, fboId);

    switch (attachType)
    {
        case FramebufferAttachType::ColorChannel0:
        case FramebufferAttachType::ColorChannel1:
        case FramebufferAttachType::ColorChannel2:
        case FramebufferAttachType::ColorChannel3:
        case FramebufferAttachType::ColorChannel4:
        case FramebufferAttachType::ColorChannel5:
        case FramebufferAttachType::ColorChannel6:
        case FramebufferAttachType::ColorChannel7:
        {
            if (texType == FramebufferAttachTextureType::Texture2D) glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + static_cast<int>(attachType), GL_TEXTURE_2D, texId, mipLevel);
            else if (texType == FramebufferAttachTextureType::RenderBuffer) glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + static_cast<int>(attachType), GL_RENDERBUFFER, texId);
            else if (texType >= FramebufferAttachTextureType::CubemapPositiveX) glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + static_cast<int>(attachType), GL_TEXTURE_CUBE_MAP_POSITIVE_X + texType, texId, mipLevel);

        } break;
        case FramebufferAttachType::Depth:
        {
            if (texType == FramebufferAttachTextureType::Texture2D) glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texId, mipLevel);
            else if (texType == FramebufferAttachTextureType::RenderBuffer)  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, texId);

        } break;
        case FramebufferAttachType::Stencil:
        {
            if (texType == FramebufferAttachTextureType::Texture2D) glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texId, mipLevel);
            else if (texType == FramebufferAttachTextureType::RenderBuffer)  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, texId);

        } break;
        default: break;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
}

// Verify render texture is complete
bool Context::FramebufferComplete(uint32_t id)
{
    bool result = false;

#if (defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)) && defined(RLGL_RENDER_TEXTURES_HINT)
    glBindFramebuffer(GL_FRAMEBUFFER, id);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        switch (status)
        {
            case GL_FRAMEBUFFER_UNSUPPORTED: TRACELOG(TraceLogLevel::Warning, "FBO: [ID %i] Framebuffer is unsupported", id); break;
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: TRACELOG(TraceLogLevel::Warning, "FBO: [ID %i] Framebuffer has incomplete attachment", id); break;
#           if defined(GRAPHICS_API_OPENGL_ES2)
            case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS: TRACELOG(TraceLogLevel::Warning, "FBO: [ID %i] Framebuffer has incomplete dimensions", id); break;
#           endif
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: TRACELOG(TraceLogLevel::Warning, "FBO: [ID %i] Framebuffer has a missing attachment", id); break;
            default: break;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    result = (status == GL_FRAMEBUFFER_COMPLETE);
#endif

    return result;
}

// Unload framebuffer from GPU memory
// NOTE: All attached textures/cubemaps/renderbuffers are also deleted
void Context::UnloadFramebuffer(uint32_t id)
{
#if (defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)) && defined(RLGL_RENDER_TEXTURES_HINT)
    // Query depth attachment to automatically delete texture/renderbuffer
    int depthType = 0, depthId = 0;
    glBindFramebuffer(GL_FRAMEBUFFER, id);   // Bind framebuffer to query depth texture type
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &depthType);

    // TODO: Review warning retrieving object name in WebGL
    // WARNING: WebGL: INVALID_ENUM: getFramebufferAttachmentParameter: invalid parameter name
    // https://registry.khronos.org/webgl/specs/latest/1.0/
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &depthId);

    uint32_t depthIdU = (uint32_t)depthId;
    if (depthType == GL_RENDERBUFFER) glDeleteRenderbuffers(1, &depthIdU);
    else if (depthType == GL_TEXTURE) glDeleteTextures(1, &depthIdU);

    // NOTE: If a texture object is deleted while its image is attached to the *currently bound* framebuffer,
    // the texture image is automatically detached from the currently bound framebuffer.

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &id);

    TRACELOG(TraceLogLevel::Info, "FBO: [ID %i] Unloaded framebuffer from VRAM (GPU)", id);
#endif
}


// Vertex data management
//-----------------------------------------------------------------------------------------
// Load a new attributes buffer
uint32_t Context::LoadVertexBuffer(const void *buffer, int size, bool dynamic)
{
    uint32_t id = 0;

#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferData(GL_ARRAY_BUFFER, size, buffer, dynamic? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
#endif

    return id;
}

// Load a new attributes element buffer
uint32_t Context::LoadVertexBufferElement(const void *buffer, int size, bool dynamic)
{
    uint32_t id = 0;

#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    glGenBuffers(1, &id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, buffer, dynamic? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
#endif

    return id;
}

// Enable vertex buffer (VBO)
void Context::EnableVertexBuffer(uint32_t id)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    glBindBuffer(GL_ARRAY_BUFFER, id);
#endif
}

// Disable vertex buffer (VBO)
void Context::DisableVertexBuffer()
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif
}

// Enable vertex buffer element (VBO element)
void Context::EnableVertexBufferElement(uint32_t id)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
#endif
}

// Disable vertex buffer element (VBO element)
void Context::DisableVertexBufferElement()
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
#endif
}

// Update vertex buffer with new data
// NOTE: dataSize and offset must be provided in bytes
void Context::UpdateVertexBuffer(uint32_t id, const void *data, int dataSize, int offset)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferSubData(GL_ARRAY_BUFFER, offset, dataSize, data);
#endif
}

// Update vertex buffer elements with new data
// NOTE: dataSize and offset must be provided in bytes
void Context::UpdateVertexBufferElements(uint32_t id, const void *data, int dataSize, int offset)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, dataSize, data);
#endif
}

// Enable vertex array object (VAO)
bool Context::EnableVertexArray(uint32_t vaoId)
{
    bool result = false;
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    if (GetExtensions().vao)
    {
        glBindVertexArray(vaoId);
        result = true;
    }
#endif
    return result;
}

// Disable vertex array object (VAO)
void Context::DisableVertexArray()
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    if (GetExtensions().vao) glBindVertexArray(0);
#endif
}

// Enable vertex attribute index
void Context::EnableVertexAttribute(uint32_t index)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    glEnableVertexAttribArray(index);
#endif
}

// Disable vertex attribute index
void Context::DisableVertexAttribute(uint32_t index)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    glDisableVertexAttribArray(index);
#endif
}

// Draw vertex array
void Context::DrawVertexArray(int offset, int count)
{
    glDrawArrays(GL_TRIANGLES, offset, count);
}

// Draw vertex array elements
void Context::DrawVertexArrayElements(int offset, int count, const void *buffer)
{
    // NOTE: Added pointer math separately from function to avoid UBSAN complaining
    uint16_t *bufferPtr = (uint16_t *)buffer;
    if (offset > 0) bufferPtr += offset;

    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, (const uint16_t *)bufferPtr);
}

// Draw vertex array instanced
void Context::DrawVertexArrayInstanced(int offset, int count, int instances)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    glDrawArraysInstanced(GL_TRIANGLES, 0, count, instances);
#endif
}

// Draw vertex array elements instanced
void Context::DrawVertexArrayElementsInstanced(int offset, int count, const void *buffer, int instances)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    // NOTE: Added pointer math separately from function to avoid UBSAN complaining
    uint16_t *bufferPtr = (uint16_t *)buffer;
    if (offset > 0) bufferPtr += offset;

    glDrawElementsInstanced(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, (const uint16_t *)bufferPtr, instances);
#endif
}

#if defined(GRAPHICS_API_OPENGL_11)
// Enable vertex state pointer
void Context::EnableStatePointer(int vertexAttribType, void *buffer)
{
    if (buffer != nullptr) glEnableClientState(vertexAttribType);
    switch (vertexAttribType)
    {
        case GL_VERTEX_ARRAY: glVertexPointer(3, GL_FLOAT, 0, buffer); break;
        case GL_TEXTURE_COORD_ARRAY: glTexCoordPointer(2, GL_FLOAT, 0, buffer); break;
        case GL_NORMAL_ARRAY: if (buffer != nullptr) glNormalPointer(GL_FLOAT, 0, buffer); break;
        case GL_COLOR_ARRAY: if (buffer != nullptr) glColorPointer(4, GL_UNSIGNED_BYTE, 0, buffer); break;
        //case GL_INDEX_ARRAY: if (buffer != nullptr) glIndexPointer(GL_SHORT, 0, buffer); break; // Indexed colors
        default: break;
    }
}

// Disable vertex state pointer
void Context::DisableStatePointer(int vertexAttribType)
{
    glDisableClientState(vertexAttribType);
}
#endif

// Load vertex array object (VAO)
uint32_t Context::LoadVertexArray()
{
    uint32_t vaoId = 0;
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    if (GetExtensions().vao)
    {
        glGenVertexArrays(1, &vaoId);
    }
#endif
    return vaoId;
}

// Set vertex attribute
void Context::SetVertexAttribute(uint32_t index, int compSize, int type, bool normalized, int stride, const void *pointer)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    glVertexAttribPointer(index, compSize, type, normalized, stride, pointer);
#endif
}

// Set vertex attribute divisor
void Context::SetVertexAttributeDivisor(uint32_t index, int divisor)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    glVertexAttribDivisor(index, divisor);
#endif
}

// Unload vertex array object (VAO)
void Context::UnloadVertexArray(uint32_t vaoId)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    if (GetExtensions().vao)
    {
        glBindVertexArray(0);
        glDeleteVertexArrays(1, &vaoId);
        TRACELOG(TraceLogLevel::Info, "VAO: [ID %i] Unloaded vertex array data from VRAM (GPU)", vaoId);
    }
#endif
}

// Unload vertex buffer (VBO)
void Context::UnloadVertexBuffer(uint32_t vboId)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    glDeleteBuffers(1, &vboId);
    //TRACELOG(TraceLogLevel::Info, "VBO: Unloaded vertex data from VRAM (GPU)");
#endif
}

// Shaders management
//-----------------------------------------------------------------------------------------------
// Load shader from code strings
// NOTE: If shader string is nullptr, using default vertex/fragment shaders
uint32_t Context::LoadShaderCode(const char *vsCode, const char *fsCode)
{
    uint32_t id = 0;

#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    uint32_t vertexShaderId = 0;
    uint32_t fragmentShaderId = 0;

    // Compile vertex shader (if provided)
    if (vsCode != nullptr) vertexShaderId = CompileShader(vsCode, GL_VERTEX_SHADER);
    // In case no vertex shader was provided or compilation failed, we use default vertex shader
    if (vertexShaderId == 0) vertexShaderId = State.defaultVShaderId;

    // Compile fragment shader (if provided)
    if (fsCode != nullptr) fragmentShaderId = CompileShader(fsCode, GL_FRAGMENT_SHADER);
    // In case no fragment shader was provided or compilation failed, we use default fragment shader
    if (fragmentShaderId == 0) fragmentShaderId = State.defaultFShaderId;

    // In case vertex and fragment shader are the default ones, no need to recompile, we can just assign the default shader program id
    if ((vertexShaderId == State.defaultVShaderId) && (fragmentShaderId == State.defaultFShaderId)) id = State.defaultShaderId;
    else
    {
        // One of or both shader are new, we need to compile a new shader program
        id = LoadShaderProgram(vertexShaderId, fragmentShaderId);

        // We can detach and delete vertex/fragment shaders (if not default ones)
        // NOTE: We detach shader before deletion to make sure memory is freed
        if (vertexShaderId != State.defaultVShaderId)
        {
            // WARNING: Shader program linkage could fail and returned id is 0
            if (id > 0) glDetachShader(id, vertexShaderId);
            glDeleteShader(vertexShaderId);
        }
        if (fragmentShaderId != State.defaultFShaderId)
        {
            // WARNING: Shader program linkage could fail and returned id is 0
            if (id > 0) glDetachShader(id, fragmentShaderId);
            glDeleteShader(fragmentShaderId);
        }

        // In case shader program loading failed, we assign default shader
        if (id == 0)
        {
            // In case shader loading fails, we return the default shader
            TRACELOG(TraceLogLevel::Warning, "SHADER: Failed to load custom shader code, using default shader");
            id = State.defaultShaderId;
        }
        /*
        else
        {
            // Get available shader uniforms
            // NOTE: This information is useful for debug...
            int uniformCount = -1;
            glGetProgramiv(id, GL_ACTIVE_UNIFORMS, &uniformCount);

            for (int i = 0; i < uniformCount; i++)
            {
                int namelen = -1;
                int num = -1;
                char name[256] = { 0 };     // Assume no variable names longer than 256
                GLenum type = GL_ZERO;

                // Get the name of the uniforms
                glGetActiveUniform(id, i, sizeof(name) - 1, &namelen, &num, &type, name);

                name[namelen] = 0;
                TRACELOGD("SHADER: [ID %i] Active uniform (%s) set at location: %i", id, name, glGetUniformLocation(id, name));
            }
        }
        */
    }
#endif

    return id;
}

// Compile custom shader and return shader id
uint32_t Context::CompileShader(const char *shaderCode, int type)
{
    uint32_t shader = 0;

#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    shader = glCreateShader(type);
    glShaderSource(shader, 1, &shaderCode, nullptr);

    GLint success = 0;
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (success == GL_FALSE)
    {
        switch (type)
        {
            case GL_VERTEX_SHADER: TRACELOG(TraceLogLevel::Warning, "SHADER: [ID %i] Failed to compile vertex shader code", shader); break;
            case GL_FRAGMENT_SHADER: TRACELOG(TraceLogLevel::Warning, "SHADER: [ID %i] Failed to compile fragment shader code", shader); break;
            //case GL_GEOMETRY_SHADER:
        #if defined(GRAPHICS_API_OPENGL_43)
            case GL_COMPUTE_SHADER: TRACELOG(TraceLogLevel::Warning, "SHADER: [ID %i] Failed to compile compute shader code", shader); break;
        #endif
            default: break;
        }

        int maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        if (maxLength > 0)
        {
            int length = 0;
            std::string log(0, maxLength);
            glGetShaderInfoLog(shader, maxLength, &length, log.data());
            TRACELOG(TraceLogLevel::Warning, "SHADER: [ID %i] Compile error: %s", shader, log.c_str());
        }
    }
    else
    {
        switch (type)
        {
            case GL_VERTEX_SHADER: TRACELOG(TraceLogLevel::Info, "SHADER: [ID %i] Vertex shader compiled successfully", shader); break;
            case GL_FRAGMENT_SHADER: TRACELOG(TraceLogLevel::Info, "SHADER: [ID %i] Fragment shader compiled successfully", shader); break;
            //case GL_GEOMETRY_SHADER:
#           if defined(GRAPHICS_API_OPENGL_43)
            case GL_COMPUTE_SHADER: TRACELOG(TraceLogLevel::Info, "SHADER: [ID %i] Compute shader compiled successfully", shader); break;
#           endif
            default: break;
        }
    }
#endif

    return shader;
}

// Load custom shader strings and return program id
uint32_t Context::LoadShaderProgram(uint32_t vShaderId, uint32_t fShaderId)
{
    uint32_t program = 0;

#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    GLint success = 0;
    program = glCreateProgram();

    glAttachShader(program, vShaderId);
    glAttachShader(program, fShaderId);

    // NOTE: Default attribute shader locations must be Bound before linking
    glBindAttribLocation(program, 0, RL_DEFAULT_SHADER_ATTRIB_NAME_POSITION);
    glBindAttribLocation(program, 1, RL_DEFAULT_SHADER_ATTRIB_NAME_TEXCOORD);
    glBindAttribLocation(program, 2, RL_DEFAULT_SHADER_ATTRIB_NAME_NORMAL);
    glBindAttribLocation(program, 3, RL_DEFAULT_SHADER_ATTRIB_NAME_COLOR);
    glBindAttribLocation(program, 4, RL_DEFAULT_SHADER_ATTRIB_NAME_TANGENT);
    glBindAttribLocation(program, 5, RL_DEFAULT_SHADER_ATTRIB_NAME_TEXCOORD2);

    // NOTE: If some attrib name is no found on the shader, it locations becomes -1

    glLinkProgram(program);

    // NOTE: All uniform variables are intitialised to 0 when a program links

    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (success == GL_FALSE)
    {
        TRACELOG(TraceLogLevel::Warning, "SHADER: [ID %i] Failed to link shader program", program);

        int maxLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

        if (maxLength > 0)
        {
            int length = 0;
            std::string log(0, maxLength);
            glGetProgramInfoLog(program, maxLength, &length, log.data());
            TRACELOG(TraceLogLevel::Warning, "SHADER: [ID %i] Link error: %s", program, log.c_str());
        }

        glDeleteProgram(program);

        program = 0;
    }
    else
    {
        // Get the size of compiled shader program (not available on OpenGL ES 2.0)
        // NOTE: If GL_LINK_STATUS is GL_FALSE, program binary length is zero.
        //GLint binarySize = 0;
        //glGetProgramiv(id, GL_PROGRAM_BINARY_LENGTH, &binarySize);

        TRACELOG(TraceLogLevel::Info, "SHADER: [ID %i] Program shader loaded successfully", program);
    }
#endif
    return program;
}

// Unload shader program
void Context::UnloadShaderProgram(uint32_t id)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    glDeleteProgram(id);

    TRACELOG(TraceLogLevel::Info, "SHADER: [ID %i] Unloaded shader program data from VRAM (GPU)", id);
#endif
}

// Get shader location uniform
int Context::GetLocationUniform(uint32_t shaderId, const char *uniformName)
{
    int location = -1;
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    location = glGetUniformLocation(shaderId, uniformName);

    //if (location == -1) TRACELOG(TraceLogLevel::Warning, "SHADER: [ID %i] Failed to find shader uniform: %s", shaderId, uniformName);
    //else TRACELOG(TraceLogLevel::Info, "SHADER: [ID %i] Shader uniform (%s) set at location: %i", shaderId, uniformName, location);
#endif
    return location;
}

// Get shader location attribute
int Context::GetLocationAttrib(uint32_t shaderId, const char *attribName)
{
    int location = -1;
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    location = glGetAttribLocation(shaderId, attribName);

    //if (location == -1) TRACELOG(TraceLogLevel::Warning, "SHADER: [ID %i] Failed to find shader attribute: %s", shaderId, attribName);
    //else TRACELOG(TraceLogLevel::Info, "SHADER: [ID %i] Shader attribute (%s) set at location: %i", shaderId, attribName, location);
#endif
    return location;
}

// Set shader value uniform
void Context::SetUniform(int locIndex, const void *value, ShaderUniformType uniformType, int count)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    switch (uniformType)
    {
        case ShaderUniformType::Float: glUniform1fv(locIndex, count, reinterpret_cast<const float*>(value)); break;
        case ShaderUniformType::Vec2: glUniform2fv(locIndex, count, reinterpret_cast<const float*>(value)); break;
        case ShaderUniformType::Vec3: glUniform3fv(locIndex, count, reinterpret_cast<const float*>(value)); break;
        case ShaderUniformType::Vec4: glUniform4fv(locIndex, count, reinterpret_cast<const float*>(value)); break;
        case ShaderUniformType::Int: glUniform1iv(locIndex, count, reinterpret_cast<const int*>(value)); break;
        case ShaderUniformType::IVec2: glUniform2iv(locIndex, count, reinterpret_cast<const int*>(value)); break;
        case ShaderUniformType::IVec3: glUniform3iv(locIndex, count, reinterpret_cast<const int*>(value)); break;
        case ShaderUniformType::IVec4: glUniform4iv(locIndex, count, reinterpret_cast<const int*>(value)); break;
        case ShaderUniformType::Sampler2D: glUniform1iv(locIndex, count, reinterpret_cast<const int*>(value)); break;
        default: TRACELOG(TraceLogLevel::Warning, "SHADER: Failed to set uniform value, data type not recognized");
    }
#endif
}

// Set shader value attribute
void Context::SetVertexAttributeDefault(int locIndex, const void *value, ShaderAttributeType attribType, int count)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    switch (attribType)
    {
        case ShaderAttributeType::Float: if (count == 1) glVertexAttrib1fv(locIndex, reinterpret_cast<const float*>(value)); break;
        case ShaderAttributeType::Vec2: if (count == 2) glVertexAttrib2fv(locIndex, reinterpret_cast<const float*>(value)); break;
        case ShaderAttributeType::Vec3: if (count == 3) glVertexAttrib3fv(locIndex, reinterpret_cast<const float*>(value)); break;
        case ShaderAttributeType::Vec4: if (count == 4) glVertexAttrib4fv(locIndex, reinterpret_cast<const float*>(value)); break;
        default: TRACELOG(TraceLogLevel::Warning, "SHADER: Failed to set attrib default value, data type not recognized");
    }
#endif
}

// Set shader value uniform matrix
void Context::SetUniformMatrix(int locIndex, Matrix mat)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    float matfloat[16] = {
        mat.m0, mat.m1, mat.m2, mat.m3,
        mat.m4, mat.m5, mat.m6, mat.m7,
        mat.m8, mat.m9, mat.m10, mat.m11,
        mat.m12, mat.m13, mat.m14, mat.m15
    };
    glUniformMatrix4fv(locIndex, 1, false, matfloat);
#endif
}

// Set shader value uniform sampler
void Context::SetUniformSampler(int locIndex, uint32_t textureId)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    // Check if texture is already active
    for (int i = 0; i < RL_DEFAULT_BATCH_MAX_TEXTURE_UNITS; i++) if (State.activeTextureId[i] == textureId) return;

    // Register a new active texture for the internal batch system
    // NOTE: Default texture is always activated as GL_TEXTURE0
    for (int i = 0; i < RL_DEFAULT_BATCH_MAX_TEXTURE_UNITS; i++)
    {
        if (State.activeTextureId[i] == 0)
        {
            glUniform1i(locIndex, 1 + i);              // Activate new texture unit
            State.activeTextureId[i] = textureId; // Save texture id for binding on drawing
            break;
        }
    }
#endif
}

// Set shader currently active (id and locations)
void Context::SetShader(uint32_t id, int *locs)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    if (State.currentShaderId != id)
    {
        DrawRenderBatch(currentBatch);
        State.currentShaderId = id;
        State.currentShaderLocs = locs;
    }
#endif
}

// Load compute shader program
uint32_t Context::LoadComputeShaderProgram(uint32_t shaderId)
{
    uint32_t program = 0;

#if defined(GRAPHICS_API_OPENGL_43)
    GLint success = 0;
    program = glCreateProgram();
    glAttachShader(program, shaderId);
    glLinkProgram(program);

    // NOTE: All uniform variables are intitialised to 0 when a program links

    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (success == GL_FALSE)
    {
        TRACELOG(TraceLogLevel::Warning, "SHADER: [ID %i] Failed to link compute shader program", program);

        int maxLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

        if (maxLength > 0)
        {
            int length = 0;
            std::string log(0, maxLength);
            glGetProgramInfoLog(program, maxLength, &length, log.data());
            TRACELOG(TraceLogLevel::Warning, "SHADER: [ID %i] Link error: %s", program, log.c_str());
        }

        glDeleteProgram(program);

        program = 0;
    }
    else
    {
        // Get the size of compiled shader program (not available on OpenGL ES 2.0)
        // NOTE: If GL_LINK_STATUS is GL_FALSE, program binary length is zero.
        //GLint binarySize = 0;
        //glGetProgramiv(id, GL_PROGRAM_BINARY_LENGTH, &binarySize);

        TRACELOG(TraceLogLevel::Info, "SHADER: [ID %i] Compute shader program loaded successfully", program);
    }
#endif

    return program;
}

// Dispatch compute shader (equivalent to *draw* for graphics pilepine)
void Context::ComputeShaderDispatch(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
#if defined(GRAPHICS_API_OPENGL_43)
    glDispatchCompute(groupX, groupY, groupZ);
#endif
}

// Load shader storage buffer object (SSBO)
uint32_t Context::LoadShaderBuffer(uint32_t size, const void *data, BufferUsage usageHint)
{
    uint32_t ssbo = 0;

#if defined(GRAPHICS_API_OPENGL_43)
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, static_cast<int>(usageHint));
    if (data == nullptr) glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);    // Clear buffer data to 0
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
#endif

    return ssbo;
}

// Unload shader storage buffer object (SSBO)
void Context::UnloadShaderBuffer(uint32_t ssboId)
{
#if defined(GRAPHICS_API_OPENGL_43)
    glDeleteBuffers(1, &ssboId);
#endif
}

// Update SSBO buffer data
void Context::UpdateShaderBuffer(uint32_t id, const void *data, uint32_t dataSize, uint32_t offset)
{
#if defined(GRAPHICS_API_OPENGL_43)
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, dataSize, data);
#endif
}

// Get SSBO buffer size
uint32_t Context::GetShaderBufferSize(uint32_t id)
{
    long long size = 0;

#if defined(GRAPHICS_API_OPENGL_43)
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
    glGetInteger64v(GL_SHADER_STORAGE_BUFFER_SIZE, &size);
#endif

    return (size > 0)? (uint32_t)size : 0;
}

// Read SSBO buffer data (GPU->CPU)
void Context::ReadShaderBuffer(uint32_t id, void *dest, uint32_t count, uint32_t offset)
{
#if defined(GRAPHICS_API_OPENGL_43)
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, count, dest);
#endif
}

// Bind SSBO buffer
void Context::BindShaderBuffer(uint32_t id, uint32_t index)
{
#if defined(GRAPHICS_API_OPENGL_43)
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, id);
#endif
}

// Copy SSBO buffer data
void Context::CopyShaderBuffer(uint32_t destId, uint32_t srcId, uint32_t destOffset, uint32_t srcOffset, uint32_t count)
{
#if defined(GRAPHICS_API_OPENGL_43)
    glBindBuffer(GL_COPY_READ_BUFFER, srcId);
    glBindBuffer(GL_COPY_WRITE_BUFFER, destId);
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, srcOffset, destOffset, count);
#endif
}

// Bind image texture
void Context::BindImageTexture(uint32_t id, uint32_t index, int format, bool readonly)
{
#if defined(GRAPHICS_API_OPENGL_43)
    uint32_t glInternalFormat = 0, glFormat = 0, glType = 0;

    GetGlTextureFormats(format, &glInternalFormat, &glFormat, &glType);
    glBindImageTexture(index, id, 0, 0, 0, readonly? GL_READ_ONLY : GL_READ_WRITE, glInternalFormat);
#endif
}


// Matrix state management
//-----------------------------------------------------------------------------------------
// Get internal modelview matrix
Matrix Context::GetMatrixModelview()
{
    Matrix matrix = Matrix::Identity();

#if defined(GRAPHICS_API_OPENGL_11)
    float mat[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, mat);
    matrix.m0 = mat[0];
    matrix.m1 = mat[1];
    matrix.m2 = mat[2];
    matrix.m3 = mat[3];
    matrix.m4 = mat[4];
    matrix.m5 = mat[5];
    matrix.m6 = mat[6];
    matrix.m7 = mat[7];
    matrix.m8 = mat[8];
    matrix.m9 = mat[9];
    matrix.m10 = mat[10];
    matrix.m11 = mat[11];
    matrix.m12 = mat[12];
    matrix.m13 = mat[13];
    matrix.m14 = mat[14];
    matrix.m15 = mat[15];
#else
    matrix = State.modelview;
#endif

    return matrix;
}

// Get internal projection matrix
Matrix Context::GetMatrixProjection()
{
#if defined(GRAPHICS_API_OPENGL_11)
    float mat[16];
    glGetFloatv(GL_PROJECTION_MATRIX,mat);
    Matrix m;
    m.m0 = mat[0];
    m.m1 = mat[1];
    m.m2 = mat[2];
    m.m3 = mat[3];
    m.m4 = mat[4];
    m.m5 = mat[5];
    m.m6 = mat[6];
    m.m7 = mat[7];
    m.m8 = mat[8];
    m.m9 = mat[9];
    m.m10 = mat[10];
    m.m11 = mat[11];
    m.m12 = mat[12];
    m.m13 = mat[13];
    m.m14 = mat[14];
    m.m15 = mat[15];
    return m;
#else
    return State.projection;
#endif
}

// Get internal accumulated transform matrix
Matrix Context::GetMatrixTransform()
{
    Matrix mat = Matrix::Identity();
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    // TODO: Consider possible transform matrices in the State.stack
    // Is this the right order? or should we start with the first stored matrix instead of the last one?
    //Matrix matStackTransform = Matrix::Identity();
    //for (int i = State.stackCounter; i > 0; i--) matStackTransform = State.stack[i] * matStackTransform;
    mat = State.transform;
#endif
    return mat;
}

// Get internal projection matrix for stereo render (selected eye)
Matrix Context::GetMatrixProjectionStereo(int eye)
{
    Matrix mat = Matrix::Identity();
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    mat = State.projectionStereo[eye];
#endif
    return mat;
}

// Get internal view offset matrix for stereo render (selected eye)
Matrix Context::GetMatrixViewOffsetStereo(int eye)
{
    Matrix mat = Matrix::Identity();
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    mat = State.viewOffsetStereo[eye];
#endif
    return mat;
}

// Set a custom modelview matrix (replaces internal modelview matrix)
void Context::SetMatrixModelview(Matrix view)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    State.modelview = view;
#endif
}

// Set a custom projection matrix (replaces internal projection matrix)
void Context::SetMatrixProjection(Matrix projection)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    State.projection = projection;
#endif
}

// Set eyes projection matrices for stereo rendering
void Context::SetMatrixProjectionStereo(Matrix right, Matrix left)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    State.projectionStereo[0] = right;
    State.projectionStereo[1] = left;
#endif
}

// Set eyes view offsets matrices for stereo rendering
void Context::SetMatrixViewOffsetStereo(Matrix right, Matrix left)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    State.viewOffsetStereo[0] = right;
    State.viewOffsetStereo[1] = left;
#endif
}

// Load and draw a quad in NDC
void Context::LoadDrawQuad()
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    uint32_t quadVAO = 0;
    uint32_t quadVBO = 0;

    float vertices[] = {
         // Positions         Texcoords
        -1.0f,  1.0f, 0.0f,   0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f,   0.0f, 0.0f,
         1.0f,  1.0f, 0.0f,   1.0f, 1.0f,
         1.0f, -1.0f, 0.0f,   1.0f, 0.0f,
    };

    // Gen VAO to contain VBO
    glGenVertexArrays(1, &quadVAO);
    glBindVertexArray(quadVAO);

    // Gen and fill vertex buffer (VBO)
    glGenBuffers(1, &quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

    // Bind vertex attributes (position, texcoords)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void *)0); // Positions
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void *)(3*sizeof(float))); // Texcoords

    // Draw quad
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    // Delete buffers (VBO and VAO)
    glDeleteBuffers(1, &quadVBO);
    glDeleteVertexArrays(1, &quadVAO);
#endif
}

// Load and draw a cube in NDC
void Context::LoadDrawCube()
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    uint32_t cubeVAO = 0;
    uint32_t cubeVBO = 0;

    float vertices[] = {
         // Positions          Normals               Texcoords
        -1.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f,   0.0f, 0.0f,
         1.0f,  1.0f, -1.0f,   0.0f,  0.0f, -1.0f,   1.0f, 1.0f,
         1.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f,   1.0f, 0.0f,
         1.0f,  1.0f, -1.0f,   0.0f,  0.0f, -1.0f,   1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f,   0.0f, 0.0f,
        -1.0f,  1.0f, -1.0f,   0.0f,  0.0f, -1.0f,   0.0f, 1.0f,
        -1.0f, -1.0f,  1.0f,   0.0f,  0.0f,  1.0f,   0.0f, 0.0f,
         1.0f, -1.0f,  1.0f,   0.0f,  0.0f,  1.0f,   1.0f, 0.0f,
         1.0f,  1.0f,  1.0f,   0.0f,  0.0f,  1.0f,   1.0f, 1.0f,
         1.0f,  1.0f,  1.0f,   0.0f,  0.0f,  1.0f,   1.0f, 1.0f,
        -1.0f,  1.0f,  1.0f,   0.0f,  0.0f,  1.0f,   0.0f, 1.0f,
        -1.0f, -1.0f,  1.0f,   0.0f,  0.0f,  1.0f,   0.0f, 0.0f,
        -1.0f,  1.0f,  1.0f,  -1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
        -1.0f,  1.0f, -1.0f,  -1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,  -1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,  -1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
        -1.0f, -1.0f,  1.0f,  -1.0f,  0.0f,  0.0f,   0.0f, 0.0f,
        -1.0f,  1.0f,  1.0f,  -1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
         1.0f,  1.0f,  1.0f,   1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
         1.0f, -1.0f, -1.0f,   1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
         1.0f,  1.0f, -1.0f,   1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
         1.0f, -1.0f, -1.0f,   1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
         1.0f,  1.0f,  1.0f,   1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
         1.0f, -1.0f,  1.0f,   1.0f,  0.0f,  0.0f,   0.0f, 0.0f,
        -1.0f, -1.0f, -1.0f,   0.0f, -1.0f,  0.0f,   0.0f, 1.0f,
         1.0f, -1.0f, -1.0f,   0.0f, -1.0f,  0.0f,   1.0f, 1.0f,
         1.0f, -1.0f,  1.0f,   0.0f, -1.0f,  0.0f,   1.0f, 0.0f,
         1.0f, -1.0f,  1.0f,   0.0f, -1.0f,  0.0f,   1.0f, 0.0f,
        -1.0f, -1.0f,  1.0f,   0.0f, -1.0f,  0.0f,   0.0f, 0.0f,
        -1.0f, -1.0f, -1.0f,   0.0f, -1.0f,  0.0f,   0.0f, 1.0f,
        -1.0f,  1.0f, -1.0f,   0.0f,  1.0f,  0.0f,   0.0f, 1.0f,
         1.0f,  1.0f,  1.0f,   0.0f,  1.0f,  0.0f,   1.0f, 0.0f,
         1.0f,  1.0f, -1.0f,   0.0f,  1.0f,  0.0f,   1.0f, 1.0f,
         1.0f,  1.0f,  1.0f,   0.0f,  1.0f,  0.0f,   1.0f, 0.0f,
        -1.0f,  1.0f, -1.0f,   0.0f,  1.0f,  0.0f,   0.0f, 1.0f,
        -1.0f,  1.0f,  1.0f,   0.0f,  1.0f,  0.0f,   0.0f, 0.0f
    };

    // Gen VAO to contain VBO
    glGenVertexArrays(1, &cubeVAO);
    glBindVertexArray(cubeVAO);

    // Gen and fill vertex buffer (VBO)
    glGenBuffers(1, &cubeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Bind vertex attributes (position, normals, texcoords)
    glBindVertexArray(cubeVAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *)0); // Positions
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *)(3*sizeof(float))); // Normals
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *)(6*sizeof(float))); // Texcoords
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Draw cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    // Delete VBO and VAO
    glDeleteBuffers(1, &cubeVBO);
    glDeleteVertexArrays(1, &cubeVAO);
#endif
}

// Get name string for pixel format
const char *Context::GetPixelFormatName(PixelFormat format)
{
    switch (format)
    {
        case PixelFormat::Grayscale:        return "GRAYSCALE"; break;      ///< 8 bit per pixel (no alpha)
        case PixelFormat::GrayAlpha:        return "GRAY_ALPHA"; break;     ///< 8*2 bpp (2 channels)
        case PixelFormat::R5G6B5:           return "R5G6B5"; break;         ///< 16 bpp
        case PixelFormat::R8G8B8:           return "R8G8B8"; break;         ///< 24 bpp
        case PixelFormat::R5G5B5A1:         return "R5G5B5A1"; break;       ///< 16 bpp (1 bit alpha)
        case PixelFormat::R4G4B4A4:         return "R4G4B4A4"; break;       ///< 16 bpp (4 bit alpha)
        case PixelFormat::R8G8B8A8:         return "R8G8B8A8"; break;       ///< 32 bpp
        case PixelFormat::R32:              return "R32"; break;            ///< 32 bpp (1 channel - float)
        case PixelFormat::R32G32B32:        return "R32G32B32"; break;      ///< 32*3 bpp (3 channels - float)
        case PixelFormat::R32G32B32A32:     return "R32G32B32A32"; break;   ///< 32*4 bpp (4 channels - float)
        case PixelFormat::R16:              return "R16"; break;            ///< 16 bpp (1 channel - half float)
        case PixelFormat::R16G16B16:        return "R16G16B16"; break;      ///< 16*3 bpp (3 channels - half float)
        case PixelFormat::R16G16B16A16:     return "R16G16B16A16"; break;   ///< 16*4 bpp (4 channels - half float)
        case PixelFormat::DXT1_RGB:         return "DXT1_RGB"; break;       ///< 4 bpp (no alpha)
        case PixelFormat::DXT1_RGBA:        return "DXT1_RGBA"; break;      ///< 4 bpp (1 bit alpha)
        case PixelFormat::DXT3_RGBA:        return "DXT3_RGBA"; break;      ///< 8 bpp
        case PixelFormat::DXT5_RGBA:        return "DXT5_RGBA"; break;      ///< 8 bpp
        case PixelFormat::ETC1_RGB:         return "ETC1_RGB"; break;       ///< 4 bpp
        case PixelFormat::ETC2_RGB:         return "ETC2_RGB"; break;       ///< 4 bpp
        case PixelFormat::ETC2_EAC_RGBA:    return "ETC2_RGBA"; break;      ///< 8 bpp
        case PixelFormat::PVRT_RGB:         return "PVRT_RGB"; break;       ///< 4 bpp
        case PixelFormat::PVRT_RGBA:        return "PVRT_RGBA"; break;      ///< 4 bpp
        case PixelFormat::ASTC_4x4_RGBA:    return "ASTC_4x4_RGBA"; break;  ///< 8 bpp
        case PixelFormat::ASTC_8x8_RGBA:    return "ASTC_8x8_RGBA"; break;  ///< 2 bpp
        default:                            return "UNKNOWN"; break;
    }
}

//----------------------------------------------------------------------------------
// Module specific Functions Definition
//----------------------------------------------------------------------------------
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
// Load default shader (just vertex positioning and texture coloring)
// NOTE: This shader program is used for internal buffers
// NOTE: Loaded: State.defaultShaderId, State.defaultShaderLocs
void Context::LoadShaderDefault()
{
    State.defaultShaderLocs = new int[RL_MAX_SHADER_LOCATIONS];

    // NOTE: All locations must be reseted to -1 (no location)
    for (int i = 0; i < RL_MAX_SHADER_LOCATIONS; i++)
    {
        State.defaultShaderLocs[i] = -1;
    }

    // Vertex shader directly defined, no external file required
    const char *defaultVShaderCode =
#if defined(GRAPHICS_API_OPENGL_21)
    "#version 120                       \n"
    "attribute vec3 vertexPosition;     \n"
    "attribute vec2 vertexTexCoord;     \n"
    "attribute vec4 vertexColor;        \n"
    "varying vec2 fragTexCoord;         \n"
    "varying vec4 fragColor;            \n"
#elif defined(GRAPHICS_API_OPENGL_33)
    "#version 330                       \n"
    "in vec3 vertexPosition;            \n"
    "in vec2 vertexTexCoord;            \n"
    "in vec4 vertexColor;               \n"
    "out vec2 fragTexCoord;             \n"
    "out vec4 fragColor;                \n"
#endif
#if defined(GRAPHICS_API_OPENGL_ES2)
    "#version 100                       \n"
    "precision mediump float;           \n"     // Precision required for OpenGL ES2 (WebGL) (on some browsers)
    "attribute vec3 vertexPosition;     \n"
    "attribute vec2 vertexTexCoord;     \n"
    "attribute vec4 vertexColor;        \n"
    "varying vec2 fragTexCoord;         \n"
    "varying vec4 fragColor;            \n"
#endif
    "uniform mat4 mvp;                  \n"
    "void main()                        \n"
    "{                                  \n"
    "    fragTexCoord = vertexTexCoord; \n"
    "    fragColor = vertexColor;       \n"
    "    gl_Position = mvp*vec4(vertexPosition, 1.0); \n"
    "}                                  \n";

    // Fragment shader directly defined, no external file required
    const char *defaultFShaderCode =
#if defined(GRAPHICS_API_OPENGL_21)
    "#version 120                       \n"
    "varying vec2 fragTexCoord;         \n"
    "varying vec4 fragColor;            \n"
    "uniform sampler2D texture0;        \n"
    "uniform vec4 colDiffuse;           \n"
    "void main()                        \n"
    "{                                  \n"
    "    vec4 texelColor = texture2D(texture0, fragTexCoord); \n"
    "    gl_FragColor = texelColor*colDiffuse*fragColor;      \n"
    "}                                  \n";
#elif defined(GRAPHICS_API_OPENGL_33)
    "#version 330       \n"
    "in vec2 fragTexCoord;              \n"
    "in vec4 fragColor;                 \n"
    "out vec4 finalColor;               \n"
    "uniform sampler2D texture0;        \n"
    "uniform vec4 colDiffuse;           \n"
    "void main()                        \n"
    "{                                  \n"
    "    vec4 texelColor = texture(texture0, fragTexCoord);   \n"
    "    finalColor = texelColor*colDiffuse*fragColor;        \n"
    "}                                  \n";
#endif
#if defined(GRAPHICS_API_OPENGL_ES2)
    "#version 100                       \n"
    "precision mediump float;           \n"     // Precision required for OpenGL ES2 (WebGL)
    "varying vec2 fragTexCoord;         \n"
    "varying vec4 fragColor;            \n"
    "uniform sampler2D texture0;        \n"
    "uniform vec4 colDiffuse;           \n"
    "void main()                        \n"
    "{                                  \n"
    "    vec4 texelColor = texture2D(texture0, fragTexCoord); \n"
    "    gl_FragColor = texelColor*colDiffuse*fragColor;      \n"
    "}                                  \n";
#endif

    // NOTE: Compiled vertex/fragment shaders are not deleted,
    // they are kept for re-use as default shaders in case some shader loading fails
    State.defaultVShaderId = CompileShader(defaultVShaderCode, GL_VERTEX_SHADER);     // Compile default vertex shader
    State.defaultFShaderId = CompileShader(defaultFShaderCode, GL_FRAGMENT_SHADER);   // Compile default fragment shader

    State.defaultShaderId = LoadShaderProgram(State.defaultVShaderId, State.defaultFShaderId);

    if (State.defaultShaderId > 0)
    {
        TRACELOG(RL_LOG_INFO, "SHADER: [ID %i] Default shader loaded successfully", State.defaultShaderId);

        // Set default shader locations: attributes locations
        State.defaultShaderLocs[static_cast<int>(ShaderLocationIndex::VertexPosition)] = glGetAttribLocation(State.defaultShaderId, "vertexPosition");
        State.defaultShaderLocs[static_cast<int>(ShaderLocationIndex::VertexTexCoord01)] = glGetAttribLocation(State.defaultShaderId, "vertexTexCoord");
        State.defaultShaderLocs[static_cast<int>(ShaderLocationIndex::VertexColor)] = glGetAttribLocation(State.defaultShaderId, "vertexColor");

        // Set default shader locations: uniform locations
        State.defaultShaderLocs[static_cast<int>(ShaderLocationIndex::MatrixMVP)]  = glGetUniformLocation(State.defaultShaderId, "mvp");
        State.defaultShaderLocs[static_cast<int>(ShaderLocationIndex::ColorDiffuse)] = glGetUniformLocation(State.defaultShaderId, "colDiffuse");
        State.defaultShaderLocs[static_cast<int>(ShaderLocationIndex::MapDiffuse)] = glGetUniformLocation(State.defaultShaderId, "texture0");
    }
    else TRACELOG(RL_LOG_WARNING, "SHADER: [ID %i] Failed to load default shader", State.defaultShaderId);
}

// Unload default shader
// NOTE: Unloads: State.defaultShaderId, State.defaultShaderLocs
void Context::UnloadShaderDefault()
{
    glUseProgram(0);

    glDetachShader(State.defaultShaderId, State.defaultVShaderId);
    glDetachShader(State.defaultShaderId, State.defaultFShaderId);
    glDeleteShader(State.defaultVShaderId);
    glDeleteShader(State.defaultFShaderId);

    glDeleteProgram(State.defaultShaderId);

    delete[] State.defaultShaderLocs;

    TRACELOG(RL_LOG_INFO, "SHADER: [ID %i] Default shader unloaded successfully", State.defaultShaderId);
}

#endif  // GRAPHICS_API_OPENGL_33 || GRAPHICS_API_OPENGL_ES2
