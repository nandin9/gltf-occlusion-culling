#include <iostream>
#include "gltfLoader/gltf.h"
#include "export_json.h"

#include "OBJ_Loader.hpp"
#include "Scene.hpp"
#include "Timer.hpp"
#include "Triangle.hpp"
#include "Zbuf.hpp"
#include "global.hpp"
#include "shaders.hpp"

#include <cctype>
#include <cstdio>
#include <cstring>

using namespace std;

void outputCulledModel(gltf::Asset &asset, Zbuf zbuf, objl::Loader loader) {
    
    ofstream fout;
    fout.open("./sceneTestCulled.bin", ios::out|ios::binary);        // 模型被剔除部分的bin文件

    int byteLengthIndexCulled = 0, byteLengthVertexCulled = 0;        // 被剔除部分的bytelength
    for(int i=0; i<zbuf.scene.realworld_triangles.size(); i++){
        if(zbuf.scene.realworld_triangles[i].deleted != 0){
            // float a = zbuf.scene.realworld_triangles[i].a()[0] ;
            float a = loader.LoadedMeshes[0].Vertices[zbuf.scene.realworld_triangles[i].index_a].Position.X ;
            fout.write((char*)&a,sizeof(float));
            a = loader.LoadedMeshes[0].Vertices[zbuf.scene.realworld_triangles[i].index_a].Position.Y ;
            fout.write((char*)&a,sizeof(float));
            a = loader.LoadedMeshes[0].Vertices[zbuf.scene.realworld_triangles[i].index_a].Position.Z ;
            fout.write((char*)&a,sizeof(float));
            a = loader.LoadedMeshes[0].Vertices[zbuf.scene.realworld_triangles[i].index_b].Position.X ;
            fout.write((char*)&a,sizeof(float));
            a = loader.LoadedMeshes[0].Vertices[zbuf.scene.realworld_triangles[i].index_b].Position.Y ;
            fout.write((char*)&a,sizeof(float));
            a = loader.LoadedMeshes[0].Vertices[zbuf.scene.realworld_triangles[i].index_b].Position.Z ;
            fout.write((char*)&a,sizeof(float));
            a = loader.LoadedMeshes[0].Vertices[zbuf.scene.realworld_triangles[i].index_c].Position.X ;
            fout.write((char*)&a,sizeof(float));
            a = loader.LoadedMeshes[0].Vertices[zbuf.scene.realworld_triangles[i].index_c].Position.Y ;
            fout.write((char*)&a,sizeof(float));
            a = loader.LoadedMeshes[0].Vertices[zbuf.scene.realworld_triangles[i].index_c].Position.Z ;
            fout.write((char*)&a,sizeof(float));
            byteLengthVertexCulled += sizeof(float) * 9 ;
        }
    }

    int IndicesAttrIndex = 0 ,meshesIndex = 0 , k= 0;  // k为存进bin的索引
    int trianglesN = 0 ,l = 0 ;
    // asset.newMeshes.resize(asset.meshesLength.size()) ;
    msg("asset.newMeshes.size(): %d",asset.newMeshes.size());  
    bool FLAG = false ;
    int newMeshesNum = 0 ;

    for(int i=0 ,j = 0; i<zbuf.scene.realworld_triangles.size(); i++){
        FLAG = false ;
        if(trianglesN == asset.meshesLength[j] ||  i==zbuf.scene.realworld_triangles.size()-1){
            if(newMeshesNum != 0){
                // asset.newMeshes[meshesIndex].name = asset.meshesName[meshesIndex] ;
                // asset.newMeshes[meshesIndex].primitives.resize(1);
                // asset.newMeshes[meshesIndex].primitives[0].indices = IndicesAttrIndex++ ;
                // asset.newMeshes[meshesIndex].primitives[0].attributes["POSITION"] = IndicesAttrIndex++ ;
                if(i != zbuf.scene.realworld_triangles.size()-1 ){
                    k = 0 ;   //重新开始索引
                    trianglesN = 0;
                    meshesIndex ++ ;
                    j++ ;
                    FLAG = true ;
                } 
            }else{
                trianglesN = 0;
                j ++ ;
            }
        }
        trianglesN ++ ;

        if(FLAG || i==zbuf.scene.realworld_triangles.size()-1){
                // asset.newmeshesLength.push_back(newMeshesNum);
                msg("%d newMeshes Tris ",newMeshesNum);
                newMeshesNum = 0 ;
        }
        if(i==0)
            msg("%d deleted?", zbuf.scene.realworld_triangles[i].deleted) ;
        if(zbuf.scene.realworld_triangles[i].deleted != 0){
            
            newMeshesNum ++ ;
            // uint16_t index = zbuf.scene.realworld_triangles[i].index_a + 1 ; 
            uint32_t index = k ; 
            fout.write((char*)&index,sizeof(uint32_t));  
            index = k + 1 ;
            fout.write((char*)&index,sizeof(uint32_t));  
            index = k + 2 ;
            fout.write((char*)&index,sizeof(uint32_t));
            k += 3 ;
            byteLengthIndexCulled += sizeof(uint32_t) * 3 ;
        }
    }

    fout.close();
}

