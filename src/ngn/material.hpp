#pragma once

#include <vector>

#include "shader.hpp"
#include "renderstateblock.hpp"
#include "uniformblock.hpp"

namespace ngn {
    class Material : public UniformList {
    public:
        ShaderProgram* mShader;
        //std::vector<Texture*> mTextures;
        RenderStateBlock mStateBlock;

        Material(ShaderProgram* shader) : mShader(shader) {}
    };
}