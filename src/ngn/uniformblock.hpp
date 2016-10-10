#pragma once

#include <cstring>
#include <unordered_map>
#include <vector>
#include <utility>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "log.hpp"
#include "texture.hpp"
#include "shaderprogram.hpp"

namespace ngn {
    class UniformBlock {
    protected:
        enum class ParamType {
            FLOAT, INT, VECF2, VECF3, VECF4, MATF2, MATF3, MATF4
        };

        struct ParamData {
            ParamType type;
            size_t count;
            const void* constData;
            void* ownedData;
            bool dirty;

            ParamData(ParamType type) : type(type), count(0), constData(nullptr), ownedData(nullptr), dirty(false) {}

            size_t getSize() const { // in bytes
                switch(type) {
                    case ParamType::FLOAT: return count * sizeof(float);
                    case ParamType::INT:   return count * sizeof(int);
                    case ParamType::VECF2: return count * sizeof(glm::vec2);
                    case ParamType::VECF3: return count * sizeof(glm::vec3);
                    case ParamType::VECF4: return count * sizeof(glm::vec4);
                    case ParamType::MATF2: return count * sizeof(glm::mat2);
                    case ParamType::MATF3: return count * sizeof(glm::mat3);
                    case ParamType::MATF4: return count * sizeof(glm::mat4);
                }
                return 0; // Just so gcc doesn't whine
            }
        };

        std::unordered_map<ShaderProgram::UniformGUID, ParamData> mParameters;
        std::vector<std::tuple<ShaderProgram::UniformGUID, ResourceHandle<Texture>, int> > mTextures;

        void deleteParam(ParamData& param) {
            if(param.ownedData) {
                switch(param.type) {
                    case ParamType::FLOAT: delete[] reinterpret_cast<const float*>(param.ownedData); break;
                    case ParamType::INT:   delete[] reinterpret_cast<const int*>(param.ownedData); break;
                    case ParamType::VECF2: delete[] reinterpret_cast<const glm::vec2*>(param.ownedData); break;
                    case ParamType::VECF3: delete[] reinterpret_cast<const glm::vec3*>(param.ownedData); break;
                    case ParamType::VECF4: delete[] reinterpret_cast<const glm::vec4*>(param.ownedData); break;
                    case ParamType::MATF2: delete[] reinterpret_cast<const glm::mat2*>(param.ownedData); break;
                    case ParamType::MATF3: delete[] reinterpret_cast<const glm::mat3*>(param.ownedData); break;
                    case ParamType::MATF4: delete[] reinterpret_cast<const glm::mat4*>(param.ownedData); break;
                }
                param.ownedData = nullptr;
            }
        }

        template<class T>
        void setParam(ShaderProgram::UniformGUID uniformGuid, ParamType type, const T* ptr, size_t count, bool copy) {
            auto it = mParameters.find(uniformGuid);
            if(it != mParameters.end()) {
                if(it->second.type != type || it->second.count != count) {
                    LOG_ERROR("Setting uniform value with a different type than previous set!");
                    return;
                }
            } else {
                it = mParameters.emplace(uniformGuid, type).first;
            }

            assert(it->second.constData == nullptr || it->second.ownedData == nullptr);
            if(copy) {
                if(it->second.count != count) deleteParam(it->second);
                if(!it->second.ownedData) it->second.ownedData = new T[count];

                it->second.constData = nullptr;
                std::memcpy(it->second.ownedData, ptr, sizeof(T)*count);
            } else {
                it->second.constData = ptr;
                deleteParam(it->second);
            }

            it->second.count = count;
            it->second.dirty = true;
        }

        template<class T>
        void setParam(const char* name, ParamType type, const T* ptr, size_t count, bool copy) {
            setParam<T>(ShaderProgram::getUniformGUID(name), type, ptr, count, copy);
        }
    public:
        UniformBlock() {}

        virtual ~UniformBlock() {
            for(auto& param : mParameters) {
                deleteParam(param.second);
            }
        }

