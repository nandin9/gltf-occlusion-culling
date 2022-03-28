#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace gltf{
   

    /**
     * @brief scenes/nodes/meshes
     * 场景的基本结构+几何体描述（meshes）
     */
    struct Scene{
        std::string name;
        std::vector<uint32_t> nodes;
    };

    struct Node{

        std::string name;
        std::vector<int> children;
        int32_t mesh = -1 ;
        int32_t skin = -1;

        float matrix[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};    // T*R*S
        float translation[3] = { 0, 0, 0 };    // 或者改为Vec3类型
        float rotation[4] = {0, 0, 0, 1};
        float scale[3] = {1, 1, 1};
        
    };


    using Attributes = std::unordered_map<std::string, uint32_t>;  // 例如<NORMAL,1><POSITION,2>
    struct Primitive{
        
        int32_t indices = -1; // Index to accessor containing the indices
        int32_t material = -1; // Index to the material

        enum class Mode : uint8_t {
            Points = 0,
            Lines = 1,
            LineLoop = 2,
            LineStrip = 3,
            Triangles = 4,
            TriangleStrip = 5,
            TriangleFan = 6
        } mode = Mode::Triangles; // primitive type

        Attributes attributes;  // 每个属性都用其名称和访问器索引映射到数据
        std::vector<Attributes> targets;

    };

    struct Mesh{
        std::string name;
        std::vector<float>weights;
        std::vector<Primitive> primitives;
    };

    
    
    /**
     * @brief buffer/bufferview/accessor
     *  数据引用和数据布局的描述
     * 
     */
    struct Buffer{
        std::string name;
        std::string uri;
        uint32_t byteLength = 0;
       // 
        char* data = nullptr; 

        // float* position = nullptr;
        // float* normal = nullptr;
        // float* tangent = nullptr;

        std::vector<float>positions ;
        std::vector<int>indices ;
        // uint32_t* indice = nullptr ;

    };

    struct BufferView{
        std::string name ;
        uint32_t buffer;  // buffer的索引
        uint32_t byteOffset = 0 ;
        uint32_t byteLength = 0 ;
        uint32_t byteStride = 0 ;

        uint32_t target = 0;

        // enum class TargetType : uint16_t {
        //     None = 0,
        //     ArrayBuffer = 34962,
        //     ElementArrayBuffer = 34963
        // } target; // The target that the GPU buffer should be bound to.
    };

    struct Accessor{
        std::string name;
        int32_t bufferView = -1 ;
        uint32_t byteOffset = 0 ;

        uint32_t componentType = 0 ;

        // enum class ComponentType : uint16_t{
        //     Byte = 5120 ,
        //     UnsignedByte = 5121,
        //     Short = 5122, 
        //     UnsignedShort = 5123,
        //     UnsignedInt = 5125,
        //     Float = 5126
        // } ComponentType;

        bool normalized = false;

        enum class Type : uint8_t {
            Scalar,
            Vec2,
            Vec3,
            Vec4,
            Mat2,
            Mat3,
            Mat4,
        } type;

        uint32_t count;
        std::vector<int32_t> max;
		std::vector<int32_t> min;

    };

    /**
     * @brief Material 材质
     * 
     */
    struct Material {
        
    };
    
    /**
     * @brief Texture/Image/Sampler
     *  模型的表面
     */
    struct Texture{

    };

    struct Image{

    };
    
    struct Sampler{

    };

    
    // 剔除之后的数据结构 
    struct newMesh{
        std::string name;
        std::vector<Primitive> primitives;

    };
    struct newAccessor{
        std::string name;
        int32_t bufferView = -1 ;
        uint32_t byteOffset = 0 ;
        uint32_t componentType = 0 ;

        uint32_t count;
        std::vector<int32_t> max;
		std::vector<int32_t> min;
    };
    struct newBufferView{
        std::string name ;
        uint32_t buffer;  // buffer的索引
        uint32_t byteOffset = 0 ;
        uint32_t byteLength = 0 ;
        uint32_t byteStride = 0 ;
        uint32_t target = 0;

    };

    struct Asset{

        struct Metadata{
            std::string copyright;
            std::string generator;
            std::string version;
            std::string minVersion;
        } metadata;

        std::vector<Accessor> accessors;
        std::vector<Buffer>buffers;
        std::vector<BufferView> bufferViews;
        std::vector<Image> images;
        std::vector<Material> materials;
        std::vector<Mesh> meshes;
        std::vector<Node> nodes;
        int32_t scene = -1; // Index to the default scene
        std::vector<Scene> scenes;
        std::vector<Texture> textures;

        std::string dirName;


         // vertex与indices数据
        std::vector<float> vV ;
        std::vector<float> vnV ;
        std::vector<int> iV ;

        // meshes的大小和meshname
        std::vector<int> meshesLength ;
        std::vector<std::string> meshesName ;

        // 剔除之后
        int newBufferLength = 0 ;
        std::vector<int> newmeshesLength ;
        std::vector<newBufferView> newBufferViews;
        std::vector<newAccessor> newAccessors;
        std::vector<newMesh> newMeshes;
    };
    



    /**
     * @brief  
     * 
     * @param fileName 文件名
     * @return Asset 
     */
    Asset load(std::string fileName) ;


}