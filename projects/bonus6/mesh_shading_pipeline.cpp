#include "mesh_shading_pipeline.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define ENABLE_STATISTICS 0

MeshShadingPipeline::MeshShadingPipeline(const Options& options) : Application(options) {
    _dirLight.reset(new DirectionalLight);
    _dirLight->intensity = 1.0f;
    _dirLight->transform.rotation =
        glm::angleAxis(glm::radians(45.0f), glm::normalize(glm::vec3(-1.0f)));

    _camera.reset(new PerspectiveCamera(
        glm::radians(50.0f), 1.0f * _windowWidth / _windowHeight, 0.1f, 1000.0f));
    _camera->transform.position = glm::vec3(0.000000f, 0.177955f, 0.367840f);
    _camera->transform.rotation = glm::quat(0.995212f, -0.097740f, -0.0f, -0.0f);


    initPrograms();

    initInstanceMatrices();

#if ENABLE_STATISTICS
    initStatistics();
#endif

    loadModel("obj/lod/horse.obj");

    loadMeshletModel("obj/lod/horse.obj");

    loadMeshletModelLod({
        getAssetFullPath("obj/lod/horse.obj"),
        getAssetFullPath("obj/lod/horse_lod1.obj"),
        getAssetFullPath("obj/lod/horse_lod2.obj"),
        getAssetFullPath("obj/lod/horse_lod3.obj"),
        getAssetFullPath("obj/lod/horse_lod4.obj") });

    // init imGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(_window, true);
    ImGui_ImplOpenGL3_Init();

    checkGLErrors();
    
    updateInstanceMatrices();
}

void MeshShadingPipeline::handleInput() {
    if (_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
        glfwSetWindowShouldClose(_window, true);
        return;
    }

    if (_input.keyboard.keyStates[GLFW_KEY_W] != GLFW_RELEASE) {
        _camera->transform.position +=
            _camera->transform.getFront() * _cameraMoveSpeed * _deltaTime;
    }

    if (_input.keyboard.keyStates[GLFW_KEY_A] != GLFW_RELEASE) {
        _camera->transform.position -=
            _camera->transform.getRight() * _cameraMoveSpeed * _deltaTime;
    }

    if (_input.keyboard.keyStates[GLFW_KEY_S] != GLFW_RELEASE) {
        _camera->transform.position -=
            _camera->transform.getFront() * _cameraMoveSpeed * _deltaTime;
    }

    if (_input.keyboard.keyStates[GLFW_KEY_D] != GLFW_RELEASE) {
        _camera->transform.position +=
            _camera->transform.getRight() * _cameraMoveSpeed * _deltaTime;
    }

    if (_input.keyboard.keyStates[GLFW_KEY_Q] != GLFW_RELEASE) {
        _camera->transform.position -=
            _camera->transform.getUp() * _cameraMoveSpeed * _deltaTime;
    }

    if (_input.keyboard.keyStates[GLFW_KEY_E] != GLFW_RELEASE) {
        _camera->transform.position +=
            _camera->transform.getUp() * _cameraMoveSpeed * _deltaTime;
    }

    //updateInstanceMatrices();
#if ENABLE_STATISTICS
    updateStatictics();
#endif
}

void MeshShadingPipeline::renderFrame() {
    showFpsInWindowTitle();

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
        renderMeshlet(true);
        break;
    case RenderCase::Meshlet2:
        renderMeshlet2();
        break;
    case RenderCase::Instance:
        renderInstance();
        break;
    case RenderCase::Cull:
        renderCull();
        break;
    case RenderCase::Lod:
        renderLod();
        break;
    case RenderCase::Full:
        renderFull();
        break;
    }

    renderUI();
}

