# Materials

## Random Notes
* There has to be a distinction between a container for all necessary values / textures for a given "Material" and the "Material" itself, being the logic to handle this data. This logic has to have the MaterialData as an input and should be able to build shaders, setup state, send uniforms, etc. => Terminology! => Material and MaterialType? (in pairs)
* Should this MaterialType be a class? Or just a function?
* Can this MaterialType be data? It just has to map input to output values. Assuming GLSL is data, this could theoretically be data.
* In Unreal this seems to be solved with a list of input values (could by any number or map) + some predefined inputs (world position, normal, etc.) and an output value, inbetween is node-stuff, but could easily be GLSL in my version. Just has to be abstracted away. If I use an interpolated vertex normal or the normal of a normal map, should not change much. This can be hard.
* Every input parameter should be able to be a value or a map
* There would have to be measures in place, that check if the described materialtype is possible in the current renderer. e.g. a deferred renderer will not be able to hold dozens of parameters (material-specific data), but only a very limited amount. The renderer gbuffer layout should be configurable anyways and provide "CUSTOM0" to "CUSTOMN", which can then be used by the material-pipeline. There has to be a way to specify bits used for material-id. If the G-Buffer doesn't store that, then there can only be one material.
* PossibleInterface:
class MaterialType, Instantiate it for every type. 
MaterialType("Phong", vertexShaderCode, fragmentShaderCode)
PhongMaterial : public Material, with virtual methods to setup uniform state, etc.
Material-Constructor should take a MaterialType-Object
* Depending on some property being a map or a value, MaterialType-Instances have to compile multiple shaders.
* A deferred renderer would have to check the number of map parameters, then query them, so it can unpack them with the right names. 
* Therefore MaterialType and Renderer have to be coupled, because the resulting shaders are different.

!materialtype
!parameter type=float min=0.0 max=0.0 map=true value=false: specularity
!parameter type=float map=true value=true: specularExponent
!glsl fragment:
ngn_albedo * dot(ngn_lightDirection, ngn_normal)

!material
!specularity: kaka.png

* In the Model described: 
    - How do Vertex Attributes as parameters fit into this picture? - Normals are kinda crazy, since they need one attribute sometimes and if they are a map, they need two. Not every attribute is easily replaced with a map. (Scalars might be)
    - This also shows a problem: VertexData of Meshes has to be partially regenerated (for example tangents) for different Materials! Also the vertex format has to be changed, depending on the material! FOR FUCKS SAKE.
* If only ngn_lightDir, ngn_lightColor and ngn_lightAttenuation are provided spot, directional, point are easy to do. Maybe encourage using ngn_NdotL as the main variable, but also expose ngn_lightDir and with hemisphere lights these get overridden:
ngn_NdotL = 1.0
ngn_lightAttenuation = 1.0
ngn_lightColor = mix(_light_groundColor, _light_skyColor, dot(ngn_lightDir, ngn_normal) * 0.5 + 0.5)
* Image Based Lighting/Spherical harmonics would replace the ambient-term (with a more complex function), ambient term should be computed by the backend?
* In theory for Forward rendering different lights should be able to be batched with a uniform array (i.e. the renderer will create a loop over multiple lights and unpack the values in to ngn_lightDir, ngn_lightAttenuation and add them or just do it multipass)

* This is how Materials are edited in Unreal: https://wiki.unrealengine.com/Intro_To_Materials_(Tutorial)

## Options

### Fully Data-Driven Approach
In this case a "Material" is just a set of parameters and a type name. For example PhongMaterial, which only contains the necessary parameters for PhongLighting + shadow + regular state changes, etc.

#### Pros
Materials are very lightweight, renderers can do clever things for certain materials

#### Cons
Data is not everything. There has to be logic somewhere and it would be a shame, if you would have to subclass the renderer and put in extra clauses, if you want to add another material

--------------------------
## Example Flow

### Preparation
class PhongMaterial : public Material {
public:
    glm::vec3 diffuse;
    float shininess;

    ShaderProgram getShaderProgram()
}

### Usage
Geometry* box = boxGeometry(100, 100, 100);

PhongMaterial mat;
mat.setDiffuse()

**either**:
VertexFormat fmt;
fmt.require(mat);
fmt.require(otherMat);

Mesh* mesh = fmt.makeMesh(box);

Object* obj = new Object(mesh, mat);

-> This constructor will check if the mesh is compatible with the material, then VAO is built for the material.

**or**:
Object* obj = new Object(box, mat);

-> This constructor will automatically do what we did in the "either" section, but only for one material

### Shader building
I think it would be a good idea if there was a slot-system for everything and it could be overridden. The Material base class just asks the current renderer for the default content of the slot and the material can override it.

At the least a Material (a Material Type and a set of parameters) should correspond to a single shader. What I don't like is that you may modify the Material values on a per-instance basis, which might trigger recompiles or shader switches. Maybe you have to compile all combinations beforehand? Or debug output the recompile so that the user of the engine can put measures in place to compile the necessary versions ahead of time. 

OR

There is one Shader per MaterialType and proper default values (for example a default normal map - one pixel blue) are provided. For things like shininess both a texture and uniform value are passed to the shader where they are always multiplied. If no shininess texture is set, but rather a single value, the default texture is white. Or add them and bind no texture and add them (according to the spec fetch an unbound sampler will always result in (0, 0, 0, 1)). In that case there should be a file for a MaterialType and files for the Material. Doing it this way makes it easier to instance models, because the same Mesh and the same Material is guaranteed to make it possible to instance them, i.e. they reference the same model. In that case though Material properties have to be instanced as well, or all the properties have to hashed and instancing is only happening, when the uniforms are equal. This should be done one way or another anyways to minimize uniform uploads. Maybe Instancing should happen explicitly?

You need multiple shader programs anyways, since you want to render different light types for example. And maybe another shader for depth prepass or something.

Material just as MaterialTypes should be Objects, so they can fit easier into a data driven approach.

Maybe optimize Uniform-Calls

The Renderer should initialize a UniformBlock-Object that anyone involved (the SceneNode, the Camera, the Material, etc.) can add Uniforms to.

In any case is a good idea to have Material-Descriptions only be a reference to a lighting model and an aggregation of parameters necessary for it.