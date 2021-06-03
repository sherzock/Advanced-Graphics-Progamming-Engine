//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include "engine.h"
#include "assimp.h"
#include "buffer_management.h"
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

#define BINDING(b) b


GLuint CreateProgramFromSource(String programSource, const char* shaderName)
{
    GLchar  infoLogBuffer[1024] = {};
    GLsizei infoLogBufferSize = sizeof(infoLogBuffer);
    GLsizei infoLogSize;
    GLint   success;

    char versionString[] = "#version 430\n";
    char shaderNameDefine[128];
    sprintf(shaderNameDefine, "#define %s\n", shaderName);
    char vertexShaderDefine[] = "#define VERTEX\n";
    char fragmentShaderDefine[] = "#define FRAGMENT\n";

    const GLchar* vertexShaderSource[] = {
        versionString,
        shaderNameDefine,
        vertexShaderDefine,
        programSource.str
    };
    const GLint vertexShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(vertexShaderDefine),
        (GLint) programSource.len
    };
    const GLchar* fragmentShaderSource[] = {
        versionString,
        shaderNameDefine,
        fragmentShaderDefine,
        programSource.str
    };
    const GLint fragmentShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(fragmentShaderDefine),
        (GLint) programSource.len
    };

    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, ARRAY_COUNT(vertexShaderSource), vertexShaderSource, vertexShaderLengths);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with vertex shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, ARRAY_COUNT(fragmentShaderSource), fragmentShaderSource, fragmentShaderLengths);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with fragment shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint programHandle = glCreateProgram();
    glAttachShader(programHandle, vshader);
    glAttachShader(programHandle, fshader);
    glLinkProgram(programHandle);
    glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programHandle, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glLinkProgram() failed with program %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    glUseProgram(0);

    glDetachShader(programHandle, vshader);
    glDetachShader(programHandle, fshader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);

    return programHandle;
}

u32 LoadProgram(App* app, const char* filepath, const char* programName)
{
    String programSource = ReadTextFile(filepath);

    Program program = {};
    program.handle = CreateProgramFromSource(programSource, programName);
    program.filepath = filepath;
    program.programName = programName;
    program.lastWriteTimestamp = GetFileLastWriteTimestamp(filepath);
    GLint attributeCount = 0;
    glGetProgramiv(program.handle, GL_ACTIVE_ATTRIBUTES, &attributeCount);

    for (int i = 0; i < attributeCount; ++i)
    {
        GLint size;
        GLenum type;
        const GLsizei bufSize = 16;
        GLchar name[bufSize];
        GLsizei length;

        glGetActiveAttrib(program.handle, (GLuint)i, bufSize, &length, &size, &type, name);
        u8 attributeLocation = glGetAttribLocation(program.handle, name);
        program.vertexInputLayout.attributes.push_back({attributeLocation, (u8)size});
    }

    app->programs.push_back(program);


    return app->programs.size() - 1;
}

Image LoadImage(const char* filename)
{
    Image img = {};
    stbi_set_flip_vertically_on_load(true);
    img.pixels = stbi_load(filename, &img.size.x, &img.size.y, &img.nchannels, 0);
    if (img.pixels)
    {
        img.stride = img.size.x * img.nchannels;
    }
    else
    {
        ELOG("Could not open file %s", filename);
    }
    return img;
}

void FreeImage(Image image)
{
    stbi_image_free(image.pixels);
}

GLuint CreateTexture2DFromImage(Image image)
{
    GLenum internalFormat = GL_RGB8;
    GLenum dataFormat     = GL_RGB;
    GLenum dataType       = GL_UNSIGNED_BYTE;

    switch (image.nchannels)
    {
        case 3: dataFormat = GL_RGB; internalFormat = GL_RGB8; break;
        case 4: dataFormat = GL_RGBA; internalFormat = GL_RGBA8; break;
        default: ELOG("LoadTexture2D() - Unsupported number of channels");
    }

    GLuint texHandle;
    glGenTextures(1, &texHandle);
    glBindTexture(GL_TEXTURE_2D, texHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.size.x, image.size.y, 0, dataFormat, dataType, image.pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texHandle;
}

u32 LoadTexture2D(App* app, const char* filepath)
{
    for (u32 texIdx = 0; texIdx < app->textures.size(); ++texIdx)
        if (app->textures[texIdx].filepath == filepath)
            return texIdx;

    Image image = LoadImage(filepath);

    if (image.pixels)
    {
        Texture tex = {};
        tex.handle = CreateTexture2DFromImage(image);
        tex.filepath = filepath;

        u32 texIdx = app->textures.size();
        app->textures.push_back(tex);

        FreeImage(image);
        return texIdx;
    }
    else
    {
        return UINT32_MAX;
    }
}

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program)
{
    Submesh& submesh = mesh.submeshes[submeshIndex];

    // Try finding a vao for this submesh/program
    for (u32 i = 0; i < (u32)submesh.vaos.size(); ++i)
    {
        if (submesh.vaos[i].programHandle == program.handle)
            return submesh.vaos[i].handle;
    }

    // Create a new vao for 
    GLuint vaoHandle = 0;
    glGenVertexArrays(1, &vaoHandle);
    glBindVertexArray(vaoHandle);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);

    // We have to link all vertex inputs attributes to attributes in the vertex buffer
    for (u32 i = 0; i < program.vertexInputLayout.attributes.size(); ++i)
    {
        bool attributeWasLinked = false;

        for (u32 j = 0; j < submesh.vertexBufferLayout.attributes.size(); ++j)
        {
            if (program.vertexInputLayout.attributes[i].location == submesh.vertexBufferLayout.attributes[j].location)
            {
                const u32 index = submesh.vertexBufferLayout.attributes[j].location;
                const u32 ncomp = submesh.vertexBufferLayout.attributes[j].componentCount;
                const u32 offset = submesh.vertexBufferLayout.attributes[j].offset + submesh.vertexOffset; // attribute offset + vertex offset
                const u32 stride = submesh.vertexBufferLayout.stride;
                glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)(u64)offset);
                glEnableVertexAttribArray(index);

                attributeWasLinked = true;
                break;
            }
        }

        assert(attributeWasLinked); // The submesh should provide an attribute for each vertex inputs
    }

    glBindVertexArray(0);

    // Store it in the list of vaos for this submesh
    Vao vao = { vaoHandle, program.handle };
    submesh.vaos.push_back(vao);

    return vaoHandle;
}