void MeshShadingPipeline::initPrograms() {
    // traditional program
    _traditionalProgram.reset(new GLSLProgram);
    _traditionalProgram->attachVertexShaderFromFile(
        getAssetFullPath("shader/bonus6/traditional.vert"));
    _traditionalProgram->attachFragmentShaderFromFile(
        getAssetFullPath("shader/bonus6/lambert.frag"));
    _traditionalProgram->link();

    // generate triangle on the fly
    _triangleProgram.reset(new GLSLProgram);
    _triangleProgram->attachMeshShaderFromFile(
        getAssetFullPath("shader/bonus6/triangle.mesh"));
    _triangleProgram->attachFragmentShaderFromFile(
        getAssetFullPath("shader/bonus6/flat_color.frag"));
    _triangleProgram->link();

    // render meshlet model use mesh shader
    _meshletProgram.reset(new GLSLProgram);
    _meshletProgram->attachMeshShaderFromFile(
        getAssetFullPath("shader/bonus6/meshlet.mesh"));
    _meshletProgram->attachFragmentShaderFromFile(
        getAssetFullPath("shader/bonus6/flat_color.frag"));
    _meshletProgram->link();

    // render meshlet model bv use mesh shader
    _meshletBVProgram.reset(new GLSLProgram);
    _meshletBVProgram->attachMeshShaderFromFile(
        getAssetFullPath("shader/bonus6/bv.mesh"));
    _meshletBVProgram->attachFragmentShaderFromFile(
        getAssetFullPath("shader/bonus6/flat_color.frag"));
    _meshletBVProgram->link();

    // render meshlet model use task shader and mesh shader
    _meshlet2Program.reset(new GLSLProgram);
    _meshlet2Program->attachTaskShaderFromFile(
        getAssetFullPath("shader/bonus6/meshlet2.task"));
    _meshlet2Program->attachMeshShaderFromFile(
        getAssetFullPath("shader/bonus6/meshlet2.mesh"));
    _meshlet2Program->attachFragmentShaderFromFile(
        getAssetFullPath("shader/bonus6/flat_color.frag"));
    _meshlet2Program->link();

    // render instanced meshlet model
    _instanceProgram.reset(new GLSLProgram);
    _instanceProgram->attachTaskShaderFromFile(
        getAssetFullPath("shader/bonus6/instance.task"));
    _instanceProgram->attachMeshShaderFromFile(
        getAssetFullPath("shader/bonus6/instance.mesh"));
    _instanceProgram->attachFragmentShaderFromFile(
        getAssetFullPath("shader/bonus6/flat_color.frag"));
    _instanceProgram->link();

    // frustum culling with instanced meshlet model
    _cullProgram.reset(new GLSLProgram);
    _cullProgram->attachTaskShaderFromFile(
        getAssetFullPath("shader/bonus6/cull.task"));
    _cullProgram->attachMeshShaderFromFile(
        getAssetFullPath("shader/bonus6/cull.mesh"));
    _cullProgram->attachFragmentShaderFromFile(
        getAssetFullPath("shader/bonus6/flat_color.frag"));
    _cullProgram->link();

    // level of detail with instanced meshlet model
    _lodProgram.reset(new GLSLProgram);
    _lodProgram->attachTaskShaderFromFile(
        getAssetFullPath("shader/bonus6/lod.task"));
    _lodProgram->attachMeshShaderFromFile(
        getAssetFullPath("shader/bonus6/lod.mesh"));
    _lodProgram->attachFragmentShaderFromFile(
        getAssetFullPath("shader/bonus6/flat_color.frag"));
    _lodProgram->link();

    // frustum culling and level of detail with instanced meshlet model
    _fullProgram.reset(new GLSLProgram);
    _fullProgram->attachTaskShaderFromFile(
        getAssetFullPath("shader/bonus6/full.task"));
    _fullProgram->attachMeshShaderFromFile(
        getAssetFullPath("shader/bonus6/full.mesh"));
    _fullProgram->attachFragmentShaderFromFile(
        getAssetFullPath("shader/bonus6/lambert.frag"));
    _fullProgram->link();
}

