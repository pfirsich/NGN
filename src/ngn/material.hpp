#pragma once

#include <vector>
#include <unordered_map>

#include "shaderprogram.hpp"
#include "renderstateblock.hpp"
#include "uniformblock.hpp"
#include "texture.hpp"
#include "shader.hpp"

#include "hash_tuple.hpp"

namespace ngn {

    class Material : public UniformList {
    friend class Renderer;

    private:
        static ShaderProgram* getShaderPermutation(uint64_t permutationHash, const Shader& frag, const Shader& vert, const std::string& fragDefines, const std::string& vertDefines) {
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

    public:
        enum class BlendMode {
            REPLACE, // disable GL_BLEND
            TRANSLUCENT, // SRC_ALPHA, ONE_MINUS_SRC_ALPHA and ADD
            ADD, // ONE, ONE and ADD
            MODULATE, // DST_COLOR, ZERO and ADD
            SCREEN, // ONE, ONE_MINUS_SRC_COLOR and ADD
        };

        /*
        wanted: SRC_FACTOR * (ambient + diffspec_1 + diffspec_2 + ... + diffspec_N) + DST_FACTOR * dst
        first pass: SRC_FACTOR * ambient + DST_FACTOR * dst
        ADD:
            naive:
                1st pass: ONE * ambient + ONE * dst
                2nd pass: ONE * src + ONE * diffspec_1 = ONE * ambient + ONE * dst + ONE * diffspec_i
                => works!
        TRANSLUCENT:
            naive:
                1st pass: SRC_ALPHA * ambient + ONE_MINUS_SRC_ALPHA * dst
                2nd pass: SRC_ALPHA * diffspec_1 + ONE_MINUS_SRC_ALPHA * dst = SRC_ALPHA * diffspec_1 + ONE_MINUS_SRC_ALPHA * (SRC_ALPHA * ambient + ONE_MINUS_SRC_ALPHA * dst)
            right: src_factor: src_factor, dst_factor: one
        MODULATE:
            naive:
                1st pass: ambient * dst
                2nd pass: diffspec_1 * dst = diffspec_1 * ambient * dst
            It's a problem that after every pass the frambuffer has to be in a valid state, because then we can not decide to
            encode something cleverly, but have to accept the fact, that we can not retrieve the original dst in the framebuffer on further passes
            => impossible without extra work
        SCREEN:
            naive:
                1st pass: ambient + (1-ambient) * dst
                2nd pass: diffspec_1 + (1-ambient) * ambient + (1-ambient)^2 * dst
            also not possible, since we want to multiply with the final output color, but it has not been determined ye
            => impossible without extra work

        So the general rule will be to set the dst factor to ONE on subsequent passes (for transparent geometry)
        For non-transparent geometry the src factor will be ONE as well.
         */

        class Pass {
        private:
            Material& mMaterial;
            int mPassIndex;
            RenderStateBlock* mStateBlock;
            const Shader* mVertexShader;
            const Shader* mFragmentShader;

            ShaderProgram* mShaderProgram;

        public:
            Pass(Material& mat, int index, const Shader* frag = nullptr, const Shader* vert = nullptr) :
                    mMaterial(mat), mPassIndex(index), mStateBlock(nullptr), mVertexShader(vert), mFragmentShader(frag), mShaderProgram(nullptr) {
                //mFragmentShader->include(&(mMaterial.getFragmentShader()));
                //mVertexShader->include(&(mMaterial.getVertexShader()));
            }

            ~Pass() {
                delete mStateBlock;
            }

            ShaderProgram* getShaderProgram() {
                if(mShaderProgram == nullptr) { // simple caching
                    std::string passDefine = "#define NGN_PASS " + std::to_string(mPassIndex) + "\n";
                    uint64_t permutationHash = mPassIndex;
                    const Shader& frag = mFragmentShader ? *mFragmentShader : mMaterial.getFragmentShader();
                    const Shader& vert = mVertexShader ? *mVertexShader : mMaterial.getVertexShader();
                    mShaderProgram = getShaderPermutation(permutationHash, frag, vert, passDefine, passDefine);
                }
                return mShaderProgram;
            }

            RenderStateBlock& addStateBlock() {
                return *(mStateBlock = new RenderStateBlock(mMaterial.getStateBlock()));
            }

            const RenderStateBlock& getStateBlock() {
                return mStateBlock ? *mStateBlock : mMaterial.getStateBlock();
            }
        };
    private:
        BlendMode mBlendMode;
        std::unordered_map<int, Pass> mPasses;
        const Shader& mVertexShader;
        const Shader& mFragmentShader;
        RenderStateBlock mStateBlock;
        ShaderProgram* mShaderProgram;

        void validate() const;

    public:
        Material(const Shader& frag, const Shader& vert) : mBlendMode(BlendMode::REPLACE), mVertexShader(vert), mFragmentShader(frag) {}
        Material(const Material& base) : UniformList(base), mBlendMode(base.mBlendMode), mPasses(base.mPasses),
                mVertexShader(base.mVertexShader), mFragmentShader(base.mFragmentShader), mStateBlock(base.mStateBlock) {}

        Pass& addPass(int passIndex, const Shader* frag = nullptr, const Shader* vert = nullptr) {
            auto it = mPasses.find(passIndex);
            if(it != mPasses.end()) {
                LOG_ERROR("Adding pass with index %d a second time!", passIndex);
            } else {
                it = mPasses.insert(std::make_pair(passIndex, Pass(*this, passIndex, frag, vert))).first;
            }
            return it->second;
        }

        bool hasPass(int passIndex) const {
            return mPasses.find(passIndex) != mPasses.end();
        }

        Pass* getPass(int passIndex) {
            auto it = mPasses.find(passIndex);
            if(it != mPasses.end()) return &(it->second);
            return nullptr;
        }

        void removePass(int passIndex) {
            mPasses.erase(passIndex);
        }

        const Shader& getVertexShader() const {return mVertexShader;}
        const Shader& getFragmentShader() const {return mFragmentShader;}
        const RenderStateBlock& getStateBlock() const {return mStateBlock;}

        // Render state
        void setBlendMode(BlendMode mode);
        BlendMode getBlendMode() const {return mBlendMode;}

        bool getDepthWrite() const {return mStateBlock.getDepthWrite();}
        void setDepthWrite(bool write) {mStateBlock.setDepthWrite(write);}

        DepthFunc getDepthTest() const {return mStateBlock.getDepthTest();}
        void setDepthTest(DepthFunc func = DepthFunc::LEQUAL) {mStateBlock.setDepthTest(func);}

        FaceDirections getCullFaces() const {return mStateBlock.getCullFaces();}
        void setCullFaces(FaceDirections dirs = FaceDirections::BACK) {mStateBlock.setCullFaces(dirs);}

        FaceOrientation getFrontFace() const {return mStateBlock.getFrontFace();}
        void setFrontFace(FaceOrientation ori) {mStateBlock.setFrontFace(ori);}
    };
}