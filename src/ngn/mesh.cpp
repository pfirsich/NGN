#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "mesh.hpp"

namespace ngn {
    GLuint Mesh::lastBoundVAO = 0;

    void Mesh::compile(ShaderProgram* shader) {
        assert(shader != nullptr);

        GLuint vao = getVAO(shader);
        if(vao == 0) glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // Not sure if this should be in VertexFormat
        for(auto& vData : mVertexData) {
            vData->bind();
            const VertexFormat& format = vData->getVertexFormat();
            const std::vector<VertexAttribute>& attributes = format.getAttributes();
            for(size_t i = 0; i < attributes.size(); ++i) {
                const VertexAttribute& attr = attributes[i];
                auto location = static_cast<GLint>(shader->getAttributeLocation(attr.name));
                if(location != -1) {
                    glEnableVertexAttribArray(location);
                    glVertexAttribPointer(  location, attr.alignedNum, static_cast<GLenum>(attr.dataType), attr.normalized,
                                            format.getStride(), reinterpret_cast<GLvoid*>(format.getAttributeOffset(i)));
                }
            }
        }

        // for ARRAY_BUFFER only the calls to glEnableVertexAttribArray/glEnableVertexPointer are stored
        // so unbind now.
        mVertexData.back()->unbind();

        if(mIndexData != nullptr) mIndexData->bind();

        glBindVertexArray(0);

        // VAO stores the last bound ELEMENT_BUFFER state, so as soon as the VAO is unbound, unbind the VBO
        if(mIndexData != nullptr) mIndexData->unbind();

        setVAO(shader, vao);
    }

    Mesh* assimpMesh(aiMesh* mesh, const VertexFormat& format) {
        auto ngnMesh = new Mesh(Mesh::DrawMode::TRIANGLES);
        ngnMesh->addVertexData(format, mesh->mNumVertices);

        if(mesh->HasPositions() && format.hasAttribute(AttributeType::POSITION)) {
            auto position = ngnMesh->getAccessor<glm::vec3>(AttributeType::POSITION);
            for(size_t i = 0; i < mesh->mNumVertices; ++i) {
                aiVector3D& vec = mesh->mVertices[i];
                position[i] = glm::vec3(vec.x, vec.y, vec.z);
            }
        }

        if(mesh->HasNormals() && format.hasAttribute(AttributeType::NORMAL)) {
            auto normal = ngnMesh->getAccessor<glm::vec3>(AttributeType::NORMAL);
            for(size_t i = 0; i < mesh->mNumVertices; ++i) {
                aiVector3D& vec = mesh->mNormals[i];
                normal[i] = glm::vec3(vec.x, vec.y, vec.z);
            }
        }

        if(mesh->HasTextureCoords(0) && format.hasAttribute(AttributeType::TEXCOORD)) {
            auto texCoord = ngnMesh->getAccessor<glm::vec2>(AttributeType::TEXCOORD);
            for(size_t i = 0; i < mesh->mNumVertices; ++i) {
                aiVector3D& vec = mesh->mTextureCoords[0][i];
                texCoord[i] = glm::vec2(vec.x, vec.y);
            }
        }

        if(mesh->HasFaces()) {
            IndexData* iData = ngnMesh->setIndexData(getIndexDataType(mesh->mNumVertices), mesh->mNumFaces*3);
            size_t index = 0;
            for(size_t i = 0; i < mesh->mNumFaces; ++i) {
                (*iData)[index++] = mesh->mFaces[i].mIndices[0];
                (*iData)[index++] = mesh->mFaces[i].mIndices[1];
                (*iData)[index++] = mesh->mFaces[i].mIndices[2];
            }
        }

        return ngnMesh;
    }

    std::vector<Mesh*> assimpMeshes(const char* filename, const VertexFormat& format) {
        Assimp::Importer importer;
        /*importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS,
            aiComponent_NORMALS | aiComponent_TANGENTS_AND_BITANGENTS | aiComponent_COLORS |
            aiComponent_LIGHTS | aiComponent_CAMERAS);*/
        const aiScene *scene = importer.ReadFile(filename, aiProcessPreset_TargetRealtime_Fast);
        if(!scene) {
            LOG_ERROR("Mesh file '%s' could not be loaded!\n", importer.GetErrorString());
        }

        std::vector<Mesh*> meshes;
        for(size_t i = 0; i < scene->mNumMeshes; ++i) {
            meshes.push_back(assimpMesh(scene->mMeshes[i], format));
        }

        return meshes;
    }

