#include "mesh_shader_pipeline.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

std::unique_ptr<GLSLProgram> debug430;
std::vector<glm::vec3> positions{
    glm::vec3(1, 2, 3),
    glm::vec3(4, 5, 6),
    glm::vec3(7, 8, 9),
    glm::vec3(10, 11, 12),
};

GLuint vertexBufferIn;
GLuint vertexBufferOut;

MeshShaderPipeline::MeshShaderPipeline(const Options& options) : Application(options) {
    checkGLErrors();

    _dirLight.reset(new DirectionalLight);
    _dirLight->intensity = 0.5f;
    _dirLight->transform.rotation =
        glm::angleAxis(glm::radians(45.0f), glm::normalize(glm::vec3(-1.0f)));


    // init camera
    _camera.reset(new PerspectiveCamera(
        glm::radians(50.0f), 1.0f * _windowWidth / _windowHeight, 0.1f, 1000.0f));
    _camera->transform.position.z = 10.0f;

    checkGLErrors();

    _model.reset(new Model(getAssetFullPath("obj/bunny.obj")));

    checkGLErrors();

    _models[0].reset(new experimental::Model(getAssetFullPath("obj/bunny.obj")));
    //_models[0].reset(new experimental::Model(getAssetFullPath("obj/lod/horse.obj")));

    initPrograms();
    initComputeDebug();

    checkGLErrors();

    // init imGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(_window, true);
    ImGui_ImplOpenGL3_Init();

    checkGLErrors();
}

void MeshShaderPipeline::handleInput() {
    constexpr float cameraMoveSpeed{ 10.0f };

    if (_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
        glfwSetWindowShouldClose(_window, true);
        return;
    }

    if (_input.keyboard.keyStates[GLFW_KEY_W] != GLFW_RELEASE) {
        _camera->transform.position +=
            _camera->transform.getFront() * cameraMoveSpeed * _deltaTime;
    }

    if (_input.keyboard.keyStates[GLFW_KEY_A] != GLFW_RELEASE) {
        _camera->transform.position -=
            _camera->transform.getRight() * cameraMoveSpeed * _deltaTime;
    }

    if (_input.keyboard.keyStates[GLFW_KEY_S] != GLFW_RELEASE) {
        _camera->transform.position -=
            _camera->transform.getFront() * cameraMoveSpeed * _deltaTime;
    }

    if (_input.keyboard.keyStates[GLFW_KEY_D] != GLFW_RELEASE) {
        _camera->transform.position +=
            _camera->transform.getRight() * cameraMoveSpeed * _deltaTime;
    }
}

void MeshShaderPipeline::renderFrame() {
    //debug430->use();
    //glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertexBufferIn);
    //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertexBufferIn);
    //glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertexBufferOut);
    //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vertexBufferOut);
    //glDispatchCompute(1, 1, 1);

    glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    switch (_renderCase) {
    case RenderCase::Traditional:
        renderTraditional();
        break;
    case RenderCase::Triangle:
        renderTriangle();
        break;
    case RenderCase::Meshlet:
        renderMeshlet();
        break;
    }

    renderUI();
}

void MeshShaderPipeline::initPrograms() {
    initTraditionalProgram();
    initTriangleProgram();
    initMeshletProgram();
    initTaskProgram();
    initInstanceProgram();
    initCullProgram();
    initLodProgram();
    initCullLodProgram();
}

