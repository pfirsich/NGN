#include <fstream>
#include <memory>
#include <sstream>

#include <glad/glad.h>

#include "shaderprogram.hpp"
#include "log.hpp"

namespace ngn {
    ShaderProgram* ShaderProgram::currentShaderProgram = nullptr;
    const char* ShaderProgram::shaderTypeNames[] = {"Vertex", "Fragment"};

    ShaderProgram::ShaderProgram() : mStatus(Status::EMPTY) {}

    ShaderProgram::ShaderProgram(const char* fragfile, const char* vertfile) : mStatus(Status::EMPTY) {
        compileAndLinkFromFiles(fragfile, vertfile);
    }

    ShaderProgram::~ShaderProgram() {
        if(glIsProgram(mProgramObject)) {
            glDeleteProgram(mProgramObject);
        }
    }

    bool ShaderProgram::link() {
        if(mStatus != Status::UNLINKED) {
            LOG_ERROR("To link a shader program, the status must be UNLINKED");
            return false;
        }

        mProgramObject = glCreateProgram();
        for(auto shader: mShaderObjects) {
            glAttachShader(mProgramObject, shader);
        }
        glLinkProgram(mProgramObject);
        for(auto shader: mShaderObjects) {
            glDetachShader(mProgramObject, shader);
            glDeleteShader(shader);
        }
        GLint linkStatus;
        glGetProgramiv(mProgramObject, GL_LINK_STATUS, &linkStatus);
        if(linkStatus == GL_FALSE) {
            mStatus = Status::LINK_FAILED;
            GLint logLength = 0;
            glGetProgramiv(mProgramObject, GL_INFO_LOG_LENGTH, &logLength);
            std::unique_ptr<GLchar> programLog(new GLchar[logLength]);
            glGetProgramInfoLog(mProgramObject, logLength, nullptr, programLog.get());
            LOG_ERROR("Program link failed: %s", programLog.get());
            return false;
        } else {
            mStatus = Status::LINKED;
            LOG_DEBUG("Linked shader %d", mProgramObject);
            return true;
        }
    }

    bool ShaderProgram::compileShaderFromString(const char* source, ShaderProgram::ShaderType type) {
        LOG_DEBUG("%s:\n%s", type == ShaderType::FRAGMENT ? "fragment" : "vertex", source);

        if(mStatus != Status::EMPTY && mStatus != Status::UNLINKED) {
            LOG_ERROR("To compile and attach a shader, the status must be EMPTY or UNLINKED");
        }

        GLenum GLtype = 0;
        if(type == ShaderType::VERTEX) {
            GLtype = GL_VERTEX_SHADER;
        } else if(type == ShaderType::FRAGMENT) {
            GLtype = GL_FRAGMENT_SHADER;
        } else {
            LOG_ERROR("Unknown shader type");
            return false;
        }

        GLuint shader = glCreateShader(GLtype);
        //LOG_DEBUG(source);
        glShaderSource(shader, 1, &source, nullptr);

        GLint compileStatus;
        glCompileShader(shader);
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
        if(compileStatus == GL_FALSE){
            mStatus = Status::COMPILE_SHADER_FAILED;
            GLint logLength;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
            std::unique_ptr<GLchar> shaderLog(new GLchar[logLength]);
            glGetShaderInfoLog(shader, logLength, nullptr, shaderLog.get());
            LOG_ERROR("%s shader compile failed: %s", shaderTypeNames[static_cast<int>(type)], shaderLog.get());
            return false;
        } else {
            mStatus = Status::UNLINKED;
            mShaderObjects.push_back(shader);
            return true;
        }
    }

    GLint ShaderProgram::getAttributeLocation(const std::string& name) {
        auto it = mAttributeLocations.find(name);
        if(it == mAttributeLocations.end()) {
            GLint loc = glGetAttribLocation(mProgramObject, name.c_str());
            if(loc == -1) {
                LOG_ERROR("Attribute '%s' does not exist in shader program.", name.c_str());
            }
            mAttributeLocations[name] = loc;
            return loc;
        } else {
            return it->second;
        }
    }

    GLint ShaderProgram::getUniformLocation(const std::string& name) {
        auto it = mUniformLocations.find(name);
        if(it == mUniformLocations.end()) {
            GLint loc = glGetUniformLocation(mProgramObject, name.c_str());
            if(loc == -1) {
                LOG_ERROR("Uniform '%s' does not exist in shader program.", name.c_str());
            }
            mUniformLocations[name] = loc;
            return loc;
        } else {
            return it->second;
        }
    }

    bool ShaderProgram::compileShaderFromFile(const char* filename, ShaderProgram::ShaderType type) {
        std::ifstream file(filename, std::ios_base::in);
        if(file){
            // It's absolutely ridiculous and embarrassing how hard it is to read a file into a string correctly.
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string contents = buffer.str();

            return compileShaderFromString(contents.c_str(), type);
        } else{
            LOG_ERROR("Shader file '%s' could not be opened.", filename);
            return false;
        }
    }
}