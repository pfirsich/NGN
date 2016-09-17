#pragma once

#include <cstring>
#include <map>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "log.hpp"

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
        };

        std::map<std::string, ParamData> mParameters;

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
        void setParam(const char* name, ParamType type, const T* ptr, size_t count, bool copy) {
            auto it = mParameters.find(name);
            if(it != mParameters.end()) {
                if(it->second.type != type || it->second.count != count) {
                    LOG_ERROR("Setting uniform value with a different type than previous set!");
                    return;
                }
            } else {
                it = mParameters.emplace(std::string(name), type).first;
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
    public:
        UniformBlock() {}
        ~UniformBlock() {
            for(auto& param : mParameters) {
                deleteParam(param.second);
            }
        }

        void setFloat(const char* name, float val) {
            setParam<float>(name, ParamType::FLOAT, &val, 1, true);
        }
        void setFloatArray(const char* name, const float* vp, size_t count = 1, bool copy = false) {
            setParam<float>(name, ParamType::FLOAT, vp, count, copy);
        }

        void setInteger(const char* name, int val) {
            setParam<int>(name, ParamType::INT, &val, 1, true);
        }
        void setIntegerArray(const char* name, const int* vp, size_t count = 1, bool copy = false) {
            setParam<int>(name, ParamType::INT, vp, count, copy);
        }

        void setVector2(const char* name, const glm::vec2& val){
            setParam<glm::vec2>(name, ParamType::VECF2, &val, 1, true);
        }
        void setVector2Array(const char* name, const glm::vec2* vp, size_t count = 1, bool copy = false){
            setParam<glm::vec2>(name, ParamType::VECF2, vp, count, copy);
        }

        void setVector3(const char* name, const glm::vec3& val){
            setParam<glm::vec3>(name, ParamType::VECF3, &val, 1, true);
        }
        void setVector3Array(const char* name, const glm::vec3* vp, size_t count = 1, bool copy = false){
            setParam<glm::vec3>(name, ParamType::VECF3, vp, count, copy);
        }

        void setVector4(const char* name, const glm::vec4& val){
            setParam<glm::vec4>(name, ParamType::VECF4, &val, 1, true);
        }
        void setVector4Array(const char* name, const glm::vec4* vp, size_t count = 1, bool copy = false){
            setParam<glm::vec4>(name, ParamType::VECF4, vp, count, copy);
        }

        void setMatrix2(const char* name, const glm::mat2& val){
            setParam<glm::mat2>(name, ParamType::MATF2, &val, 1, true);
        }
        void setMatrix2Array(const char* name, const glm::mat2* vp, size_t count = 1, bool copy = false){
            setParam<glm::mat2>(name, ParamType::MATF2, vp, count, copy);
        }

        void setMatrix3(const char* name, const glm::mat3& val){
            setParam<glm::mat3>(name, ParamType::MATF3, &val, 1, true);
        }
        void setMatrix3Array(const char* name, const glm::mat3* vp, size_t count = 1, bool copy = false){
            setParam<glm::mat3>(name, ParamType::MATF3, vp, count, copy);
        }

        void setMatrix4(const char* name, const glm::mat4& val){
            setParam<glm::mat4>(name, ParamType::MATF4, &val, 1, true);
        }
        void setMatrix4Array(const char* name, const glm::mat4* vp, size_t count = 1, bool copy = false){
            setParam<glm::mat4>(name, ParamType::MATF4, vp, count, copy);
        }

        virtual void apply() = 0;
    };

    class UniformList : public UniformBlock {
    public:
        void apply();
    };
}
