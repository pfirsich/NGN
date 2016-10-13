#include "shadercache.hpp"

namespace ngn  {
    ShaderProgram* ShaderCache::getShaderPermutation(uint64_t permutationHash, const FragmentShader* frag, const VertexShader* vert,
            const std::string& fragDefines, const std::string& vertDefines) {

        auto keyTuple = std::make_tuple(permutationHash, frag, vert);
        auto it = mCacheEntries.find(keyTuple);
        if(it == mCacheEntries.end()) {
            ShaderProgram* prog = new ShaderProgram;
            if(!prog->compileAndLinkFromStrings(frag->getFullString(fragDefines).c_str(),
                                                vert->getFullString(vertDefines).c_str())) {
                delete prog;
                return nullptr;
            }
            mCacheEntries.insert(std::make_pair(keyTuple, prog));
            return prog;
        } else {
            return it->second;
        }
    }
}