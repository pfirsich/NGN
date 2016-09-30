#include <fstream>
#include <sstream>

#include "log.hpp"
#include "shader.hpp"
#include "mesh_vertexattribute.hpp"

namespace ngn {
    bool Shader::staticInitialized = false;
    // I really don't like this
    // Also I'm not a fan of the values being hardcoded here. But there is not actually helpful way of doing this programmatically
    std::string Shader::globalShaderPreamble = "#version 330 core\n\n";


    void Shader::staticInitialize() {
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

        staticInitialized = true;
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

    bool Shader::parsePragmas(const std::string& str) {
        bool newLine = true;
        for(size_t i = 0; i < str.length(); ++i) {
            if(newLine && str[i] == '#') { // preprocessor directive
                std::string directive("#pragma ngn ");
                if(str.substr(i, directive.length()) == directive) {
                    PragmaInfo pragmaInfo;
                    pragmaInfo.pragmaStart = i;
                    pragmaInfo.lineStart = getLineInString(str, i);

                    i += directive.length();
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
                    pragmaInfo.name = str.substr(pragmaInfo.pragmaStart, nextWhitespace - pragmaInfo.pragmaStart);

                    if(pragmaInfo.nextLineStart == std::string::npos) {
                        LOG_ERROR("No newline found after pragma '%s'", pragmaInfo.name.c_str());
                        return false;
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
                            LOG_ERROR("Found no matching '}' to '{' of the block after pragma '%s'", pragmaInfo.name.c_str());
                            return false;
                        }
                    }

                    mPragmas.push_back(pragmaInfo);
                    //LOG_DEBUG("'%s': %d, %d, %d, %d", pragmaInfo.name.c_str(), pragmaInfo.pragmaStart, pragmaInfo.nextLineStart, pragmaInfo.nextBlockStart, pragmaInfo.nextBlockEnd);
                }
            }

            if(str[i] == '\n') newLine = true;
            if(str[i] > ' ') newLine = false; // non-whitespace
        }

        return true;
    }

    std::vector<std::string> Shader::getPragmaSlots() const {
        std::vector<std::string> ret;
        for(auto& pragma : mPragmas) ret.push_back(pragma.name);
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
        for(auto& pragma : mPragmas) slots.emplace_back(pragma.name);

        std::vector<std::string> parts;
        for(int i = mIncludes.size() - 1; i >= 0; i -= 1) {
            const Shader* include = mIncludes[i];
            parts.insert(parts.begin(), include->getFullString("", slots, false));
            mergeIntoVectorSet(slots, include->getPragmaSlots());
        }
        for(auto& part : parts) ret += part + "\n";
        //LOG_DEBUG("includes done");

        for(auto& var : mInVariables) {
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
            //LOG_DEBUG("i: %d, cond: %d", i, i >= 0);
            const PragmaInfo& pragma = mPragmas[i];
            bool keep = true;
            for(auto& slotName : overrideSlots) {
                if(slotName == pragma.name) keep = false;
            }
            if(!keep) {
                // remove body and turn it into forward declaration (also it seems to be okay to have multiple forward declarations)
                //LOG_DEBUG("erase '%s'", strippedBody.substr(pragma.nextBlockStart, pragma.nextBlockEnd - pragma.nextBlockStart + 1).c_str());
                strippedBody.erase(pragma.nextBlockStart, pragma.nextBlockEnd - pragma.nextBlockStart + 1);
                // +1 because the #line directive sets the line number of the next line and +1 because the forward declaration takes one line (hopefully)
                strippedBody.insert(pragma.nextBlockStart, ";\n#line " + std::to_string(pragma.lineNextBlockEnd + 1 + 1));
            }
            //LOG_DEBUG("keep '%s': %d", pragma.name.c_str(), keep);
        }
        ret += "#line 1\n" + strippedBody;

        return ret;
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