void MeshShaderPipeline::initTraditionalProgram() {
    const char* vsCode = R"(
        #version 330 core
        layout(location = 0) in vec3 aPosition;
        layout(location = 1) in vec3 aNormal;
        layout(location = 2) in vec2 aTexCoord;

        out vec3 fPosition;
        out vec3 fNormal;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main() {
            fPosition = vec3(model * vec4(aPosition, 1.0f));
            fNormal = mat3(transpose(inverse(model))) * aNormal;
            gl_Position = projection * view * model * vec4(aPosition, 1.0f);
        };
    )";

    const char* fsCode = R"(
        #version 330 core
        in vec3 fPosition;
        in vec3 fNormal;
        out vec4 color;

        struct Material {
            vec3 kd;
        };

        struct DirectionalLight {
            vec3 direction;
            float intensity;
            vec3 color;
        };

        uniform Material material;
        uniform DirectionalLight directionalLight;

        vec3 calcDirectionalLight(vec3 normal) {
            vec3 lightDir = normalize(-directionalLight.direction);
            vec3 diffuse = directionalLight.color * max(dot(lightDir, normal), 0.0f) * material.kd;
            return directionalLight.intensity * diffuse ;
        }

        void main() {
            vec3 normal = normalize(fNormal);
            vec3 diffuse = calcDirectionalLight(normal);
            color = vec4(diffuse, 1.0f);
        };
    )";

    _traditionalProgram.reset(new GLSLProgram);
    _traditionalProgram->attachVertexShader(vsCode);
    _traditionalProgram->attachFragmentShader(fsCode);
    _traditionalProgram->link();
}

void MeshShaderPipeline::initComputeDebug() {
    char const* csCode = R"(
        #version 430 core

        layout(local_size_x = 4) in;

        layout(std430, binding = 0) readonly buffer PositionBuffer { vec3 positions[]; };
        layout(std430, binding = 1) writeonly buffer OutBuffer { vec3 outs[]; };

        void main() {
            uint tid = gl_LocalInvocationID.x;
            outs[tid] = positions[tid];
        }
    )";

    debug430.reset(new GLSLProgram);
    debug430->attachComputeShader(csCode);
    debug430->link();

    glGenBuffers(1, &vertexBufferIn);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertexBufferIn);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
        positions.size() * sizeof(glm::vec3), positions.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


    glGenBuffers(1, &vertexBufferOut);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertexBufferOut);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
        positions.size() * sizeof(glm::vec3), nullptr, GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void MeshShaderPipeline::initTriangleProgram() {
    char const* msCode = R"(
        #version 450 core
        #extension GL_NV_mesh_shader : require

        layout(local_size_x = 1) in;
        layout(triangles, max_vertices = 3, max_primitives = 1) out;

        layout(location = 0) out PerVertexData {
            vec3 color;
        } vOut[];

        const vec3 vertices[3] = {
            vec3(-1, -1, 0),
            vec3( 0,  1, 0),
            vec3( 1, -1, 0)
        };

        const vec3 colors[3] = {
            vec3(1.0, 0.0, 0.0),
            vec3(0.0, 1.0, 0.0),
            vec3(0.0, 0.0, 1.0)
        };

        void main() {
            // position for rasterization
            gl_MeshVerticesNV[0].gl_Position = vec4(vertices[0], 1.0); 
            gl_MeshVerticesNV[1].gl_Position = vec4(vertices[1], 1.0); 
            gl_MeshVerticesNV[2].gl_Position = vec4(vertices[2], 1.0); 

            // color for fragment shader
            vOut[0].color = colors[0];
            vOut[1].color = colors[1];
            vOut[2].color = colors[2];

            // indices for rasterization
            gl_PrimitiveIndicesNV[0] = 0;
            gl_PrimitiveIndicesNV[1] = 1;
            gl_PrimitiveIndicesNV[2] = 2;

            // primitive count
            gl_PrimitiveCountNV = 1;
        }
    )";

    char const* fsCode = R"(
        #version 450 core

        out vec4 fragColor;
        
        layout(location = 0) in PerVertexData {
            vec3 color;
        } fragIn;

        void main() {
            fragColor = vec4(fragIn.color, 1.0);
        }
    )";

    _triangleProgram.reset(new GLSLProgram);
    _triangleProgram->attachMeshShader(msCode);
    _triangleProgram->attachFragmentShader(fsCode);
    _triangleProgram->link();
}

