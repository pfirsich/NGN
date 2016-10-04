#pragma once

#include <string>
#include <vector>

#include "shaderprogram.hpp"
#include "texture.hpp"
#include "resource.hpp"

namespace ngn {
    //TODO: Möglichkeit der Engine zu sagen, dass manche Parameter colors sind, sodass gamma-correction applied werden kann.
    // default texture values angeben (vermutlich einfach nur eine Farbe - also vector type)

    class Shader : public Resource {
    public:
        struct PragmaInfo {
            std::string name;
            std::string params;
            int lineStart;
            size_t pragmaStart;
            size_t nextLineStart;
            size_t nextBlockStart;
            size_t nextBlockEnd;
            int lineNextBlockEnd;

            PragmaInfo() : nextLineStart(std::string::npos), nextBlockStart(std::string::npos), nextBlockEnd(std::string::npos), lineNextBlockEnd(0) {}
        };

    private:
        std::vector<ResourceHandle<Shader> > mIncludes;
        std::vector<PragmaInfo> mPragmas;
        std::string mSource;

        static bool staticInitialized;
        static void staticInitialize();

        bool parsePragmas(const std::string& str);
        std::vector<std::string> getPragmaSlots() const;

    public:
        static std::string globalShaderPreamble; // I really don't like this
        static Shader* fallback;

        static Shader* fromFile(const char* filename);

        Shader(const char* filename = nullptr) {
            if(!staticInitialized) staticInitialize();
            if(filename) loadFromFile(filename);
        }

        //TODO: Avoid multiple inclusion of the same shader
        void include(ResourceHandle<Shader>&& shader) {mIncludes.push_back(shader);}

        bool setSource(const std::string& src) {mSource = src; return parsePragmas(mSource);}
        std::string getSource() const {return mSource;}

        std::vector<PragmaInfo> getPragmas() const {return mPragmas;}

        std::string getFullString(const std::string& preamble = "", const std::vector<std::string>& overrideSlots = {}, bool globalPreamble = true) const;

        bool loadFromFile(const char* file);
    };

    // Just for fallbacks
    class FragmentShader : public Shader {
    public:
        static FragmentShader* fallback;

        template<typename ...Args>
        FragmentShader(Args&&... args) : Shader(std::forward<Args>(args)...) {}
    };

    class VertexShader : public Shader {
    public:
        static VertexShader* fallback;

        template<typename ...Args>
        VertexShader(Args&&... args) : Shader(std::forward<Args>(args)...) {}
    };
}