void MeshShadingPipeline::loadModel(const std::string& filepath) {
    _model.reset(new Model(getAssetFullPath(filepath)));

    glBindVertexArray(_model->getVao());
    glBindBuffer(GL_ARRAY_BUFFER, _ssboInstanceMatricesBuffer->getNativeHandle());

    constexpr GLsizei stride = sizeof(glm::mat4);
    constexpr GLsizei unitSize = sizeof(glm::vec4);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void*)(0 * unitSize));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, (void*)(1 * unitSize));
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, stride, (void*)(2 * unitSize));
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, stride, (void*)(3 * unitSize));

    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);
    glVertexAttribDivisor(6, 1);

    glBindVertexArray(0);
}

void MeshShadingPipeline::loadMeshletModel(const std::string& filepath) {
    _meshletModel.reset(new MeshletModel(getAssetFullPath(filepath)));

    // + vertex data
    _ssboVerticesBuffer.reset(new ShaderStorageBuffer);
    _ssboVerticesBuffer->bind();
    _ssboVerticesBuffer->upload(GL_STATIC_DRAW,
        _meshletModel->getVertices().size() * sizeof(MeshletModel::Vertex),
        _meshletModel->getVertices().data());
    _ssboVerticesBuffer->unbind();

    // + vertex indices
    _ssboVertexIndicesBuffer.reset(new ShaderStorageBuffer);
    _ssboVertexIndicesBuffer->bind();
    _ssboVertexIndicesBuffer->upload(GL_STATIC_DRAW,
        _meshletModel->getVertexIndices().size() * sizeof(uint32_t),
        _meshletModel->getVertexIndices().data());
    _ssboVertexIndicesBuffer->unbind();

    // + primitive indices
    _ssboPrimitiveIndicesBuffer.reset(new ShaderStorageBuffer);
    _ssboPrimitiveIndicesBuffer->bind();
    _ssboPrimitiveIndicesBuffer->upload(GL_STATIC_DRAW,
        _meshletModel->getPrimitiveIndices().size() * sizeof(uint8_t),
        _meshletModel->getPrimitiveIndices().data());
    _ssboPrimitiveIndicesBuffer->unbind();

    // + meshlet
    _ssboMeshletBuffer.reset(new ShaderStorageBuffer);
    _ssboMeshletBuffer->bind();
    _ssboMeshletBuffer->upload(GL_STATIC_DRAW,
        _meshletModel->getMeshlets().size() * sizeof(MeshletModel::Meshlet),
        _meshletModel->getMeshlets().data());
    _ssboMeshletBuffer->unbind();

    // + meshlet BV
    _ssboMeshletBVBuffer.reset(new ShaderStorageBuffer);
    _ssboMeshletBVBuffer->bind();
    _ssboMeshletBVBuffer->upload(GL_STATIC_DRAW,
        _meshletModel->getMeshletBVs().size() * sizeof(MeshletModel::BV),
        _meshletModel->getMeshletBVs().data());
    _ssboMeshletBVBuffer->unbind();
}

