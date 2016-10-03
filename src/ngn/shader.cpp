#include <fstream>
#include <sstream>

#include <yaml-cpp/yaml.h>

#include "log.hpp"
#include "shader.hpp"
#include "mesh_vertexattribute.hpp"

namespace ngn {
    bool Shader::staticInitialized = false;

    Shader* Shader::fallback = nullptr;
    FragmentShader* FragmentShader::fallback = nullptr;
    VertexShader* VertexShader::fallback = nullptr;

    // I really don't like this
    // Also I'm not a fan of the values being hardcoded here. But there is not actually helpful way of doing this programmatically
    std::string Shader::globalShaderPreamble = "#version 330 core\n\n";


    void Shader::staticInitialize() {
        staticInitialized = true;

        // ¯\_(ツ)_/¯
        #define STRINGIFY_ATTR_DEFINE(x) ("NGN_ATTR_" #x " " + std::to_string(static_cast<int>(AttributeType::x)))
        globalShaderPreamble += "#define " + STRINGIFY_ATTR_DEFINE(POSITION) + "\n";
        globalShaderPreamble += "#define " + STRINGIFY_ATTR_DEFINE(NORMAL) + "\n";
        globalShaderPreamble += "#define " + STRINGIFY_ATTR_DEFINE(TANGENT) + "\n";
        globalShaderPreamble += "#define " + STRINGIFY_ATTR_DEFINE(BITANGENT) + "\n";

        globalShaderPreamble += "#define " + STRINGIFY_ATTR_DEFINE(COLOR0) + "\n";
        globalShaderPreamble += "#define " + STRINGIFY_ATTR_DEFINE(COLOR1) + "\n";

        globalShaderPreamble += "#define " + STRINGIFY_ATTR_DEFINE(BONEINDICES) + "\n";
        globalShaderPreamble += "#define " + STRINGIFY_ATTR_DEFINE(BONEWEIGHTS) + "\n";

        globalShaderPreamble += "#define " + STRINGIFY_ATTR_DEFINE(TEXCOORD0) + "\n";
        globalShaderPreamble += "#define " + STRINGIFY_ATTR_DEFINE(TEXCOORD1) + "\n";
        globalShaderPreamble += "#define " + STRINGIFY_ATTR_DEFINE(TEXCOORD2) + "\n";
        globalShaderPreamble += "#define " + STRINGIFY_ATTR_DEFINE(TEXCOORD3) + "\n";

        globalShaderPreamble += "#define " + STRINGIFY_ATTR_DEFINE(CUSTOM0) + "\n";
        globalShaderPreamble += "#define " + STRINGIFY_ATTR_DEFINE(CUSTOM1) + "\n";
        globalShaderPreamble += "#define " + STRINGIFY_ATTR_DEFINE(CUSTOM2) + "\n";
        globalShaderPreamble += "#define " + STRINGIFY_ATTR_DEFINE(CUSTOM3) + "\n";
        globalShaderPreamble += "#define " + STRINGIFY_ATTR_DEFINE(CUSTOM4) + "\n";
        globalShaderPreamble += "#define " + STRINGIFY_ATTR_DEFINE(CUSTOM5) + "\n";
        globalShaderPreamble += "#define " + STRINGIFY_ATTR_DEFINE(CUSTOM6) + "\n";
        globalShaderPreamble += "#define " + STRINGIFY_ATTR_DEFINE(CUSTOM7) + "\n";
        globalShaderPreamble += "\n";

        FragmentShader::fallback = new FragmentShader;
        FragmentShader::fallback->setSource("out vec3 fragColor;\nvoid main() {fragColor = vec3(1.0, 0.0, 0.0);}");

        VertexShader::fallback = new VertexShader;
        VertexShader::fallback->addAttribute("attrPos", "vec3", {{"location", "NGN_ATTR_POSITION"}});
        VertexShader::fallback->addUniform("ngn_modelViewProjectionMatrix", "mat4");
        VertexShader::fallback->setSource("void main() {gl_Position = ngn_modelViewProjectionMatrix * vec4(attrPos, 1.0);}");
    }

