#pragma once

#include <vector>
#include <utility>

#include <glad/glad.h>

#include "mesh_vertexdata.hpp"
#include "mesh_vertexaccessor.hpp"
#include "shader.hpp"
#include "log.hpp"

namespace ngn {
    class Mesh {
    public:
        enum class DrawMode : GLenum {
            POINTS = GL_POINTS,
            LINES = GL_LINES,
            LINE_LOOP = GL_LINE_LOOP,
            LINE_STRIP = GL_LINE_STRIP,
            TRIANGLES = GL_TRIANGLES,
            TRIANGLE_FAN = GL_TRIANGLE_FAN,
            TRIANGLE_STRIP = GL_TRIANGLE_STRIP,
        };

    private:
        static GLuint lastBoundVAO;

        DrawMode mMode;
        std::vector<std::pair<const ngn::ShaderProgram*, GLuint> > mVAOs;
        std::vector<std::unique_ptr<VertexData> > mVertexData;
        std::unique_ptr<IndexData> mIndexData;

        GLuint getVAO(const ShaderProgram* shader) {
            for(auto& pair : mVAOs) {
                if(pair.first == shader) return pair.second;
            }
            return 0;
        }

        void setVAO(const ShaderProgram* shader, GLuint vao) {
            for(auto& pair : mVAOs) {
                if(pair.first == shader) {
                    pair.second = vao;
                    return;
                }
            }
            mVAOs.push_back(std::make_pair(shader, vao));
        }

        static std::string attrIdToString(const char* name) {
            return "name '" + std::string(name) + "'";
        }
        static std::string attrIdToString(AttributeType attrType) {
            return "hint " + std::string(AttributeTypeNames[static_cast<int>(attrType)]);
        }

    public:
        Mesh(DrawMode mode) : mMode(mode), mIndexData(nullptr) {}

        // I'm not really sure what I want this to do
        Mesh(const Mesh& other) = delete;
        Mesh& operator=(const Mesh& other) = delete;

        template <typename... Ts>
        VertexData* addVertexData(Ts&&... args) {
            std::unique_ptr<VertexData> vData(new VertexData(std::forward<Ts>(args)...));
            // This is not enough if someone changes the size of an already added VertexData object (not the last one)
            if(mVertexData.size() > 0 && mVertexData.back()->getNumVertices() != vData->getNumVertices()) {
                LOG_ERROR("Multiple VertexData objects with different sizes in a single Mesh.");
                return nullptr;
            } else {
                VertexData* ret = vData.get();
                mVertexData.push_back(std::move(vData));
                return ret;
            }
        }

        // add might be confusing since every Mesh object can only hold a single instance of IndexData
        // if another one is added, the old one is detached and free'd
        template <typename... Ts>
        IndexData* setIndexData(Ts&&... args) {
            IndexData* iData = new IndexData(std::forward<Ts>(args)...);
            mIndexData.reset(iData);
            return iData;
        }

        template<typename T, typename argType>
        VertexAttributeAccessor<T> getAccessor(argType id) {
            VertexData* vData = nullptr;
            for(size_t i = 0; i < mVertexData.size(); ++i) {
                if(mVertexData[i]->getVertexFormat().hasAttribute(id)) {
                    if(vData != nullptr)
                        LOG_ERROR("A mesh seems to have multiple VertexData objects attached that share an attribute with %s",
                            attrIdToString(id).c_str());
                    vData = mVertexData[i].get();
                }
            }
            return vData->getAccessor<T>(id);
        }

        // non-const argument, because we call .bind()
        void compile(ShaderProgram* shader);

        // In the header because of the slim possibility that it might be inlined
        void draw() {
            GLuint vao = getVAO(ShaderProgram::currentShaderProgram);
            if(vao == 0) {
                compile(ShaderProgram::currentShaderProgram);
                vao = getVAO(ShaderProgram::currentShaderProgram);
            }

            if(lastBoundVAO != vao) glBindVertexArray(vao);

            // A lof of this can go wrong if someone compiles this Mesh without an index buffer attached, then attaches one and compiles it with another
            // shader, while both are in use
            GLenum mode = static_cast<GLenum>(mMode);
            if(mIndexData != nullptr) {
                glDrawElements(mode, mIndexData->getNumIndices(), static_cast<GLenum>(mIndexData->getDataType()), nullptr);
            } else {
                // If someone had the great idea of having multiple VertexData objects attached and changing their size after attaching
                // this might break
                size_t size = 0;
                if(mVertexData.size() > 0) size = mVertexData[0]->getNumVertices();
                glDrawArrays(mode, 0, size);
            }
        }
    };

    Mesh* assimpMesh(const char* filename, const VertexFormat& format);
    std::vector<Mesh*> assimpMeshes(const char* filename, const VertexFormat& format);

    // width, height, depth along x, y, z, center is 0, 0, 0
    Mesh* boxMesh(float width, float height, float depth, const VertexFormat& format);

    // Stacks represents the number of elements on the y axis
    Mesh* sphereMesh(float radius, int slices, int stacks, const VertexFormat& format);

    ///////////////////////////////////////////////////////////////////////////
    /*
    circleMesh(int radius, int segments, const VertexFormat&format = defaultFormat);
    rectangleMesh(int width, int height, int subdivisionX = 1, int subdivisionY)
        .normalize to get a plain thats centered
        setPosition and lookAt to get a line + proper scaling
    cylinderMesh(radiusTop, radiusBottom) -> Cylinder
    subdivide(Mesh, iterations) // http://answers.unity3d.com/questions/259127/does-anyone-have-any-code-to-subdivide-a-mesh-and.html like this
    normalsMesh - generates normals from another VertexData - maybe GL_LINES or actual arrows?
    gridMesh
    frustumMesh - from camera/any perspective matrix, inverts is and converts ndc - corners
    coordinateSystem

    Helper-Klasse für Ribbons/extruded geometry (eine shape, die dann angehangen wird und vllt. radius per Zeit ändern kann oder so)
    */
}