void MeshShadingPipeline::loadMeshletModelLod(const std::vector<std::string>& filepaths) {
    _meshletModelLod.reset(new MeshletModelLod(filepaths));

    // + vertex data
    _ssboVerticesLodBuffer.reset(new ShaderStorageBuffer);
    _ssboVerticesLodBuffer->bind();
    _ssboVerticesLodBuffer->upload(GL_STATIC_DRAW,
        _meshletModelLod->getVertices().size() * sizeof(MeshletModelLod::Vertex),
        _meshletModelLod->getVertices().data());
    _ssboVerticesLodBuffer->unbind();

    // + vertex indices
    _ssboVertexIndicesLodBuffer.reset(new ShaderStorageBuffer);
    _ssboVertexIndicesLodBuffer->bind();
    _ssboVertexIndicesLodBuffer->upload(GL_STATIC_DRAW,
        _meshletModelLod->getVertexIndices().size() * sizeof(uint32_t),
        _meshletModelLod->getVertexIndices().data());
    _ssboVertexIndicesLodBuffer->unbind();

    // + primitive indices
    _ssboPrimitiveIndicesLodBuffer.reset(new ShaderStorageBuffer);
    _ssboPrimitiveIndicesLodBuffer->bind();
    _ssboPrimitiveIndicesLodBuffer->upload(GL_STATIC_DRAW,
        _meshletModelLod->getPrimitiveIndices().size() * sizeof(uint8_t),
        _meshletModelLod->getPrimitiveIndices().data());
    _ssboPrimitiveIndicesLodBuffer->unbind();

    // + meshlet
    _ssboMeshletLodBuffer.reset(new ShaderStorageBuffer);
    _ssboMeshletLodBuffer->bind();
    _ssboMeshletLodBuffer->upload(GL_STATIC_DRAW,
        _meshletModelLod->getMeshlets().size() * sizeof(MeshletModelLod::Meshlet),
        _meshletModelLod->getMeshlets().data());
    _ssboMeshletLodBuffer->unbind();

    // + meshlet BV
    _ssboMeshletLodBVBuffer.reset(new ShaderStorageBuffer);
    _ssboMeshletLodBVBuffer->bind();
    _ssboMeshletLodBVBuffer->upload(GL_STATIC_DRAW,
        _meshletModelLod->getMeshletBVs().size() * sizeof(MeshletModelLod::BV),
        _meshletModelLod->getMeshletBVs().data());
    _ssboMeshletLodBVBuffer->unbind();

    // + meshlet lod info
    _ssboMeshletLodInfoBuffer.reset(new ShaderStorageBuffer);
    _ssboMeshletLodInfoBuffer->bind();
    _ssboMeshletLodInfoBuffer->upload(GL_STATIC_DRAW,
        _meshletModelLod->getMeshletLodInfos().size() * sizeof(MeshletModelLod::LodInfo),
        _meshletModelLod->getMeshletLodInfos().data());
    _ssboMeshletLodInfoBuffer->unbind();
}

void MeshShadingPipeline::initInstanceMatrices() {
    _ssboInstanceMatricesBuffer.reset(new ShaderStorageBuffer);
    _ssboInstanceMatricesBuffer->bind();
    _ssboInstanceMatricesBuffer->upload(
        GL_DYNAMIC_DRAW, _instanceSpanXCount * _instanceSpanZCount * sizeof(glm::mat4));
    _ssboInstanceMatricesBuffer->unbind();
}

void MeshShadingPipeline::initStatistics() {
    _ssboStatistics.reset(new ShaderStorageBuffer);
    _ssboStatistics->bind();
    _ssboStatistics->upload(GL_DYNAMIC_COPY, sizeof(uint32_t));

    _ssboStatistics->unbind();
}

void MeshShadingPipeline::updateInstanceMatrices() {
    const auto aabb{ _meshletModel->getAABB() };
    float maxSpan{ std::max<float>(aabb.max.x - aabb.min.x, aabb.max.z - aabb.min.z) };
    float instanceSpanX{ 2.0f * maxSpan };
    float instanceSpanZ{ 4.5f * maxSpan };
    float totalSpanX{ instanceSpanX * _instanceSpanXCount };
    float totalSpanZ{ instanceSpanZ * _instanceSpanZCount };

    _ssboInstanceMatricesBuffer->bind();
    auto ptr{ reinterpret_cast<glm::mat4*>(_ssboInstanceMatricesBuffer->map(GL_WRITE_ONLY)) };

    for (size_t j = 0; j < _instanceSpanZCount; ++j) {
        for (size_t i = 0; i < _instanceSpanXCount; ++i) {
            float x{ i * instanceSpanX - (totalSpanX / 2.0f) + instanceSpanX / 2.0f };
            float y{ 0 };
            float z{ j * instanceSpanZ - (totalSpanZ / 2.0f) - 2.15f * instanceSpanZ };
            float theta = static_cast<float>(glfwGetTime());
            //float theta = 0.0f;

            glm::mat4 model{ 1.0f };
            model = glm::translate(model, glm::vec3(x, y, z));
            model = glm::rotate(model, theta, glm::vec3(0.0f, 1.0f, 0.0f));

            ptr[j * _instanceSpanXCount + i] = model;
        }
    }

    _ssboInstanceMatricesBuffer->unmap();
    _ssboInstanceMatricesBuffer->unbind();
}

