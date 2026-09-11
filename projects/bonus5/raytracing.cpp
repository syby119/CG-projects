#include <iostream>
#include <string>
#include <unordered_map>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include "../base/vertex.h"
#include "random.h"
#include "raytracing.h"

static constexpr int BufferWidth = 2048;

const std::string lucyRelPath = "obj/lucy.obj";

const std::string quadVsRelPath = "shader/bonus5/quad.vert";
const std::string quadFsRelPath = "shader/bonus5/quad.frag";

const std::string raytracingVsRelPath = "shader/bonus5/quad.vert";
const std::string raytracingFsRelPath = "shader/bonus5/raytracing.frag";

const std::vector<std::string> skyboxTextureRelPaths = {
    "texture/skyboxrt/right.jpg",  "texture/skyboxrt/left.jpg",  "texture/skyboxrt/top.jpg",
    "texture/skyboxrt/bottom.jpg", "texture/skyboxrt/front.jpg", "texture/skyboxrt/back.jpg",
};

RayTracing::RayTracing(const Options& options) : Application(options) {
    m_lucy.reset(new Model(getAssetFullPath(lucyRelPath)));

    std::vector<std::string> skyBoxTexturePaths;
    for (size_t i = 0; i < skyboxTextureRelPaths.size(); ++i) {
        skyBoxTexturePaths.push_back(getAssetFullPath(skyboxTextureRelPaths[i]));
    }
    m_skybox.reset(new ImageTextureCubemap(skyBoxTexturePaths));

    m_camera.reset(new PerspectiveCamera(
        glm::radians(60.0f), static_cast<float>(m_windowWidth) / m_windowHeight, 0.1f, 1000.0f));
    m_camera->transform.position = glm::vec3(15.0f, 3.0f, 4.0f);
    m_camera->transform.lookAt(glm::vec3(0.0f));

    createBalls();

    initShaders();

    m_screenQuad.reset(new FullscreenQuad);

    // rngInitState
    const int pixelCount = m_windowWidth * m_windowHeight;
    std::vector<unsigned int> rngStateInitVals;
    rngStateInitVals.reserve(pixelCount);
    for (int i = 0; i < pixelCount; ++i) {
        rngStateInitVals.push_back(1664525 * i + 1013904223);
    }

    for (int i = 0; i < 2; ++i) {
        m_sampleFramebuffers[i].reset(new Framebuffer);
        m_sampleFramebuffers[i]->bind();
        m_sampleFramebuffers[i]->drawBuffers({GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1});

        m_outFrames[i].reset(
            new Texture2D(GL_RGBA32F, m_windowWidth, m_windowHeight, GL_RGBA, GL_FLOAT));
        m_outFrames[i]->bind();
        m_outFrames[i]->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        m_outFrames[i]->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        m_outFrames[i]->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        m_outFrames[i]->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        m_sampleFramebuffers[i]->attachTexture2D(
            *m_outFrames[i], GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);

        m_rngStates[i].reset(new Texture2D(
            GL_R32UI, m_windowWidth, m_windowHeight, GL_RED_INTEGER, GL_UNSIGNED_INT,
            rngStateInitVals.data()));
        m_rngStates[i]->bind();
        m_rngStates[i]->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        m_rngStates[i]->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        m_rngStates[i]->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        m_rngStates[i]->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        m_sampleFramebuffers[i]->attachTexture2D(
            *m_rngStates[i], GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D);

        m_sampleFramebuffers[i]->unbind();
    }

    createRenderScene(m_renderSceneIndex);

    // init imGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
#if defined(__EMSCRIPTEN__)
    ImGui_ImplOpenGL3_Init("#version 100");
#elif defined(USE_GLES)
    ImGui_ImplOpenGL3_Init("#version 150");
#else
    ImGui_ImplOpenGL3_Init();
#endif
}

RayTracing::~RayTracing() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void RayTracing::handleInput() {
    if (m_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
        glfwSetWindowShouldClose(m_window, true);
        return;
    }

    static int lastSceneIndex = m_renderSceneIndex;
    if (lastSceneIndex != m_renderSceneIndex) {
        createRenderScene(m_renderSceneIndex);
        lastSceneIndex = m_renderSceneIndex;
        m_sampleCount = 0;
    }
}

