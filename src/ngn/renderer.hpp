#pragma once

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "scenenode.hpp"
#include "renderstateblock.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "camera.hpp"
#include "uniformblock.hpp"

namespace ngn {
    // ForwardRenderer, DeferredRenderer
    // most classes should only contain data, while the renderer objects contain most of the rendering logic
    // not sure how materials will be decoupled from the renderers (if at all)

    class Renderer {
    protected:
        struct RenderQueueEntry {
            ShaderProgram* shaderProgram;
            std::vector<UniformBlock*> uniformBlocks;
            Mesh* mesh;
            const RenderStateBlock* stateBlock;

            RenderQueueEntry(ShaderProgram* shader, Mesh* mesh, const RenderStateBlock* state) :
                    shaderProgram(shader), mesh(mesh), stateBlock(state) {}
        };

        void renderRenderQueue(std::vector<RenderQueueEntry>& queue) {
            for(auto& entry : queue) {
                entry.stateBlock->apply();
                entry.shaderProgram->bind();
                for(auto block : entry.uniformBlocks) block->apply();
                entry.mesh->draw();
            }
        }

    public:
        // Variables that store the current GL state
        static glm::vec4 currentClearColor;
        static float currentClearDepth;
        static GLint currentClearStencil;
        static glm::ivec4 currentViewport;
        static glm::ivec4 currentScissor;
        static bool currentScissorTest;

        bool autoClear, autoClearColor, autoClearDepth, autoClearStencil;

        glm::vec4 clearColor;
        float clearDepth;
        GLint clearStencil;

        bool scissorTest;

        RenderStateBlock stateBlock;

        // x, y, width, height
        glm::ivec4 viewport;
        glm::ivec4 scissor;

        Renderer() : autoClear(true), autoClearColor(true), autoClearDepth(true), autoClearStencil(false),
                clearColor(currentClearColor), clearDepth(currentClearDepth), clearStencil(currentClearStencil), scissorTest(currentScissorTest),
                viewport(currentViewport), scissor(currentScissor) {
            stateBlock.setCullFaces(RenderStateBlock::FaceDirections::BACK);
            stateBlock.setDepthTest(RenderStateBlock::DepthFunc::LESS);
            stateBlock.apply(true);
        }
        ~Renderer() {}

        // implement: single color, trilight (ground, sky, equator), cubemap
        //void setAmbientLightingModel(const LightingModel* model);

        void updateState() const;
        //setRenderTarget
        void clear(bool color, bool depth, bool stencil) const;
        void clear() const {clear(autoClearColor, autoClearDepth, autoClearStencil);}
        virtual void render(SceneNode* root, Camera* camera);
    };
}