void MeshShadingPipeline::updateStatictics() {
    _ssboStatistics->bind();
    auto ptr{ reinterpret_cast<uint32_t*>(_ssboStatistics->map(GL_WRITE_ONLY)) };
    *ptr = 0;
    _ssboStatistics->unmap();
    _ssboStatistics->unbind();
}

void MeshShadingPipeline::renderTraditional() {
    _traditionalProgram->use();
    _traditionalProgram->setUniformMat4("viewProjection",
        _camera->getProjectionMatrix() * _camera->getViewMatrix());

    _traditionalProgram->setUniformVec3("material.kd", _material.kd);
    _traditionalProgram->setUniformVec3(
        "directionalLight.direction", _dirLight->transform.getFront());
    _traditionalProgram->setUniformFloat("directionalLight.intensity", _dirLight->intensity);
    _traditionalProgram->setUniformVec3("directionalLight.color", _dirLight->color);

    glBindVertexArray(_model->getVao());
    glDrawElementsInstanced(GL_TRIANGLES, 
        static_cast<GLsizei>(_model->getIndices().size()),
        GL_UNSIGNED_INT, nullptr, _instanceSpanXCount * _instanceSpanZCount);
    glBindVertexArray(0);
}

void MeshShadingPipeline::renderTriangle() {
    _triangleProgram->use();
    glDrawMeshTasksNV(0, 1);
}

void MeshShadingPipeline::renderMeshlet(bool showBV) {
    glm::mat4 viewProjection{ _camera->getProjectionMatrix() * _camera->getViewMatrix() };
    //glm::mat4 model{ glm::mat4(1.0f) };
    float theta{ (float)glfwGetTime() };
    glm::mat4 model{ glm::rotate(glm::mat4(1.0f), theta, glm::vec3(0.0f, 1.0f, 0.0f)) };

    _meshletProgram->use();
    _meshletProgram->setUniformMat4("viewProjection", viewProjection);
    _meshletProgram->setUniformMat4("model", model);

    _ssboVerticesBuffer->bind();
    _ssboVerticesBuffer->setBindingPoint(_vertexBinding);

    _ssboVertexIndicesBuffer->bind();
    _ssboVertexIndicesBuffer->setBindingPoint(_vertexIndicesBinding);

    _ssboPrimitiveIndicesBuffer->bind();
    _ssboPrimitiveIndicesBuffer->setBindingPoint(_primitiveIndicesBinding);

    _ssboMeshletBuffer->bind();
    _ssboMeshletBuffer->setBindingPoint(_meshletBinding);

    // each block handles a meshlet, 
    // all threads of the block assemble the vertices and indices of the meshlet
    glDrawMeshTasksNV(0, static_cast<uint32_t>(_meshletModel->getMeshlets().size()));

    ShaderStorageBuffer::unbind();

    if (showBV) {
        _meshletBVProgram->use();
        _meshletBVProgram->setUniformMat4("viewProjection", viewProjection);
        _meshletBVProgram->setUniformMat4("model", model);
        _meshletBVProgram->setUniformUint("meshletBVCount", 
            static_cast<uint32_t>(_meshletModel->getMeshletBVs().size()));
        _meshletBVProgram->setUniformVec3("lineColor", glm::vec3(0.0f, 1.0f, 0.0f));

        _ssboMeshletBVBuffer->bind();
        _ssboMeshletBVBuffer->setBindingPoint(_bvBinding);

        constexpr uint32_t bvPerMesh{ 8 };
        const uint32_t count{
            snapUp(static_cast<uint32_t>(_meshletModel->getMeshletBVs().size()), bvPerMesh) };
        glDrawMeshTasksNV(0, count);
    }
}

