#include "mesh_shading_pipeline.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define ENABLE_STATISTICS 0

MeshShadingPipeline::MeshShadingPipeline(const Options& options) : Application(options) {
    m_dirLight.reset(new DirectionalLight);
    m_dirLight->intensity = 1.0f;
    m_dirLight->transform.rotation =
        glm::angleAxis(glm::radians(45.0f), glm::normalize(glm::vec3(-1.0f)));

    m_camera.reset(new PerspectiveCamera(
        glm::radians(50.0f), 1.0f * m_windowWidth / m_windowHeight, 0.1f, 1000.0f));
    m_camera->transform.position = glm::vec3(0.000000f, 0.177955f, 0.367840f);
    m_camera->transform.rotation = glm::quat(0.995212f, -0.097740f, -0.0f, -0.0f);


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
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init();

    checkGLErrors();
    
    updateInstanceMatrices();
}

void MeshShadingPipeline::handleInput() {
    if (m_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
        glfwSetWindowShouldClose(m_window, true);
        return;
    }

    if (m_input.keyboard.keyStates[GLFW_KEY_W] != GLFW_RELEASE) {
        m_camera->transform.position +=
            m_camera->transform.getFront() * m_cameraMoveSpeed * m_deltaTime;
    }

    if (m_input.keyboard.keyStates[GLFW_KEY_A] != GLFW_RELEASE) {
        m_camera->transform.position -=
            m_camera->transform.getRight() * m_cameraMoveSpeed * m_deltaTime;
    }

    if (m_input.keyboard.keyStates[GLFW_KEY_S] != GLFW_RELEASE) {
        m_camera->transform.position -=
            m_camera->transform.getFront() * m_cameraMoveSpeed * m_deltaTime;
    }

    if (m_input.keyboard.keyStates[GLFW_KEY_D] != GLFW_RELEASE) {
        m_camera->transform.position +=
            m_camera->transform.getRight() * m_cameraMoveSpeed * m_deltaTime;
    }

    if (m_input.keyboard.keyStates[GLFW_KEY_Q] != GLFW_RELEASE) {
        m_camera->transform.position -=
            m_camera->transform.getUp() * m_cameraMoveSpeed * m_deltaTime;
    }

    if (m_input.keyboard.keyStates[GLFW_KEY_E] != GLFW_RELEASE) {
        m_camera->transform.position +=
            m_camera->transform.getUp() * m_cameraMoveSpeed * m_deltaTime;
    }

    //updateInstanceMatrices();
#if ENABLE_STATISTICS
    updateStatictics();
#endif
}

