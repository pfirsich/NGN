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
        GLuint mVAO;
        std::vector<std::unique_ptr<VertexBuffer> > mVertexBuffers;
        std::unique_ptr<IndexBuffer> mIndexBuffer;

    public:
        Mesh(DrawMode mode) : mMode(mode), mVAO(0), mIndexBuffer(nullptr) {}

        // I'm not really sure what I want these to do
        Mesh(const Mesh& other) = delete;
        Mesh& operator=(const Mesh& other) = delete;

        template <typename... Ts>
        VertexBuffer* addVertexBuffer(Ts&&... args) {
            VertexBuffer* vBuf = new VertexBuffer(std::forward<Ts>(args)...);
            for(auto& attr : vBuf->getVertexFormat().getAttributes()) {
                if(hasAttribute(attr.type) != nullptr) {
                    LOG_ERROR("You are trying to add a vertex buffer to a mesh that contains vertex attributes that are already present in another vertex buffer already in that mesh.");
                    return nullptr;
                }
            }
            mVertexBuffers.emplace_back(vBuf);
            return vBuf;
        }

        // add might be confusing since every Mesh object can only hold a single instance of IndexBuffer
        // if another one is added, the old one is detached and free'd
        template <typename... Ts>
        IndexBuffer* setIndexBuffer(Ts&&... args) {
            IndexBuffer* iData = new IndexBuffer(std::forward<Ts>(args)...);
            mIndexBuffer.reset(iData);
            return iData;
        }

        // returns nullptr if the given attribute is not present in any vertexbuffer
        VertexBuffer* hasAttribute(AttributeType attrType) {
            for(auto& vBuffer : mVertexBuffers) {
                if(vBuffer->getVertexFormat().hasAttribute(attrType)) return vBuffer.get();
            }
            return nullptr;
        }

        template<typename T>
        VertexAttributeAccessor<T> getAccessor(AttributeType id) {
            VertexBuffer* vBuf = hasAttribute(id);
            if(vBuf == nullptr) {
                LOG_ERROR("You are requesting an accessor for a vertex attribute that is not present in the current mesh: '%s'", getVertexAttributeTypeName(id));
                return VertexAttributeAccessor<T>();
            }
            return vBuf->getAccessor<T>(id);
        }

        void compile();

        // In the header because of the slim possibility that it might be inlined
        void draw() {
            if(mVAO == 0) {
                compile();
            }

            if(lastBoundVAO != mVAO) glBindVertexArray(mVAO);

            // A lof of this can go wrong if someone compiles this Mesh without an index buffer attached, then attaches one and compiles it with another
            // shader, while both are in use
            GLenum mode = static_cast<GLenum>(mMode);
            if(mIndexBuffer != nullptr) {
                glDrawElements(mode, mIndexBuffer->getNumIndices(), static_cast<GLenum>(mIndexBuffer->getDataType()), nullptr);
            } else {
                // If someone had the great idea of having multiple VertexBuffer objects attached and changing their size after attaching
                // this might break
                size_t size = 0;
                if(mVertexBuffers.size() > 0) size = mVertexBuffers[0]->getNumVertices();
                glDrawArrays(mode, 0, size);
            }
        }

        // ---- geometry manipulation
        // these functions are here (and not in VertexBuffer), because some of them have to
        // for example read positions and write normals or read normals and write tangents, which might
        // reside in different buffers

        void calculateVertexNormals(bool faceAreaWeighted = true);
        // sets normals so that in the fragment shader the normals can be interpolated using a "flat" varying - https://www.opengl.org/wiki/Type_Qualifier_(GLSL)
        void calculateFaceNormals(bool lastVertexConvention = true);
        void calculateTangents();

        // moves center to 0, 0, 0 and radius to 1.0 if rescale = true
        void normalize(bool rescale = false);
        // Transform positions, normals, tangents and bitangents
        void transform(const glm::mat4& transform);
        // I don't think this is the proper prototype of this function, maybe merge with another Mesh?
        void merge(const VertexBuffer& other, const glm::mat4& transform);

        // pair of position and sizes
        std::pair<glm::vec3, glm::vec3> boundingBox();
        // position and radius
        std::pair<glm::vec3, float> boundingSphere();
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
    normalsMesh - generates normals from another VertexBuffer - maybe GL_LINES or actual arrows?
    gridMesh
    frustumMesh - from camera/any perspective matrix, inverts is and converts ndc - corners
    coordinateSystem

    Helper-Klasse für Ribbons/extruded geometry (eine shape, die dann angehangen wird und vllt. radius per Zeit ändern kann oder so)
    */
}