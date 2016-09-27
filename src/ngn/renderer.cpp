#include <stack>

#include "renderer.hpp"

namespace ngn {
    // These values represent the OpenGL default values
    glm::vec4 Renderer::currentClearColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    float Renderer::currentClearDepth = 1.0f;
    GLint Renderer::currentClearStencil = 0.0f;
    // In fact these are not the default values for glViewport and glScissor,
    // but no one will ever want to render something with these values, so
    // I just force a first set with them
    glm::ivec4 Renderer::currentViewport = glm::ivec4(0, 0, 0, 0);
    glm::ivec4 Renderer::currentScissor = glm::ivec4(0, 0, 0, 0);
    bool Renderer::currentScissorTest = false;

    int Renderer::nextRendererIndex = 0;

    void Renderer::updateState() const {
        if(currentViewport != viewport) {
            currentViewport = viewport;
            glViewport(viewport.x, viewport.y, viewport.z, viewport.w);
            LOG_DEBUG("viewport: %d, %d, %d, %d\n", viewport.x, viewport.y, viewport.z, viewport.w);
        }
        if(scissorTest && currentScissor != scissor) {
            currentScissor = scissor;
            glScissor(scissor.x, scissor.y, scissor.z, scissor.w);
            LOG_DEBUG("scissor");
        }

        // straight up comparing float values should be fine,
        // since they are also only set here
        if(currentClearColor != clearColor) {
            currentClearColor = clearColor;
            glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        }

        if(currentClearDepth != clearDepth) {
            currentClearDepth = clearDepth;
            glClearDepth(clearDepth);
        }

        if(currentClearStencil != clearStencil) {
            currentClearStencil = clearStencil;
            glClearStencil(clearStencil);
        }

        if(currentScissorTest != scissorTest) {
            LOG_DEBUG("scissor test");
            currentScissorTest = scissorTest;
            if(scissorTest)
                glEnable(GL_SCISSOR_TEST);
            else
                glDisable(GL_SCISSOR_TEST);
        }
    }

    void Renderer::clear(bool color, bool depth, bool stencil) const {
        updateState();
        GLbitfield mask = 0;
        if(color) mask |= GL_COLOR_BUFFER_BIT;
        if(depth) mask |= GL_DEPTH_BUFFER_BIT;
        if(stencil) mask |= GL_STENCIL_BUFFER_BIT;
        glClear(mask);
    }

    void Renderer::render(SceneNode* root, Camera* camera, bool regenerateQueue) {
        updateState();
        stateBlock.apply();
        if(autoClear) clear();

        static std::vector<SceneNode*> linearizedSceneGraph;
        if(linearizedSceneGraph.capacity() == 0) linearizedSceneGraph.reserve(131072);

        static std::vector<RenderQueueEntry> renderQueue;
        if(renderQueue.capacity() == 0) renderQueue.reserve(2048);

        constexpr int LIGHT_TYPE_COUNT = static_cast<int>(LightData::LightType::LIGHT_TYPES_LAST);
        static std::vector<SceneNode*> lightLists[LIGHT_TYPE_COUNT];
        for(int i = 0; i < LIGHT_TYPE_COUNT; ++i) if(lightLists[i].capacity() == 0) lightLists[i].reserve(1024);

        if(regenerateQueue) {
            glm::mat4 viewMatrix(camera->getViewMatrix());
            glm::mat4 projectionMatrix(camera->getProjectionMatrix());

            // linearize scene graph (this should in theory not be done every frame)
            linearizedSceneGraph.clear(); // resize(0) might retain capacity?
            if(linearizedSceneGraph.capacity() == 0)
                LOG_DEBUG("vector::clear() sets vector capacity to 0 on this platform");

            for(int i = 0; i < LIGHT_TYPE_COUNT; ++i) lightLists[i].clear();

            std::stack<SceneNode*> traversalStack;
            traversalStack.push(root);
            while(!traversalStack.empty()) {
                SceneNode* node = traversalStack.top();
                traversalStack.pop();

                linearizedSceneGraph.push_back(node);

                RendererData* rendererData = node->rendererData[mRendererIndex];
                if(rendererData == nullptr) {
                    node->rendererData[mRendererIndex] = rendererData = new RendererData;
                }

                if(node->getMesh()) {
                    glm::mat4 model = node->getWorldMatrix();
                    glm::mat4 modelview = viewMatrix * model;
                    glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelview)));
                    rendererData->uniforms.setMatrix4("model", model);
                    rendererData->uniforms.setMatrix4("view", viewMatrix);
                    rendererData->uniforms.setMatrix4("projection", projectionMatrix);
                    rendererData->uniforms.setMatrix4("modelview", modelview);
                    rendererData->uniforms.setMatrix3("normalMatrix", normalMatrix);
                    rendererData->uniforms.setMatrix4("modelviewprojection", projectionMatrix * modelview);
                }

