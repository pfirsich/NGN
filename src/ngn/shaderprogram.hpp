#pragma once

#include <vector>
#include <unordered_map>
#include <string>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "texture.hpp"

namespace ngn {
    // Maybe introduce a new class Shader that represents a Shader object, so they can be
    // attached to multiple program objects
    class ShaderProgram {
    public:
        enum class ShaderType {
            VERTEX = 0, FRAGMENT = 1
        };
        static const char* shaderTypeNames[];
        enum class Status {
            EMPTY, UNLINKED, LINKED, COMPILE_SHADER_FAILED, LINK_FAILED
        };

        using UniformLocation = GLint;
        using UniformGUID = uint32_t;

        static const ShaderProgram* currentShaderProgram;

    private:
        GLuint mProgramObject;
        std::vector<GLuint> mShaderObjects;
        Status mStatus;
        mutable std::unordered_map<std::string, UniformLocation> mAttributeLocations; // only caches
        mutable std::unordered_map<std::string, UniformLocation> mUniformLocations;
        // bool holds if the location is already initialized
        mutable std::vector<UniformLocation> mUniformGUIDLocationMap;

        static std::unordered_map<std::string, UniformGUID> uniformNameGUIDMap;
        static std::vector<std::string> uniformGUIDNameMap;
        static UniformGUID nextUniformGUID;

    public:
        inline static UniformLocation getUniformGUID(const char* name) {
            auto it = uniformNameGUIDMap.find(name);
            if(it == uniformNameGUIDMap.end()) {
                UniformGUID guid = nextUniformGUID++;
                uniformNameGUIDMap[name] = guid;
                uniformGUIDNameMap.push_back(name);
                return guid;
            }
            return it->second;
        }

        ShaderProgram();
        ShaderProgram(const char* fragfile, const char* vertfile);
        ~ShaderProgram();

        bool compileShaderFromString(const char* source, ShaderType type);
        bool compileVertShaderFromString(const char* source) {
            return compileShaderFromString(source, ShaderType::VERTEX);
        }
        bool compileFragShaderFromString(const char* source) {
            return compileShaderFromString(source, ShaderType::FRAGMENT);
        }
        bool compileAndLinkFromStrings(const char* frag, const char* vert) {
            return  compileFragShaderFromString(frag) &&
                    compileVertShaderFromString(vert) &&
                    link();
        }

        // Should this even exist?
        bool compileShaderFromFile(const char* filename, ShaderType type);
        bool compileVertShaderFromFile(const char* filename) {
            return compileShaderFromFile(filename, ShaderType::VERTEX);
        }
        bool compileFragShaderFromFile(const char* filename) {
            return compileShaderFromFile(filename, ShaderType::FRAGMENT);
        }
        bool compileAndLinkFromFiles(const char* fragfile, const char* vertfile) {
            return  compileFragShaderFromFile(fragfile) &&
                    compileVertShaderFromFile(vertfile) &&
                    link();
        }

        UniformLocation getAttributeLocation(const std::string& name) const;
        UniformLocation getUniformLocation(const std::string& name) const;

        inline UniformLocation getUniformLocation(UniformGUID guid) const {
            int sizeDiff = guid - mUniformGUIDLocationMap.size() + 1;
            if(sizeDiff > 0) {
                mUniformGUIDLocationMap.reserve(sizeDiff*2);
                for(int i = 0; i < sizeDiff - 1; ++i) mUniformGUIDLocationMap.push_back(-0xFF);

                UniformLocation loc = getUniformLocation(uniformGUIDNameMap[guid]);
                mUniformGUIDLocationMap.push_back(loc);
                return loc;
            } else {
                UniformLocation loc = mUniformGUIDLocationMap[guid];
                if(loc < -1) loc = mUniformGUIDLocationMap[guid] = getUniformLocation(uniformGUIDNameMap[guid]);
                return loc;
            }
        }

        bool link();

        inline void bind() const {
            if(currentShaderProgram != this) {
                glUseProgram(mProgramObject);
                currentShaderProgram = this;
            }
        }
        // This could be static, but I want it to look like other bindables
        void unbind() const {
            glUseProgram(0);
            currentShaderProgram = nullptr;
        }

        GLuint getProgramObject() const {
            return mProgramObject;
        }
        Status getStatus() const {
            return mStatus;
        }

        template<typename T>
        void setUniform(const std::string& name, const T& val) const {
            UniformLocation loc = getUniformLocation(name);
            if(loc != -1) setUniform(loc, val);
        }

        void setUniform(const std::string& name, const Texture& tex) const {
            UniformLocation loc = getUniformLocation(name);
            if(loc != -1) setUniform(loc, tex);
        }

        void setUniform(UniformLocation loc, int value) const {
            glUniform1i(loc, value);
        }

        void setUniform(UniformLocation loc, float value) const {
            glUniform1f(loc, value);
        }

        void setUniform(UniformLocation loc, const glm::vec2& val) const {
            glUniform2fv(loc, 1, glm::value_ptr(val));
        }

        void setUniform(UniformLocation loc, const glm::vec3& val) const {
            glUniform3fv(loc, 1, glm::value_ptr(val));
        }

        void setUniform(UniformLocation loc, const glm::vec4& val) const {
            glUniform4fv(loc, 1, glm::value_ptr(val));
        }

        void setUniform(UniformLocation loc, const glm::mat2& val) const {
            glUniformMatrix2fv(loc, 1, GL_FALSE, glm::value_ptr(val));
        }

        void setUniform(UniformLocation loc, const glm::mat3& val) const {
            glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(val));
        }

        void setUniform(UniformLocation loc, const glm::mat4& val) const {
            glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(val));
        }

        void setUniform(UniformLocation loc, const Texture& tex) const {
            int unit = tex.bind();
            //LOG_DEBUG("bind texture %d to %d and write unit to uniform location %d", tex.getTextureObject(), unit, loc);
            glUniform1i(loc, unit);
        }
    };
}