void MeshShaderPipeline::initMeshletProgram() {
#if !DEBUG
    char const* msCode = R"(
        #version 450 core
        #extension GL_NV_mesh_shader : require
        #extension GL_NV_gpu_shader5 : require

        #define WORK_GROUP_SIZE    32
        #define MAX_VERTICES       64
        #define MAX_PRIMITIVES    126

        #define VERTEX_ITERATION_COUNT ((MAX_VERTICES + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE)
        #define PRIMITIVE_ITERATION_COUNT ((MAX_PRIMITIVES + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE)

        layout(local_size_x = WORK_GROUP_SIZE) in;
        layout(triangles, max_vertices = MAX_VERTICES, max_primitives = MAX_PRIMITIVES) out;

        layout(location = 0) out PerVertexData {
            vec3 color;
        } vOut[];

        struct Vertex {
            vec3 position;
            float u;
            vec3 normal;
            float v;
        };

        struct Meshlet {
            uint vertexCount;
            uint vertexOffset;
            uint primitiveCount;
            uint primitiveOffset;
        };

        layout(std430, binding = 0) readonly buffer VertexBuffer { Vertex vertices[]; };
        //layout(std430, binding = 0) readonly buffer PositionBuffer { vec3 positions[]; };
        layout(std430, binding = 3) readonly buffer VertexIndicesBuffer { uint vertexIndices[]; };
        layout(std430, binding = 4) readonly buffer PrimitiveIndicesBuffer { uint8_t primitiveIndices[]; };
        layout(std430, binding = 5) readonly buffer MeshletBuffer { Meshlet meshlets[]; };

        uniform mat4 projection;
        uniform mat4 view;
        uniform mat4 model;

        void main() {
            uint gid = gl_WorkGroupID.x;        // blockIdx.x
            uint tid = gl_LocalInvocationID.x;  // threadIdx.x
            Meshlet m = meshlets[gid];

            // vertices
            for (uint i = 0; i < VERTEX_ITERATION_COUNT; ++i) {
                uint index = tid + i * WORK_GROUP_SIZE;
                if (index < m.vertexCount) {
                    uint vertexIndex = vertexIndices[m.vertexOffset + index];
                    gl_MeshVerticesNV[index].gl_Position = 
                        projection * view * model * vec4(vertices[vertexIndex].position, 1.0);
                    //gl_MeshVerticesNV[index].gl_Position = 
                    //    projection * view * model * vec4(positions[vertexIndex], 1.0);
                    vOut[index].color = vec3(
                        float(gid & 1), 
                        float(gid & 3) / 4, 
                        float(gid & 7) / 8);
                }
            }
            
            // indices
            for (uint i = 0; i < PRIMITIVE_ITERATION_COUNT; ++i) {
                uint index = 3 * (tid + i * WORK_GROUP_SIZE);
                if (index < 3 * m.primitiveCount) {
                    gl_PrimitiveIndicesNV[index + 0] = primitiveIndices[m.primitiveOffset + index + 0];
                    gl_PrimitiveIndicesNV[index + 1] = primitiveIndices[m.primitiveOffset + index + 1];
                    gl_PrimitiveIndicesNV[index + 2] = primitiveIndices[m.primitiveOffset + index + 2];
                }
            }

            // primitive count
            if (tid == 0) {
                gl_PrimitiveCountNV = m.primitiveCount;
            }
        }
    )";

    char const* fsCode = R"(
        #version 450 core

        out vec4 fragColor;

        layout(location = 0) in PerVertexData {
            vec3 color;
        } fragIn;

        void main() {
            fragColor = vec4(fragIn.color, 1.0);
        }
    )";

#else
    char const* msCode = R"(
        #version 450 core
        #extension GL_NV_mesh_shader : require
        layout(local_size_x = 3) in;
        layout(triangles, max_vertices = 3, max_primitives = 1) out;

        struct Vertex {
            vec3 position;
            float u;
            vec3 normal;
            float v;
        };

        // layout(std430, binding = 0) readonly buffer VertexBuffer { Vertex vertices[]; };
        layout(std430, binding = 0) readonly buffer PositionBuffer { vec3 positions[]; };

        void main() {
            uint tid = gl_LocalInvocationID.x;

            gl_MeshVerticesNV[tid].gl_Position = vec4(positions[tid], 1.0);
            // gl_MeshVerticesNV[tid].gl_Position = vec4(vertices[tid].position, 1.0);

            gl_PrimitiveIndicesNV[tid] = tid;

            gl_PrimitiveCountNV = 1;
        }
    )";

    char const* fsCode = R"(
        #version 450 core
        out vec4 fragColor;
        void main() {
            fragColor = vec4(1.0);
        }
    )";
