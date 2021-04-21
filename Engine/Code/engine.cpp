//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include "engine.h"
#include "assimp.h"
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>

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


void Init(App* app)
{
    if (GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 3))
    {
        glDebugMessageCallback(OnGlError, app);
    }

    //VBO Initialization
    glGenBuffers(1, &app->embeddedVertices);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &app->embeddedElements);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    //EBO Initialization
    glGenVertexArrays(1, &app->vao);
    glBindVertexArray(app->vao);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)12);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    glBindVertexArray(0);

    //Program 1 Initialization
    app->texturedGeometryProgramIdx = LoadProgram(app, "shaders.glsl", "TEXTURED_GEOMETRY");
    Program& texturedGeometryProgram = app->programs[app->texturedGeometryProgramIdx];
    app->programUniformTexture = glGetUniformLocation(texturedGeometryProgram.handle, "uTexture");

    //Program 2 Initialization
    app->texturedMeshProgramIdx = LoadProgram(app, "shaders.glsl", "SHOW_TEXTURED_MESH");
    Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
    

    //Load model patrick
    app->model = LoadModel(app, "Patrick/Patrick.obj");


    //Texture Initialization
    /*app->diceTexIdx = LoadTexture2D(app, "dice.png");
    app->whiteTexIdx = LoadTexture2D(app, "color_white.png");
    app->blackTexIdx = LoadTexture2D(app, "color_black.png");
    app->normalTexIdx = LoadTexture2D(app, "color_normal.png");
    app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");*/

    app->mode = Mode_TexturedMesh;

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
    camera.position = glm::vec3(0.0, 0.0, 10.0);

    //Uniform buffers parameters
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxUniformBufferSize);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBlockAlignment);

    glGenBuffers(1, &app->uniformBufferHandle);
    glBindBuffer(GL_UNIFORM_BUFFER, app->uniformBufferHandle);
    glBufferData(GL_UNIFORM_BUFFER, app->maxUniformBufferSize, NULL, GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    //Entities Creation
    Entity patrick1;
    patrick1.worldMatrix = TransformPositionScale({ 5.0, 0.0, 0.0 }, {1.0,1.0,1.0});
    patrick1.modelIndex = app->model;
    app->entities.push_back(patrick1);

    Entity patrick2;
    patrick2.worldMatrix = TransformPositionScale({ 0.0, 0.0, 0.0 }, { 1.0,1.0,1.0 });
    patrick2.modelIndex = app->model;
    app->entities.push_back(patrick2);

    Entity patrick3;
    patrick3.worldMatrix = TransformPositionScale({ -5.0, 0.0, 0.0 }, { 2.0,2.0,2.0 });
    patrick3.modelIndex = app->model;
    app->entities.push_back(patrick3);
}

void Gui(App* app)
{
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

u32 Align(u32 value, u32 alignment)
{
    return(value + alignment - 1) & ~(alignment - 1);
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
    
    
    if (app->input.mouseButtons[RIGHT] == BUTTON_PRESSED)
    {
        c.y += app->input.mouseDelta.x * TAU / 360.0f;
        c.p -= app->input.mouseDelta.y * TAU / 360.0f;
    }
    
    c.y = glm::mod(c.y, TAU);
    c.p = glm::clamp(c.p, -PI / 2.1f, PI / 2.1f);
    c.forward = glm::vec3(cosf(c.p) * sinf(c.y), sinf(c.p), -cosf(c.p) * cosf(c.y));
    c.right = glm::vec3(cosf(c.y), 0.0f, sinf(c.y));

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

    glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);
    float aspectRatio = (float)app->displaySize.x / (float)app->displaySize.y;
    
    app->projection = glm::perspective(glm::radians(60.0f), aspectRatio, 0.1f, 1000.0f);
    app->view = glm::lookAt(c.position, c.position + c.forward, upVector);
    app->modl = glm::mat4(1.0f);

    //Uniform Buffer update
    glBindBuffer(GL_UNIFORM_BUFFER, app->uniformBufferHandle);
    u8* bufferData = (u8*)glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    u32 bufferHead = 0;

    for (int i = 0; i < app->entities.size(); ++i)
    {

        bufferHead = Align(bufferHead, app->uniformBlockAlignment);

        app->entities[i].localParamsOffset = bufferHead;


        memcpy(bufferData + bufferHead, glm::value_ptr(app->entities[i].worldMatrix), sizeof(glm::mat4));
        bufferHead += sizeof(glm::mat4);

        memcpy(bufferData + bufferHead, glm::value_ptr(app->view), sizeof(glm::mat4));
        bufferHead += sizeof(glm::mat4);

        memcpy(bufferData + bufferHead, glm::value_ptr(app->projection), sizeof(glm::mat4));
        bufferHead += sizeof(glm::mat4);

        app->entities[i].localParamsSize = bufferHead - app->entities[i].localParamsOffset;
    }
    glUnmapBuffer(GL_UNIFORM_BUFFER);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Render(App* app)
{

    // Clear the framebuffer
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set the viewport
    glViewport(0, 0, app->displaySize.x, app->displaySize.y);

    switch (app->mode)
    {
        case Mode_TexturedQuad:
            {
                glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Rendered Texture Quad");
                // Bind the program
                Program& texturedGeometryProgram = app->programs[app->texturedGeometryProgramIdx];
                glUseProgram(texturedGeometryProgram.handle);

                // Bind the vao       
                glBindVertexArray(app->vao);

                // Set the blending state
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                // Bind the texture into unit 0
                glActiveTexture(GL_TEXTURE0);

                GLuint textureHandle = app->textures[app->diceTexIdx].handle;
                glBindTexture(GL_TEXTURE_2D, textureHandle);

                // Draw elements
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

                glPopDebugGroup();

            }
            break;

        case Mode_TexturedMesh:
        {
            glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Rendered Mesh");

            glEnable(GL_DEPTH_TEST);

            

            for (int i = 0; i < app->entities.size(); ++i)
            {

                // Bind the program
                Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
                glUseProgram(texturedMeshProgram.handle);

                //Send Uniforms

                glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->uniformBufferHandle, app->entities[i].localParamsOffset, app->entities[i].localParamsSize);

                Model& model = app->models[app->entities[i].modelIndex];
                Mesh& mesh = app->meshes[model.meshIdx];

                for (u32 i = 0; i < mesh.submeshes.size(); ++i)
                {
                    GLuint vao = FindVAO(mesh, i, texturedMeshProgram);
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


            glPopDebugGroup();
        }
        break;

        
    }

    glBindVertexArray(0);
    glUseProgram(0);
}