#include <yaml-cpp/yaml.h>

#include "material.hpp"
#include "renderer.hpp"

namespace ngn {
    bool Material::staticInitialized = false;
    Material* Material::fallback = nullptr;
    ShaderCache Material::shaderCache;

    void Material::staticInitialize() {
        Material::staticInitialized = true;

        Shader(); // force static initialization of Shader to make sure fallbacks are initialized :/
        Material::fallback = new Material(FragmentShader::fallback, VertexShader::fallback);
        // Our unset ResourceHandles will automatically fall back to the shader fallbacks
        Material::fallback->addPass(Renderer::FORWARD_AMBIENT_PASS);
    }

    void Material::validate() const {
        if((mBlendMode == BlendMode::MODULATE || mBlendMode == BlendMode::SCREEN) && (hasPass(Renderer::FORWARD_LIGHT_PASS)))
            LOG_WARNING("Blend mode MODULATE and SCREEN don't work properly with lit materials!");
    }

    void Material::setBlendMode(BlendMode mode) {
        mBlendMode = mode;
        switch(mode) {
            case BlendMode::REPLACE:
                mStateBlock.setBlendEnabled(false);
                mStateBlock.setDepthWrite(true);
                break;
            case BlendMode::TRANSLUCENT:
                mStateBlock.setBlendEnabled(true);
                mStateBlock.setBlendEquation(RenderStateBlock::BlendEq::ADD);
                mStateBlock.setBlendFactors(RenderStateBlock::BlendFactor::SRC_ALPHA, RenderStateBlock::BlendFactor::ONE_MINUS_SRC_ALPHA);
                mStateBlock.setDepthWrite(false);
                break;
            case BlendMode::ADD:
                mStateBlock.setBlendEnabled(true);
                mStateBlock.setBlendEquation(RenderStateBlock::BlendEq::ADD);
                mStateBlock.setBlendFactors(RenderStateBlock::BlendFactor::ONE, RenderStateBlock::BlendFactor::ONE);
                mStateBlock.setDepthWrite(false);
                break;
            case BlendMode::MODULATE:
                mStateBlock.setBlendEnabled(true);
                mStateBlock.setBlendEquation(RenderStateBlock::BlendEq::ADD);
                mStateBlock.setBlendFactors(RenderStateBlock::BlendFactor::DST_COLOR, RenderStateBlock::BlendFactor::ZERO);
                mStateBlock.setDepthWrite(false);
                break;
            case BlendMode::SCREEN:
                mStateBlock.setBlendEnabled(true);
                mStateBlock.setBlendEquation(RenderStateBlock::BlendEq::ADD);
                mStateBlock.setBlendFactors(RenderStateBlock::BlendFactor::ONE, RenderStateBlock::BlendFactor::ONE_MINUS_SRC_COLOR);
                mStateBlock.setDepthWrite(false);
                break;
        }
        validate();
    }

    bool strIsOneOf(const std::string& str, const std::vector<std::string>& options) {
        return std::find(options.begin(), options.end(), str) != options.end();
    }

    bool allAreOneOf(const std::vector<std::string>& all, const std::vector<std::string>& options) {
        for(auto& elem : all) if(!strIsOneOf(elem, options)) return false;
        return true;
    }

    bool nodeIsOneOf(const YAML::Node& node, const std::vector<std::string>& options) {
        return node.IsScalar() && strIsOneOf(node.as<std::string>(), options);
    }

    std::string joinComma(const std::vector<std::string>& vals) {
        std::string ret = "";
        for(size_t i = 0; i < vals.size() - 1; ++i) ret += vals[i] + ", ";
        ret += vals[vals.size()-1];
        return ret;
    }

    template<typename T>
    std::vector<std::string> getKeys(const T& map) {
        std::vector<std::string> ret;
        for(auto it = map.begin(); it != map.end(); ++it) ret.emplace_back(it->first);
        return ret;
    }

    template<>
    std::vector<std::string> getKeys<YAML::Node>(const YAML::Node& map) {
        std::vector<std::string> ret;
        for(auto it = map.begin(); it != map.end(); ++it) ret.emplace_back(it->first.as<std::string>());
        return ret;
    }