    std::string Shader::ShaderVariable::getString(const std::string& qualifier) const {
        std::string ret;

        if(layoutQualifiers.size() > 0) {
            ret += "layout(";
            for(auto& qual : layoutQualifiers) {
                ret += qual.first;
                if(qual.second.length() > 0) {
                    ret += "=" + qual.second;
                }
                ret += ", ";
            }
            ret = ret.substr(0, ret.length() - 2) + ") ";
        }

        ret += qualifier + " " + type + " " + name + ";";

        return ret;
    }

    int getLineInString(const std::string& str, size_t pos) {
        int line = 1;
        for(size_t i = 0; i < pos; ++i) {
            if(str[i] == '\n') line += 1;
        }
        return line;
    }

    std::string strTrim(const std::string& str) {
        size_t start = std::string::npos, end = std::string::npos;
        size_t len = str.length();
        for(size_t i = 0; i < len; ++i) {
            if(str[i] > ' ' && start == std::string::npos) start = i;
            if(str[len-1-i] > ' ' && end == std::string::npos) end = len-i;
        }
        if(start == std::string::npos || end == std::string::npos) {
            assert(start == end); // Something's fishy if only one is npos
            return "";
        }
        return str.substr(start, end-start);
    }
    inline size_t advanceToWhitespace(const std::string& str, size_t& i) {
        size_t len = str.length();
        for(; i < len; ++i) if(str[i] <= ' ') break;
        return i;
    }

    inline size_t advanceToNonWhitespace(const std::string& str, size_t& i) {
        size_t len = str.length();
        for(; i < len; ++i) if(str[i] > ' ') break;
        return i;
    }

    bool inIdentifier(char c) {
        return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
    }

    bool Shader::parsePragmas(const std::string& str) {
        mPragmas.clear();

        bool newLine = true;
        for(size_t i = 0; i < str.length(); ++i) {
            if(newLine && str[i] == '#') { // preprocessor directive
                std::string pragmaSig("#pragma ngn ");
                if(str.substr(i, pragmaSig.length()) == pragmaSig) {
                    PragmaInfo pragmaInfo;
                    pragmaInfo.pragmaStart = i;
                    pragmaInfo.lineStart = getLineInString(str, i);

                    i += pragmaSig.length();
                    auto nameStart = advanceToNonWhitespace(str, i);
                    // find next whitespace to extract name
                    auto nextWhitespace = std::string::npos;
                    for(; i < str.length(); ++i) {
                        if(nextWhitespace == std::string::npos && str[i] <= ' ') nextWhitespace = i;
                        if(str[i] == '\n') {
                            pragmaInfo.nextLineStart = i;
                            break;
                        }
                    }

                    if(nextWhitespace == std::string::npos) {
                        LOG_ERROR("No whitespace after pragma name");
                        return false;
                    }
                    pragmaInfo.name = str.substr(nameStart, nextWhitespace - nameStart);

                    if(pragmaInfo.nextLineStart == std::string::npos) {
                        LOG_ERROR("No newline found after pragma '%s'", pragmaInfo.name.c_str());
                        return false;
                    }

                    pragmaInfo.params = strTrim(str.substr(nextWhitespace, pragmaInfo.nextLineStart - nextWhitespace));

                    if(pragmaInfo.name == "slot") {
                        // function declaration: type name (...) ...
                        // first advance to the start of type
                        advanceToNonWhitespace(str, i);
                        // advance to the first whitespace (in front of name)
                        advanceToWhitespace(str, i);
                        // advance to the beginning of name
                        auto nameStart = advanceToNonWhitespace(str, i);
                        for(; i < str.length(); ++i) if(!inIdentifier(str[i])) break;
                        std::string funcName = str.substr(nameStart, i - nameStart);

                        if(pragmaInfo.params.length() == 0) {
                            pragmaInfo.params = funcName;
                        } else {
                            if(funcName != pragmaInfo.params)
                                LOG_ERROR("slot name '%s' does not match with declared function name '%s'", pragmaInfo.params.c_str(), funcName.c_str());
                        }

                        // find next open curly brace
                        for(; i < str.length(); ++i) {
                            if(str[i] == '{') {
                                pragmaInfo.nextBlockStart = i;
                                break;
                            }
                            // If we find a ';' before we find a '{', it's probably a forward declaration
                            if(str[i] == ';') {
                                break;
                            }
                        }
                        if(pragmaInfo.nextBlockStart == std::string::npos) {
                            // If forward declared, pretend it's a block that has size 0 right after the declaration
                            pragmaInfo.nextBlockStart = i;
                            pragmaInfo.nextBlockEnd = i;
                            pragmaInfo.lineNextBlockEnd = getLineInString(str, i);
                        } else {
                            bool inComment = false;
                            bool multilineComment = false;
                            int braceCounter = 0;
                            for(; i < str.length(); ++i) {
                                if(str[i] == '{' && !inComment) braceCounter += 1;
                                if(str[i] == '}' && !inComment) {
                                    braceCounter -= 1;
                                    if(braceCounter == 0) {
                                        pragmaInfo.nextBlockEnd = i;
                                        pragmaInfo.lineNextBlockEnd = getLineInString(str, i);
                                        break;
                                    }
                                }
                                if(str.substr(i, 2) == "//") {
                                    inComment = true;
                                    multilineComment = false;
                                }
                                if(str.substr(i, 2) == "/*") {
                                    inComment = true;
                                    multilineComment = true;
                                }
                                if(inComment) {
                                    if(multilineComment) {
                                        if(str.substr(i, 2) == "*/") inComment = false;
                                    } else {
                                        if(str[i] == '\n') inComment = false;
                                    }
                                }
                            }

                            if(pragmaInfo.nextBlockEnd == std::string::npos) {
                                LOG_ERROR("Found no matching '}' to '{' of the block after pragma '%s' (%s)",
                                    pragmaInfo.name.c_str(), pragmaInfo.params.c_str());
                                return false;
                            }
                        }
                    } else if(pragmaInfo.name == "include") {

                    } else if(pragmaInfo.name == "uniform") {

                    } else if(pragmaInfo.name == "attribute") {

                    }

                    mPragmas.push_back(pragmaInfo);
                    LOG_DEBUG("pragma '%s' ('%s'): lineStart: %d, pragmaStart: %d, nextLineStart: %d, nextBlockStart: %d, nextBlockEnd: %d, lineNextBlockEnd: %d",
                        pragmaInfo.name.c_str(), pragmaInfo.params.c_str(), pragmaInfo.lineStart, pragmaInfo.pragmaStart,
                        pragmaInfo.nextLineStart, pragmaInfo.nextBlockStart, pragmaInfo.nextBlockEnd, pragmaInfo.lineNextBlockEnd);
                }
            }

            if(str[i] == '\n') newLine = true;
            if(str[i] > ' ') newLine = false; // non-whitespace
        }

        return true;
    }

