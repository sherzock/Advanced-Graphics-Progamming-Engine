//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#include "platform.h"
#include <glad/glad.h>

typedef glm::vec2  vec2;
typedef glm::vec3  vec3;
typedef glm::quat  quat;
typedef glm::vec4  vec4;
typedef glm::ivec2 ivec2;
typedef glm::ivec3 ivec3;
typedef glm::ivec4 ivec4;

struct Image
{ 
    void* pixels;
    ivec2 size;
    i32   nchannels;
    i32   stride;
};

struct Texture
{
    GLuint      handle;
    std::string filepath;
};

enum Mode
{
    Mode_TexturedQuad,
    Mode_TexturedMesh,
    Mode_Count
};
enum Modes
{
    Mode_Color,
    Mode_Depth,
    Mode_Albedo,
    Mode_Normal
};

struct OpenGLInfo
{
    std::string GLVers;
    std::string GLRender;
    std::string GLVendor;
    std::string GLSLVersion;
    std::vector<std::string> GLExtensions;
};

struct VertexV3V2
{
    glm::vec3 pos;
    glm::vec2 uv;
};

const VertexV3V2 vertices[] = {
    {glm::vec3(-0.5, -0.5, 0.0), glm::vec2(0.0, 0.0) }, //bottom-left vertex
    {glm::vec3(0.5, -0.5, 0.0), glm::vec2(1.0, 0.0) }, //bottom-right vertex
    {glm::vec3(0.5, 0.5, 0.0), glm::vec2(1.0, 1.0) }, //top-right vertex
    {glm::vec3(-0.5, 0.5, 0.0), glm::vec2(0.0, 1.0) }, //top-left vertex
};

const u16 indices[] = {
    0, 1, 2,
    0, 2, 3
};

struct VertexBufferAttribute
{
    u8 location;
    u8 componentCount;
    u8 offset;
};

struct VertexBufferLayout
{
    std::vector<VertexBufferAttribute> attributes;
    u8                                 stride;
};

struct VertexShaderAttribute
{
    u8 location;
    u8 componentCount;
};

struct VertexShaderLayout
{
    std::vector<VertexShaderAttribute> attributes;
};

struct Vao
{
    GLuint handle;
    GLuint programHandle;
};

struct Submesh
{
    VertexBufferLayout vertexBufferLayout;
    std::vector<float> vertices;
    std::vector<u32>   indices;
    u32                vertexOffset;
    u32                indexOffset;

    std::vector<Vao>   vaos;
};

struct Mesh
{
    std::vector<Submesh> submeshes;
    GLuint               vertexBufferHandle;
    GLuint               indexBufferHandle;
};

struct Material
{
    std::string name;
    vec3        albedo;
    vec3        emissive;
    f32         smoothness;
    u32         albedoTextureIdx;
    u32         emissiveTextureIdx;
    u32         specularTextureIdx;
    u32         normalsTextureIdx;
    u32         bumpTextureIdx;
};

struct Model
{
    u32              meshIdx;
    std::vector<u32> materialIdx;
};

struct Program
{
    GLuint             handle;
    std::string        filepath;
    std::string        programName;
    u64                lastWriteTimestamp; // What is this for?
    VertexShaderLayout vertexInputLayout;
};

struct Camera
{
    float y;//yaw
    float p;//pitch
    glm::vec3 position;
    glm::vec3 forward;
    glm::vec3 right;
    glm::vec3 speed;
};

enum class GOType
{
    NONE,
    ENTITY,
    LIGHT
};

struct GameObject
{
    GameObject(std::string _name, unsigned int _id, unsigned int _index, GOType _type, glm::mat4* matrix = nullptr) {
        name = _name, id = _id, index = _index, type = _type; modelMatrix = matrix;
    }

    std::string name;
    unsigned int id;
    unsigned int index;
    GOType type;
    glm::mat4* modelMatrix;
};

struct Entity
{
    unsigned int id;
    glm::mat4   worldMatrix;
    u32         modelIndex;
    u32         localParamsOffset;
    u32         localParamsSize;
};

struct Buffer
{
    GLuint      handle;
    GLenum      type;
    u32         size;
    u32         head;
    void*       data; //mapped data
};

enum LightType
{
    LightType_Directional,
    LightType_Point
};

struct Light
{
    unsigned int id;
    LightType   type;
    vec3        color;
    vec3        direction;
    vec3        position;
};


struct App
{
    //OpenGL info
    OpenGLInfo info;
    GameObject* active_gameObject;

    // Loop
    f32  deltaTime;
    bool isRunning;

    // Input
    Input input;

    // Graphics
    char gpuName[64];
    char openGlVersion[64];

    ivec2 displaySize;
    ivec2 displaySizeLastFrame;

    std::vector<Texture>  textures;
    std::vector<Material> materials;
    std::vector<Mesh>     meshes;
    std::vector<Model>    models;
    std::vector<Program>  programs;
    std::vector<Entity>   entities;
    std::vector<Light>    lights;
    std::vector<GameObject> gameObjects;

    // program indices
    u32 texturedGeometryProgramIdx;
    u32 texturedMeshProgramIdx;

    // texture indices
    u32 diceTexIdx;
    u32 whiteTexIdx;
    u32 blackTexIdx;
    u32 normalTexIdx;
    u32 magentaTexIdx;

    // Model indices
    u32 model;
    u32 plane;

    // Mode
    Mode mode;
    Modes modes;

    // Location of the texture uniform in the textured quad shader
    GLuint programUniformTexture;

    // VAO object to link our screen filling quad with our textured quad shader
    GLuint vao;

    //Camera
    Camera cam;

    //Buffers
    Buffer vertexBuff;
    Buffer elementBuff;
    Buffer uniformBuff;

    //Uniforms
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 modl;

    //Uniform buffers parameters
    GLint maxUniformBufferSize;
    GLint uniformBlockAlignment;

    //Global params
    u32 GlobalParamsOffset;
    u32 GlobalParamsSize;

    //Framebuffer
    GLuint framebufferHandle;

    //framebuffer Attachments
    GLuint depthAttachmentHandle;
    GLuint colorTexHandle;
    GLuint normalTexhandle;
    GLuint albedoTexhandle;
    GLuint depthTexhandle;

    glm::vec3 vposition;
    glm::vec3 vrotation;
    glm::vec3 vscale;

    //imgui stuff
    const char* rmodes[4] = { "color","depth","albedo","normals" };
    int selectedmode = 0;

};

void Init(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);

void GetTrasform(App* app, glm::mat4 matrix);

void CreateHierarchy(App* app, GameObject* parent);

u32 LoadTexture2D(App* app, const char* filepath);

glm::mat4 TransformScale(const vec3& scaleFactors);

glm::mat4 TransformPositionScale(const vec3& pos,const vec3& scaleFactors);

void FrameBufferObject(App* app);



