#pragma once

#include <unordered_map>
#include <tuple>

#include "shaderprogram.hpp"
#include "shader.hpp"

#include "hash_tuple.hpp"

namespace ngn {
    class ShaderCache {
    private:
        using keyType = std::tuple<uint64_t, const FragmentShader*, const VertexShader*>;
        std::unordered_map<keyType, ShaderProgram*, hash_tuple::hash<keyType> > mCacheEntries;

    public:
        ~ShaderCache() {
            for(auto& entry : mCacheEntries) delete entry.second;
        }

        ShaderProgram* getShaderPermutation(uint64_t permutationHash, const FragmentShader* frag, const VertexShader* vert,
            const std::string& fragDefines = "", const std::string& vertDefines = "");
    };
}