#endif

    _meshletProgram.reset(new GLSLProgram);
    _meshletProgram->attachMeshShader(msCode);
    _meshletProgram->attachFragmentShader(fsCode);
    _meshletProgram->link();
}

void MeshShaderPipeline::initTaskProgram() {
    //_taskProgram.reset(new GLSLProgram);
    //_taskProgram->attachMeshShader(meshVsRelPath));
    //_taskProgram->attachFragmentShader(meshFsRelPath));
    //_taskProgram->link();
}

void MeshShaderPipeline::initInstanceProgram() {
    //_instanceProgram.reset(new GLSLProgram);
    //_instanceProgram->attachMeshShader(meshVsRelPath));
    //_instanceProgram->attachFragmentShader(meshFsRelPath));
    //_instanceProgram->link();
}

void MeshShaderPipeline::initCullProgram() {
    //_cullProgram.reset(new GLSLProgram);
    //_cullProgram->attachMeshShader(meshVsRelPath));
    //_cullProgram->attachFragmentShader(meshFsRelPath));
    //_cullProgram->link();
}

void MeshShaderPipeline::initLodProgram() {
    //_lodProgram.reset(new GLSLProgram);
    //_lodProgram->attachMeshShader(meshVsRelPath));
    //_lodProgram->attachFragmentShader(meshFsRelPath));
    //_lodProgram->link();
}

void MeshShaderPipeline::initCullLodProgram() {
    //_cullLodProgram.reset(new GLSLProgram);
    //_cullLodProgram->attachMeshShader(meshVsRelPath));
    //_cullLodProgram->attachFragmentShader(meshFsRelPath));
    //_cullLodProgram->link();
}

void MeshShaderPipeline::renderTraditional() {
    _traditionalProgram->use();
    _traditionalProgram->setUniformMat4("projection", _camera->getProjectionMatrix());
    _traditionalProgram->setUniformMat4("view", _camera->getViewMatrix());
    _traditionalProgram->setUniformMat4("model", _model->transform.getLocalMatrix());
    _traditionalProgram->setUniformVec3("material.kd", glm::vec3(0.8f));
    _traditionalProgram->setUniformVec3(
        "directionalLight.direction", _dirLight->transform.getFront());
    _traditionalProgram->setUniformFloat("directionalLight.intensity", _dirLight->intensity);
    _traditionalProgram->setUniformVec3("directionalLight.color", _dirLight->color);
    
    _model->draw();
}

void MeshShaderPipeline::renderTriangle() {
    _triangleProgram->use();
    glDrawMeshTasksNV(0, 1);
}

void MeshShaderPipeline::renderMeshlet() {
    _meshletProgram->use();

#if !DEBUG
    _meshletProgram->setUniformMat4("projection", _camera->getProjectionMatrix());
    _meshletProgram->setUniformMat4("view", _camera->getViewMatrix());
    _meshletProgram->setUniformMat4("model", _models[0]->transform.getLocalMatrix());
#else
    _meshletProgram->setUniformMat4("projection", glm::mat4(1.0f));
    _meshletProgram->setUniformMat4("view", glm::mat4(1.0f));
    _meshletProgram->setUniformMat4("model", glm::mat4(1.0f));
#endif

    _models[0]->draw();
}

void MeshShaderPipeline::renderUI() {
    // draw ui elements
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    //ImGui::ShowDemoWindow();

    const auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin("Control Panel", nullptr, flags)) {
        ImGui::End();
    }
    else {
        const char* items[] = {
            "Traditional",
            "Triangle",
            "Meshlet",
            "Task",
            "Instance",
            "Cull",
            "Lod",
            "Full"
        };

        if (ImGui::BeginCombo("Render Mode", items[_renderCase])) {
            for (int i = 0; i < IM_ARRAYSIZE(items); ++i) {
                bool selected{ static_cast<RenderCase>(i) == _renderCase };
                if (ImGui::Selectable(items[i], &selected)) {
                    _renderCase = static_cast<RenderCase>(i);
                }

                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}