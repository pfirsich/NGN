#include "uniformblock.hpp"
#include "shader.hpp"

namespace ngn {
    void UniformList::apply() {
        ShaderProgram* current = ShaderProgram::currentShaderProgram; // TODO!
        for(auto& param : mParameters) {
            GLuint loc = current->getUniformLocation(param.first);
            size_t c = param.second.count;
            assert(param.second.constData == nullptr || param.second.ownedData == nullptr);
            const void* data = param.second.constData ? param.second.constData : param.second.ownedData;
            switch(param.second.type) {
                case ParamType::FLOAT: glUniform1fv(loc, c, reinterpret_cast<const float*>(data)); break;
                case ParamType::INT:   glUniform1iv(loc, c, reinterpret_cast<const int*>(data)); break;
                case ParamType::VECF2: glUniform2fv(loc, c, glm::value_ptr(*reinterpret_cast<const glm::vec2*>(data))); break;
                case ParamType::VECF3: glUniform3fv(loc, c, glm::value_ptr(*reinterpret_cast<const glm::vec3*>(data))); break;
                case ParamType::VECF4: glUniform4fv(loc, c, glm::value_ptr(*reinterpret_cast<const glm::vec4*>(data))); break;
                case ParamType::MATF2: glUniformMatrix2fv(loc, c, GL_FALSE, glm::value_ptr(*reinterpret_cast<const glm::mat2*>(data))); break;
                case ParamType::MATF3: glUniformMatrix3fv(loc, c, GL_FALSE, glm::value_ptr(*reinterpret_cast<const glm::mat3*>(data))); break;
                case ParamType::MATF4: glUniformMatrix4fv(loc, c, GL_FALSE, glm::value_ptr(*reinterpret_cast<const glm::mat4*>(data))); break;
            }
        }
    }
}