    std::vector<std::string> Shader::getPragmaSlots() const {
        std::vector<std::string> ret;
        for(auto& pragma : mPragmas)
            if(pragma.name == "slot")
                ret.emplace_back(pragma.params);
        return ret;
    }

    template<typename T>
    void mergeIntoVectorSet(std::vector<T>& a, const std::vector<T>& b) {
        for(auto& bval : b) {
            bool found = false;
            for(auto& aval : a) if(aval == bval) found = true;
            if(!found) a.push_back(bval);
        }
    }

    std::string Shader::getFullString(const std::string& preamble, const std::vector<std::string>& overrideSlots, bool globalPreamble) const {
        //LOG_DEBUG("dbg");
        // If you don't specify the version, it will assume OpenGL 1.1
        std::string ret = globalPreamble ? globalShaderPreamble : "";
        ret += preamble;

        std::vector<std::string> slots(overrideSlots);
        mergeIntoVectorSet(slots, getPragmaSlots());

        std::vector<std::string> parts;
        for(int i = mIncludes.size() - 1; i >= 0; i -= 1) {
            const Shader* include = mIncludes[i];
            parts.insert(parts.begin(), include->getFullString("", slots, false));
            mergeIntoVectorSet(slots, include->getPragmaSlots());
        }
        for(auto& part : parts) ret += part + "\n";
        //LOG_DEBUG("includes done");

        for(auto& var : mAttributes) {
            ret += var.getString("in") + "\n";
        }
        ret += "\n";
        //LOG_DEBUG("invariables done");

        for(auto& var : mUniforms) {
            ret += var.getString("uniform") + "\n";
        }
        ret += "\n";
        //LOG_DEBUG("uniforms done");

        // iterate from the back, because otherwise the indices are off
        std::string strippedBody = mSource;
        for(int i = mPragmas.size() - 1; i >= 0; i -= 1) {
            const PragmaInfo& pragma = mPragmas[i];
            if(pragma.name == "slot") {
                bool keep = true;
                for(auto& slotName : overrideSlots) {
                    if(slotName == pragma.params) keep = false;
                }
                if(!keep) {
                    // remove body and turn it into forward declaration (also it seems to be okay to have multiple forward declarations)
                    //LOG_DEBUG("erase '%s'", strippedBody.substr(pragma.nextBlockStart, pragma.nextBlockEnd - pragma.nextBlockStart + 1).c_str());
                    strippedBody.erase(pragma.nextBlockStart, pragma.nextBlockEnd - pragma.nextBlockStart + 1);
                    // +1 because the #line directive sets the line number of the next line and +1 because the forward declaration takes one line (hopefully)
                    strippedBody.insert(pragma.nextBlockStart, ";\n#line " + std::to_string(pragma.lineNextBlockEnd + 1 + 1));
                }
            }
        }
        ret += "#line 1\n" + strippedBody;

        return ret;
    }

