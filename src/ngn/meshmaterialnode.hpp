#pragma once

#include "scenenode.hpp"
#include "mesh.hpp"
#include "material.hpp"

namespace ngn {
    class MeshMaterialNode : public SceneNode {
    private:
        Mesh* mMesh;
        Material* mMaterial;
    public:
        MeshMaterialNode(Mesh* mesh = nullptr, Material* mat = nullptr) : mMesh(mesh), mMaterial(mat) {}
        Mesh* getMesh() {return mMesh;}
        void setMesh(Mesh* mesh) {mMesh = mesh;}
        Material* getMaterial() {return mMaterial;}
        void setMaterial(Material* mat) {mMaterial = mat;}
    };

    using Object = MeshMaterialNode;
}