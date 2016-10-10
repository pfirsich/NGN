#include <stack>

#include "renderer.hpp"
#include "shader.hpp"

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
    bool Renderer::staticInitialized = false;

    const int Renderer::AMBIENT_PASS = 1;
    const int Renderer::LIGHT_PASS = 2;
    const int Renderer::SHADOWMAP_PASS = 4;

    namespace UniformGUIDs {
        ShaderProgram::UniformGUID ngn_modelMatrixGUID;
        ShaderProgram::UniformGUID ngn_viewMatrixGUID;
        ShaderProgram::UniformGUID ngn_modelViewMatrixGUID;
        ShaderProgram::UniformGUID ngn_normalMatrixGUID;
        ShaderProgram::UniformGUID ngn_projectionMatrixGUID;
        ShaderProgram::UniformGUID ngn_modelViewProjectionMatrixGUID;

        ShaderProgram::UniformGUID ngn_light_typeGUID;
        ShaderProgram::UniformGUID ngn_light_radiusGUID;
        ShaderProgram::UniformGUID ngn_light_attenCutoffGUID;
        ShaderProgram::UniformGUID ngn_light_colorGUID;
        ShaderProgram::UniformGUID ngn_light_positionGUID;
        ShaderProgram::UniformGUID ngn_light_directionGUID;

        ShaderProgram::UniformGUID ngn_light_innerAngleGUID;
        ShaderProgram::UniformGUID ngn_light_outerAngleGUID;

        ShaderProgram::UniformGUID ngn_light_shadowedGUID;
        ShaderProgram::UniformGUID ngn_light_shadowMapGUID;
        ShaderProgram::UniformGUID ngn_light_shadowBiasGUID;
        ShaderProgram::UniformGUID ngn_light_toLightSpaceGUID;
    }

    void Renderer::staticInitialize() {
        Shader::globalShaderPreamble += "#define NGN_PASS_FORWARD_AMBIENT " + std::to_string(AMBIENT_PASS) + "\n";
        Shader::globalShaderPreamble += "#define NGN_PASS_FORWARD_LIGHT " + std::to_string(LIGHT_PASS) + "\n";
        Shader::globalShaderPreamble += "#define NGN_PASS_FORWARD(x) (x >= " + std::to_string(AMBIENT_PASS) + " && x <= " + std::to_string(LIGHT_PASS) + ")\n\n";
        Shader::globalShaderPreamble += "#define NGN_PASS_SHADOWMAP_PASS " + std::to_string(SHADOWMAP_PASS) + "\n";
        Shader::globalShaderPreamble += "\n";

        #define STRINGIFY_LIGHT_TYPE(x) ("NGN_LIGHT_TYPE_" #x " " + std::to_string(static_cast<int>(LightData::LightType::x)))
        Shader::globalShaderPreamble += "#define " + STRINGIFY_LIGHT_TYPE(POINT) + "\n";
        Shader::globalShaderPreamble += "#define " + STRINGIFY_LIGHT_TYPE(DIRECTIONAL) + "\n";
        Shader::globalShaderPreamble += "#define " + STRINGIFY_LIGHT_TYPE(SPOT) + "\n";
        Shader::globalShaderPreamble +=
R"(
#extension GL_ARB_explicit_uniform_location : enable
layout(location = 1) uniform mat4 ngn_modelMatrix;
layout(location = 2) uniform mat4 ngn_viewMatrix;
layout(location = 3) uniform mat4 ngn_modelViewMatrix;
layout(location = 4) uniform mat3 ngn_normalMatrix;
layout(location = 5) uniform mat4 ngn_projectionMatrix;
layout(location = 6) uniform mat4 ngn_modelViewProjectionMatrix;

struct ngn_LightParameters {
    int type;
    float radius;
    float attenCutoff;
    vec3 color;
    vec3 position; // view/camera space
    vec3 direction; // view/camera space

    float innerAngle; // cos(angle)
    float outerAngle; // cos(angle)

    bool shadowed;
    sampler2DShadow shadowMap;
    float shadowBias;
    mat4 toLightSpace;
};
layout(location = 7) uniform ngn_LightParameters ngn_light;

)";

        UniformGUIDs::ngn_modelMatrixGUID = ShaderProgram::getUniformGUID("ngn_modelMatrix");
        UniformGUIDs::ngn_viewMatrixGUID = ShaderProgram::getUniformGUID("ngn_viewMatrix");
        UniformGUIDs::ngn_modelViewMatrixGUID = ShaderProgram::getUniformGUID("ngn_modelViewMatrix");
        UniformGUIDs::ngn_normalMatrixGUID = ShaderProgram::getUniformGUID("ngn_normalMatrix");
        UniformGUIDs::ngn_projectionMatrixGUID = ShaderProgram::getUniformGUID("ngn_projectionMatrix");
        UniformGUIDs::ngn_modelViewProjectionMatrixGUID = ShaderProgram::getUniformGUID("ngn_modelViewProjectionMatrix");

        UniformGUIDs::ngn_light_typeGUID = ShaderProgram::getUniformGUID("ngn_light.type");
        UniformGUIDs::ngn_light_radiusGUID = ShaderProgram::getUniformGUID("ngn_light.radius");
        UniformGUIDs::ngn_light_attenCutoffGUID = ShaderProgram::getUniformGUID("ngn_light.attenCutoff");
        UniformGUIDs::ngn_light_colorGUID = ShaderProgram::getUniformGUID("ngn_light.color");
        UniformGUIDs::ngn_light_positionGUID = ShaderProgram::getUniformGUID("ngn_light.position");
        UniformGUIDs::ngn_light_directionGUID = ShaderProgram::getUniformGUID("ngn_light.direction");
        UniformGUIDs::ngn_light_innerAngleGUID = ShaderProgram::getUniformGUID("ngn_light.innerAngle");
        UniformGUIDs::ngn_light_outerAngleGUID = ShaderProgram::getUniformGUID("ngn_light.outerAngle");

        UniformGUIDs::ngn_light_shadowedGUID = ShaderProgram::getUniformGUID("ngn_light.shadowed");
        UniformGUIDs::ngn_light_shadowMapGUID = ShaderProgram::getUniformGUID("ngn_light.shadowMap");
        UniformGUIDs::ngn_light_shadowBiasGUID = ShaderProgram::getUniformGUID("ngn_light.shadowBias");
        UniformGUIDs::ngn_light_toLightSpaceGUID = ShaderProgram::getUniformGUID("ngn_light.toLightSpace");

        Renderer::staticInitialized = true;
    }

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

    void Renderer::render(SceneNode* root, Camera* camera, bool regenerateQueue, bool doRenderQueue) {
        updateState();
        // make sure depth write is enabled before we clear
        glDepthMask((RenderStateBlock::currentDepthWrite = true) ? GL_TRUE : GL_FALSE);
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
                    rendererData->uniforms.setMatrix4(UniformGUIDs::ngn_modelMatrixGUID, model);
                    rendererData->uniforms.setMatrix4(UniformGUIDs::ngn_viewMatrixGUID, viewMatrix);
                    rendererData->uniforms.setMatrix4(UniformGUIDs::ngn_projectionMatrixGUID, projectionMatrix);
                    rendererData->uniforms.setMatrix4(UniformGUIDs::ngn_modelViewMatrixGUID, modelview);
                    rendererData->uniforms.setMatrix3(UniformGUIDs::ngn_normalMatrixGUID, normalMatrix);
                    rendererData->uniforms.setMatrix4(UniformGUIDs::ngn_modelViewProjectionMatrixGUID, projectionMatrix * modelview);
                }

                LightData* lightData = node->getLightData();
                if(lightData) {
                    if(lightData->getShadow()) lightData->getShadow()->updateCamera();
                    lightLists[static_cast<int>(lightData->getType())].push_back(node);
                }

                for(auto child : node->getChildren()) {
                    traversalStack.push(child);
                }
            }

            // build render queue
            renderQueue.clear();

            //LOG_DEBUG("----- frame");

            // generate shadow maps
            glColorMask(false, false, false, false);
            for(size_t ltype = 0; ltype < LIGHT_TYPE_COUNT; ++ltype) {
                for(size_t l = 0; l < lightLists[ltype].size(); ++l) {
                    SceneNode* light = lightLists[ltype][l];
                    LightData* lightData = light->getLightData();
                    LightData::Shadow* shadow = lightData->getShadow();
                    if(shadow) {
                        glm::mat4 lightViewMatrix(shadow->getCamera()->getViewMatrix());
                        glm::mat4 lightProjectionMatrix(shadow->getCamera()->getProjectionMatrix());

                        for(size_t i = 0; i < linearizedSceneGraph.size(); ++i) {
                            SceneNode* node = linearizedSceneGraph[i];
                            Mesh* mesh = node->getMesh();
                            if(mesh) {
                                Material* mat = node->getMaterial();
                                assert(mat != nullptr);
                                Material::Pass* pass = mat->getPass(SHADOWMAP_PASS);
                                if(!pass) pass = mat->getPass(AMBIENT_PASS);

                                if(pass) {
                                    renderQueue.emplace_back(mat, pass, mesh);
                                    RenderQueueEntry& entry = renderQueue.back();

                                    glm::mat4 model = node->getWorldMatrix();
                                    glm::mat4 modelview = lightViewMatrix * model;
                                    glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelview)));
                                    entry.perEntryUniforms.setMatrix4(UniformGUIDs::ngn_modelMatrixGUID, model);
                                    entry.perEntryUniforms.setMatrix4(UniformGUIDs::ngn_viewMatrixGUID, lightViewMatrix);
                                    entry.perEntryUniforms.setMatrix4(UniformGUIDs::ngn_projectionMatrixGUID, lightProjectionMatrix);
                                    entry.perEntryUniforms.setMatrix4(UniformGUIDs::ngn_modelViewMatrixGUID, modelview);
                                    entry.perEntryUniforms.setMatrix3(UniformGUIDs::ngn_normalMatrixGUID, normalMatrix);
                                    entry.perEntryUniforms.setMatrix4(UniformGUIDs::ngn_modelViewProjectionMatrixGUID, lightProjectionMatrix * modelview);
                                }
                            }
                        }
                        shadow->mShadowMap.bind();
                        glClear(GL_DEPTH_BUFFER_BIT);
                        if(doRenderQueue) renderRenderQueue(renderQueue);
                        renderQueue.clear();
                    }
                }
            }

            Rendertarget::unbind();
            glViewport(viewport.x, viewport.y, viewport.z, viewport.w);
            glColorMask(true, true, true, true);
            for(bool drawTransparent = false; ; drawTransparent = !drawTransparent) {
                // ambient pass
                for(size_t i = 0; i < linearizedSceneGraph.size(); ++i) {
                    SceneNode* node = linearizedSceneGraph[i];
                    Mesh* mesh = node->getMesh();
                    if(mesh) {
                        Material* mat = node->getMaterial();
                        assert(mat != nullptr);
                        Material::Pass* pass = mat->getPass(AMBIENT_PASS);

                        if(pass) {
                            if(drawTransparent == pass->getStateBlock().getBlendEnabled()) {
                                renderQueue.emplace_back(mat, pass, mesh);
                                RenderQueueEntry& entry = renderQueue.back();

                                RendererData* rendererData = node->rendererData[mRendererIndex];
                                entry.uniformBlocks.push_back(&(rendererData->uniforms));
                                //LOG_DEBUG("ambient (obj %d) - transparent: %d\n", node->getId(), drawTransparent);
                            }
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
                        Material::Pass* pass = mat->getPass(LIGHT_PASS);
                        if(pass) {
                            if(drawTransparent == pass->getStateBlock().getBlendEnabled()) {
                                for(size_t ltype = 0; ltype < LIGHT_TYPE_COUNT; ++ltype) {
                                    // later: sort by influence and take the N most influential lights
                                    for(size_t l = 0; l < lightLists[ltype].size(); ++l) {
                                        SceneNode* light = lightLists[ltype][l];
                                        LightData* lightData = light->getLightData();

                                        renderQueue.emplace_back(mat, pass, mesh);
                                        RenderQueueEntry& entry = renderQueue.back();

                                        RendererData* rendererData = node->rendererData[mRendererIndex];
                                        entry.uniformBlocks.push_back(&(rendererData->uniforms));

                                        // TODO: Move this into a separate uniform block per light!
                                        entry.perEntryUniforms.setInteger(UniformGUIDs::ngn_light_typeGUID,        static_cast<int>(lightData->getType()));
                                        entry.perEntryUniforms.setFloat(  UniformGUIDs::ngn_light_radiusGUID,      lightData->getRadius());
                                        entry.perEntryUniforms.setFloat(  UniformGUIDs::ngn_light_attenCutoffGUID, lightData->getAttenCutoff());
                                        entry.perEntryUniforms.setVector3(UniformGUIDs::ngn_light_colorGUID,       lightData->getColor());
                                        entry.perEntryUniforms.setVector3(UniformGUIDs::ngn_light_positionGUID,    glm::vec3(viewMatrix * glm::vec4(light->getPosition(), 1.0f)));
                                        entry.perEntryUniforms.setVector3(UniformGUIDs::ngn_light_directionGUID,   glm::vec3(viewMatrix * glm::vec4(light->getForward(), 0.0f)));
                                        if(lightData->getType() == LightData::LightType::SPOT) {
                                            entry.perEntryUniforms.setFloat(UniformGUIDs::ngn_light_innerAngleGUID, lightData->getInnerAngle());
                                            entry.perEntryUniforms.setFloat(UniformGUIDs::ngn_light_outerAngleGUID, lightData->getOuterAngle());
                                        }

                                        LightData::Shadow* shadow = lightData->getShadow();
                                        if(shadow) {
                                            glm::mat4 toLightSpace = shadow->getCamera()->getProjectionMatrix() * shadow->getCamera()->getViewMatrix();
                                            entry.perEntryUniforms.setInteger(UniformGUIDs::ngn_light_shadowedGUID, 1);
                                            entry.perEntryUniforms.setFloat(UniformGUIDs::ngn_light_shadowBiasGUID, shadow->getBias());
                                            entry.perEntryUniforms.setTexture(UniformGUIDs::ngn_light_shadowMapGUID, &shadow->mShadowMapTexture, 15);
                                            entry.perEntryUniforms.setMatrix4(UniformGUIDs::ngn_light_toLightSpaceGUID, toLightSpace);
                                        } else {
                                            entry.perEntryUniforms.setInteger(UniformGUIDs::ngn_light_shadowedGUID, 0);
                                        }

                                        std::pair<RenderStateBlock::BlendFactor, RenderStateBlock::BlendFactor> blendFactors = entry.stateBlock.getBlendFactors();
                                        blendFactors.second = RenderStateBlock::BlendFactor::ONE;
                                        if(!drawTransparent) {
                                            blendFactors.first = RenderStateBlock::BlendFactor::ONE;
                                        }
                                        entry.stateBlock.setBlendFactors(blendFactors);
                                        entry.stateBlock.setBlendEnabled(true);

                                        entry.stateBlock.setDepthTest(entry.stateBlock.getAdditionalPassDepthFunc());
                                        // If the ambient pass already wrote depth, we don't have to do it again
                                        // If it didn't then we certainly don't want to do it now
                                        entry.stateBlock.setDepthWrite(false);
                                        //LOG_DEBUG("light %d (obj %d) - transparent: %d\n", light->getId(), node->getId(), drawTransparent);
                                    }
                                }
                            }
                        }
                    }
                }
                if(drawTransparent) break;
            }
        }

        if(doRenderQueue) renderRenderQueue(renderQueue);
    }
}