void occlusionCulling(gltf::Asset &asset) {

    objl::Loader loader;
    if (!loader.LoadFile("./scene.obj")) {
        cout<<"objfile cannot loaded..."<<endl;
    }
    
    // Resolution (horizontal)
    int width = 1920;
    // Resolution (vertical)
    int height = 1080;
    // Field of view (in degrees)
    flt fovy = 45;
    std::function<Color(Triangle const &, Triangle const &,
                        std::tuple<flt, flt, flt> const &barycentric)>
        selected_fragment_shader = shdr::normal_shader;

    // Scene world{loader.LoadedMeshes[0]};             // 1. 创建scene
    Scene world{loader.LoadedMeshes[0],asset.meshesLength,asset.meshesName};  // 20220211 改
    Zbuf zbuf{world, static_cast<size_t>(width), static_cast<size_t>(height)};   // 2.创建zbuffer
    zbuf.set_shader(selected_fragment_shader);
    // auto [eye, gaze, up] = world.generate_camera();
    vec3 pos = vec3{
        // // Viewpoint-1
        // this->root->maxcord[0] * 1.5,
        // this->root->maxcord[1] * 1,
        // this->root->maxcord[2] * 1.2,
        // // Viewpoint-2
        // this->root->maxcord[0] * 1,
        // this->root->maxcord[1] * 2,
        // this->root->maxcord[2] * 1,
        // Viewpoint-default
        
        world.root->maxcord[0] * 1.5,
        world.root->maxcord[1] * 2,
        world.root->maxcord[2] * 1.2,
    };
    vec3 gaze = glm::normalize(vec3{
                              world.root->midcord[0],
                              world.root->midcord[1],
                              world.root->midcord[2],
                          } -
                          pos);
    vec3 up  = glm::normalize(vec3{
        -pos.x,
        std::fabs(gaze.y) < epsilon
              ? 1.0
              : (pos.x * gaze.x + pos.z * gaze.z) / gaze.y,
        -pos.z,
    });
    flt aspect_ratio     = 1.0 * width / height;
    flt znear            = -.1;
    flt zfar             = -50;
    // Camera
    Camera camera{pos, fovy, aspect_ratio, znear, zfar, gaze, up};
    zbuf.init_cam(camera);
    zbuf.set_model_transformation(glm::identity<mat4>());

    // Octree
    zbuf.reset();
    zbuf.render(rendering_method::octree);

    /**
     * @brief 20220214 test
     * 测试输出deleted属性
     */
    msg("zbuf.scene.realworld_triangles.size(): %d \n",zbuf.scene.realworld_triangles.size() );
    // std::cout<<"zbuf.scene.realworld_triangles.size(): "<< zbuf.scene.realworld_triangles.size() << std::endl;
    int a = 0 , b = 0;

    msg("%d 个 meshes ",asset.meshesLength.size());
    msg("%d 个 meshes ",asset.meshesName.size());

    fstream fout ;
    fout.open("./sceneTest.bin", ios::out|ios::binary);               // 模型剩余部分的bin文件
    int byteLengthIndex = 0 , byteLengthVertex = 0;                   // 剩余部分的byteLength
    
    for(int i=0; i<zbuf.scene.realworld_triangles.size(); i++){
        if(zbuf.scene.realworld_triangles[i].deleted == 0){
            // float a = zbuf.scene.realworld_triangles[i].a()[0] ;
            float a = loader.LoadedMeshes[0].Vertices[zbuf.scene.realworld_triangles[i].index_a].Position.X ;
            fout.write((char*)&a,sizeof(float));
            a = loader.LoadedMeshes[0].Vertices[zbuf.scene.realworld_triangles[i].index_a].Position.Y ;
            fout.write((char*)&a,sizeof(float));
            a = loader.LoadedMeshes[0].Vertices[zbuf.scene.realworld_triangles[i].index_a].Position.Z ;
            fout.write((char*)&a,sizeof(float));
            a = loader.LoadedMeshes[0].Vertices[zbuf.scene.realworld_triangles[i].index_b].Position.X ;
            fout.write((char*)&a,sizeof(float));
            a = loader.LoadedMeshes[0].Vertices[zbuf.scene.realworld_triangles[i].index_b].Position.Y ;
            fout.write((char*)&a,sizeof(float));
            a = loader.LoadedMeshes[0].Vertices[zbuf.scene.realworld_triangles[i].index_b].Position.Z ;
            fout.write((char*)&a,sizeof(float));
            a = loader.LoadedMeshes[0].Vertices[zbuf.scene.realworld_triangles[i].index_c].Position.X ;
            fout.write((char*)&a,sizeof(float));
            a = loader.LoadedMeshes[0].Vertices[zbuf.scene.realworld_triangles[i].index_c].Position.Y ;
            fout.write((char*)&a,sizeof(float));
            a = loader.LoadedMeshes[0].Vertices[zbuf.scene.realworld_triangles[i].index_c].Position.Z ;
            fout.write((char*)&a,sizeof(float));
            byteLengthVertex += sizeof(float) * 9 ;
        }
        
        // msg("%d ",asset.meshesLength[j]);
        // msg("%s ",asset.meshesName[i]);
    }
    for(int i=0;i<asset.meshesLength.size();i++)
        msg("%d",asset.meshesLength[i]);   // 每个mesh的三角形数？
    msg("%d",zbuf.scene.realworld_triangles.size());    
    
    int IndicesAttrIndex = 0 ,meshesIndex = 0 , k= 0;  // k为存进bin的索引
    int trianglesN = 0 ,l = 0 ;
    asset.newMeshes.resize(asset.meshesLength.size()) ;
    msg("asset.newMeshes.size(): %d",asset.newMeshes.size());  
    bool FLAG = false ;
    int newMeshesNum = 0 ;
    for(int i=0 ,j = 0; i<zbuf.scene.realworld_triangles.size(); i++){
        FLAG = false ;
        if(trianglesN == asset.meshesLength[j] ||  i==zbuf.scene.realworld_triangles.size()-1){
            if(newMeshesNum != 0){
                asset.newMeshes[meshesIndex].name = asset.meshesName[meshesIndex] ;
                asset.newMeshes[meshesIndex].primitives.resize(1);
                asset.newMeshes[meshesIndex].primitives[0].indices = IndicesAttrIndex++ ;
                asset.newMeshes[meshesIndex].primitives[0].attributes["POSITION"] = IndicesAttrIndex++ ;
                if(i != zbuf.scene.realworld_triangles.size()-1 ){
                    k = 0 ;   //重新开始索引
                    trianglesN = 0;
                    meshesIndex ++ ;
                    j++ ;
                    FLAG = true ;
                } 
            }else{
                trianglesN = 0;
                j ++ ;
            }
           
            
        }
        trianglesN ++ ;

        if(FLAG || i==zbuf.scene.realworld_triangles.size()-1){
                asset.newmeshesLength.push_back(newMeshesNum);
                msg("%d newMeshes Tris ",newMeshesNum);
                newMeshesNum = 0 ;
        }
        if(i==0)
            msg("%d deleted?", zbuf.scene.realworld_triangles[i].deleted) ;
        if(zbuf.scene.realworld_triangles[i].deleted == 0){
            
            newMeshesNum ++ ;
            // uint16_t index = zbuf.scene.realworld_triangles[i].index_a + 1 ; 
            uint32_t index = k ; 
            fout.write((char*)&index,sizeof(uint32_t));  
            index = k + 1 ;
            fout.write((char*)&index,sizeof(uint32_t));  
            index = k + 2 ;
            fout.write((char*)&index,sizeof(uint32_t));
            k += 3 ;
            byteLengthIndex += sizeof(uint32_t) * 3 ;
        }
        
    }


    // new bufferviews
    asset.newBufferViews.resize(2);
    msg("asset.newBufferViews.size(): %d",asset.newBufferViews.size());  
    msg("%d %d ",byteLengthVertex,byteLengthIndex);
    asset.newBufferViews[0].target = 34962 ;
    asset.newBufferViews[0].buffer = 0;
    asset.newBufferViews[0].byteLength = byteLengthVertex;  // ???对吗
    asset.newBufferViews[0].byteStride = sizeof(float)*3 ;
    asset.newBufferViews[1].target = 34963 ;
    asset.newBufferViews[1].buffer = 0;
    asset.newBufferViews[1].byteOffset = asset.newBufferViews[0].byteLength ;
    asset.newBufferViews[1].byteLength = byteLengthIndex; // 对吗？？？
    asset.newBufferViews[1].byteStride = sizeof(uint32_t)*3 ;
    asset.newBufferLength = asset.newBufferViews[0].byteLength + asset.newBufferViews[1].byteLength ;

    // new Accessor
    asset.newAccessors.resize(IndicesAttrIndex) ;
    int byteOffset0 = 0 , byteOffset1 = 0;
    for(int i=0 , j=0;i<IndicesAttrIndex;i+=2,j++){
        // 
        asset.newAccessors[i].bufferView = 1 ;
        asset.newAccessors[i].byteOffset = byteOffset0 ;
        // asset.newAccessors[i].count =  byteLengthIndex / sizeof(short) ;   // 待修改 。。 
        asset.newAccessors[i].count = asset.newmeshesLength[j] * 3 ;
        asset.newAccessors[i].componentType = 5125 ;   // unsigned short
        byteOffset0 += asset.newAccessors[i].count * sizeof(uint32_t) ;
        msg("%d count",asset.newAccessors[i].count); 

        asset.newAccessors[i+1].bufferView = 0 ;
        asset.newAccessors[i+1].byteOffset = byteOffset1 ;
        asset.newAccessors[i+1].componentType = 5126 ;  // float 
        // asset.newAccessors[i+1].count = asset.newBufferViews[0].byteLength / asset.newBufferViews[0].byteStride ;
        asset.newAccessors[i+1].count = asset.newmeshesLength[j]*3 ;  // ?
        byteOffset1 += asset.newAccessors[i+1].count * 12 ;
    }

    for(int i=0;i<5;i++){
        msg("%f %f %f",zbuf.scene.realworld_triangles[i].a()[0],zbuf.scene.realworld_triangles[i].a()[1],zbuf.scene.realworld_triangles[i].a()[2]) ;
        msg("%f %f %f",zbuf.scene.realworld_triangles[i].b()[0],zbuf.scene.realworld_triangles[i].b()[1],zbuf.scene.realworld_triangles[i].b()[2]) ;
        msg("%f %f %f",zbuf.scene.realworld_triangles[i].c()[0],zbuf.scene.realworld_triangles[i].c()[1],zbuf.scene.realworld_triangles[i].c()[2]) ;
        msg("%d %d %d\n",zbuf.scene.realworld_triangles[i].index_a,zbuf.scene.realworld_triangles[i].index_b,zbuf.scene.realworld_triangles[i].index_c) ;
    }

    fout.close();

    outputCulledModel(asset, zbuf, loader);

}

