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
        TEXCOORD,
        TANGENT,
        COLOR,
        CUSTOM // This has to be the last
    };

    static const char* AttributeTypeNames[] = {
        "POSITION", "NORMAL", "TEXCOORD", "TANGENT", "COLOR", "CUSTOM"
    };

    static_assert(  static_cast<int>(AttributeType::CUSTOM) + 1 == sizeof(AttributeTypeNames) / sizeof(AttributeTypeNames[0]),
                    "There should be just as many AttributeType names as there are AttributeTypes");

    struct VertexAttribute {
        std::string name;
        AttributeDataType dataType;
        int num, alignedNum;
        bool normalized;
        AttributeType type;

        VertexAttribute(const std::string& name, AttributeType attrType, int num, AttributeDataType dataType, bool normalized = false) :
                name(name), dataType(dataType), num(num), alignedNum(num), normalized(normalized), type(attrType) {
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