void MeshShadingPipeline::renderMeshlet2() {
    glm::mat4 viewProjection{ _camera->getProjectionMatrix() * _camera->getViewMatrix() };
    //glm::mat4 model{ glm::mat4(1.0f) };
    //float theta{ (float)glfwGetTime() };
    float theta{ 0.0f };
    glm::mat4 model{ glm::rotate(glm::mat4(1.0f), theta, glm::vec3(0.0f, 1.0f, 0.0f)) };

    _meshlet2Program->use();
    _meshlet2Program->setUniformMat4("viewProjection", viewProjection);
    _meshlet2Program->setUniformMat4("model", model);

    const uint32_t meshletCount{ static_cast<uint32_t>(_meshletModel->getMeshlets().size()) };
    _meshlet2Program->setUniformUint("meshletCount", meshletCount);

    _ssboVerticesBuffer->bind();
    _ssboVerticesBuffer->setBindingPoint(_vertexBinding);

    _ssboVertexIndicesBuffer->bind();
    _ssboVertexIndicesBuffer->setBindingPoint(_vertexIndicesBinding);

    _ssboPrimitiveIndicesBuffer->bind();
    _ssboPrimitiveIndicesBuffer->setBindingPoint(_primitiveIndicesBinding);

    _ssboMeshletBuffer->bind();
    _ssboMeshletBuffer->setBindingPoint(_meshletBinding);

    // each task shader block handles at most 32 meshlets, and dispatch #meshlets mesh shaders
    // each mesh shader block handles a meshlet, 
    // all threads of the mesh shader block assemble the vertices and indices of the meshlet
    constexpr uint32_t meshDispatchPerTask{ 32 };
    const uint32_t taskCount{ snapUp(meshletCount, meshDispatchPerTask) };
    glDrawMeshTasksNV(0, taskCount);

    ShaderStorageBuffer::unbind();
}

void MeshShadingPipeline::renderInstance() {
    const uint32_t instanceCount{ _instanceSpanXCount * _instanceSpanZCount };
    const uint32_t meshletCount{ static_cast<uint32_t>(_meshletModel->getMeshlets().size()) };

    _instanceProgram->use();
    _instanceProgram->setUniformMat4("viewProjection",
        _camera->getProjectionMatrix() * _camera->getViewMatrix());
    _instanceProgram->setUniformUint("meshletCount", meshletCount);

    _ssboVerticesBuffer->bind();
    _ssboVerticesBuffer->setBindingPoint(_vertexBinding);

    _ssboVertexIndicesBuffer->bind();
    _ssboVertexIndicesBuffer->setBindingPoint(_vertexIndicesBinding);

    _ssboPrimitiveIndicesBuffer->bind();
    _ssboPrimitiveIndicesBuffer->setBindingPoint(_primitiveIndicesBinding);

    _ssboMeshletBuffer->bind();
    _ssboMeshletBuffer->setBindingPoint(_meshletBinding);

    _ssboInstanceMatricesBuffer->bind();
    _ssboInstanceMatricesBuffer->setBindingPoint(_instanceMatricesBinding);

#if ENABLE_STATISTICS
    _ssboStatistics->bind();
    _ssboStatistics->setBindingPoint(_statisticsBinding);
#endif

    constexpr uint32_t meshDispatchPerTask{ 32 };
    const uint32_t taskCount{ instanceCount * snapUp(meshletCount, meshDispatchPerTask) };
    glDrawMeshTasksNV(0, taskCount);

    ShaderStorageBuffer::unbind();
}