int main(int argc, char *argv[]){

    if(argc != 2){
        cout<<"Usage: ./demo <model.gltf>"<<endl;
        return 0;
    }
    string modelName = argv[1];

    gltf::Asset asset ;
    asset = gltf::load(modelName);

    // cout<<asset.dirName<<endl<<asset.metadata.generator<<endl<<asset.metadata.version<<endl;
    // // nodes test
    // cout<<"nodes: "<<asset.scenes[0].nodes[0]<<endl;
    // // meshes test
    // cout<<"meshes test:"<< asset.meshes.size()<< "  "<< asset.meshes[0].primitives.size()<<"  "<<endl;
    // cout<<asset.meshes[0].primitives[0].indices<< "  "<<asset.meshes[0].primitives[0].attributes["POSITION"] <<endl;
    // // nodes test
    // cout<<"\nnodes test:\n"<<asset.nodes.size()<<endl;
    // for(int i=0;i<asset.nodes[0].children.size();i++)
    //     cout<<asset.nodes[0].children[i]<< " ";

    // // Buffers & BufferViews test
    // cout<<"\nBuffers & BufferViews test" <<endl ;
    // cout<<"uri: "<<asset.buffers[0].uri<<endl;
    // cout<<"ByteLength: "<<asset.buffers[0].byteLength<<endl;
    // cout<<"bufferView.size: "<<asset.bufferViews.size()<<endl;
    // for(int i=0;i<asset.bufferViews.size();i++){
    //     cout<<asset.bufferViews[i].byteLength<<" "<<asset.bufferViews[i].target<<endl;
    // }

    // // accessors 
    // cout<<"\naccessors test\n";
    // cout<<asset.accessors.size();
    // for(int i=0;i<asset.accessors.size();i++){
    //     cout<<asset.accessors[i].componentType<<" "<<asset.accessors[i].count<<endl;
    // }

    // cout<<"\n================================"<<endl;
    
    occlusionCulling(asset);   // 处理遮挡剔除并输出'.bin'文件

    cout<<"aaaaa"<<endl;
    cout<<"bbbbb"<<endl;

    // JSON::exportGLTF(asset);   // 输出'.gltf'文件

    return 0;
}