void OnGlError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
        return;

    ELOG("OpenGL debug message: %s", message);

    switch (source)
    {
    case GL_DEBUG_SOURCE_API:             ELOG(" - source: GL_DEBUG_SOURCE_API"); break; // Calls to the OpenGL API
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   ELOG(" - source: GL_DEBUG_SOURCE_WINDOW_SYSTEM"); break; // Calls to a window-system API
    case GL_DEBUG_SOURCE_SHADER_COMPILER: ELOG(" - source: GL_DEBUG_SOURCE_SHADER_COMPILER"); break; // A compiler for a shading language
    case GL_DEBUG_SOURCE_THIRD_PARTY:     ELOG(" - source: GL_DEBUG_SOURCE_THIRD_PARTY"); break; // An application associated with OpenGL
    case GL_DEBUG_SOURCE_APPLICATION:     ELOG(" - source: GL_DEBUG_SOURCE_APPLICATION"); break; // Generated by the user of this application
    case GL_DEBUG_SOURCE_OTHER:           ELOG(" - source: GL_DEBUG_SOURCE_OTHER"); break; // Some source that isn't one of these
    }

    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:               ELOG(" - type: GL_DEBUG_TYPE_ERROR"); break; // An error, typically from the API
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: ELOG(" - type: GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR"); break; // Some behavior marked deprecated has been used
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  ELOG(" - type: GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR"); break; // Something has invoked undefined behavior
    case GL_DEBUG_TYPE_PORTABILITY:         ELOG(" - type: GL_DEBUG_TYPE_PORTABILITY"); break; // Some functionality the user relies upon is not portable
    case GL_DEBUG_TYPE_PERFORMANCE:         ELOG(" - type: GL_DEBUG_TYPE_PERFORMANCE"); break; // Code has triggered possible performance issues
    case GL_DEBUG_TYPE_MARKER:              ELOG(" - type: GL_DEBUG_TYPE_MARKER"); break; // Command stream annotation
    case GL_DEBUG_TYPE_PUSH_GROUP:          ELOG(" - type: GL_DEBUG_TYPE_PUSH_GROUP"); break; // Group pushing
    case GL_DEBUG_TYPE_POP_GROUP:           ELOG(" - type: GL_DEBUG_TYPE_POP_GROUP"); break; // foo
    case GL_DEBUG_TYPE_OTHER:               ELOG(" - type: GL_DEBUG_TYPE_OTHER"); break; // Some type that isn't one of these
    }

    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:         ELOG(" - severity: GL_DEBUG_SEVERITY_HIGH"); break; // All OpenGL Errors, shader compilation/linking errors, or highly-dangerous undefined behavior
    case GL_DEBUG_SEVERITY_MEDIUM:       ELOG(" - severity: GL_DEBUG_SEVERITY_MEDIUM"); break; // Major performance warnings, shader compilation/linking warnings, or the use of deprecated functionality
    case GL_DEBUG_SEVERITY_LOW:          ELOG(" - severity: GL_DEBUG_SEVERITY_LOW"); break; // Redundant state change performance warning, or unimportant undefined behavior
    case GL_DEBUG_SEVERITY_NOTIFICATION: ELOG(" - severity: GL_DEBUG_SEVERITY_NOTIFICATION"); break; // Anything that isn't an error or performance issue.
    }
}

glm::mat4 TransformScale(const vec3& scaleFactors)
{
    glm::mat4 transform = scale(scaleFactors);
    return transform;
}

glm::mat4 TransformPositionScale(const vec3& pos, const vec3& scaleFactors)
{
    glm::mat4 transform = translate(pos);
    transform = scale(transform, scaleFactors);
    return transform;
}

glm::mat4 TransformPositionRotationScale(const vec3& pos, const vec3& rot, const vec3& scaleFactors)
{
    glm::mat4 transform = translate(pos);
    transform = rotate(transform, rot.x, { 1,0,0 });
    transform = rotate(transform, rot.y, { 0,1,0 });
    transform = rotate(transform, rot.z, { 0,0,1 });
    transform = scale(transform, scaleFactors);
    return transform;
}

