#pragma once

#include <vector>
#include <utility>

#include <glad/glad.h>

#include "mesh_vertexdata.hpp"
#include "mesh_vertexaccessor.hpp"

namespace ngn {
    ///////////////////////////////////////////////////////////////////////////

    // without index data, this will use glDrawArrays
    class Mesh {
    private:
        std::vector<std::pair<const ShaderProgram*, GLuint> > mVAOs;
        std::vector<VertexData*> mVertexData;
        IndexData* mIndexData;

        GLuint getVAO(const ShaderProgram* shader) {
            for(auto pair : mVAOs) {
                if(pair->first == shader) return pair->second;
            }
            return 0;
        }

        void setVao(const ShaderProgram* shader, GLuint vao) {
            for(auto pair : mVAOs) {
                if(pair->first == shader) {
                    pair->second = vao;
                    return;
                }
            }
            mVAOs.push_back(std::make_pair(shader, vao));
        }

        static GLuint lastBoundVAO;
    public:

        Mesh(const VertexData& vertexData);
        Mesh(const VertexData& vertexData, const IndexData& indexData);

        void addVertexData(const VertexData& vertexData) {
            // This is not enough if someone changes the size of an already added VertexData object (not the last one)
            if(mVertexData.size() > 0 && mVertexData.back().getNumVertices() != vertexData.getNumVertices()) {
                LOG_ERROR("Multiple VertexData objects with different sizes in a single Mesh");
            }
            mVertexData.push_back(&vertexData);
        }
        void setIndices(const IndexData& indexData);

        void compile(const ShaderProgram* shader) {
            assert(shader != nullptr);

            GLuint vao = getVAO(shader);
            if(vao == 0) glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            // Not sure if this should be in VertexFormat
            for(auto vData : mVertexData) {
                vData->bind();
                uint8_t* offset = 0;
                size_t stride = vData->getVertexFormat().getAttrCount() * 4;
                for(auto attr : vData->getVertexFormat().getAttributes()) {
                    auto location = static_cast<GLint>(shader->getAttributeLocation(attr.name));
                    glEnableVertexAttribArray(location);
                    glVertexAttribPointer(  location, attr.alignedNum, static_cast<GLenum>(attr.dataType),
                                            attr.normalized, stride, reininterpret_cast<GLvoid*>(offset));
                    offset += 4;
                }
            }

            // for ARRAY_BUFFER only the calls to glEnableVertexAttribArray/glEnableVertexPointer are stored
            // so unbind now.
            mVertexData.back()->unbind();

            if(mIndexData != nullptr) mIndexData->bind();

            glBindVertexArray(0);

            // VAO stores the last bound ELEMENT_BUFFER state, so as soon as the VAO is unbound, unbind the VBO
            if(mIndexData != nullptr) mIndexData->unbind();

            setVao(shader, vao);
        }

        void draw() {
            GLuint vao = getVAO(ShaderProgram::currentShaderProgram);
            if(vao == 0) {
                compile(ShaderProgram::currentShaderProgram);
                vao = getVAO(ShaderProgram::currentShaderProgram);
            }

            if(lastBoundVAO != vao) glBindVertexArray(vao);

            // A lof of this can go wrong if someone compiles this Mesh without an index buffer attached, then attaches one and compiles it with another
            // shader, while both are in use
            if(mIndexData != nullptr) {
                glDrawElements(mMode, mIndexData->getNumIndices(), static_cast<GLenum>(mIndexData->getDataType()), nullptr);
            } else {
                // If someone had the great idea of having multiple VertexData objects attached and changing their size after attaching
                // this might break
                size_t size = 0;
                if(mVertexData.size() > 0) size = mVertexData[0].getNumVertices();
                glDrawArrays(mMode, 0, size);
            }
        }
    };

    ///////////////////////////////////////////////////////////////////////////

    VertexFormat P3N3T2;
    P3N3T2.add(VertexAttribute("inPosition", VertexAttribute::F32, 3, false, VertexAttribute::POSITION));
    P3N3T2.add(VertexAttribute("inTexCoords", VertexAttribute::UI16, 2, true, VertexAttribute::TEXCOORD));

    VertexData vData = P3N3T2.allocate(36);
    auto position = vData["inPosition"];
    for(int i = 0; i < vData.getNumVertices(); ++i) {
        // Die Template-Parameter inferren den returntype F32 * 3 -> vec3, muss man alles mit Template-Spezialisierung machen
        position[i] = glm::vec3(0.0f, 0.0f, 0.0f);
    }

    ///////////////////////////////////////////////////////////////////////////
    /*
    boxMesh(int width, int height, int depth, int subdivison = 1, const VertexFormat& format = defaultFormat);
    circleMesh(int radius, int segments, const VertexFormat&format = defaultFormat);
    rectangleMesh(int width, int height, int subdivisionX = 1, int subdivisionY)
        .normalize to get a plain thats centered
        setPosition and lookAt to get a line + proper scaling
    cylinderMesh(radiusTop, radiusBottom) -> Cylinder
    sphereMesh
    normalsMesh - generates normals from another VertexData - maybe GL_LINES or actual arrows?
    gridMesh
    frustumMesh - from camera/any perspective matrix, inverts is and converts ndc - corners
    coordinateSystem

    Helper-Klasse für Ribbons/extruded geometry (eine shape, die dann angehangen wird und vllt. radius per Zeit ändern kann oder so)
    */
}