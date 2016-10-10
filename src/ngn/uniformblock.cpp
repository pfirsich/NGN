#include "uniformblock.hpp"
#include "shaderprogram.hpp"

namespace ngn {
    void UniformList::apply() {
        const ShaderProgram* current = ShaderProgram::currentShaderProgram;
        if(current) {
            for(auto& param : mParameters) {
                ShaderProgram::UniformLocation loc = current->getUniformLocation(param.first);
                if(loc != -1) {
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

            for(bool explicitUnit = true;; explicitUnit = false) {
                for(size_t i = 0; i < mTextures.size(); ++i) {
                    auto& tex = mTextures[i];
                    ShaderProgram::UniformLocation loc = current->getUniformLocation(std::get<0>(tex));
                    if(loc != -1) {
                        if(explicitUnit) {
                            if(std::get<2>(tex) >= 0) {
                                std::get<1>(tex).getResource()->bind(std::get<2>(tex));
                                glUniform1i(loc, std::get<2>(tex));
                            }
                        } else {
                            if(std::get<2>(tex) < 0) {
                                glUniform1i(loc, std::get<1>(tex).getResource()->bind());
                            }
                        }

                    }
                }
                if(!explicitUnit) break;
            }
        }
    }
}