void RayTracing::renderFrame() {
    showFpsInWindowTitle();

    glDisable(GL_DEPTH_TEST);

    glm::mat4 cameraToWorld = glm::inverse(m_camera->getViewMatrix());
    glm::mat4 cameraToScreen = m_camera->getProjectionMatrix();
    glm::mat4 screenToRaster =
        glm::scale(
            glm::mat4(1.0f),
            glm::vec3(float(m_windowWidth) / 2.0f, float(m_windowHeight) / 2.0f, 1.0f))
        * glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 0.0f));

    glm::mat4 rasterToScreen = glm::inverse(screenToRaster);
    glm::mat4 rasterToCamera = glm::inverse(cameraToScreen) * rasterToScreen;

    m_sampleFramebuffers[m_currentWriteBufferID]->bind();
    m_raytracingShader->use();
    m_raytracingShader->setUniformUint("totalSamples", m_sampleCount);
    m_raytracingShader->setUniformMat4("camera.cameraToWorld", cameraToWorld);
    m_raytracingShader->setUniformMat4("camera.rasterToCamera", rasterToCamera);

    m_raytracingShader->setUniformInt("sky", 0);
    m_skybox->bind(0);

    m_sphereBuffer->bind(1);
    m_raytracingShader->setUniformInt("sphereBuffer", 1);

    m_raytracingShader->setUniformInt("materialBuffer", 2);
    m_materialBuffer->bind(2);

    m_raytracingShader->setUniformInt("primitiveBuffer", 3);
    m_primitiveBuffer->bind(3);

    m_raytracingShader->setUniformInt("RTResult", 4);
    m_outFrames[m_currentReadBufferID]->bind(4);

    m_raytracingShader->setUniformInt("oldRngState", 5);
    m_rngStates[m_currentReadBufferID]->bind(5);

    m_indexBuffer->bind(6);
    m_raytracingShader->setUniformInt("triangleIndexBuffer", 6);
    m_vertexBuffer->bind(7);
    m_raytracingShader->setUniformInt("vertexBuffer", 7);

    m_bvhBuffer->bind(8);
    m_raytracingShader->setUniformInt("bvh", 8);

    m_screenQuad->draw();

    m_sampleFramebuffers[m_currentWriteBufferID]->unbind();

    // render the result to the screen
    m_drawScreenShader->use();
    m_drawScreenShader->setUniformInt("frame", 0);

    m_outFrames[m_currentWriteBufferID]->bind(0);
    m_screenQuad->draw();

    // update
    ++m_sampleCount;
    std::swap(m_currentReadBufferID, m_currentWriteBufferID);

    // render UI
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Once, ImVec2(0.0f, 0.0f));

    const auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin("Control Panel", nullptr, flags)) {
        ImGui::End();
    } else {
        ImGui::Text("switch scenes");
        ImGui::Separator();
        static const char* scenes[] = {"scene 1", "scene 2", "scene 3"};

        ImGui::Combo("##1", &m_renderSceneIndex, scenes, IM_ARRAYSIZE(scenes));

        ImGui::NewLine();

        ImGui::Text("statistics");
        ImGui::Separator();
        ImGui::Text("samples: %u", m_sampleCount);

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void RayTracing::initShaders() {
    const char* version =
#ifdef USE_GLES
        "300 es"
#else
        "330 core"
#endif
        ;

    // TODO: modify raytracing.frag code to achieve raytracing
    m_raytracingShader.reset(new GLSLProgram);
    m_raytracingShader->attachVertexShaderFromFile(getAssetFullPath(raytracingVsRelPath), version);
    m_raytracingShader->attachFragmentShaderFromFile(
        getAssetFullPath(raytracingFsRelPath), version);
    m_raytracingShader->link();

    m_drawScreenShader.reset(new GLSLProgram);
    m_drawScreenShader->attachVertexShaderFromFile(getAssetFullPath(quadVsRelPath), version);
    m_drawScreenShader->attachFragmentShaderFromFile(getAssetFullPath(quadFsRelPath), version);
    m_drawScreenShader->link();
}

int RayTracing::getBufferHeight(size_t nObjects, size_t objectSize, size_t texComponent) const {
    size_t componentPerObject = objectSize / (sizeof(float) * texComponent);
    return static_cast<int>(nObjects * componentPerObject + BufferWidth - 1) / BufferWidth;
}

void RayTracing::createBalls() {
    m_balls.push_back(Sphere(glm::vec3(0.0f, -1000.0f, 0.0f), 1000.0f));
    m_ballMaterials.push_back(
        Material(Material::Type::Lambertian, 1.0f, 0.0f, glm::vec3(0.5f, 0.5f, 0.5f)));
    for (int a = -12; a < 12; ++a) {
        for (int b = -12; b < 12; ++b) {
            auto chooseMat = randomFloat();
            glm::vec3 center(a + 0.9f * randomFloat(), 0.2f, b + 0.9f * randomFloat());

            if ((glm::length(center - glm::vec3(0.0f, 0.2f, 0.0f)) > 2.0f)
                && (glm::length(center - glm::vec3(4.0f, 0.2f, -2.0f)) > 2.0f)
                && (glm::length(center - glm::vec3(-4.0f, 0.2f, 2.0f)) > 2.0f)
                && (glm::length(center - glm::vec3(4.0f, 0.0f, 5.0f)) > 1.0f)) {
                Material material;
                if (chooseMat < 0.8f) {
                    material.type = Material::Type::Lambertian;
                    material.ior = 1.0f;
                    material.fuzz = 0.0f;
                    material.albedo = randomVec3() * randomVec3();
                } else if (chooseMat < 0.95f) {
                    material.type = Material::Type::Metal;
                    material.ior = 1.0f;
                    material.fuzz = randomFloat(0.0f, 0.5f);
                    material.albedo = randomVec3(0.5f, 1.0f);
                } else {
                    material.type = Material::Type::Dielectric;
                    material.ior = 1.5f;
                    material.fuzz = 0.0f;
                    material.albedo = glm::vec3(1.0f, 1.0f, 1.0f);
                }

                m_balls.push_back(Sphere(center, randomFloat(0.15f, 0.2f)));
                m_ballMaterials.push_back(material);
            }
        }
    }

    // init three big sphere
    m_balls.push_back(Sphere(glm::vec3(4.0f, 1.0f, 5.0f), 1.0f));
    m_ballMaterials.push_back(
        Material(Material::Type::Dielectric, 1.5f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f)));

    m_balls.push_back(Sphere(glm::vec3(-8.0f, 2.0f, 14.0f), 2.0f));
    m_ballMaterials.push_back(
        Material(Material::Type::Lambertian, 1.0f, 0.0f, glm::vec3(0.2f, 0.4f, 0.8f)));

    m_balls.push_back(Sphere(glm::vec3(3.0f, 3.0f, -8.0f), 2.0f));
    m_ballMaterials.push_back(
        Material(Material::Type::Metal, 1.0f, 0.0f, glm::vec3(0.7f, 0.6f, 0.5f)));
}