//Framebuffer
void BufferTextureInit(GLuint& handle, ivec2 size)
{
    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_2D, handle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void BufferBloomInit(App* app, GLuint& handle, int level) {

    //Bloom FrameBuffer
    glGenFramebuffers(1, &handle);
    glBindFramebuffer(GL_FRAMEBUFFER, handle);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->rtBright, level);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, app->rtBloomH, level);

    //check errors
    GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        switch (framebufferStatus)
        {
        case GL_FRAMEBUFFER_UNDEFINED:                                  ELOG("GL_FRAMEBUFFER_UNDEFINED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:                      ELOG("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:              ELOG("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:                     ELOG("GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:                     ELOG("GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); break;
        case GL_FRAMEBUFFER_UNSUPPORTED:                                ELOG("GL_FRAMEBUFFER_UNSUPPORTED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:                     ELOG("GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:                   ELOG("GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"); break;
        default: ELOG("Unknown framebuffer status error");
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBufferObject(App* app)
{
    //color Texture
    BufferTextureInit(app->colorTexHandle, app->displaySize);
    BufferTextureInit(app->normalTexhandle, app->displaySize);
    BufferTextureInit(app->albedoTexhandle, app->displaySize);
    BufferTextureInit(app->depthTexhandle, app->displaySize);
    BufferTextureInit(app->positionTexhandle, app->displaySize);

    //depth Texture
    glGenTextures(1, &app->depthAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->depthAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    //framebuffer
    glGenFramebuffers(1, &app->framebufferHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, app->framebufferHandle);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->colorTexHandle, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, app->normalTexhandle, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, app->albedoTexhandle, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, app->depthTexhandle, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, app->positionTexhandle, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, app->depthAttachmentHandle, 0);

    //check errors
    GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        switch (framebufferStatus)
        {
            case GL_FRAMEBUFFER_UNDEFINED:                                  ELOG("GL_FRAMEBUFFER_UNDEFINED"); break;
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:                      ELOG("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:              ELOG("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:                     ELOG("GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); break;
            case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:                     ELOG("GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); break;
            case GL_FRAMEBUFFER_UNSUPPORTED:                                ELOG("GL_FRAMEBUFFER_UNSUPPORTED"); break;
            case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:                     ELOG("GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;
            case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:                   ELOG("GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"); break;
            default: ELOG("Unknown framebuffer status error");
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //Bloom Textures
    if (app->rtBright != 0) glDeleteTextures(1, &app->rtBright);
    glGenFramebuffers(1, &app->rtBright);
    glBindTexture(GL_TEXTURE_2D, app->rtBright);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, MIPMAP_BASE_LEVEL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MIPMAP_MAX_LEVEL);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x / 2, app->displaySize.y / 2, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, app->displaySize.x / 4, app->displaySize.y / 4, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexImage2D(GL_TEXTURE_2D, 2, GL_RGBA16F, app->displaySize.x / 8, app->displaySize.y / 8, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexImage2D(GL_TEXTURE_2D, 3, GL_RGBA16F, app->displaySize.x / 16, app->displaySize.y / 16, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexImage2D(GL_TEXTURE_2D, 4, GL_RGBA16F, app->displaySize.x / 32, app->displaySize.y / 32, 0, GL_RGBA, GL_FLOAT, NULL);
    glGenerateMipmap(GL_TEXTURE_2D); 
    glBindTexture(GL_TEXTURE_2D, 0);

    if (app->rtBloomH != 0) glDeleteTextures(1, &app->rtBloomH);
    glGenFramebuffers(1, &app->rtBloomH);
    glBindTexture(GL_TEXTURE_2D, app->rtBloomH);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, MIPMAP_BASE_LEVEL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MIPMAP_MAX_LEVEL);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x / 2, app->displaySize.y / 2, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, app->displaySize.x / 4, app->displaySize.y / 4, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexImage2D(GL_TEXTURE_2D, 2, GL_RGBA16F, app->displaySize.x / 8, app->displaySize.y / 8, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexImage2D(GL_TEXTURE_2D, 3, GL_RGBA16F, app->displaySize.x / 16, app->displaySize.y / 16, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexImage2D(GL_TEXTURE_2D, 4, GL_RGBA16F, app->displaySize.x / 32, app->displaySize.y / 32, 0, GL_RGBA, GL_FLOAT, NULL);
    glGenerateMipmap(GL_TEXTURE_2D); 
    glBindTexture(GL_TEXTURE_2D, 0);

    //Bloom FrameBuffer
    BufferBloomInit(app, app->fboBloom1, 0);
    BufferBloomInit(app, app->fboBloom2, 1);
    BufferBloomInit(app, app->fboBloom3, 2);
    BufferBloomInit(app, app->fboBloom4, 3);
    BufferBloomInit(app, app->fboBloom5, 4);
}

void Init(App* app)
{
    if (GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 3))
    {
        glDebugMessageCallback(OnGlError, app);
    }

    //VBO Initialization
    //Create vertex buffer
    app->vertexBuff = CreateStaticVertexBuffer(sizeof(vertices));

    //Create indices buffer
    app->elementBuff = CreateStaticIndexBuffer(sizeof(indices));

    //EBO Initialization
    glGenVertexArrays(1, &app->vao);
    glBindVertexArray(app->vao);
    BindBuffer(app->vertexBuff);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)12);
    glEnableVertexAttribArray(1);
    BindBuffer(app->elementBuff);
    glBindVertexArray(0);


    //init quad for deferred
    float quadVertices[] =
    {
        // positions        // texture Coords
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };
    // setup plane VAO
    glGenVertexArrays(1, &app->quadVAO);
    glGenBuffers(1, &app->quadVBO);
    glBindVertexArray(app->quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, app->quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    //Program 1 Initialization
    app->texturedGeometryProgramIdx = LoadProgram(app, "shaders.glsl", "TEXTURED_GEOMETRY");
    Program& texturedGeometryProgram = app->programs[app->texturedGeometryProgramIdx];
    //app->programUniformTexture = glGetUniformLocation(texturedGeometryProgram.handle, "uTexture");

    //Program Forward shading Initialization
    app->ForwardShadingIdx = LoadProgram(app, "shaders.glsl", "Mode_ForwardShading");
    Program& texturedMeshProgram = app->programs[app->ForwardShadingIdx];

    //Program Deferred shading Initialization
    app->DeferredGeometryIdx = LoadProgram(app, "shaders.glsl", "Mode_DeferredGeometry");
    app->DeferredLightingIdx = LoadProgram(app, "shaders.glsl", "Mode_DeferredLighting");

    app->blitBrightestPixelsProgramIdx = LoadProgram(app, "shaders.glsl", "Mode_BrightestPixels");
    app->blurIdx = LoadProgram(app, "shaders.glsl", "Mode_Blur");
    app->bloomIdx = LoadProgram(app, "shaders.glsl", "Mode_Bloom");

    //Texture Initialization
    app->whiteTexIdx = LoadTexture2D(app, "color_white.png");
    app->diceTexIdx = LoadTexture2D(app, "dice.png");
    app->blackTexIdx = LoadTexture2D(app, "color_black.png");
    app->normalTexIdx = LoadTexture2D(app, "color_normal.png");
    app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");

    //Load model patrick
    app->model = LoadModel(app, "Patrick/Patrick.obj");
    app->plane = LoadModel(app, "../WorkingDir/Plane.obj");

    //app->plane = LoadPlane(app);

    app->mode = Mode::Mode_DeferredShading;
    app->modes = Modes::Mode_Color;

    app->info.GLVers = (const char *)glGetString(GL_VERSION);
    app->info.GLRender = (const char*)glGetString(GL_RENDERER);
    app->info.GLVendor = (const char*)glGetString(GL_VENDOR);
    app->info.GLSLVersion = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);

    GLint num_extensions;
    glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
    for (int i = 0; i < num_extensions; ++i)
    {
        app->info.GLExtensions.push_back((const char*)glGetStringi(GL_EXTENSIONS, GLuint(i)));
    }

    //Camera stuff
    Camera& camera = app->cam;
    camera.y = 0.0f;
    camera.p = 0.0f;
    camera.position = glm::vec3(0.0, 1.0, 10.0);
    camera.reference = glm::vec3(0.0,0.0,0.0);

    //Framebuffer Init
    FrameBufferObject(app);
    app->displaySizeLastFrame = app->displaySize;

    //Uniform buffers parameters
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxUniformBufferSize);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBlockAlignment);

    //create unifomr buffer
    app->uniformBuff = CreateConstantBuffer(app->maxUniformBufferSize);

    int id = -1;

    //Entities Creation
    Entity plane;
    plane.worldMatrix = TransformPositionScale({ 0.0, 0.0, 0.0 }, { 25.0,1.0,25.0 });
    plane.modelIndex = app->plane;
    plane.id = ++id;
    app->entities.push_back(plane);
    app->gameObjects.push_back(GameObject("Plane", id, app->entities.size() - 1, GOType::ENTITY, &plane.worldMatrix));

    Entity patrick1;
    patrick1.worldMatrix = TransformPositionScale({ 4.3, 4.7, 5.2 }, {1.0,1.0,1.0});
    patrick1.modelIndex = app->model;
    patrick1.id = ++id;
    app->entities.push_back(patrick1);
    app->gameObjects.push_back(GameObject("patrick1", id, app->entities.size() - 1, GOType::ENTITY, &patrick1.worldMatrix));

    Entity patrick2;
    patrick2.worldMatrix = TransformPositionScale({ -3.3, 4.5, 5.0 }, { 1.0,1.0,1.0 });
    patrick2.modelIndex = app->model;
    patrick2.id = ++id;
    app->entities.push_back(patrick2);
    app->gameObjects.push_back(GameObject("patrick2", id, app->entities.size() - 1, GOType::ENTITY, &patrick2.worldMatrix));

    Entity patrick3;
    patrick3.worldMatrix = TransformPositionScale({ 0.0, 6.7, 0.0 }, { 1.5,1.5,1.5 });
    patrick3.modelIndex = app->model;
    patrick3.id = ++id;
    app->entities.push_back(patrick3);
    app->gameObjects.push_back(GameObject("patrick3", id, app->entities.size() - 1, GOType::ENTITY, &patrick3.worldMatrix));

    // lights Creation
    Light light1;
    light1.type = LightType::LightType_Directional;
    light1.direction = vec3(0.0, 1.0, 0.0);
    light1.color = vec3(1.0, 1.0, 1.0);
    light1.position = vec3(0.0, 5.0, 0.0);
    light1.id = ++id;
    app->lights.push_back(light1);
    app->gameObjects.push_back(GameObject("Directional1", id, app->lights.size() - 1, GOType::LIGHT));

    Light light2;
    light2.type = LightType::LightType_Point;
    light2.direction = vec3(50.0, 0.0, 0.0);
    light2.color = vec3(1.0, 1.0, 1.0);
    light2.position = vec3(-1.0, 1.0, -1.5);
    light2.id = ++id;
    app->lights.push_back(light2);
    app->gameObjects.push_back(GameObject("Point1", id, app->lights.size() - 1, GOType::LIGHT));

    Light light3;
    light3.type = LightType::LightType_Point;
    light3.direction = vec3(-50.0, 0.0, 0.0);
    light3.color = vec3(1.0, 1.0, 1.0);
    light3.position = vec3(1.2, 1.1, 1.0);
    light3.id = ++id;
    app->lights.push_back(light3);
    app->gameObjects.push_back(GameObject("Point2", id, app->lights.size() - 1, GOType::LIGHT));

    app->active_gameObject = &app->gameObjects[0];
    GetTrasform(app, *app->active_gameObject->modelMatrix);

    Camera& c = app->cam;

    c.up = vec3(0, 1, 0);
    c.forward = vec3(0, 0, 1);
    c.right = glm::cross(c.up, c.forward);
}


void Gui(App* app)
{
    bool active = true;
    ImGui::Begin("Scene", &active, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PushItemWidth(400);
    ImGui::Combo("Render Type", &app->selectedmodes, app->rmodes, IM_ARRAYSIZE(app->rmodes));
    //ImGui::SameLine();
    ImGui::Combo("Render Mode", &app->selectedmode, app->rmode, IM_ARRAYSIZE(app->rmode));

    switch (app->selectedmodes)
    {
    case 0:
        app->modes = Modes::Mode_Color;
        break;
    case 1:
        app->modes = Modes::Mode_Depth;
        break;
    case 2:
        app->modes = Modes::Mode_Albedo;
        break;
    case 3:
        app->modes = Modes::Mode_Normal;
        break;
    case 4:
        app->modes = Modes::Mode_Position;
        break;
    }

    switch (app->modes)
    {
    case Modes::Mode_Color:
        ImGui::Image((void*)app->colorTexHandle, ImVec2(app->displaySize.x, app->displaySize.y), ImVec2(0, 1), ImVec2(1, 0));
        break;

    case Modes::Mode_Normal:
        ImGui::Image((void*)app->normalTexhandle, ImVec2(app->displaySize.x, app->displaySize.y), ImVec2(0, 1), ImVec2(1, 0));
        break;

    case Modes::Mode_Albedo:
        ImGui::Image((void*)app->albedoTexhandle, ImVec2(app->displaySize.x, app->displaySize.y), ImVec2(0, 1), ImVec2(1, 0));
        break;

    case Modes::Mode_Depth:
        ImGui::Image((void*)app->depthTexhandle, ImVec2(app->displaySize.x, app->displaySize.y), ImVec2(0, 1), ImVec2(1, 0));
        break;
    case Modes::Mode_Position:
        ImGui::Image((void*)app->rtBright, ImVec2(app->displaySize.x, app->displaySize.y), ImVec2(0, 1), ImVec2(1, 0));
        //ImGui::Image((void*)app->positionTexhandle, ImVec2(app->displaySize.x, app->displaySize.y), ImVec2(0, 1), ImVec2(1, 0));
        break;
    }
    switch (app->selectedmode)
    {
    case 0:
        app->mode = Mode::Mode_DeferredShading;
        break;
    case 1:
        app->mode = Mode::Mode_ForwardShading;
        break;
    }


    //ImGui::Image((void*)app->colorTexHandle, ImVec2(app->displaySize.x, app->displaySize.y), ImVec2(0, 1), ImVec2(1, 0));
    ImGui::End();

    ImGui::Begin("Hierarchy");
    for (int i = 0; i < app->gameObjects.size(); i++)
    {
        CreateHierarchy(app, &app->gameObjects[i]);
    }
    ImGui::End();

    ImGui::Begin("Inspector");
    ImGui::SetNextItemWidth(200);
    ImGui::InputText("Name", (char*)app->active_gameObject->name.c_str(), 20);
    if (app->active_gameObject) {
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            vec3 last_position = app->vposition;
            vec3 last_rotation = app->vrotation;
            vec3 last_scale = app->vscale;

            ImGui::Text("Position:");
            ImGui::SameLine(); ImGui::PushItemWidth(60);  ImGui::PushID("pos"); ImGui::DragFloat("X", &app->vposition.x, 0.1f); ImGui::PopID(); ImGui::PopItemWidth();
            ImGui::SameLine(); ImGui::PushItemWidth(60);  ImGui::PushID("pos"); ImGui::DragFloat("Y", &app->vposition.y, 0.1f); ImGui::PopID(); ImGui::PopItemWidth();
            ImGui::SameLine(); ImGui::PushItemWidth(60);  ImGui::PushID("pos"); ImGui::DragFloat("Z", &app->vposition.z, 0.1f); ImGui::PopID(); ImGui::PopItemWidth();

            if (app->active_gameObject->type == GOType::ENTITY) {
                ImGui::Text("Rotation:");
                ImGui::SameLine(); ImGui::PushItemWidth(60); ImGui::PushID("rot"); ImGui::DragFloat("X", &app->vrotation.x, 0.1f); ImGui::PopID(); ImGui::PopItemWidth();
                ImGui::SameLine(); ImGui::PushItemWidth(60); ImGui::PushID("rot");  ImGui::DragFloat("Y", &app->vrotation.y, 0.1f); ImGui::PopID(); ImGui::PopItemWidth();
                ImGui::SameLine(); ImGui::PushItemWidth(60); ImGui::PushID("rot");  ImGui::DragFloat("Z", &app->vrotation.z, 0.1f); ImGui::PopID(); ImGui::PopItemWidth();

                ImGui::Text("Scale:   ");
                ImGui::SameLine(); ImGui::PushItemWidth(60);  ImGui::PushID("scale"); ImGui::DragFloat("X", &app->vscale.x, 0.1f); ImGui::PopID(); ImGui::PopItemWidth();
                ImGui::SameLine(); ImGui::PushItemWidth(60);  ImGui::PushID("scale"); ImGui::DragFloat("Y", &app->vscale.y, 0.1f); ImGui::PopID(); ImGui::PopItemWidth();
                ImGui::SameLine(); ImGui::PushItemWidth(60);  ImGui::PushID("scale"); ImGui::DragFloat("Z", &app->vscale.z, 0.1f); ImGui::PopID(); ImGui::PopItemWidth();
            }
            else if (app->active_gameObject->type == GOType::LIGHT) {
                
                if (app->lights[app->active_gameObject->index].type == LightType::LightType_Directional) {
                    vec3 direction = app->lights[app->active_gameObject->index].direction;

                    ImGui::Text("Direction:");
                    ImGui::SameLine(); ImGui::PushItemWidth(60);  ImGui::PushID("dir"); ImGui::DragFloat("X", &direction.x, 0.1f); ImGui::PopID(); ImGui::PopItemWidth();
                    ImGui::SameLine(); ImGui::PushItemWidth(60);  ImGui::PushID("dir"); ImGui::DragFloat("Y", &direction.y, 0.1f); ImGui::PopID(); ImGui::PopItemWidth();
                    ImGui::SameLine(); ImGui::PushItemWidth(60);  ImGui::PushID("dir"); ImGui::DragFloat("Z", &direction.z, 0.1f); ImGui::PopID(); ImGui::PopItemWidth();

                    if (app->lights[app->active_gameObject->index].direction != direction) {
                        app->lights[app->active_gameObject->index].direction = direction;
                    }
                }

                float color[3] = { app->lights[app->active_gameObject->index].color.r, app->lights[app->active_gameObject->index].color.g, app->lights[app->active_gameObject->index].color.b };
                ImGui::ColorPicker3("color", color);

                app->lights[app->active_gameObject->index].color.r = color[0];
                app->lights[app->active_gameObject->index].color.g = color[1];
                app->lights[app->active_gameObject->index].color.b = color[2];
            }
            
            if (last_position.x != app->vposition.x || last_position.y != app->vposition.y || last_position.z != app->vposition.z ||
                last_rotation.x != app->vrotation.x || last_rotation.y != app->vrotation.y || last_rotation.z != app->vrotation.z ||
                last_scale.x != app->vscale.x || last_scale.y != app->vscale.y || last_scale.z != app->vscale.z)
            {
                if (app->active_gameObject->type == GOType::ENTITY)
                    app->entities[app->active_gameObject->index].worldMatrix = TransformPositionRotationScale(app->vposition, (app->vrotation * 3.14159f) / 180.f, app->vscale);
                else if (app->active_gameObject->type == GOType::LIGHT)
                    app->lights[app->active_gameObject->index].position = app->vposition;
            }
        }
    }

    ImGui::NewLine();
    ImGui::Checkbox("Active Bloom", &app->renderBloom);
    ImGui::DragFloat("Threshold", &app->threshold, 0.01, 0, 1);
    ImGui::DragFloat("Kernel Radius", &app->kernelRadius, 0.01, 0, 1);


    ImGui::End();

    ImGui::Begin("Info");
    ImGui::Text("FPS:");
    ImGui::Text("   %f", 1.0f/app->deltaTime);
    ImGui::Text("OpenGL version:");
    ImGui::Text("   %s", app->info.GLVers.c_str());
    ImGui::Text("OpenGL render:");
    ImGui::Text("   %s", app->info.GLRender.c_str());
    ImGui::Text("OpenGL vendor:");
    ImGui::Text("   %s", app->info.GLVendor.c_str());
    ImGui::Text("OpenGL GLSL version:");
    ImGui::Text("   %s", app->info.GLSLVersion.c_str());
    ImGui::Text("OpenGL extensions:");
    ImGui::BeginChild("Extensions:", { 0, 0 }, false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    for (int i = 0; i < app->info.GLExtensions.size(); i++)
    {
        ImGui::Text("   %s", app->info.GLExtensions[i].c_str());
    }
    ImGui::EndChild();
    ImGui::End();
}

void GetTrasform(App* app, glm::mat4 matrix) {
    glm::quat rotation;
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(matrix, app->vscale, rotation, app->vposition, skew, perspective);

    app->vrotation = (glm::eulerAngles(rotation) * 180.f) / 3.14159f;
}

void GetTrasform(App* app, glm::vec3 position) {
    app->vposition = position;
}

void CreateHierarchy(App* app, GameObject* parent) {

    ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (parent->id == app->active_gameObject->id)
        node_flags |= ImGuiTreeNodeFlags_Selected;
    
    node_flags |= ImGuiTreeNodeFlags_Leaf;

    if (ImGui::TreeNodeEx((void*)(intptr_t)parent->id, node_flags, parent->name.c_str())) {

        if (ImGui::IsItemClicked()) {
            app->active_gameObject = parent;
            if (app->active_gameObject->type == GOType::ENTITY)
            {
                GetTrasform(app, app->entities[app->active_gameObject->index].worldMatrix);
            }
            else if (app->active_gameObject->type == GOType::LIGHT)
            {
                GetTrasform(app, app->lights[app->active_gameObject->index].position);
            }
        }

        ImGui::TreePop();
    }
}


// -----------------------------------------------------------------
void Look(Camera c, const vec3& Position, const vec3& Reference)
{
    c.position = Position;
    c.reference = Reference;

    c.forward = glm::normalize(Reference - Position);
    c.right = glm::normalize(glm::cross(vec3(0.0f, 1.0f, 0.0f), c.forward));
    c.up = glm::cross(c.forward, c.right);

 /*   if (!RotateAroundReference)
    {*/
        //c.reference = c.position;
        //c.position += c.forward * 0.05f;
    /*}*/

}

// -----------------------------------------------------------------
void LookAt(Camera c, const vec3& Spot)
{
    c.reference = Spot;
    vec3 position = { Spot.x,Spot.y,Spot.z };
    Look(c, position, c.reference);
}

void Update(App* app)
{
    // You can handle app->input keyboard/mouse here
    for (u64 i = 0; i < app->programs.size(); ++i)
    {
        Program& program = app->programs[i];
        u64 currentTimestamp = GetFileLastWriteTimestamp(program.filepath.c_str());
        if (currentTimestamp > program.lastWriteTimestamp)
        {
            glDeleteProgram(program.handle);
            String programSource = ReadTextFile(program.filepath.c_str());
            const char* programName = program.programName.c_str();
            program.handle = CreateProgramFromSource(programSource, programName);
            program.lastWriteTimestamp = currentTimestamp;
        }
    }

    // Camera update
    Camera& c = app->cam;
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    if (app->input.mouseButtons[RIGHT] == BUTTON_PRESSED)
    {
        c.y += app->input.mouseDelta.x * TAU / 360.0f;
        c.p -= app->input.mouseDelta.y * TAU / 360.0f;
    
    }
    
    c.y = glm::mod(c.y, TAU);
    c.p = glm::clamp(c.p, -PI / 2.1f, PI / 2.1f);
    c.right = glm::normalize(glm::vec3(cosf(c.y), 0.0f, sinf(c.y)));
    c.forward = glm::normalize(glm::vec3(cosf(c.p) * sinf(c.y), sinf(c.p), -cosf(c.p) * cosf(c.y)));
    c.up = glm::cross(c.right, c.forward);
    app->view = glm::lookAt(c.position, c.position + c.forward, c.up);

    //if (app->input.mouseButtons[LEFT] == BUTTON_PRESSED)
    //{
    //    if (app->input.mouseDelta.x != 0 || app->input.mouseDelta.y != 0) {
    //
    //        int dx = -app->input.mouseDelta.x;
    //        int dy = -app->input.mouseDelta.y;
    //
    //        glm::quat quat_y(c.up.x, c.up.y, c.up.z, dx * 0.0005);
    //        glm::quat quat_x(c.right.x, c.right.y, c.right.z, dy * 0.005);
    //
    //        vec3 vect = c.position - c.reference;
    //        vect = glm::rotate(quat_x, vect);
    //        vect = glm::rotate(quat_y, vect);
    //        c.position = vect + c.reference;
    //
    //        app->view = glm::lookAt(c.position, c.reference, c.up);
    //
    //        c.right = app->view * glm::normalize(vec4(c.right, 0));
    //        c.forward = app->view * glm::normalize(vec4(c.forward, 0));
    //        c.up = glm::cross(c.forward, c.right);
    //
    //        //c.forward = glm::normalize(app->view * vec4(c.forward, 1));
    //        //c.up = glm::normalize(app->view * vec4(c.up, 1));
    //        //c.right = glm::normalize(glm::cross(up, c.right));
    //    }
    //}





    bool acc = false;
    
    if (app->input.keys[K_W] == BUTTON_PRESSED) 
    {
        acc = true; c.speed += c.forward;
    }
    
    if (app->input.keys[K_S] == BUTTON_PRESSED) 
    {
        acc = true; c.speed -= c.forward;
    }
    
    if (app->input.keys[K_D] == BUTTON_PRESSED) 
    {
        acc = true; c.speed += c.right;
    }
    
    if (app->input.keys[K_A] == BUTTON_PRESSED) 
    {
        acc = true; c.speed -= c.right;
    }
    
    if (!acc) { c.speed *= 0.8; }

    if (glm::length(c.speed) > 100.0f) 
    {
        c.speed = 100.0f * glm::normalize(c.speed);
    }
    else if (glm::length(c.speed) < 0.01f) 
    {
        c.speed = glm::vec3(0.0f);
    }

    c.position += c.speed * app->deltaTime;


    
    //glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);
    float aspectRatio = (float)app->displaySize.x / (float)app->displaySize.y;
    
    app->projection = glm::perspective(glm::radians(60.0f), aspectRatio, 0.1f, 1000.0f);
    app->modl = glm::mat4(1.0f);

    //Uniform Buffer update
    MapBuffer(app->uniformBuff, GL_WRITE_ONLY);

    //Global params

    app->GlobalParamsOffset = app->uniformBuff.head;

    PushVec3(app->uniformBuff, app->cam.position);

    PushUInt(app->uniformBuff, app->lights.size());

    for (int i = 0; i < app->lights.size(); ++i)
    {
        AlignHead(app->uniformBuff, sizeof(vec4));

        Light& light = app->lights[i];
        PushUInt(app->uniformBuff, light.type);
        PushVec3(app->uniformBuff, light.color);
        PushVec3(app->uniformBuff, light.direction);
        PushVec3(app->uniformBuff, light.position);
    }

    app->GlobalParamsSize = app->uniformBuff.head - app->GlobalParamsOffset;

    //Local Params
    for (int i = 0; i < app->entities.size(); ++i)
    {

        AlignHead(app->uniformBuff, app->uniformBlockAlignment);

        Entity& entity = app->entities[i];
        glm::mat4        model = entity.worldMatrix;
        glm::mat4        view = app->view;
        glm::mat4        projection = app->projection;

        entity.localParamsOffset = app->uniformBuff.head;
        PushMat4(app->uniformBuff, model);
        PushMat4(app->uniformBuff, view);
        PushMat4(app->uniformBuff, projection);
        entity.localParamsSize = app->uniformBuff.head - entity.localParamsOffset;

    }
    
    UnmapBuffer(app->uniformBuff);

    //framebuffer check if window resize
    if (app->displaySize != app->displaySizeLastFrame)
    {
        FrameBufferObject(app);
        app->displaySizeLastFrame = app->displaySize;
    }
}

void DeferredGeometryPass(App * app)
{
    // Clear the framebuffer
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, app->framebufferHandle);


    //Select on which render targets to draw
    GLuint drawbuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
    glDrawBuffers(ARRAY_COUNT(drawbuffers), drawbuffers);

    // Clear the framebuffer
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, app->displaySize.x, app->displaySize.y);

    // Bind the program
    Program& GeoDeferredShadingProgram = app->programs[app->DeferredGeometryIdx];
    glUseProgram(GeoDeferredShadingProgram.handle);
        
    glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->uniformBuff.handle, app->GlobalParamsOffset, app->GlobalParamsSize);

    for (int i = 0; i < app->entities.size(); ++i)
    {

        Model& model = app->models[app->entities[i].modelIndex];
        Mesh& mesh = app->meshes[model.meshIdx];

        //Send Uniforms
        glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->uniformBuff.handle, app->entities[i].localParamsOffset, app->entities[i].localParamsSize);

        glEnable(GL_DEPTH_TEST);

        for (u32 i = 0; i < mesh.submeshes.size(); ++i)
        {
            GLuint vao = FindVAO(mesh, i, GeoDeferredShadingProgram);
            glBindVertexArray(vao);

            u32 submeshMaterialIdx = model.materialIdx[i];
            Material& submeshMaterial = app->materials[submeshMaterialIdx];

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);
            glUniform1i(app->programUniformTexture, 0);

            // Draw elements
            Submesh& submesh = mesh.submeshes[i];
            glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
        }

    }
}

void DeferredShadingPass(App * app)
{
    //glEnable(GL_DEPTH_TEST);
    //glDepthMask(GL_FALSE);
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_ONE, GL_ONE);

    //Render on this framebuffer render targets
    //glBindFramebuffer(GL_FRAMEBUFFER, app->framebufferHandle);

     // Bind the program
    Program& ShadDeferredShadingProgram = app->programs[app->DeferredLightingIdx];
    glUseProgram(ShadDeferredShadingProgram.handle);

    glUniform1i(glGetUniformLocation(ShadDeferredShadingProgram.handle, "oNormals"), 0);
    glUniform1i(glGetUniformLocation(ShadDeferredShadingProgram.handle, "oAlbedo"), 1);
    glUniform1i(glGetUniformLocation(ShadDeferredShadingProgram.handle, "oDepth"), 2);
    glUniform1i(glGetUniformLocation(ShadDeferredShadingProgram.handle, "oPosition"), 3);
     
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, app->normalTexhandle);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, app->albedoTexhandle);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, app->depthTexhandle);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, app->positionTexhandle);

    //Select on which render targets to draw
    GLuint drawbuffers[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(ARRAY_COUNT(drawbuffers), drawbuffers);
    
    glDepthMask(GL_FALSE); //Send Uniforms
    glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->uniformBuff.handle, app->GlobalParamsOffset, app->GlobalParamsSize);
    
    //quad for deferred
    glBindVertexArray(app->quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE); 
}