    // width, height, depth along x, y, z, center is 0, 0, 0
    Mesh* boxMesh(float width, float height, float depth, const VertexFormat& format) {
        static float vertices[] = {
            // +z
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,

            // -z
             1.0f,  1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            // +x
             1.0f,  1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,

            // -x
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,

            // +y
             1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,

            // -y
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f
        };

        static uint8_t indices[6] = {
            0, 1, 3,
            3, 1, 2
        };

        static float normals[6][3] = {
            { 0.0f,  0.0f,  1.0f},
            { 0.0f,  0.0f, -1.0f},
            { 1.0f,  0.0f,  0.0f},
            {-1.0f,  0.0f,  0.0f},
            { 0.0f,  1.0f,  0.0f},
            { 0.0f, -1.0f,  0.0f},
        };

        glm::vec3 size = glm::vec3(width * 0.5f, height * 0.5f, depth * 0.5f);

        Mesh* mesh = new Mesh(Mesh::DrawMode::TRIANGLES);
        VertexData* vData = mesh->addVertexData(format, 24);

        auto position = mesh->getAccessor<glm::vec3>(AttributeType::POSITION);
        auto normal = mesh->getAccessor<glm::vec3>(AttributeType::NORMAL);

        for(size_t i = 0; i < vData->getNumVertices(); ++i) {
            position[i] = glm::make_vec3(vertices + i*3) * size;
            int side = i / 4;
            normal[i] = glm::make_vec3(normals[side]);
        }

        if(format.hasAttribute(AttributeType::TEXCOORD)) {
            auto texCoord = mesh->getAccessor<glm::vec2>(AttributeType::TEXCOORD);
            for(int side = 0; side < 6; ++side) {
                texCoord[side*4+0] = glm::vec2(0.0f, 0.0f);
                texCoord[side*4+1] = glm::vec2(0.0f, 1.0f);
                texCoord[side*4+2] = glm::vec2(1.0f, 1.0f);
                texCoord[side*4+3] = glm::vec2(1.0f, 0.0f);
            }
        }

        IndexData* iData = mesh->setIndexData(IndexDataType::UI8, 36);

        uint8_t* indexBuffer = iData->getData<uint8_t>();
        for(int side = 0; side < 6; ++side) {
            for(int vertex = 0; vertex < 6; ++vertex) {
                indexBuffer[side*6+vertex] = 4*side + indices[vertex];
            }
        }

        //if(format.hasAttribute(AttributeType::TANGENT)) vData->calculateTangents();

        return mesh;
    }

    // Stacks represents the subdivision on the y axis (excluding the poles)
    Mesh* sphereMesh(float radius, int slices, int stacks, const VertexFormat& format) {
        assert(slices > 3 && stacks > 2);
        Mesh* mesh = new Mesh(Mesh::DrawMode::TRIANGLES);
        mesh->addVertexData(format, slices*stacks);

        auto position = mesh->getAccessor<glm::vec3>(AttributeType::POSITION);
        auto normal = mesh->getAccessor<glm::vec3>(AttributeType::NORMAL);
        auto texCoord = mesh->getAccessor<glm::vec2>(AttributeType::TEXCOORD);

        /* This should probably be:
        auto normal = VertexAttributeAccessor();
        if(format.hasAttribute(AttributeType::NORMAL)) {
            normal = (*vData)[AttributeType::NORMAL];
        }

        That way there will not be a debug log message every time you create a sphere mesh,
        but I did it like this to show that it's possible to do it like this without everything going up in flames
         */

        int index = 0;
        for(int stack = 0; stack < stacks; ++stack) {
            float stackAngle = glm::pi<float>() / (stacks - 1) * stack;
            float xzRadius = glm::sin(stackAngle) * radius;
            float y = glm::cos(stackAngle) * radius;
            for(int slice = 0; slice < slices; ++slice) {
                float sliceAngle = 2.0f * glm::pi<float>() / (slices - 1) * slice;
                position[index] = glm::vec3(glm::cos(sliceAngle) * xzRadius, y, glm::sin(sliceAngle) * xzRadius);
                normal[index] = glm::normalize(position.get(index));
                texCoord[index++] = glm::vec2(sliceAngle / 2.0f / glm::pi<float>(), stackAngle / glm::pi<float>());
            }
        }

        int triangles = 2 * (slices - 1) * (stacks - 1);

        IndexData* iData = mesh->setIndexData(IndexDataType::UI16, triangles * 3);
        uint16_t* indexBuffer = iData->getData<uint16_t>();
        index = 0;
        for(int stack = 0; stack < stacks - 1; ++stack) {
            int firstStackVertex = stack * slices;
            for(int slice = 0; slice < slices - 1; ++slice) {
                int firstFaceVertex = firstStackVertex + slice;
                int nextVertex = firstFaceVertex + 1;
                indexBuffer[index++] = firstFaceVertex;
                indexBuffer[index++] = firstFaceVertex + slices;
                indexBuffer[index++] = nextVertex + slices;

                indexBuffer[index++] = firstFaceVertex;
                indexBuffer[index++] = nextVertex + slices;
                indexBuffer[index++] = nextVertex;
            }
        }

        //if(format.hasAttribute(AttributeType::TANGENT)) vData->calculateTangents();

        return mesh;
    }
}