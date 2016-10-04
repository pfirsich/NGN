#pragma once

#include <vector>
#include <unordered_map>

#include "shaderprogram.hpp"
#include "renderstateblock.hpp"
#include "uniformblock.hpp"
#include "texture.hpp"
#include "shader.hpp"
#include "resource.hpp"

#include "hash_tuple.hpp"

namespace ngn {

    class Material : public UniformList, public Resource {
    friend class Renderer;

    private:
        static ShaderProgram* getShaderPermutation(uint64_t permutationHash, const FragmentShader* frag, const VertexShader* vert,
                    const std::string& fragDefines, const std::string& vertDefines);

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
            const Material& mMaterial;
            int mPassIndex;
            RenderStateBlock* mStateBlock;
            ResourceHandle<FragmentShader>* mFragmentShader;
            ResourceHandle<VertexShader>* mVertexShader;

            mutable const ShaderProgram* mShaderProgram; // just used as cache

        public:
            //mMaterial(mat), mPassIndex(index), mStateBlock(nullptr), mShadersDirty(true),
            //mVertexShader(nullptr), mFragmentShader(nullptr), mShaderProgram(nullptr)
            Pass(const Material& mat, int index) : mMaterial(mat), mPassIndex(index), mStateBlock(nullptr),
                    mFragmentShader(nullptr), mVertexShader(nullptr), mShaderProgram(nullptr) {}

            Pass(const Pass& other) = delete;

            Pass(const Material& mat, const Pass& other) : mMaterial(mat), mPassIndex(other.mPassIndex), mStateBlock(nullptr),
                    mFragmentShader(nullptr), mVertexShader(nullptr), mShaderProgram(other.mShaderProgram) {
                if(other.mFragmentShader) mFragmentShader = new ResourceHandle<FragmentShader>(*other.mFragmentShader);
                if(other.mVertexShader) mVertexShader = new ResourceHandle<VertexShader>(*other.mVertexShader);
                if(other.mStateBlock) mStateBlock = new RenderStateBlock(*other.mStateBlock);
            }

            ~Pass() {
                delete mStateBlock;
                delete mVertexShader;
                delete mFragmentShader;
            }

            RenderStateBlock& addStateBlock() {
                return *(mStateBlock = new RenderStateBlock(mMaterial.getStateBlock()));
            }

            const RenderStateBlock& getStateBlock() {
                return mStateBlock ? *mStateBlock : mMaterial.getStateBlock();
            }

            RenderStateBlock* getPassStateBlock() {
                return mStateBlock;
            }

            void setFragmentShader(const ResourceHandle<FragmentShader>& frag) {
                if(mFragmentShader) {
                    *mFragmentShader = frag;
                } else {
                    mFragmentShader = new ResourceHandle<FragmentShader>(frag);
                }
            }

            void setVertexShader(const ResourceHandle<VertexShader>& vert) {
                if(mVertexShader) {
                    *mVertexShader = vert;
                } else {
                    mVertexShader = new ResourceHandle<VertexShader>(vert);
                }
            }

            const ResourceHandle<FragmentShader>& getFragmentShader() const {return *mFragmentShader;}
            const ResourceHandle<VertexShader>& getVertexShader() const {return *mVertexShader;}

            int getPassIndex() const {return mPassIndex;}

            const ShaderProgram* getShaderProgram() const {
                const ResourceHandle<FragmentShader>& frag = mFragmentShader ? *mFragmentShader : mMaterial.getFragmentShader();
                const ResourceHandle<VertexShader>& vert = mVertexShader ? *mVertexShader : mMaterial.getVertexShader();
                if(frag.dirty() || vert.dirty()) { // handles point to different resource
                    std::string defines = "#define NGN_PASS " + std::to_string(mPassIndex) + "\n";
                    uint64_t permutationHash = mPassIndex;
                    mShaderProgram = getShaderPermutation(permutationHash, frag.getResource(), vert.getResource(), defines, defines);
                }
                return mShaderProgram;
            }
        };
    private:
        BlendMode mBlendMode;
        std::unordered_map<int, Pass> mPasses;
        ResourceHandle<FragmentShader> mFragmentShader;
        ResourceHandle<VertexShader> mVertexShader;
        RenderStateBlock mStateBlock;

        void validate() const;

        static bool staticInitialized;
        static void staticInitialize();

    public:
        static Material* fallback;
        static Material* fromFile(const char* filename);

        Material(const ResourceHandle<FragmentShader>& frag, const ResourceHandle<VertexShader>& vert) :
                mBlendMode(BlendMode::REPLACE), mFragmentShader(frag), mVertexShader(vert) {
            if(!staticInitialized) staticInitialize();
        }

        Material(const Material& base, const ResourceHandle<FragmentShader>& frag, const ResourceHandle<VertexShader>& vert) :
                UniformList(base), mBlendMode(base.mBlendMode), mFragmentShader(frag), mVertexShader(vert), mStateBlock(base.mStateBlock) {
            if(!staticInitialized) staticInitialize();
            for(auto& it : base.mPasses) {
                addPass(it.second);
            }
        }

        Material(const Material& base) : Material(base, base.mFragmentShader, base.mVertexShader) {}
        Material(const Material& base, const ResourceHandle<FragmentShader>& frag) : Material(base, frag, base.mVertexShader) {}
        Material(const Material& base, const ResourceHandle<VertexShader>& vert) : Material(base, base.mFragmentShader, vert) {}

        Material& operator=(const Material& other) = delete;

        //void setVertexShader(ResourceHandle<VertexShader>&& vert) {mVertexShader = vert;}
        //void setFragmentShader(ResourceHandle<FragmentShader>&& frag) {mFragmentShader = frag;}
        const ResourceHandle<FragmentShader>& getFragmentShader() const {return mFragmentShader;}
        const ResourceHandle<VertexShader>& getVertexShader() const {return mVertexShader;}
        RenderStateBlock& getStateBlock() {return mStateBlock;}
        const RenderStateBlock& getStateBlock() const {return mStateBlock;}

        Pass& addPass(int passIndex) {
            auto it = mPasses.find(passIndex);
            if(it != mPasses.end()) {
                LOG_ERROR("Adding pass with index %d a second time!", passIndex);
            } else {
                // I hate this so much and it's a farce that it's so hard to emplace an object into a map without invoking the copy constructor
                it = mPasses.emplace(std::piecewise_construct, std::forward_as_tuple(passIndex),
                                                               std::forward_as_tuple(*this, passIndex)).first;
            }
            return it->second;
        }

        Pass& addPass(const Pass& other) {
            auto it = mPasses.find(other.getPassIndex());
            if(it != mPasses.end()) {
                LOG_ERROR("Adding pass with index %d a second time!", other.getPassIndex());
            } else {
                it = mPasses.emplace(std::piecewise_construct, std::forward_as_tuple(other.getPassIndex()),
                                                               std::forward_as_tuple(*this, other)).first;
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

        void setBlendMode(BlendMode mode);
        // If you modify the state block yourself (regarding blending of course) this will return the last set value
        BlendMode getBlendMode() const {return mBlendMode;}
    };
}