                LightData* lightData = node->getLightData();
                if(lightData) {
                    lightLists[static_cast<int>(lightData->getType())].push_back(node);
                }

                for(auto child : node->getChildren()) {
                    traversalStack.push(child);
                }
            }

            // build render queue
            renderQueue.clear();

            //LOG_DEBUG("----- frame");

            for(bool drawTransparent = false; ; drawTransparent = !drawTransparent) {
                // ambient pass
                for(size_t i = 0; i < linearizedSceneGraph.size(); ++i) {
                    SceneNode* node = linearizedSceneGraph[i];
                    Mesh* mesh = node->getMesh();
                    if(mesh) {
                        Material* mat = node->getMaterial();
                        assert(mat != nullptr);

                        if(drawTransparent == mat->mStateBlock.getBlendEnabled()) {
                            renderQueue.emplace_back(node);
                            RenderQueueEntry& queueEntry = renderQueue.back();
                            RendererData* rendererData = node->rendererData[mRendererIndex];
                            renderQueue.back().uniformBlocks.push_back(&(rendererData->uniforms));

                            queueEntry.perEntryUniforms.setInteger("ambientPass", 1);
                            //LOG_DEBUG("ambient (obj %d) - transparent: %d\n", node->getId(), drawTransparent);
                        }
                    }
                }

                // light pass
                for(size_t i = 0; i < linearizedSceneGraph.size(); ++i) {
                    SceneNode* node = linearizedSceneGraph[i];
                    Mesh* mesh = node->getMesh();
                    if(mesh) {
                        Material* mat = node->getMaterial();
                        assert(mat != nullptr);
                        if(drawTransparent == mat->mStateBlock.getBlendEnabled() && mat->getLit()) {
                            for(size_t ltype = 0; ltype < LIGHT_TYPE_COUNT; ++ltype) {
                                // later: sort by influence and take the N most influential lights
                                for(size_t l = 0; l < lightLists[ltype].size(); ++l) {
                                    SceneNode* light = lightLists[ltype][l];
                                    LightData* lightData = light->getLightData();

                                    renderQueue.emplace_back(node);
                                    RenderQueueEntry& queueEntry = renderQueue.back();
                                    RendererData* rendererData = node->rendererData[mRendererIndex];
                                    queueEntry.uniformBlocks.push_back(&(rendererData->uniforms));

                                    // TODO: Move this into a separate uniform block per light!
                                    queueEntry.perEntryUniforms.setInteger("ambientPass", 0);
                                    queueEntry.perEntryUniforms.setInteger("light.type", static_cast<int>(lightData->getType()));
                                    queueEntry.perEntryUniforms.setFloat("light.range", lightData->getRange());
                                    queueEntry.perEntryUniforms.setVector3("light.color", lightData->getColor());
                                    queueEntry.perEntryUniforms.setVector3("light.position", glm::vec3(viewMatrix * glm::vec4(light->getPosition(), 1.0f)));
                                    queueEntry.perEntryUniforms.setVector3("light.direction", glm::vec3(viewMatrix * glm::vec4(light->getForward(), 0.0f)));

                                    std::pair<RenderStateBlock::BlendFactor, RenderStateBlock::BlendFactor> blendFactors = queueEntry.stateBlock.getBlendFactors();
                                    blendFactors.second = RenderStateBlock::BlendFactor::ONE;
                                    if(!drawTransparent) {
                                        blendFactors.first = RenderStateBlock::BlendFactor::ONE;
                                    }
                                    queueEntry.stateBlock.setBlendFactors(blendFactors);
                                    queueEntry.stateBlock.setBlendEnabled(true);

                                    queueEntry.stateBlock.setDepthTest(queueEntry.stateBlock.getAdditionalPassDepthFunc());
                                    //LOG_DEBUG("light %d (obj %d) - transparent: %d\n", light->getId(), node->getId(), drawTransparent);
                                }
                            }
                        }
                    }
                }
                if(drawTransparent) break;
            }
        }

        renderRenderQueue(renderQueue);
    }
}