void MeshShadingPipeline::renderFrame() {
    showFpsInWindowTitle();

    glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    switch (m_renderCase) {
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
    m_traditionalProgram.reset(new GLSLProgram);
    m_traditionalProgram->attachVertexShaderFromFile(
        getAssetFullPath("shader/mesh_shading_pipeline/traditional.vert"));
    m_traditionalProgram->attachFragmentShaderFromFile(
        getAssetFullPath("shader/mesh_shading_pipeline/lambert.frag"));
    m_traditionalProgram->link();

    // generate triangle on the fly
    m_triangleProgram.reset(new GLSLProgram);
    m_triangleProgram->attachMeshShaderFromFile(
        getAssetFullPath("shader/mesh_shading_pipeline/triangle.mesh"));
    m_triangleProgram->attachFragmentShaderFromFile(
        getAssetFullPath("shader/mesh_shading_pipeline/flat_color.frag"));
    m_triangleProgram->link();

    // render meshlet model use mesh shader
    m_meshletProgram.reset(new GLSLProgram);
    m_meshletProgram->attachMeshShaderFromFile(
        getAssetFullPath("shader/mesh_shading_pipeline/meshlet.mesh"));
    m_meshletProgram->attachFragmentShaderFromFile(
        getAssetFullPath("shader/mesh_shading_pipeline/flat_color.frag"));
    m_meshletProgram->link();

    // render meshlet model bv use mesh shader
    m_meshletBVProgram.reset(new GLSLProgram);
    m_meshletBVProgram->attachMeshShaderFromFile(
        getAssetFullPath("shader/mesh_shading_pipeline/bv.mesh"));
    m_meshletBVProgram->attachFragmentShaderFromFile(
        getAssetFullPath("shader/mesh_shading_pipeline/flat_color.frag"));
    m_meshletBVProgram->link();

    // render meshlet model use task shader and mesh shader
    m_meshlet2Program.reset(new GLSLProgram);
    m_meshlet2Program->attachTaskShaderFromFile(
        getAssetFullPath("shader/mesh_shading_pipeline/meshlet2.task"));
    m_meshlet2Program->attachMeshShaderFromFile(
        getAssetFullPath("shader/mesh_shading_pipeline/meshlet2.mesh"));
    m_meshlet2Program->attachFragmentShaderFromFile(
        getAssetFullPath("shader/mesh_shading_pipeline/flat_color.frag"));
    m_meshlet2Program->link();

    // render instanced meshlet model
    m_instanceProgram.reset(new GLSLProgram);
    m_instanceProgram->attachTaskShaderFromFile(
        getAssetFullPath("shader/mesh_shading_pipeline/instance.task"));
    m_instanceProgram->attachMeshShaderFromFile(
        getAssetFullPath("shader/mesh_shading_pipeline/instance.mesh"));
    m_instanceProgram->attachFragmentShaderFromFile(
        getAssetFullPath("shader/mesh_shading_pipeline/flat_color.frag"));
    m_instanceProgram->link();

    // frustum culling with instanced meshlet model
    m_cullProgram.reset(new GLSLProgram);
    m_cullProgram->attachTaskShaderFromFile(
        getAssetFullPath("shader/mesh_shading_pipeline/cull.task"));
    m_cullProgram->attachMeshShaderFromFile(
        getAssetFullPath("shader/mesh_shading_pipeline/cull.mesh"));
    m_cullProgram->attachFragmentShaderFromFile(
        getAssetFullPath("shader/mesh_shading_pipeline/flat_color.frag"));
    m_cullProgram->link();

    // level of detail with instanced meshlet model
    m_lodProgram.reset(new GLSLProgram);
    m_lodProgram->attachTaskShaderFromFile(
        getAssetFullPath("shader/mesh_shading_pipeline/lod.task"));
    m_lodProgram->attachMeshShaderFromFile(
        getAssetFullPath("shader/mesh_shading_pipeline/lod.mesh"));
    m_lodProgram->attachFragmentShaderFromFile(
        getAssetFullPath("shader/mesh_shading_pipeline/flat_color.frag"));
    m_lodProgram->link();

    // frustum culling and level of detail with instanced meshlet model
    m_fullProgram.reset(new GLSLProgram);
    m_fullProgram->attachTaskShaderFromFile(
        getAssetFullPath("shader/mesh_shading_pipeline/full.task"));
    m_fullProgram->attachMeshShaderFromFile(
        getAssetFullPath("shader/mesh_shading_pipeline/full.mesh"));
    m_fullProgram->attachFragmentShaderFromFile(
        getAssetFullPath("shader/mesh_shading_pipeline/lambert.frag"));
    m_fullProgram->link();
}

void MeshShadingPipeline::loadModel(const std::string& filepath) {
    m_model.reset(new Model(getAssetFullPath(filepath)));

    glBindVertexArray(m_model->getVao());
    glBindBuffer(GL_ARRAY_BUFFER, m_ssboInstanceMatricesBuffer->getNativeHandle());

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
    m_meshletModel.reset(new MeshletModel(getAssetFullPath(filepath)));

    // + vertex data
    m_ssboVerticesBuffer.reset(new ShaderStorageBuffer);
    m_ssboVerticesBuffer->bind();
    m_ssboVerticesBuffer->upload(GL_STATIC_DRAW,
        m_meshletModel->getVertices().size() * sizeof(MeshletModel::Vertex),
        m_meshletModel->getVertices().data());
    m_ssboVerticesBuffer->unbind();

    // + vertex indices
    m_ssboVertexIndicesBuffer.reset(new ShaderStorageBuffer);
    m_ssboVertexIndicesBuffer->bind();
    m_ssboVertexIndicesBuffer->upload(GL_STATIC_DRAW,
        m_meshletModel->getVertexIndices().size() * sizeof(uint32_t),
        m_meshletModel->getVertexIndices().data());
    m_ssboVertexIndicesBuffer->unbind();

    // + primitive indices
    m_ssboPrimitiveIndicesBuffer.reset(new ShaderStorageBuffer);
    m_ssboPrimitiveIndicesBuffer->bind();
    m_ssboPrimitiveIndicesBuffer->upload(GL_STATIC_DRAW,
        m_meshletModel->getPrimitiveIndices().size() * sizeof(uint8_t),
        m_meshletModel->getPrimitiveIndices().data());
    m_ssboPrimitiveIndicesBuffer->unbind();

    // + meshlet
    m_ssboMeshletBuffer.reset(new ShaderStorageBuffer);
    m_ssboMeshletBuffer->bind();
    m_ssboMeshletBuffer->upload(GL_STATIC_DRAW,
        m_meshletModel->getMeshlets().size() * sizeof(MeshletModel::Meshlet),
        m_meshletModel->getMeshlets().data());
    m_ssboMeshletBuffer->unbind();

    // + meshlet BV
    m_ssboMeshletBVBuffer.reset(new ShaderStorageBuffer);
    m_ssboMeshletBVBuffer->bind();
    m_ssboMeshletBVBuffer->upload(GL_STATIC_DRAW,
        m_meshletModel->getMeshletBVs().size() * sizeof(MeshletModel::BV),
        m_meshletModel->getMeshletBVs().data());
    m_ssboMeshletBVBuffer->unbind();
}

void MeshShadingPipeline::loadMeshletModelLod(const std::vector<std::string>& filepaths) {
    m_meshletModelLod.reset(new MeshletModelLod(filepaths));

    // + vertex data
    m_ssboVerticesLodBuffer.reset(new ShaderStorageBuffer);
    m_ssboVerticesLodBuffer->bind();
    m_ssboVerticesLodBuffer->upload(GL_STATIC_DRAW,
        m_meshletModelLod->getVertices().size() * sizeof(MeshletModelLod::Vertex),
        m_meshletModelLod->getVertices().data());
    m_ssboVerticesLodBuffer->unbind();

    // + vertex indices
    m_ssboVertexIndicesLodBuffer.reset(new ShaderStorageBuffer);
    m_ssboVertexIndicesLodBuffer->bind();
    m_ssboVertexIndicesLodBuffer->upload(GL_STATIC_DRAW,
        m_meshletModelLod->getVertexIndices().size() * sizeof(uint32_t),
        m_meshletModelLod->getVertexIndices().data());
    m_ssboVertexIndicesLodBuffer->unbind();

    // + primitive indices
    m_ssboPrimitiveIndicesLodBuffer.reset(new ShaderStorageBuffer);
    m_ssboPrimitiveIndicesLodBuffer->bind();
    m_ssboPrimitiveIndicesLodBuffer->upload(GL_STATIC_DRAW,
        m_meshletModelLod->getPrimitiveIndices().size() * sizeof(uint8_t),
        m_meshletModelLod->getPrimitiveIndices().data());
    m_ssboPrimitiveIndicesLodBuffer->unbind();

    // + meshlet
    m_ssboMeshletLodBuffer.reset(new ShaderStorageBuffer);
    m_ssboMeshletLodBuffer->bind();
    m_ssboMeshletLodBuffer->upload(GL_STATIC_DRAW,
        m_meshletModelLod->getMeshlets().size() * sizeof(MeshletModelLod::Meshlet),
        m_meshletModelLod->getMeshlets().data());
    m_ssboMeshletLodBuffer->unbind();

    // + meshlet BV
    m_ssboMeshletLodBVBuffer.reset(new ShaderStorageBuffer);
    m_ssboMeshletLodBVBuffer->bind();
    m_ssboMeshletLodBVBuffer->upload(GL_STATIC_DRAW,
        m_meshletModelLod->getMeshletBVs().size() * sizeof(MeshletModelLod::BV),
        m_meshletModelLod->getMeshletBVs().data());
    m_ssboMeshletLodBVBuffer->unbind();

    // + meshlet lod info
    m_ssboMeshletLodInfoBuffer.reset(new ShaderStorageBuffer);
    m_ssboMeshletLodInfoBuffer->bind();
    m_ssboMeshletLodInfoBuffer->upload(GL_STATIC_DRAW,
        m_meshletModelLod->getMeshletLodInfos().size() * sizeof(MeshletModelLod::LodInfo),
        m_meshletModelLod->getMeshletLodInfos().data());
    m_ssboMeshletLodInfoBuffer->unbind();
}

void MeshShadingPipeline::initInstanceMatrices() {
    m_ssboInstanceMatricesBuffer.reset(new ShaderStorageBuffer);
    m_ssboInstanceMatricesBuffer->bind();
    m_ssboInstanceMatricesBuffer->upload(
        GL_DYNAMIC_DRAW, m_instanceSpanXCount * m_instanceSpanZCount * sizeof(glm::mat4));
    m_ssboInstanceMatricesBuffer->unbind();
}

void MeshShadingPipeline::initStatistics() {
    m_ssboStatistics.reset(new ShaderStorageBuffer);
    m_ssboStatistics->bind();
    m_ssboStatistics->upload(GL_DYNAMIC_COPY, sizeof(uint32_t));

    m_ssboStatistics->unbind();
}

void MeshShadingPipeline::updateInstanceMatrices() {
    const auto aabb{ m_meshletModel->getAABB() };
    float maxSpan{ std::max<float>(aabb.max.x - aabb.min.x, aabb.max.z - aabb.min.z) };
    float instanceSpanX{ 2.0f * maxSpan };
    float instanceSpanZ{ 4.5f * maxSpan };
    float totalSpanX{ instanceSpanX * m_instanceSpanXCount };
    float totalSpanZ{ instanceSpanZ * m_instanceSpanZCount };

    m_ssboInstanceMatricesBuffer->bind();
    auto ptr{ reinterpret_cast<glm::mat4*>(m_ssboInstanceMatricesBuffer->map(GL_WRITE_ONLY)) };

    for (size_t j = 0; j < m_instanceSpanZCount; ++j) {
        for (size_t i = 0; i < m_instanceSpanXCount; ++i) {
            float x{ i * instanceSpanX - (totalSpanX / 2.0f) + instanceSpanX / 2.0f };
            float y{ 0 };
            float z{ j * instanceSpanZ - (totalSpanZ / 2.0f) - 2.15f * instanceSpanZ };
            float theta = static_cast<float>(glfwGetTime());
            //float theta = 0.0f;

            glm::mat4 model{ 1.0f };
            model = glm::translate(model, glm::vec3(x, y, z));
            model = glm::rotate(model, theta, glm::vec3(0.0f, 1.0f, 0.0f));

            ptr[j * m_instanceSpanXCount + i] = model;
        }
    }

    m_ssboInstanceMatricesBuffer->unmap();
    m_ssboInstanceMatricesBuffer->unbind();
}

void MeshShadingPipeline::updateStatictics() {
    m_ssboStatistics->bind();
    auto ptr{ reinterpret_cast<uint32_t*>(m_ssboStatistics->map(GL_WRITE_ONLY)) };
    *ptr = 0;
    m_ssboStatistics->unmap();
    m_ssboStatistics->unbind();
}

void MeshShadingPipeline::renderTraditional() {
    m_traditionalProgram->use();
    m_traditionalProgram->setUniformMat4("viewProjection",
        m_camera->getProjectionMatrix() * m_camera->getViewMatrix());

    m_traditionalProgram->setUniformVec3("material.kd", m_material.kd);
    m_traditionalProgram->setUniformVec3(
        "directionalLight.direction", m_dirLight->transform.getFront());
    m_traditionalProgram->setUniformFloat("directionalLight.intensity", m_dirLight->intensity);
    m_traditionalProgram->setUniformVec3("directionalLight.color", m_dirLight->color);

    glBindVertexArray(m_model->getVao());
    glDrawElementsInstanced(GL_TRIANGLES, 
        static_cast<GLsizei>(m_model->getIndices().size()),
        GL_UNSIGNED_INT, nullptr, m_instanceSpanXCount * m_instanceSpanZCount);
    glBindVertexArray(0);
}

void MeshShadingPipeline::renderTriangle() {
    m_triangleProgram->use();
    glDrawMeshTasksNV(0, 1);
}

void MeshShadingPipeline::renderMeshlet(bool showBV) {
    glm::mat4 viewProjection{ m_camera->getProjectionMatrix() * m_camera->getViewMatrix() };
    float theta{ (float)glfwGetTime() };
    glm::mat4 model{ glm::rotate(glm::mat4(1.0f), theta, glm::vec3(0.0f, 1.0f, 0.0f)) };

    m_meshletProgram->use();
    m_meshletProgram->setUniformMat4("viewProjection", viewProjection);
    m_meshletProgram->setUniformMat4("model", model);

    m_ssboVerticesBuffer->bind();
    m_ssboVerticesBuffer->setBindingPoint(m_vertexBinding);

    m_ssboVertexIndicesBuffer->bind();
    m_ssboVertexIndicesBuffer->setBindingPoint(m_vertexIndicesBinding);

    m_ssboPrimitiveIndicesBuffer->bind();
    m_ssboPrimitiveIndicesBuffer->setBindingPoint(m_primitiveIndicesBinding);

    m_ssboMeshletBuffer->bind();
    m_ssboMeshletBuffer->setBindingPoint(m_meshletBinding);

    // each block handles a meshlet, 
    // all threads of the block assemble the vertices and indices of the meshlet
    glDrawMeshTasksNV(0, static_cast<uint32_t>(m_meshletModel->getMeshlets().size()));

    ShaderStorageBuffer::unbind();

    if (showBV) {
        m_meshletBVProgram->use();
        m_meshletBVProgram->setUniformMat4("viewProjection", viewProjection);
        m_meshletBVProgram->setUniformMat4("model", model);
        m_meshletBVProgram->setUniformUint("meshletBVCount",
            static_cast<uint32_t>(m_meshletModel->getMeshletBVs().size()));
        m_meshletBVProgram->setUniformVec3("lineColor", glm::vec3(0.0f, 1.0f, 0.0f));

        m_ssboMeshletBVBuffer->bind();
        m_ssboMeshletBVBuffer->setBindingPoint(m_bvBinding);

        constexpr uint32_t bvPerMesh{ 8 };
        const uint32_t count{
            snapUp(static_cast<uint32_t>(m_meshletModel->getMeshletBVs().size()), bvPerMesh) };
        glDrawMeshTasksNV(0, count);
    }
}

void MeshShadingPipeline::renderMeshlet2() {
    glm::mat4 viewProjection{ m_camera->getProjectionMatrix() * m_camera->getViewMatrix() };
    float theta{ (float)glfwGetTime() };
    glm::mat4 model{ glm::rotate(glm::mat4(1.0f), theta, glm::vec3(0.0f, 1.0f, 0.0f)) };

    m_meshlet2Program->use();
    m_meshlet2Program->setUniformMat4("viewProjection", viewProjection);
    m_meshlet2Program->setUniformMat4("model", model);

    const uint32_t meshletCount{ static_cast<uint32_t>(m_meshletModel->getMeshlets().size()) };
    m_meshlet2Program->setUniformUint("meshletCount", meshletCount);

    m_ssboVerticesBuffer->bind();
    m_ssboVerticesBuffer->setBindingPoint(m_vertexBinding);

    m_ssboVertexIndicesBuffer->bind();
    m_ssboVertexIndicesBuffer->setBindingPoint(m_vertexIndicesBinding);

    m_ssboPrimitiveIndicesBuffer->bind();
    m_ssboPrimitiveIndicesBuffer->setBindingPoint(m_primitiveIndicesBinding);

    m_ssboMeshletBuffer->bind();
    m_ssboMeshletBuffer->setBindingPoint(m_meshletBinding);

    // each task shader block handles at most 32 meshlets, and dispatch #meshlets mesh shaders
    // each mesh shader block handles a meshlet, 
    // all threads of the mesh shader block assemble the vertices and indices of the meshlet
    constexpr uint32_t meshDispatchPerTask{ 32 };
    const uint32_t taskCount{ snapUp(meshletCount, meshDispatchPerTask) };
    glDrawMeshTasksNV(0, taskCount);

    ShaderStorageBuffer::unbind();
}

void MeshShadingPipeline::renderInstance() {
    const uint32_t instanceCount{ m_instanceSpanXCount * m_instanceSpanZCount };
    const uint32_t meshletCount{ static_cast<uint32_t>(m_meshletModel->getMeshlets().size()) };

    m_instanceProgram->use();
    m_instanceProgram->setUniformMat4("viewProjection",
        m_camera->getProjectionMatrix() * m_camera->getViewMatrix());
    m_instanceProgram->setUniformUint("meshletCount", meshletCount);

    m_ssboVerticesBuffer->bind();
    m_ssboVerticesBuffer->setBindingPoint(m_vertexBinding);

    m_ssboVertexIndicesBuffer->bind();
    m_ssboVertexIndicesBuffer->setBindingPoint(m_vertexIndicesBinding);

    m_ssboPrimitiveIndicesBuffer->bind();
    m_ssboPrimitiveIndicesBuffer->setBindingPoint(m_primitiveIndicesBinding);

    m_ssboMeshletBuffer->bind();
    m_ssboMeshletBuffer->setBindingPoint(m_meshletBinding);

    m_ssboInstanceMatricesBuffer->bind();
    m_ssboInstanceMatricesBuffer->setBindingPoint(m_instanceMatricesBinding);

#if ENABLE_STATISTICS
    m_ssboStatistics->bind();
    m_ssboStatistics->setBindingPoint(m_statisticsBinding);
#endif

    constexpr uint32_t meshDispatchPerTask{ 32 };
    const uint32_t taskCount{ instanceCount * snapUp(meshletCount, meshDispatchPerTask) };
    glDrawMeshTasksNV(0, taskCount);

    ShaderStorageBuffer::unbind();
}

void MeshShadingPipeline::renderCull() {
    const uint32_t instanceCount{ m_instanceSpanXCount * m_instanceSpanZCount };
    const uint32_t meshletCount{ static_cast<uint32_t>(m_meshletModel->getMeshlets().size()) };
    
    m_cullProgram->use();
    m_cullProgram->setUniformMat4("viewProjection",
        m_camera->getProjectionMatrix() * m_camera->getViewMatrix());

    m_cullProgram->setUniformUint("meshletCount", meshletCount);

    m_ssboVerticesBuffer->bind();
    m_ssboVerticesBuffer->setBindingPoint(m_vertexBinding);

    m_ssboVertexIndicesBuffer->bind();
    m_ssboVertexIndicesBuffer->setBindingPoint(m_vertexIndicesBinding);

    m_ssboPrimitiveIndicesBuffer->bind();
    m_ssboPrimitiveIndicesBuffer->setBindingPoint(m_primitiveIndicesBinding);

    m_ssboMeshletBuffer->bind();
    m_ssboMeshletBuffer->setBindingPoint(m_meshletBinding);

    m_ssboInstanceMatricesBuffer->bind();
    m_ssboInstanceMatricesBuffer->setBindingPoint(m_instanceMatricesBinding);

    m_ssboMeshletBVBuffer->bind();
    m_ssboMeshletBVBuffer->setBindingPoint(m_bvBinding);

#if ENABLE_STATISTICS
    m_ssboStatistics->bind();
    m_ssboStatistics->setBindingPoint(m_statisticsBinding);
#endif

    constexpr uint32_t meshDispatchPerTask{ 32 };
    const uint32_t taskCount{ instanceCount * snapUp(meshletCount, meshDispatchPerTask) };
    glDrawMeshTasksNV(0, taskCount);

    ShaderStorageBuffer::unbind();
}

void MeshShadingPipeline::renderLod() {
    const uint32_t instanceCount{ m_instanceSpanXCount * m_instanceSpanZCount };
    const uint32_t meshletCount{ static_cast<uint32_t>(m_meshletModel->getMeshlets().size()) };

    m_lodProgram->use();
    m_lodProgram->setUniformMat4("viewProjection",
        m_camera->getProjectionMatrix()* m_camera->getViewMatrix());
    m_lodProgram->setUniformVec3("viewPositionWS", m_camera->transform.position);
    m_lodProgram->setUniformVec3("centerMS", m_meshletModelLod->getCenter());
    m_lodProgram->setUniformUint("lodCount", m_meshletModelLod->getLodCount());
    m_lodProgram->setUniformFloat("maxLodDistance", m_maxLodDistance);

    m_ssboVerticesLodBuffer->bind();
    m_ssboVerticesLodBuffer->setBindingPoint(m_vertexBinding);

    m_ssboVertexIndicesLodBuffer->bind();
    m_ssboVertexIndicesLodBuffer->setBindingPoint(m_vertexIndicesBinding);

    m_ssboPrimitiveIndicesLodBuffer->bind();
    m_ssboPrimitiveIndicesLodBuffer->setBindingPoint(m_primitiveIndicesBinding);

    m_ssboMeshletLodBuffer->bind();
    m_ssboMeshletLodBuffer->setBindingPoint(m_meshletBinding);

    m_ssboMeshletLodInfoBuffer->bind();
    m_ssboMeshletLodInfoBuffer->setBindingPoint(m_lodInfoBinding);

    m_ssboInstanceMatricesBuffer->bind();
    m_ssboInstanceMatricesBuffer->setBindingPoint(m_instanceMatricesBinding);

#if ENABLE_STATISTICS
    m_ssboStatistics->bind();
    m_ssboStatistics->setBindingPoint(m_statisticsBinding);
#endif

    constexpr uint32_t meshDispatchPerTask{ 32 };
    const uint32_t taskCount{ instanceCount * snapUp(
        static_cast<uint32_t>(m_meshletModelLod->getMeshletLodInfos()[0].meshletCount),
        meshDispatchPerTask)};
    glDrawMeshTasksNV(0, taskCount);

    ShaderStorageBuffer::unbind();
}

void MeshShadingPipeline::renderFull() {
    const uint32_t instanceCount{ m_instanceSpanXCount * m_instanceSpanZCount };
    const uint32_t meshletCount{ static_cast<uint32_t>(m_meshletModel->getMeshlets().size()) };

    m_fullProgram->use();

    m_fullProgram->setUniformMat4("viewProjection",
        m_camera->getProjectionMatrix() * m_camera->getViewMatrix());
    m_fullProgram->setUniformVec3("viewPositionWS", m_camera->transform.position);
    m_fullProgram->setUniformVec3("centerMS", m_meshletModelLod->getCenter());
    m_fullProgram->setUniformUint("lodCount", m_meshletModelLod->getLodCount());
    m_fullProgram->setUniformFloat("maxLodDistance", m_maxLodDistance);
    m_fullProgram->setUniformVec3("material.kd", m_material.kd);
    m_fullProgram->setUniformVec3(
        "directionalLight.direction", m_dirLight->transform.getFront());
    m_fullProgram->setUniformFloat("directionalLight.intensity", m_dirLight->intensity);
    m_fullProgram->setUniformVec3("directionalLight.color", m_dirLight->color);

    m_ssboVerticesLodBuffer->bind();
    m_ssboVerticesLodBuffer->setBindingPoint(m_vertexBinding);

    m_ssboVertexIndicesLodBuffer->bind();
    m_ssboVertexIndicesLodBuffer->setBindingPoint(m_vertexIndicesBinding);

    m_ssboPrimitiveIndicesLodBuffer->bind();
    m_ssboPrimitiveIndicesLodBuffer->setBindingPoint(m_primitiveIndicesBinding);

    m_ssboMeshletLodBuffer->bind();
    m_ssboMeshletLodBuffer->setBindingPoint(m_meshletBinding);

    m_ssboMeshletLodInfoBuffer->bind();
    m_ssboMeshletLodInfoBuffer->setBindingPoint(m_lodInfoBinding);

    m_ssboMeshletLodBVBuffer->bind();
    m_ssboMeshletLodBVBuffer->setBindingPoint(m_bvBinding);

    m_ssboInstanceMatricesBuffer->bind();
    m_ssboInstanceMatricesBuffer->setBindingPoint(m_instanceMatricesBinding);

#if ENABLE_STATISTICS
    m_ssboStatistics->bind();
    m_ssboStatistics->setBindingPoint(m_statisticsBinding);
#endif

    constexpr uint32_t meshDispatchPerTask{ 32 };
    const uint32_t taskCount{ instanceCount * snapUp(
        static_cast<uint32_t>(m_meshletModelLod->getMeshletLodInfos()[0].meshletCount),
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
        if (ImGui::BeginCombo("##Render Mode", items[m_renderCase])) {
            for (int i = 0; i < IM_ARRAYSIZE(items); ++i) {
                bool selected{ static_cast<RenderCase>(i) == m_renderCase };
                if (ImGui::Selectable(items[i], &selected)) {
                    m_renderCase = static_cast<RenderCase>(i);
                }

                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }

        ImGui::Text("Camera Speed");
        ImGui::SliderFloat("##Camera Speed", &m_cameraMoveSpeed, 0.1f, 10.0f);

#if ENABLE_STATISTICS
        if (m_renderCase == RenderCase::Instance ||
            m_renderCase == RenderCase::Cull ||
            m_renderCase == RenderCase::Lod ||
            m_renderCase == RenderCase::Full) {
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            m_ssboStatistics->bind();
            uint32_t* ptr{ (uint32_t*)m_ssboStatistics->map(GL_READ_ONLY) };
            ImGui::Text("Primitive Count %d", *ptr);
            m_ssboStatistics->unmap();
            m_ssboStatistics->unbind();
        }
#endif

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
