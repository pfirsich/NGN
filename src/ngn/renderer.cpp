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

    void Renderer::render(SceneNode* root, Camera* camera) {
        updateState();
        if(autoClear) clear();
        stateBlock.apply();

        static std::vector<RenderQueueEntry> renderQueue;
        if(renderQueue.size() == 0) renderQueue.reserve(2048);
        renderQueue.clear();

        UniformList globalUniforms;
        globalUniforms.setMatrix4("projection", camera->getProjectionMatrix());
        globalUniforms.setMatrix4("view", camera->getViewMatrix());

        std::vector<UniformList*> perObjectUniformList;

        // Build queue
        std::stack<SceneNode*> traversalStack;
        traversalStack.push(root);
        while(!traversalStack.empty()) {
            SceneNode* top = traversalStack.top();
            traversalStack.pop();

            // handle current node
            Mesh* mesh = top->getMesh();
            if(mesh) {
                Material* mat = top->getMaterial();
                assert(mat != nullptr); // Rendering a mesh without a material is impossible

                UniformList* objUniforms = new UniformList;
                perObjectUniformList.push_back(objUniforms);
                glm::mat4 model = top->getWorldMatrix();
                glm::mat4 modelview = camera->getViewMatrix() * model;
                glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelview)));
                objUniforms->setMatrix4("model", model);
                objUniforms->setMatrix4("modelview", modelview);
                objUniforms->setMatrix3("normalMatrix", normalMatrix);
                objUniforms->setMatrix4("modelviewprojection", camera->getProjectionMatrix() * modelview);

                renderQueue.emplace_back(mat->mShader, mesh, &(mat->mStateBlock));
                renderQueue.back().uniformBlocks.push_back(objUniforms);
                renderQueue.back().uniformBlocks.push_back(&globalUniforms);
                renderQueue.back().uniformBlocks.push_back(mat);
            }

            // chilren
            for(auto child : top->getChildren()) {
                traversalStack.push(child);
            }
        }

        renderRenderQueue(renderQueue);

        // Can I do this differently? (i.e. better)
        for(auto ulist : perObjectUniformList) delete ulist;
    }
}