void MeshShadingPipeline::renderCull() {
    const uint32_t instanceCount{ _instanceSpanXCount * _instanceSpanZCount };
    const uint32_t meshletCount{ static_cast<uint32_t>(_meshletModel->getMeshlets().size()) };
    
    _cullProgram->use();
    _cullProgram->setUniformMat4("viewProjection",
        _camera->getProjectionMatrix() * _camera->getViewMatrix());

    _cullProgram->setUniformUint("meshletCount", meshletCount);

    _ssboVerticesBuffer->bind();
    _ssboVerticesBuffer->setBindingPoint(_vertexBinding);

    _ssboVertexIndicesBuffer->bind();
    _ssboVertexIndicesBuffer->setBindingPoint(_vertexIndicesBinding);

    _ssboPrimitiveIndicesBuffer->bind();
    _ssboPrimitiveIndicesBuffer->setBindingPoint(_primitiveIndicesBinding);

    _ssboMeshletBuffer->bind();
    _ssboMeshletBuffer->setBindingPoint(_meshletBinding);

    _ssboInstanceMatricesBuffer->bind();
    _ssboInstanceMatricesBuffer->setBindingPoint(_instanceMatricesBinding);

    _ssboMeshletBVBuffer->bind();
    _ssboMeshletBVBuffer->setBindingPoint(_bvBinding);

#if ENABLE_STATISTICS
    _ssboStatistics->bind();
    _ssboStatistics->setBindingPoint(_statisticsBinding);
#endif

    constexpr uint32_t meshDispatchPerTask{ 32 };
    const uint32_t taskCount{ instanceCount * snapUp(meshletCount, meshDispatchPerTask) };
    glDrawMeshTasksNV(0, taskCount);

    ShaderStorageBuffer::unbind();
}

void MeshShadingPipeline::renderLod() {
    const uint32_t instanceCount{ _instanceSpanXCount * _instanceSpanZCount };
    const uint32_t meshletCount{ static_cast<uint32_t>(_meshletModel->getMeshlets().size()) };

    _lodProgram->use();
    _lodProgram->setUniformMat4("viewProjection",
        _camera->getProjectionMatrix()* _camera->getViewMatrix());
    _lodProgram->setUniformVec3("viewPositionWS", _camera->transform.position);
    _lodProgram->setUniformVec3("centerMS", _meshletModelLod->getCenter());
    _lodProgram->setUniformUint("lodCount", _meshletModelLod->getLodCount());
    _lodProgram->setUniformFloat("maxLodDistance", _maxLodDistance);

    _ssboVerticesLodBuffer->bind();
    _ssboVerticesLodBuffer->setBindingPoint(_vertexBinding);

    _ssboVertexIndicesLodBuffer->bind();
    _ssboVertexIndicesLodBuffer->setBindingPoint(_vertexIndicesBinding);

    _ssboPrimitiveIndicesLodBuffer->bind();
    _ssboPrimitiveIndicesLodBuffer->setBindingPoint(_primitiveIndicesBinding);

    _ssboMeshletLodBuffer->bind();
    _ssboMeshletLodBuffer->setBindingPoint(_meshletBinding);

    _ssboMeshletLodInfoBuffer->bind();
    _ssboMeshletLodInfoBuffer->setBindingPoint(_lodInfoBinding);

    _ssboInstanceMatricesBuffer->bind();
    _ssboInstanceMatricesBuffer->setBindingPoint(_instanceMatricesBinding);

#if ENABLE_STATISTICS
    _ssboStatistics->bind();
    _ssboStatistics->setBindingPoint(_statisticsBinding);
#endif

    constexpr uint32_t meshDispatchPerTask{ 32 };
    const uint32_t taskCount{ instanceCount * snapUp(
        static_cast<uint32_t>(_meshletModelLod->getMeshletLodInfos()[0].meshletCount),
        meshDispatchPerTask)};
    glDrawMeshTasksNV(0, taskCount);

    ShaderStorageBuffer::unbind();
}