void Render(App* app)
{
    GLuint drawBBuffers[] =
    {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
    };

    glBindFramebuffer(GL_FRAMEBUFFER, app->fboBloom1);
    glDrawBuffers(ARRAY_COUNT(drawBBuffers), drawBBuffers);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Clear the framebuffer
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    // Set the viewport
    glViewport(0, 0, app->displaySize.x, app->displaySize.y);

    switch (app->mode)
    {
    case Mode_ForwardShading:
    {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Forward Shading");

        //Render on this framebuffer render targets
        glBindFramebuffer(GL_FRAMEBUFFER, app->framebufferHandle);

        //Select on which render targets to draw
        GLuint drawbuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
        glDrawBuffers(ARRAY_COUNT(drawbuffers), drawbuffers);

        // Clear the framebuffer
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, app->displaySize.x, app->displaySize.y);

        // Bind the program
        Program& ForwardShadingProgram = app->programs[app->ForwardShadingIdx];
        glUseProgram(ForwardShadingProgram.handle);

        for (int i = 0; i < app->entities.size(); ++i)
        {

            Model& model = app->models[app->entities[i].modelIndex];
            Mesh& mesh = app->meshes[model.meshIdx];

            //Send Uniforms
            glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->uniformBuff.handle, app->GlobalParamsOffset, app->GlobalParamsSize);
            glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->uniformBuff.handle, app->entities[i].localParamsOffset, app->entities[i].localParamsSize);

            glEnable(GL_DEPTH_TEST);

            for (u32 i = 0; i < mesh.submeshes.size(); ++i)
            {
                GLuint vao = FindVAO(mesh, i, ForwardShadingProgram);
                glBindVertexArray(vao);

                u32 submeshMaterialIdx = model.materialIdx[i];
                Material& submeshMaterial = app->materials[submeshMaterialIdx];

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);
                glUniform1i(app->programUniformTexture, 0);

                // Draw elements
                Submesh& submesh = mesh.submeshes[i];
                glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
            }

        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glUseProgram(0);
        glPopDebugGroup();
    }
    break;
    case Mode_DeferredShading:
    {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Deferred Shading");

        DeferredGeometryPass(app);
        DeferredShadingPass(app);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if (app->renderBloom) RenderBloom(app);

        Program& programTexturedGeometry = app->programs[app->texturedGeometryProgramIdx];
        glUseProgram(programTexturedGeometry.handle);
        glBindVertexArray(app->vao);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glUniform1i(app->programUniformTexture, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, app->colorTexHandle);

        glUseProgram(0);
        glPopDebugGroup();
    }
    break;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void RenderBloom(App* app) {
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Render Bloom");
#define LOD(x) x
    const vec2 horizontal(1.0, 0.0);
    const vec2 vertical(1.0, 0.0);

    const float w = app->displaySize.x;
    const float h = app->displaySize.y;

    //Copy Bright Pixels
    //app->colorTexHandle == deferred texture resultant

    passBlitBrightPixels(app, app->fboBloom1, vec2(w / 2, h / 2), GL_COLOR_ATTACHMENT0, app->colorTexHandle, LOD(0), app->threshold);
    glBindTexture(GL_TEXTURE_2D, app->rtBright);
    glGenerateMipmap(GL_TEXTURE_2D);

    //Blur
    passBlur(app, app->fboBloom1, vec2(w / 2, h / 2), GL_COLOR_ATTACHMENT1, app->rtBright, LOD(0), horizontal);
    passBlur(app, app->fboBloom2, vec2(w / 4, h / 4), GL_COLOR_ATTACHMENT1, app->rtBright, LOD(1), horizontal);
    passBlur(app, app->fboBloom3, vec2(w / 8, h / 8), GL_COLOR_ATTACHMENT1, app->rtBright, LOD(2), horizontal);
    passBlur(app, app->fboBloom4, vec2(w / 16, h / 16), GL_COLOR_ATTACHMENT1, app->rtBright, LOD(3), horizontal);
    passBlur(app, app->fboBloom5, vec2(w / 32, h / 32), GL_COLOR_ATTACHMENT1, app->rtBright, LOD(4), horizontal);
    
    passBlur(app, app->fboBloom1, vec2(w / 2, h / 2), GL_COLOR_ATTACHMENT0, app->rtBloomH, LOD(0), vertical);
    passBlur(app, app->fboBloom2, vec2(w / 4, h / 4), GL_COLOR_ATTACHMENT0, app->rtBloomH, LOD(1), vertical);
    passBlur(app, app->fboBloom3, vec2(w / 8, h / 8), GL_COLOR_ATTACHMENT0, app->rtBloomH, LOD(2), vertical);
    passBlur(app, app->fboBloom4, vec2(w / 16, h / 16), GL_COLOR_ATTACHMENT0, app->rtBloomH, LOD(3), vertical);
    passBlur(app, app->fboBloom5, vec2(w / 32, h / 32), GL_COLOR_ATTACHMENT0, app->rtBloomH, LOD(4), vertical);
    
    //Apply Blurred Pixels on top of Original
    //passBloom(app, app->framebufferHandle, GL_COLOR_ATTACHMENT0, app->rtBloomH, 5);

#undef LOD
    glPopDebugGroup();
}