    bool loadShaderVariables(std::vector<Shader::ShaderVariable>& vars, const YAML::Node& node) {
        if(node.Type() != YAML::NodeType::Map) {
            LOG_ERROR("uniforms have to a sequence of maps");
            return false;
        }
        for(auto it = node.begin(); it != node.end(); ++it) {
            const std::string& name = it->first.as<std::string>();
            if(it->second.Type() == YAML::NodeType::Scalar) {
                vars.emplace_back(name, it->second.as<std::string>());
            } else if(it->second.Type() == YAML::NodeType::Map) {
                if(it->second["type"]) {
                    std::vector<std::pair<std::string, std::string> > layout;
                    if(it->second["layout"]) {
                        for(auto qualifier = it->second["layout"].begin(); qualifier != it->second["layout"].end(); ++qualifier) {
                            layout.push_back(std::make_pair(qualifier->first.as<std::string>(), qualifier->second.as<std::string>()));
                        }
                    }
                    vars.emplace_back(name, it->second["type"].as<std::string>(), layout);
                } else {
                    LOG_ERROR("'type' has to be specified for a shader variable!");
                    return false;
                }
            } else {
                LOG_ERROR("a shader variable value has to either be a scalar (the type) or a map (including a type field)");
                return false;
            }
        }
        return true;
    }

    bool Shader::load(const char* filename) {
        *this = Shader();

        try {
            YAML::Node root = YAML::LoadFile(filename);
            if(root.IsNull()) {
                LOG_ERROR("YAML file could not be opened/parsed.");
                return false;
            }

            if(root.Type() == YAML::NodeType::Map && root.size() == 1 && root["shader"]) {
                const YAML::Node& shader = root["shader"];

                if(shader["uniforms"]) {
                    loadShaderVariables(mUniforms, shader["uniforms"]);
                }
                if(shader["attributes"]) {
                    loadShaderVariables(mAttributes, shader["attributes"]);
                }
                if(shader["includes"]) {
                    if(shader["includes"].Type() != YAML::NodeType::Sequence) {
                        LOG_ERROR("'includes' should be a sequence of file paths");
                        return false;
                    }
                    for(auto it = shader["includes"].begin(); it != shader["includes"].end(); ++it) {
                        if(it->Type() != YAML::NodeType::Scalar) {
                            LOG_ERROR("'includes' should be a sequence of file paths");
                            return false;
                        } else {
                            LOG_WARNING("includes for shader files are not yet implemented!");
                            //TODO: Asset manager needed!
                        }
                    }
                }
                if(shader["glsl"]) {
                    if(!loadSourceFromFile(shader["glsl"].as<std::string>().c_str())) return false;
                }
            } else {
                LOG_ERROR("Shader file should only contain one key 'shader'");
                return false;
            }
            return true;
        } catch(YAML::Exception exc) {
            LOG_ERROR("Error loading YAML file: '%s'", exc.msg.c_str());
            return false;
        }
    }

    bool Shader::loadSourceFromFile(const char* filename) {
        //LOG_DEBUG("load from file: %s", filename);
        std::ifstream file(filename, std::ios_base::in);
        if(file){
            // It's absolutely ridiculous and embarrassing how hard it is to read a file into a string correctly.
            std::stringstream buffer;
            buffer << file.rdbuf();
            setSource(buffer.str());
            return true;
        } else{
            LOG_ERROR("Shader source file '%s' could not be opened.", filename);
            return false;
        }
    }
}
