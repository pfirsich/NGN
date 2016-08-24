#pragma once

#include <string>
#include <vector>
#include <cstring>
#include <utility>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "mesh_vertexaccessor.hpp"

namespace ngn {
    class VertexData;
    class VertexAttributeAccessor;

    enum class AttributeDataType : GLenum {
        I8 = GL_BYTE,
        UI8 = GL_UNSIGNED_BYTE
        I16 = GL_SHORT,
        UI16 = GL_UNSIGNED_SHORT,
        I32 = GL_INT,
        UI32 = GL_UNSIGNED_INT,
        //F16 = GL_HALF_FLOAT,
        F32 = GL_FLOAT,
        //F64 = GL_DOUBLE,
        I2_10_10_10 = GL_INT_2_10_10_10_REV,
        UI2_10_10_10 = GL_UNSIGNED_INT_2_10_10_10_REV,
    };

    enum class AttributeType : GLenum {
        POSITION,
        NORMAL,
        TEXCOORD,
        TANGENT,
        CUSTOM
    };

    struct VertexAttribute {
        std::string name;
        AttributeDataType dataType;
        int num, alignedNum;
        bool normalized;
        AttributeType type;

        VertexAttribute(const std::string& name, AttributeDataType dataType, int num, bool normalized, AttributeType type = AttributeType::CUSTOM) :
                name(name), dataType(dataType), num(num), alignedNum(num), normalized(normalized), attrType(type) {
            // Align to 4 Bytes
            // https://www.opengl.org/wiki/Vertex_Specification_Best_Practices#Attribute_sizes
            switch(type) {
                case AttributeType::I8:
                case AttributeType::UI8:
                    int overlap = num % 4;
                    if(overlap > 0) alignedNum = num + (4 - overlap);
                    break;
                case AttributeType::I16:
                case AttributeType::UI16:
                    int overlap = (num * 2) % 4;
                    if(overlap > 0) alignedNum = num + (4 - overlap) / 2;
                    break;
                // all others should already be multiples of 4
            }
        }
    };

    enum class UsageHint : GLenum {
        STATIC = GL_STATIC_DRAW,
        STREAM = GL_STREAM_DRAW,
        DYNAMIC = GL_DYNAMIC_DRAW
    };

    struct VertexFormat {
    private:
        std::vector<VertexAttribute> mAttributes;
        int mAttrCount;

    public:
        const auto& getAttributes() const {return mAttributes;}
        int getAttrCount() const {return mAttrCount;}

        void add(const VertexAttribute& _attr) {
            mAttributes.push_back(_attr);
            mAttrCount++;
            // This is done, so I avoid copying attr twice, because I want to modify it
            // VertexAttribute& attr = mAttributes.back();
        }

        VertexData allocate(int numVertices, UsageHint usage = UsageHint::STATIC) {
            return VertexData(*this, numVertices, usage);
        }

        // Think about what these guys should return if they don't find an attribute!
        // Maybe implement hasAttribute("name") and hasAttribute(AttributeType)
        VertexAttributeAccessor getAccessor(const char* name, mData) {
            // If we want the vector to be reorder-able and resizable, we have to look for the element every time
            // Also if the number of elements is small (which it mostly is), this is probably even faster than std::map
            for(std::size_t i = 0; i < mAttributes.size(); ++i) {
                if(mAttributes[i].name == name) {
                    return VertexAttributeAccessor(*this, i, mData);
                }
            }
        }

        VertexAttributeAccessor getAccessor(AttributeType attrType) {
            for(std::size_t i = 0; i < mAttributes.size(); ++i) {
                if(mAttributes[i].type == attrType) {
                    return VertexAttributeAccessor(*this, i, mData);
                }
            }
        }
    };

    class VBOWrapper {
    protected:
        int mSize;
        UsageHint mUsage;
        void* mData;
        GLuint mVBO;
        int mLastUploadedSize;
        bool mUploadedOnce;