void passBlitBrightPixels(App* app, GLuint& fbo, const vec2& size, GLenum attachment, GLuint& inputTexture, GLint LOD, float threshold) 
{
    //Render on this framebuffer render targets
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    //Select on which render targets to draw
    glDrawBuffer(attachment);

    // Clear the framebuffer
    glViewport(0, 0, size.x, size.y);

    Program& BrightestPixelsProgram = app->programs[app->blitBrightestPixelsProgramIdx];
    glUseProgram(BrightestPixelsProgram.handle);

    // Bind the texture into unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glUniform1i(glGetUniformLocation(BrightestPixelsProgram.handle, "colorTexture"), 0);
    glUniform1f(glGetUniformLocation(BrightestPixelsProgram.handle, "threshold"), threshold);

    //DRAW Quad
    glBindVertexArray(app->quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    //Release
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void passBlur(App* app, GLuint& fbo, const vec2& size, int attachment, GLuint& inputTexture, int LOD, vec2 orientation) {
    
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glDrawBuffer(attachment);
    glViewport(0, 0, size.x, size.y);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    Program& BlurProgram = app->programs[app->blurIdx];
    glUseProgram(BlurProgram.handle);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    glUniform1i(glGetUniformLocation(BlurProgram.handle, "colorMap"), 0);
    glUniform1i(glGetUniformLocation(BlurProgram.handle, "inputLod"), LOD);
    glUniform1i(glGetUniformLocation(BlurProgram.handle, "kernelRadius"), app->kernelRadius);
    glUniform2i(glGetUniformLocation(BlurProgram.handle, "direction"), orientation.x, orientation.y);

    //DRAW Quad
    glBindVertexArray(app->quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glEnable(GL_DEPTH_TEST);
}

void passBloom(App* app, GLuint& fbo, int attachment, GLuint& inputTexture, int LOD) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glDrawBuffer(attachment);
    glViewport(0, 0, app->displaySize.x, app->displaySize.y);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    Program& BloomProgram = app->programs[app->bloomIdx];
    glUseProgram(BloomProgram.handle);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    glUniform1i(glGetUniformLocation(BloomProgram.handle, "colorMap"), 0);
    glUniform1i(glGetUniformLocation(BloomProgram.handle, "maxLod"), LOD);
    glUniform1i(glGetUniformLocation(BloomProgram.handle, "lodI0"), app->lodIntensity0);
    glUniform1i(glGetUniformLocation(BloomProgram.handle, "lodI1"), app->lodIntensity1);
    glUniform1i(glGetUniformLocation(BloomProgram.handle, "lodI2"), app->lodIntensity2);
    glUniform1i(glGetUniformLocation(BloomProgram.handle, "lodI3"), app->lodIntensity3);
    glUniform1i(glGetUniformLocation(BloomProgram.handle, "lodI4"), app->lodIntensity4);

    //DRAW Quad
    glBindVertexArray(app->quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBlendFunc(GL_ONE, GL_ZERO);
    glEnable(GL_DEPTH_TEST);
}