    template<typename T, typename Func>
    bool parseOption(const YAML::Node& node, const std::string& name, const std::unordered_map<std::string, T> options,
                Func func) {
        if(node[name]) {
            std::vector<std::string> keys = getKeys(options);

            if(nodeIsOneOf(node[name], keys)) {
                func(options.at(node[name].as<std::string>()));
                return true;
            } else {
                LOG_ERROR("Error parsing %s: Value has to be one of: %s", name.c_str(), joinComma(keys).c_str());
                return false;
            }
        }
        return false;
    }

    // I handle of stupid cases in here and it's still not all. Please just never break it, okay?
    Material* Material::fromFile(const char* filename) {
        try {
            YAML::Node root = YAML::LoadFile(filename);
            if(root.Type() == YAML::NodeType::Map && root.size() == 1 && root["material"]) {
                const YAML::Node& material = root["material"];

                Material* base = nullptr;
                if(material["base"]) {
                    if(material["base"].Type() != YAML::NodeType::Scalar) {
                        LOG_ERROR("'base' should be a scalar (a file path)");
                        return nullptr;
                    }
                    base = Resource::getPrepare<Material>(material["base"].as<std::string>().c_str());
                }

                FragmentShader* frag = nullptr;
                VertexShader* vert = nullptr;
                if(material["shaders"]) {
                    const YAML::Node& shaders = material["shaders"];
                    if(shaders.Type() == YAML::NodeType::Map) {
                        for(auto it = shaders.begin(); it != shaders.end(); ++it) {
                            const std::string name = it->first.as<std::string>();
                            if(name == "fragment") {
                                frag = Resource::getPrepare<FragmentShader>(it->second.as<std::string>().c_str());
                                if(!frag) return nullptr;
                            } else if(name == "vertex") {
                                vert = Resource::getPrepare<VertexShader>(it->second.as<std::string>().c_str());
                                if(!vert) return nullptr;
                            } else {
                                LOG_ERROR("Invalid shader field '%s'", name.c_str());
                                return nullptr;
                            }
                        }
                    } else {
                        LOG_ERROR("'shaders' has to be map!");
                        return nullptr;
                    }
                }

                if(!base && (!frag || !vert)) {
                    LOG_ERROR("'shaders' has to be specified (both 'fragment' and 'vertex') if no 'base' is given!");
                    return nullptr;
                }

                Material* retMat = nullptr;
                if(base) {
                    if(frag) {
                        if(vert)
                            retMat = new Material(*base, frag, vert);
                        else
                            retMat = new Material(*base, frag);
                    } else {
                        if(vert)
                            retMat = new Material(*base, vert);
                        else
                            retMat = new Material(*base);
                    }
                } else {
                    // because of a condition above this both frag and vert are set
                    retMat = new Material(frag, vert);
                }
                assert(retMat);

                if(material["renderstate"] && !material["renderstate"].IsNull()) {
                    std::vector<std::string> stateOptions = {"blendmode", "depthwrite", "depthfunc", "cullfaces", "frontface",
                            "blendenabled", "blendsrc", "blenddst", "blendeq"};

                    const YAML::Node& renderstate = material["renderstate"];
                    if(!renderstate.IsMap() || !allAreOneOf(getKeys(renderstate), stateOptions)) {
                        LOG_ERROR("renderstate should be a map with possible values: %s", joinComma(stateOptions).c_str());
                        delete retMat; return nullptr;
                    }

                    if(renderstate["depthwrite"]) {
                        if(!renderstate["depthwrite"].IsScalar()) {
                            LOG_ERROR("depthwrite has to be a boolean scalar!");
                            delete retMat; return nullptr;
                        }
                        retMat->getStateBlock().setDepthWrite(renderstate["depthwrite"].as<bool>());
                    }

                    // depth func
                    std::unordered_map<std::string, DepthFunc> depthFuncOptions = {{"disabled", DepthFunc::DISABLED}, {"never", DepthFunc::NEVER},
                        {"less", DepthFunc::LESS}, {"equal", DepthFunc::EQUAL}, {"lequal", DepthFunc::LEQUAL}, {"greater", DepthFunc::GREATER},
                        {"notequal", DepthFunc::NOTEQUAL}, {"gequal", DepthFunc::GEQUAL}, {"always", DepthFunc::ALWAYS}
                    };
                    if(!parseOption(renderstate, "depthfunc", depthFuncOptions,
                            [retMat](DepthFunc v){retMat->getStateBlock().setDepthTest(v);}))
                        {delete retMat; return nullptr;}

                    // blend mode
                    std::unordered_map<std::string, BlendMode> blendModeOptions = {{"replace", BlendMode::REPLACE},
                        {"translucent", BlendMode::TRANSLUCENT}, {"add", BlendMode::ADD},
                        {"modulate", BlendMode::MODULATE}, {"screen", BlendMode::SCREEN}
                    };
                    if(!parseOption(renderstate, "blendmode", blendModeOptions,
                            [retMat](BlendMode v){retMat->setBlendMode(v);}))
                        {delete retMat; return nullptr;}

                    // cull faces
                    std::unordered_map<std::string, FaceDirections> cullFacesOptions = {{"none", FaceDirections::NONE},
                        {"front", FaceDirections::FRONT}, {"back", FaceDirections::BACK}
                    };
                    if(!parseOption(renderstate, "cullfaces", cullFacesOptions,
                            [retMat](FaceDirections v){retMat->getStateBlock().setCullFaces(v);}))
                        {delete retMat; return nullptr;}

                    // front face
                    std::unordered_map<std::string, FaceOrientation> frontFaceOptions = {
                        {"cw", FaceOrientation::CW}, {"ccw", FaceOrientation::CCW}
                    };
                    if(!parseOption(renderstate, "frontface", frontFaceOptions,
                            [retMat](FaceOrientation v){retMat->getStateBlock().setFrontFace(v);}))
                        {delete retMat; return nullptr;}

                    if(renderstate["blendenabled"]) {
                        if(!renderstate["blendenabled"].IsScalar()) {
                            LOG_ERROR("blendenabled has to be a boolean scalar!");
                            delete retMat; return nullptr;
                        }
                        retMat->getStateBlock().setBlendEnabled(renderstate["blendenabled"].as<bool>());
                    }

                    std::unordered_map<std::string, RenderStateBlock::BlendFactor> blendFactorOptions = {
                            {"zero", RenderStateBlock::BlendFactor::ZERO},
                            {"one", RenderStateBlock::BlendFactor::ONE},
                            {"src_color", RenderStateBlock::BlendFactor::SRC_COLOR},
                            {"one_minus_src_color", RenderStateBlock::BlendFactor::ONE_MINUS_SRC_COLOR},
                            {"dst_color", RenderStateBlock::BlendFactor::DST_COLOR},
                            {"one_minus_dst_color", RenderStateBlock::BlendFactor::ONE_MINUS_DST_COLOR},
                            {"src_alpha", RenderStateBlock::BlendFactor::SRC_ALPHA},
                            {"one_minus_src_alpha", RenderStateBlock::BlendFactor::ONE_MINUS_SRC_ALPHA},
                            {"dst_alpha", RenderStateBlock::BlendFactor::DST_ALPHA},
                            {"one_minus_dst_alpha", RenderStateBlock::BlendFactor::ONE_MINUS_DST_ALPHA},
                            {"constant_color", RenderStateBlock::BlendFactor::CONSTANT_COLOR},
                            {"one_minus_constant_color", RenderStateBlock::BlendFactor::ONE_MINUS_CONSTANT_COLOR},
                            {"constant_alpha", RenderStateBlock::BlendFactor::CONSTANT_ALPHA},
                            {"one_minus_constant_alpha", RenderStateBlock::BlendFactor::ONE_MINUS_CONSTANT_ALPHA}
                    };

                    // blend src
                    if(!parseOption(renderstate, "blendsrc", blendFactorOptions,
                            [retMat](RenderStateBlock::BlendFactor v){retMat->getStateBlock().setBlendSrcFactor(v);}))
                        {delete retMat; return nullptr;}

                    // blend dst
                    if(!parseOption(renderstate, "blenddst", blendFactorOptions,
                            [retMat](RenderStateBlock::BlendFactor v){retMat->getStateBlock().setBlendDstFactor(v);}))
                        {delete retMat; return nullptr;}

                    // blend eq
                    std::unordered_map<std::string, RenderStateBlock::BlendEq> blendEqOptions = {
                            {"add", RenderStateBlock::BlendEq::ADD}, {"subtract", RenderStateBlock::BlendEq::SUBTRACT},
                            {"reverse_subtract", RenderStateBlock::BlendEq::REVERSE_SUBTRACT},
                            {"min", RenderStateBlock::BlendEq::MIN}, {"max", RenderStateBlock::BlendEq::MAX}
                    };
                    if(!parseOption(renderstate, "blendeq", blendEqOptions,
                            [retMat](RenderStateBlock::BlendEq v){retMat->getStateBlock().setBlendEquation(v);}))
                        {delete retMat; return nullptr;}

                    return retMat;
                }

                if(material["passes"]) {
                    std::unordered_map<std::string, int> passIndices = {
                        {"ambient", Renderer::FORWARD_AMBIENT_PASS},
                        {"light", Renderer::FORWARD_LIGHT_PASS},
                        {"gbuffer", Renderer::GBUFFER_PASS},
                        {"shadowmap", Renderer::SHADOWMAP_PASS}
                    };

                    auto passNameOptions = getKeys(passIndices);
                    if(!material["passes"].IsMap() || !allAreOneOf(getKeys(material["passes"]), passNameOptions)) {
                        LOG_ERROR("'passes' must be a map with possible values: %s", joinComma(passNameOptions).c_str());
                        delete retMat; return nullptr;
                    }

                    for(auto passIt : material["passes"]) {
                        auto passIndex = passIndices[passIt.first.as<std::string>()];
                        YAML::Node& pass = passIt.second;
                        if(pass.IsNull()) {
                            retMat->removePass(passIndex);
                        } else if(pass.IsMap()) {
                            if(retMat->hasPass(passIndex)) retMat->removePass(passIndex);
                            //TODO: parse pass properly
                            retMat->addPass(passIndex);
                        }
                    }
                } else {
                    if(!base) {
                        LOG_ERROR("'passes' is specified if there is no 'base' material");
                        delete retMat; return nullptr;
                    }
                }

                if(material["parameters"]) {
                    if(!material["parameters"].IsMap()) {
                        LOG_ERROR("'parameters' must be a map");
                        delete retMat; return nullptr;
                    }

                    for(auto paramIt : material["parameters"]) {
                        auto paramName = paramIt.first.as<std::string>().c_str();
                        auto type = paramIt.second.Tag().substr(18); // remove prefix: 'tag:yaml.org,2002:'
                        if(type == "texture") {
                            retMat->setTexture(paramName, ngn::Resource::getPrepare<Texture>(paramIt.second.as<std::string>().c_str()));
                        } else if(type == "pixelTexture") {
                            //TODO: somehow handle this as a named resource?
                            auto nums = paramIt.second.as<std::vector<float> >();
                            if(nums.size() != 4) {
                                LOG_ERROR("parameters of type pixelTexture should contain a list of four scalars!");
                                delete retMat; return nullptr;
                            }
                            retMat->setTexture(paramName, ngn::Texture::pixelTexture(glm::vec4(nums[0], nums[1], nums[2], nums[3])));
                        } else if(type == "vec2") {
                            auto nums = paramIt.second.as<std::vector<float> >();
                            if(nums.size() != 2) {
                                LOG_ERROR("parameters of type vec2 should contain a list of two scalars!");
                                delete retMat; return nullptr;
                            }
                            retMat->setVector2(paramName, glm::vec2(nums[0], nums[1]));
                        } else if(type == "vec3") {
                            auto nums = paramIt.second.as<std::vector<float> >();
                            if(nums.size() != 3) {
                                LOG_ERROR("parameters of type vec3 should contain a list of three scalars!");
                                delete retMat; return nullptr;
                            }
                            retMat->setVector3(paramName, glm::vec3(nums[0], nums[1], nums[2]));
                        } else if(type == "vec4") {
                            auto nums = paramIt.second.as<std::vector<float> >();
                            if(nums.size() != 4) {
                                LOG_ERROR("parameters of type vec4 should contain a list of four scalars!");
                                delete retMat; return nullptr;
                            }
                            retMat->setVector4(paramName, glm::vec4(nums[0], nums[1], nums[2], nums[3]));
                        } else if(type == "float") {
                            retMat->setFloat(paramName, paramIt.second.as<float>());
                        } else if(type == "int") {
                            retMat->setInteger(paramName, paramIt.second.as<int>());
                        } else {
                            LOG_ERROR("Unrecognized parameter type '%s'", type.c_str());
                            delete retMat; return nullptr;
                        }
                    }
                }
                return retMat;
            } else {
                LOG_ERROR("Material file should only contain one key: 'material'");
                return nullptr;
            }
        } catch(YAML::Exception exc) {
            LOG_ERROR("Error loading YAML file '%s'", exc.msg.c_str());
            return nullptr;
        }
        return nullptr;
    }
}