    public:
        VBOWrapper() : mSize(0), mUsage(UsageHint::STATIC), mData(nullptr), mVBO(0), mLastUploadedSize(0), mUploadedOnce(false) {}

        ~VBOWrapper() {
            if(mVBO != 0) glDeleteBuffers(1, &mVBO);
        }

        // http://hacksoflife.blogspot.de/2015/06/glmapbuffer-no-longer-cool.html - Don't implement map()?
        void upload() {
            mUploadedOnce = true;
            if(mVBO == 0) {
                glGenVertexArrays(1, &mVBO);
            }
            glBindBuffer(GL_ARRAY_BUFFER, mVBO);
            if(mLastUploadedSize != mSize) {
                glBufferData(GL_ARRAY_BUFFER, mSize, mData, mHint);
                mLastUploadedSize = mSize;
            } else {
                glBufferSubData(GL_ARRAY_BUFFER, 0, mSize, mData);
            }
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        void bind() {
            if(!mUploadedOnce) upload();
            glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        }
    };

    // Think about who is supposed to own the memory in mData!!! - delete if necessary
    class VertexData : public VBOWrapper {
    private:
        const VertexFormat& mVertexFormat;
        int mNumVertices;

    public:
        VertexData(const VertexFormat& format, UsageHint usage = UsageHint::STATIC) :
                mVertexFormat(format), mNumVertices(0), mUsage(usage) {}

        VertexData(const VertexFormat& format, int numVertices, UsageHint usage = UsageHint::STATIC) :
                mVertexFormat(format), mUsage(usage) {
            allocate(numVertices);
        }

        VertexData(const VertexFormat& format, int numVertices, void* data, UsageHint usage = UsageHint::STATIC) :
                mVertexFormat(format), mNumVertices(numVertices), mUsage(usage), mData(data) {
            mSize = format.getAttrCount()*numVertices*4;
        }

        void allocate(int numVertices, bool copyOld = false) {
            int newSize = format.getAttrCount()*numVertices*4;
            void* newData = reinterpret_cast<void*>(new uint8_t[newSize]); // every attribute aligned to 4 byte
            if(copyOld) std::memcpy(newData, mData, mSize]);
            mData = newData;
            mNumVertices = numVertices;
            mSize = newSize;
        }

        int getNumVertices() const {return mNumVertices;}

        const VertexFormat& getVertexFormat() const {
            return mVertexFormat;
        }

        void setData(void* data, int numVertices) {
            mNumVertices = numVertices;
            mSize = format.getAttrCount()*mNumVertices*4;
            mData = data;
        }

        VertexAttributeAccessor operator[](const char* name) {
            return mVertexFormat.getAccessor(name, mData);
        }

        VertexAttributeAccessor operator[](AttributeType attrType) {
            return mVertexFormat.getAccessor(name, attrType);
        }

        // geometry manipulation
        void calculateVertexNormals(bool faceAreaWeighted);
        // sets normals so that in the fragment shader the normals can be interpolated using a "flat" varying - https://www.opengl.org/wiki/Type_Qualifier_(GLSL)
        void calculateFaceNormals(bool lastVertexConvention = true);
        // moves center to 0, 0, 0 and radius to 1.0 if rescale = true
        void calculateTangents();

        void normalize(bool rescale = false);
        void transform(const glm:::mat4& transform);
        void merge(const VertexData& other, const glm::mat4& transform);

        Mesh convertVertexFormat(const VertexFormat& format);

        // pair of position and sizes
        std::pair<glm::vec3, glm::vec3> boundingBox();
        // position and radius
        std::pair<glm::vec3, float> boundingSphere();
    };

    class IndexData : public VBOWrapper {
    public:
        enum class DataType : GLenum {
            UI8 = GL_UNSIGNED_BYTE,
            UI16 = GL_UNSIGNED_SHORT,
            UI32 = GL_UNSIGNED_INT
        };

    private:
        DataType mDataType;
        int mNumIndices;

    public:

    };
}