        UniformBlock(const UniformBlock& other) {
            mTextures = other.mTextures;
            mParameters = other.mParameters;
            for(auto& param : mParameters) {
                const void* temp = param.second.ownedData;
                if(temp) {
                    size_t size = param.second.getSize();
                    param.second.ownedData = new uint8_t[size];
                    std::memcpy(param.second.ownedData, temp, size);
                }
            }
        }

        UniformBlock& operator=(const UniformBlock&) = delete;

        template<class T>
        void setFloat(const T& id, float val) {
            setParam<float>(id, ParamType::FLOAT, &val, 1, true);
        }
        template<class T>
        void setFloatArray(const T& id, const float* vp, size_t count = 1, bool copy = false) {
            setParam<float>(id, ParamType::FLOAT, vp, count, copy);
        }

        template<class T>
        void setInteger(const T& id, int val) {
            setParam<int>(id, ParamType::INT, &val, 1, true);
        }
        template<class T>
        void setIntegerArray(const T& id, const int* vp, size_t count = 1, bool copy = false) {
            setParam<int>(id, ParamType::INT, vp, count, copy);
        }

        template<class T>
        void setVector2(const T& id, const glm::vec2& val) {
            setParam<glm::vec2>(id, ParamType::VECF2, &val, 1, true);
        }
        template<class T>
        void setVector2Array(const T& id, const glm::vec2* vp, size_t count = 1, bool copy = false) {
            setParam<glm::vec2>(id, ParamType::VECF2, vp, count, copy);
        }

        template<class T>
        void setVector3(const T& id, const glm::vec3& val) {
            setParam<glm::vec3>(id, ParamType::VECF3, &val, 1, true);
        }
        template<class T>
        void setVector3Array(const T& id, const glm::vec3* vp, size_t count = 1, bool copy = false) {
            setParam<glm::vec3>(id, ParamType::VECF3, vp, count, copy);
        }

        template<class T>
        void setVector4(const T& id, const glm::vec4& val) {
            setParam<glm::vec4>(id, ParamType::VECF4, &val, 1, true);
        }
        template<class T>
        void setVector4Array(const T& id, const glm::vec4* vp, size_t count = 1, bool copy = false) {
            setParam<glm::vec4>(id, ParamType::VECF4, vp, count, copy);
        }

        template<class T>
        void setMatrix2(const T& id, const glm::mat2& val) {
            setParam<glm::mat2>(id, ParamType::MATF2, &val, 1, true);
        }
        template<class T>
        void setMatrix2Array(const T& id, const glm::mat2* vp, size_t count = 1, bool copy = false) {
            setParam<glm::mat2>(id, ParamType::MATF2, vp, count, copy);
        }

        template<class T>
        void setMatrix3(const T& id, const glm::mat3& val) {
            setParam<glm::mat3>(id, ParamType::MATF3, &val, 1, true);
        }
        template<class T>
        void setMatrix3Array(const T& id, const glm::mat3* vp, size_t count = 1, bool copy = false) {
            setParam<glm::mat3>(id, ParamType::MATF3, vp, count, copy);
        }

        template<class T>
        void setMatrix4(const T& id, const glm::mat4& val) {
            setParam<glm::mat4>(id, ParamType::MATF4, &val, 1, true);
        }
        template<class T>
        void setMatrix4Array(const T& id, const glm::mat4* vp, size_t count = 1, bool copy = false) {
            setParam<glm::mat4>(id, ParamType::MATF4, vp, count, copy);
        }

        void setTexture(ShaderProgram::UniformGUID uniformGuid, const ResourceHandle<Texture>& tex, int unit = -1) {
            for(auto& elem : mTextures) {
                if(std::get<0>(elem) == uniformGuid) {
                    std::get<1>(elem) = tex;
                    return;
                }
            }
            mTextures.push_back(std::make_tuple(uniformGuid, tex, unit));
        }
        void setTexture(const char* name, const ResourceHandle<Texture>& tex, int unit = -1) {
            setTexture(ShaderProgram::getUniformGUID(name), tex, unit);
        }

        virtual void apply() = 0;
    };

    class UniformList : public UniformBlock {
    public:
        UniformList() : UniformBlock() {}
        UniformList(const UniformList& other) : UniformBlock(other) {}

        void apply();
    };
}
