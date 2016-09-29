#include <string>
#include <vector>

#include "shaderprogram.hpp"
#include "texture.hpp"

namespace ngn {
    //TODO: Möglichkeit der Engine zu sagen, dass manche Parameter colors sind, sodass gamma-correction applied werden kann.
    // default texture values angeben (vermutlich einfach nur eine Farbe - also vector type)

    class Shader {
    public:
        struct ShaderVariable {
            enum class ShaderVariableType {
                FLOAT, BOOL, INT, VEC2, VEC3, VEC4, TEXTURE
            };

            std::string name;
            std::string type;
            std::vector<std::pair<std::string, std::string> > layoutQualifiers;

            ShaderVariable(const std::string& name, const std::string& type, const std::vector<std::pair<std::string, std::string> >& layout = {}) :
                    name(name), type(type), layoutQualifiers(layout) {}

            std::string getString(const std::string& qualifier) const;
        };

        struct PragmaInfo {
            std::string name;
            int lineStart;
            size_t pragmaStart;
            size_t nextLineStart;
            size_t nextBlockStart;
            size_t nextBlockEnd;
            int lineNextBlockEnd;

            PragmaInfo() : nextLineStart(std::string::npos), nextBlockStart(std::string::npos), nextBlockEnd(std::string::npos), lineNextBlockEnd(0) {}
        };

    private:
        std::vector<ShaderVariable> mUniforms;
        std::vector<ShaderVariable> mInVariables;
        std::vector<const Shader*> mIncludes;
        std::vector<PragmaInfo> mPragmas;
        std::string mSource;

        bool parsePragmas(const std::string& str);
        std::vector<std::string> getPragmaSlots() const;

    public:
        Shader() {}

        void include(const Shader* shdr) {mIncludes.push_back(shdr);}

        void addUniform(const std::string& name, const std::string& type, std::vector<std::pair<std::string, std::string> > layoutQualifiers = {}) {
            mUniforms.emplace_back(name, type, layoutQualifiers);
        }

        void addInVariable(const std::string& name, const std::string& type, std::vector<std::pair<std::string, std::string> > layoutQualifiers = {}) {
            mInVariables.emplace_back(name, type, layoutQualifiers);
        }

        void setSource(const std::string& src) {mSource = src; parsePragmas(mSource);}
        std::string getSource() const {return mSource;}

        std::vector<PragmaInfo> getPragmas() const {return mPragmas;}

        std::string getFullString(const std::vector<std::string>& overrideSlots = {}, bool versionString = true) const;

        bool loadFromFile(const char* file);
        bool loadSourceFromFile(const char* file);
    };

}