void MeshShadingPipeline::renderFull() {
    const uint32_t instanceCount{ _instanceSpanXCount * _instanceSpanZCount };
    const uint32_t meshletCount{ static_cast<uint32_t>(_meshletModel->getMeshlets().size()) };

    _fullProgram->use();

    _fullProgram->setUniformMat4("viewProjection",
        _camera->getProjectionMatrix() * _camera->getViewMatrix());
    _fullProgram->setUniformVec3("viewPositionWS", _camera->transform.position);
    _fullProgram->setUniformVec3("centerMS", _meshletModelLod->getCenter());
    _fullProgram->setUniformUint("lodCount", _meshletModelLod->getLodCount());
    _fullProgram->setUniformFloat("maxLodDistance", _maxLodDistance);
    _fullProgram->setUniformVec3("material.kd", _material.kd);
    _fullProgram->setUniformVec3(
        "directionalLight.direction", _dirLight->transform.getFront());
    _fullProgram->setUniformFloat("directionalLight.intensity", _dirLight->intensity);
    _fullProgram->setUniformVec3("directionalLight.color", _dirLight->color);

    _ssboVerticesLodBuffer->bind();
    _ssboVerticesLodBuffer->setBindingPoint(_vertexBinding);

    _ssboVertexIndicesLodBuffer->bind();
    _ssboVertexIndicesLodBuffer->setBindingPoint(_vertexIndicesBinding);

    _ssboPrimitiveIndicesLodBuffer->bind();
    _ssboPrimitiveIndicesLodBuffer->setBindingPoint(_primitiveIndicesBinding);

    _ssboMeshletLodBuffer->bind();
    _ssboMeshletLodBuffer->setBindingPoint(_meshletBinding);

    _ssboMeshletLodInfoBuffer->bind();
    _ssboMeshletLodInfoBuffer->setBindingPoint(_lodInfoBinding);

    _ssboMeshletLodBVBuffer->bind();
    _ssboMeshletLodBVBuffer->setBindingPoint(_bvBinding);

    _ssboInstanceMatricesBuffer->bind();
    _ssboInstanceMatricesBuffer->setBindingPoint(_instanceMatricesBinding);

#if ENABLE_STATISTICS
    _ssboStatistics->bind();
    _ssboStatistics->setBindingPoint(_statisticsBinding);
#endif

    constexpr uint32_t meshDispatchPerTask{ 32 };
    const uint32_t taskCount{ instanceCount * snapUp(
        static_cast<uint32_t>(_meshletModelLod->getMeshletLodInfos()[0].meshletCount),
        meshDispatchPerTask) };
    glDrawMeshTasksNV(0, taskCount);

    ShaderStorageBuffer::unbind();
}

void MeshShadingPipeline::renderUI() {
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
            "Meshlet2",
            "Instance",
            "Cull",
            "Lod",
            "Full"
        };

        ImGui::Text("Render Mode");
        if (ImGui::BeginCombo("##Render Mode", items[_renderCase])) {
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

        ImGui::Text("Camera Speed");
        ImGui::SliderFloat("##Camera Speed", &_cameraMoveSpeed, 0.1f, 10.0f);

#if ENABLE_STATISTICS
        if (_renderCase == RenderCase::Instance ||
            _renderCase == RenderCase::Cull ||
            _renderCase == RenderCase::Lod ||
            _renderCase == RenderCase::Full) {
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            _ssboStatistics->bind();
            uint32_t* ptr{ (uint32_t*)_ssboStatistics->map(GL_READ_ONLY) };
            ImGui::Text("Primitive Count %d", *ptr);
            _ssboStatistics->unmap();
            _ssboStatistics->unbind();
        }
#endif

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}