void RayTracing::createRenderScene(int index) {
    switch (index) {
    case 0: createScene1(); break;
    case 1: createScene2(); break;
    case 2: createScene3(); break;
    default: createScene3(); break;
    }
}

void RayTracing::createPrimitiveBuffer(
    const std::vector<Sphere>& spheres, const std::vector<Model*> models,
    const std::vector<glm::mat4>& transforms, const std::vector<Material>& sphereMaterials,
    const std::vector<Material>& modelMaterials) {
    size_t totalPrimitives = 0;
    size_t totalTriangles = 0;
    size_t totalVertices = 0;
    totalPrimitives += spheres.size();
    for (const auto& model : models) {
        totalVertices += model->getVertices().size();
        totalTriangles += model->getIndices().size() / 3;
    }

    totalPrimitives += totalTriangles;
    size_t primitiveBufferSize = roundUp(totalPrimitives, BufferWidth);
    size_t materialBufferSize =
        roundUp(sphereMaterials.size() + modelMaterials.size(), BufferWidth);
    size_t vertexBufferSize = roundUp(totalVertices, BufferWidth);
    size_t triangleBufferSize = roundUp(totalTriangles, BufferWidth);
    std::vector<Vertex> vertices(vertexBufferSize);
    std::vector<Triangle> triangles(totalTriangles);
    std::vector<Primitive> primitives;
    std::vector<Material> materials(materialBufferSize);
    int primitiveCnt = 0;
    int materialCnt = 0;
    int vertexCnt = 0;
    int triangleCnt = 0;

    if (!spheres.empty()) {
        for (int i = 0; i < spheres.size(); ++i) {
            primitives.push_back(Primitive(
                Primitive::Type::Sphere, primitiveCnt + i, materialCnt + i,
                const_cast<Sphere*>(&spheres[i])));
        }

        for (const auto& material : sphereMaterials) {
            materials[materialCnt++] = material;
        }

        std::vector<Sphere> sphereBuffer(roundUp(spheres.size(), BufferWidth));
        for (int i = 0; i < spheres.size(); ++i) {
            sphereBuffer[i] = spheres[i];
        }

        m_sphereBuffer.reset(new Texture2D(
            GL_RGBA32F, BufferWidth,
            getBufferHeight(sphereBuffer.size(), sizeof(Sphere), Sphere::getTexDataComponent()),
            GL_RGBA, GL_FLOAT, sphereBuffer.data()));
        m_sphereBuffer->bind();
        m_sphereBuffer->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        m_sphereBuffer->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        m_sphereBuffer->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        m_sphereBuffer->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        m_sphereBuffer->unbind();
    } else {
        m_sphereBuffer.reset(new Texture2D(
            GL_RGBA32F, BufferWidth,
            getBufferHeight(1, sizeof(Sphere), Sphere::getTexDataComponent()), GL_RGBA, GL_FLOAT,
            nullptr));
    }

    if (!models.empty()) {
        for (int i = 0; i < models.size(); ++i) {
            const auto& modelVertices = models[i]->getVertices();
            const auto& vertIndices = models[i]->getIndices();
            for (int j = 0, k = 0; j < vertIndices.size(); j += 3, ++k) {
                triangles[triangleCnt] = Triangle(
                    vertIndices[j] + vertexCnt, vertIndices[j + 1] + vertexCnt,
                    vertIndices[j + 2] + vertexCnt, vertices.data());
                primitives.push_back(Primitive(
                    Primitive::Type::Triangle, triangleCnt, materialCnt + i,
                    const_cast<Triangle*>(&triangles[triangleCnt])));
                triangleCnt++;
            }

            if (transforms[i] != glm::mat4(1.0f)) {
                const auto& transform = transforms[i];
                auto invTransposeTransform = glm::mat3(glm::transpose(glm::inverse(transform)));
                for (const auto& vertex : modelVertices) {
                    vertices[vertexCnt++] = {
                        transform * glm::vec4(vertex.position, 1.0f),
                        invTransposeTransform * vertex.normal, vertex.texCoord};
                }
            } else {
                for (const auto& vertex : modelVertices) {
                    vertices[vertexCnt++] = vertex;
                }
            }
        }

        for (const auto& material : modelMaterials) {
            materials[materialCnt++] = material;
        }

        m_vertexBuffer.reset(new Texture2D(
            GL_RGBA32F, BufferWidth,
            getBufferHeight(vertexBufferSize, sizeof(Vertex), Sphere::getTexDataComponent()),
            GL_RGBA, GL_FLOAT, vertices.data()));
        m_vertexBuffer->bind();
        m_vertexBuffer->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        m_vertexBuffer->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        m_vertexBuffer->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        m_vertexBuffer->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        m_vertexBuffer->unbind();

        std::vector<glm::ivec3> triangleIndex(triangleBufferSize);
        int triangleIndexCnt = 0;
        for (const auto& triangle : triangles) {
            triangleIndex[triangleIndexCnt++] = {triangle.v[0], triangle.v[1], triangle.v[2]};
        }

        m_indexBuffer.reset(new Texture2D(
            GL_RGB32I, BufferWidth,
            getBufferHeight(
                triangleIndex.size(), sizeof(glm::ivec3), Triangle::getIndexTexDataComponent()),
            GL_RGB_INTEGER, GL_INT, triangleIndex.data()));
        m_indexBuffer->bind();
        m_indexBuffer->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        m_indexBuffer->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        m_indexBuffer->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        m_indexBuffer->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        m_indexBuffer->unbind();
    } else {
        m_vertexBuffer.reset(new Texture2D(
            GL_RGBA32F, BufferWidth,
            getBufferHeight(1, sizeof(Vertex), Sphere::getTexDataComponent()), GL_RGBA, GL_FLOAT,
            nullptr));
        m_indexBuffer.reset(new Texture2D(
            GL_RGB32I, BufferWidth,
            getBufferHeight(1, sizeof(glm::ivec3), Triangle::getIndexTexDataComponent()),
            GL_RGB_INTEGER, GL_INT, nullptr));
    }

    if (!materials.empty()) {
        for (auto& material : materials) {
            material.type =
                static_cast<Material::Type>(toFloatLayout(static_cast<int>(material.type)));
        }

        m_materialBuffer.reset(new Texture2D(
            GL_RGB32F, BufferWidth,
            getBufferHeight(materials.size(), sizeof(Material), Material::getTexDataComponent()),
            GL_RGB, GL_FLOAT, materials.data()));
        m_materialBuffer->bind(0);
        m_materialBuffer->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        m_materialBuffer->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        m_materialBuffer->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        m_materialBuffer->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        m_materialBuffer->unbind();
    } else {
        m_materialBuffer.reset(new Texture2D(
            GL_RGB32F, BufferWidth,
            getBufferHeight(1, sizeof(Material), Material::getTexDataComponent()), GL_RGB, GL_FLOAT,
            nullptr));
    }

    if (!primitives.empty()) {
        if (!m_useBVH) {
            std::vector<ShaderPrimitive> orderedPrim(roundUp(primitives.size(), BufferWidth));
            int primCnt = 0;
            for (const auto& prim : primitives) {
                orderedPrim[primCnt++] = {
                    static_cast<int>(prim.type), prim.shapeIdx, prim.materialIdx};
            }
            m_bvhBuffer.reset(new Texture2D(
                GL_RGB32F, BufferWidth,
                getBufferHeight(1, sizeof(BVHNode), BVHNode::getTexDataComponent()), GL_RGB,
                GL_FLOAT, nullptr));

            m_primitiveBuffer.reset(new Texture2D(
                GL_RGB32I, BufferWidth,
                getBufferHeight(
                    orderedPrim.size(), sizeof(ShaderPrimitive), Primitive::getTexDataComponent()),
                GL_RGB_INTEGER, GL_INT, orderedPrim.data()));
            m_primitiveBuffer->bind();
            m_primitiveBuffer->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            m_primitiveBuffer->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            m_primitiveBuffer->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            m_primitiveBuffer->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            m_primitiveBuffer->unbind();
            m_raytracingShader->use();
            m_raytracingShader->setUniformInt("nPrimitives", static_cast<int>(primitives.size()));

        } else {
            // build BVH
            BVH bvh(primitives);
            auto& linearBVH = bvh.nodes;
            for (auto& node : linearBVH) {
                node.type = static_cast<BVHNode::Type>(toFloatLayout(static_cast<int>(node.type)));
                node.leftChild = toFloatLayout(node.leftChild);
                node.rightChild = toFloatLayout(node.rightChild);
            }

            std::vector<BVHNode> nodes(roundUp(linearBVH.size(), BufferWidth));
            int nodeCnt = 0;
            for (const auto& node : linearBVH) {
                nodes[nodeCnt++] = node;
            }

            m_bvhBuffer.reset(new Texture2D(
                GL_RGB32F, BufferWidth,
                getBufferHeight(nodes.size(), sizeof(BVHNode), BVHNode::getTexDataComponent()),
                GL_RGB, GL_FLOAT, nodes.data()));
            m_bvhBuffer->bind();
            m_bvhBuffer->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            m_bvhBuffer->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            m_bvhBuffer->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            m_bvhBuffer->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            m_bvhBuffer->unbind();

            std::vector<ShaderPrimitive> orderedPrim(
                roundUp(bvh.orderedPrimitives.size(), BufferWidth));
            int primCnt = 0;
            for (const auto& prim : bvh.orderedPrimitives) {
                orderedPrim[primCnt++] = {
                    static_cast<int>(prim.type), prim.shapeIdx, prim.materialIdx};
            }

            m_primitiveBuffer.reset(new Texture2D(
                GL_RGB32I, BufferWidth,
                getBufferHeight(
                    orderedPrim.size(), sizeof(ShaderPrimitive), Primitive::getTexDataComponent()),
                GL_RGB_INTEGER, GL_INT, orderedPrim.data()));
            m_primitiveBuffer->bind();
            m_primitiveBuffer->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            m_primitiveBuffer->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            m_primitiveBuffer->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            m_primitiveBuffer->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            m_primitiveBuffer->unbind();
        }
    }

    std::cout << "Scene Statistics" << std::endl;
    std::cout << "+ Spheres: " << spheres.size() << std::endl;
    std::cout << "+ Models:  " << models.size() << std::endl;
    std::cout << "  + vertices:  " << totalVertices << std::endl;
    std::cout << "  + triangles: " << totalTriangles << std::endl;
}

