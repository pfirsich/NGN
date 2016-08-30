#pragma once

#include <string>

#include <glad/glad.h>

namespace ngn {
    enum class AttributeDataType : GLenum {
        I8 = GL_BYTE,
        UI8 = GL_UNSIGNED_BYTE,
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

    int getAttributeDataTypeSize(AttributeDataType type);

    enum class AttributeType : int {
        POSITION = 0,
        NORMAL,
        TANGENT,
        BITANGENT,
        COLOR0,
        COLOR1,
        BONEINDICES,
        BONEWEIGHTS,
        TEXCOORD0, TEXCOORD1, TEXCOORD2, TEXCOORD3,
        CUSTOM0, CUSTOM1, CUSTOM2, CUSTOM3, CUSTOM4, CUSTOM5, CUSTOM6, CUSTOM7,
        FINAL_COUNT_ENTRY // This is not a real type, just used to count
    };

    const char* getVertexAttributeTypeName(AttributeType attrType);

    struct VertexAttribute {
        AttributeType type;
        int num, alignedNum;
        AttributeDataType dataType;
        bool normalized;

        VertexAttribute(AttributeType attrType, int num, AttributeDataType dataType, bool normalized = false) :
                type(attrType), num(num), alignedNum(num), dataType(dataType), normalized(normalized) {
            // Align to 4 Bytes
            // https://www.opengl.org/wiki/Vertex_Specification_Best_Practices#Attribute_sizes
            int overlap = 0;
            switch(dataType) {
                case AttributeDataType::I8:
                case AttributeDataType::UI8:
                    overlap = num % 4;
                    if(overlap > 0) alignedNum = num + (4 - overlap);
                    break;
                case AttributeDataType::I16:
                case AttributeDataType::UI16:
                    overlap = (num * 2) % 4;
                    if(overlap > 0) alignedNum = num + (4 - overlap) / 2;
                    break;
                default:
                    // all others should already be multiples of 4
                    break;
            }
        }
    };
}