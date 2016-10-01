#include "material.hpp"
#include "renderer.hpp"

namespace ngn {
    static ShaderProgram* Material::getShaderPermutation(uint64_t permutationHash, const Shader& frag, const Shader& vert, const std::string& fragDefines, const std::string& vertDefines) {
        using keyType = std::tuple<uint64_t, const Shader*, const Shader*>;
        static std::unordered_map<keyType, ShaderProgram*, hash_tuple::hash<keyType> > shaderCache;

        auto keyTuple = std::make_tuple(permutationHash, &frag, &vert);
        auto it = shaderCache.find(keyTuple);
        if(it == shaderCache.end()) {
            ShaderProgram* prog = new ShaderProgram;
            if(!prog->compileAndLinkFromStrings(frag.getFullString(fragDefines).c_str(),
                                                vert.getFullString(vertDefines).c_str())) {
                delete prog;
                return nullptr;
            }
            shaderCache.insert(std::make_pair(keyTuple, prog));
            return prog;
        } else {
            return it->second;
        }
    }

    void Material::validate() const {
        if((mBlendMode == BlendMode::MODULATE || mBlendMode == BlendMode::SCREEN) && (hasPass(Renderer::LIGHT_PASS)))
            LOG_WARNING("Blend mode MODULATE and SCREEN don't work properly with lit materials!");
    }

    void Material::setBlendMode(BlendMode mode) {
        mBlendMode = mode;
        switch(mode) {
            case BlendMode::REPLACE:
                mStateBlock.setBlendEnabled(false);
                mStateBlock.setDepthWrite(true);
                break;
            case BlendMode::TRANSLUCENT:
                mStateBlock.setBlendEnabled(true);
                mStateBlock.setBlendEquation(RenderStateBlock::BlendEq::ADD);
                mStateBlock.setBlendFactors(RenderStateBlock::BlendFactor::SRC_ALPHA, RenderStateBlock::BlendFactor::ONE_MINUS_SRC_ALPHA);
                mStateBlock.setDepthWrite(false);
                break;
            case BlendMode::ADD:
                mStateBlock.setBlendEnabled(true);
                mStateBlock.setBlendEquation(RenderStateBlock::BlendEq::ADD);
                mStateBlock.setBlendFactors(RenderStateBlock::BlendFactor::ONE, RenderStateBlock::BlendFactor::ONE);
                mStateBlock.setDepthWrite(false);
                break;
            case BlendMode::MODULATE:
                mStateBlock.setBlendEnabled(true);
                mStateBlock.setBlendEquation(RenderStateBlock::BlendEq::ADD);
                mStateBlock.setBlendFactors(RenderStateBlock::BlendFactor::DST_COLOR, RenderStateBlock::BlendFactor::ZERO);
                mStateBlock.setDepthWrite(false);
                break;
            case BlendMode::SCREEN:
                mStateBlock.setBlendEnabled(true);
                mStateBlock.setBlendEquation(RenderStateBlock::BlendEq::ADD);
                mStateBlock.setBlendFactors(RenderStateBlock::BlendFactor::ONE, RenderStateBlock::BlendFactor::ONE_MINUS_SRC_COLOR);
                mStateBlock.setDepthWrite(false);
                break;
        }
        validate();
    }
}