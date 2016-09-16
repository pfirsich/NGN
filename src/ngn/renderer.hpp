#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "object.hpp"
#include "camera.hpp"

namespace ngn {
    // ForwardRenderer, DeferredRenderer
    // most classes should only contain data, while the renderer objects contain most of the rendering logic
    // not sure how materials will be decoupled from the renderers (if at all)

    class Renderer {
    public:
        // Variables that store the current GL state
        static glm::vec4 currentClearColor;
        static float currentClearDepth;
        static GLint currentClearStencil;
        static glm::ivec4 currentViewport;
        static glm::ivec4 currentScissor;

        bool autoClear, autoClearColor, autoClearDepth, autoClearStencil;

        glm::vec4 clearColor;
        float clearDepth;
        GLint clearStencil;

        bool scissorTest;

        // x, y, width, height
        glm::ivec4 viewport;
        glm::ivec4 scissor;

        Renderer() : autoClear(true), autoClearColor(true), autoClearDepth(true), autoClearStencil(false),
                clearColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f)), clearDepth(0.0f), clearStencil(0), scissorTest(false),
                viewport(glm::ivec4(0, 0, 0, 0)), scissor(glm::ivec4(0, 0, 0, 0)) {}
        ~Renderer() {}

        void updateState() const;
        //setRenderTarget
        void clear(bool color, bool depth, bool stencil) const;
        void clear() const {clear(autoClearColor, autoClearDepth, autoClearStencil);}
        virtual void render(const Object& root, const Camera& camera);
    };
}
