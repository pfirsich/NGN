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

    void Renderer::updateState() const {
        if(currentViewport != viewport) {
            currentViewport = viewport;
            glViewport(viewport.x, viewport.y, viewport.z, viewport.w);
        }
        if(scissorTest && currentScissor != scissor) {
            currentScissor = scissor;
            glScissor(scissor.x, scissor.y, scissor.z, scissor.w);
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
            currentScissorTest = scissorTest;
            if(scissorTest)
                glEnable(GL_SCISSOR_TEST);
            else
                glDisable(GL_SCISSOR_TEST);
        }

        if(currentDepthWrite != depthWrite) {
            currentDepthWrite = depthWrite;
            glDepthMask(depthWrite);
        }

        if(currentDepthFunc != depthFunc) {
            currentDepthFunc = depthFunc;
            glDepthFunc(depthFunc);
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

    void Renderer::render(const Object& root, const Camera& camera) {
        if(autoClear) clear();
    }
}