void RayTracing::createScene1() {
    m_camera->transform.position = glm::vec3(0.0f, 0.0f, 12.0f);
    m_camera->transform.lookAt(glm::vec3(0.0f));

    m_useBVH = true;

    createPrimitiveBuffer(
        {Sphere(glm::vec3(0.0f, 0.0f, 0.0f), 1.5f), Sphere(glm::vec3(4.0f, 0.0f, 0.0f), 1.5f),
         Sphere(glm::vec3(-4.0f, 0.0f, 0.0f), 1.5f)},
        {}, {},
        {Material(Material::Type::Dielectric, 1.5f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f)),
         Material(Material::Type::Metal, 1.0f, 0.0f, glm::vec3(0.7f, 0.6f, 0.5f)),
         Material(Material::Type::Lambertian, 1.0f, 0.0f, glm::vec3(0.8f, 0.4f, 0.2f))},
        {});
}

void RayTracing::createScene2() {
    m_camera->transform.position = glm::vec3(15.0f, 3.0f, 4.0f);
    m_camera->transform.lookAt(glm::vec3(0.0f));

    m_useBVH = true;

    createPrimitiveBuffer(m_balls, {}, {}, m_ballMaterials, {});
}

void RayTracing::createScene3() {
    m_camera->transform.position = glm::vec3(15.0f, 3.0f, 4.0f);
    m_camera->transform.lookAt(glm::vec3(0.0f));

    glm::mat4 scaleT = glm::scale(glm::mat4(1.0f), glm::vec3(0.6f, 0.6f, 0.6f));
    glm::mat4 rotateT =
        glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    m_useBVH = true;

    std::vector<glm::mat4> transformations = {
        rotateT * scaleT,
        glm::translate(glm::mat4(1.0f), glm::vec3(-4.0f, 0.0f, 2.0f)) * rotateT * scaleT,
        glm::translate(glm::mat4(1.0f), glm::vec3(4.0f, 0.0f, -2.0f)) * rotateT * scaleT};

    std::vector<Material> modelMaterials = {
        Material(Material::Type::Dielectric, 1.5f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f)),
        Material(Material::Type::Metal, 1.0f, 0.0f, glm::vec3(0.7f, 0.6f, 0.5f)),
        Material(Material::Type::Lambertian, 1.0f, 0.0f, glm::vec3(0.8f, 0.4f, 0.2f))};

    createPrimitiveBuffer(
        m_balls, {m_lucy.get(), m_lucy.get(), m_lucy.get()}, transformations, m_ballMaterials,
        modelMaterials);
}

int RayTracing::toFloatLayout(int v) {
    union {
        float f;
        int i;
    } tmp;
    tmp.f = static_cast<float>(v);
    return tmp.i;
}

size_t RayTracing::roundUp(size_t val, size_t number) {
    return ((val + number - 1) / number) * number;
}