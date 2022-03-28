#include <iostream>
#include <fstream>
#include <string>
#include "json.hpp"
#include "gltf.h"
#include "Exceptions.hpp"

namespace gltf {
    
    static void loadAsset(Asset& asset, nlohmann::json& json);
    static void loadScenes(Asset& asset, nlohmann::json& json);
    static void loadMeshes(Asset& asset, nlohmann::json& json);
    static void loadNodes(Asset& asset, nlohmann::json& json);

    static void loadBuffers(Asset& asset, nlohmann::json& json);
    static void loadAccessors(Asset& asset, nlohmann::json& json);
    static void loadBufferViews(Asset& asset, nlohmann::json& json);
    static void loadBufferData(Asset& asset, Buffer& buffer);


    // 加载 metadata （version、copyright、generator）
    static void loadAsset(Asset& asset, nlohmann::json& json){
        if (json.find("asset") == json.end()) {
            throw MisformattedExceptionIsRequired("asset");
        }
         // version
        if (json["asset"].find("version") == json["asset"].end()) {
            throw MisformattedExceptionIsRequired("asset[version]");
        }
        if (!json["asset"]["version"].is_string()) {
            throw MisformattedExceptionNotString("version");
        }
        asset.metadata.version = json["asset"]["version"].get<std::string>();

        // copyright
        if (json["asset"].find("copyright") != json["asset"].end()) {
            if (!json["asset"]["copyright"].is_string()) {
                throw MisformattedExceptionNotString("copyright");
            }

            asset.metadata.copyright = json["asset"]["copyright"].get<std::string>();
        }

        // generator
        if (json["asset"].find("generator") != json["asset"].end()) {
            if (!json["asset"]["generator"].is_string()) {
                throw MisformattedExceptionNotString("generator");
            }

            asset.metadata.generator = json["asset"]["generator"].get<std::string>();
        }
    }
    
    // 加载scenes （通常只包含一个场景，由nodes数组组成） 
    static void loadScenes(Asset& asset, nlohmann::json& json){

        if (json.find("scene") != json.end()) {
            if (!json["scene"].is_number()) {
                throw MisformattedExceptionNotNumber("scene");
            }
            asset.scene = json["scene"].get<int32_t>();
        }
        if (json.find("scenes") == json.end()) {
        return;
    }

        auto& scenes = json["scenes"];
        if (!scenes.is_array()) {
            throw MisformattedExceptionNotArray("scenes");
        }

        if (asset.scene == -1) {
            asset.scene = 0;
        }

        asset.scenes.resize(scenes.size());
        for (uint32_t i = 0; i < scenes.size(); ++i) {
            // name
            if (scenes[i].find("name") != scenes[i].end()) {
                if (!scenes[i]["name"].is_string()) {
                    throw MisformattedExceptionNotString("scenes[i][name]");
                }

                std::string n = scenes[i]["name"];
                asset.scenes[i].name = n ;
            }

            // nodes
            if (scenes[i].find("nodes") != scenes[i].end()) {
                auto& nodes = scenes[i]["nodes"];
                if (!nodes.is_array()) {
                    throw MisformattedExceptionNotArray("scenes[i][nodes]");
                }

                asset.scenes[i].nodes.resize(nodes.size());
                for (uint32_t j = 0; j < nodes.size(); ++j) {
                    asset.scenes[i].nodes[j] = nodes[j].get<uint32_t>();
                }
            }
        }
    }

    // Meshes: 主要由primitives构成
    // primitives 包含attribute(几何数据)、indices(索引)、material(材质)等信息
    static void loadMeshes(Asset& asset, nlohmann::json& json){

        if (json.find("meshes") == json.end()) {
            return;
        }

        auto& meshes = json["meshes"];

        if (!meshes.is_array()) {
            throw MisformattedExceptionNotArray("meshes");
        }

        asset.meshes.resize(meshes.size());
        for (uint32_t i = 0; i < meshes.size(); ++i) {
            // name
            if (meshes[i].find("name") != meshes[i].end()) {
                if (!meshes[i]["name"].is_string()) {
                    throw MisformattedExceptionNotString("meshes[i][name]");
                }

                std::string n = meshes[i]["name"];
                asset.meshes[i].name = n;
            }


            if (meshes[i].find("primitives") == meshes[i].end()) {
                throw MisformattedExceptionIsRequired("meshes[i][primitives]");
            }

            auto& primitives = meshes[i]["primitives"];
            if (!primitives.is_array()) {
                throw MisformattedExceptionNotArray("meshes[i][primitives]");
            }

            asset.meshes[i].primitives.resize(primitives.size());
            // 每个primitive 包含attribute(几何数据)、indices(索引)、material(材质)等信息
            // 值都是 accessor 的索引 
            for (uint32_t j = 0; j < primitives.size(); ++j) {
                // indices
                if (primitives[j].find("indices") != primitives[j].end()) {
                    if (!primitives[j]["indices"].is_number()) {
                        throw MisformattedExceptionNotNumber("meshes[i][primitives][j][indices]");
                    }

                    asset.meshes[i].primitives[j].indices = primitives[j]["indices"].get<int32_t>();
                }

                // material
                if (primitives[j].find("material") != primitives[j].end()) {
                    if (!primitives[j]["material"].is_number()) {
                        throw MisformattedExceptionNotNumber("meshes[i][primitives][j][material]");
                    }

                    asset.meshes[i].primitives[j].material = primitives[j]["material"].get<int32_t>();
                }

                // mode
                if (primitives[j].find("mode") != primitives[j].end()) {
                    if (!primitives[j]["mode"].is_number()) {
                        throw MisformattedExceptionNotNumber("meshes[i][primitives][j][mode]");
                    }

                    asset.meshes[i].primitives[j].mode = static_cast<Primitive::Mode>(primitives[j]["mode"].get<uint8_t>());
                }

                if (primitives[j].find("attributes") == primitives[j].end()) {
                    throw MisformattedExceptionIsRequired("meshes[i][primitives][j][attributes]");
                }

                auto& attributes = primitives[j]["attributes"];
                if (!attributes.is_object()) {
                    throw MisformattedExceptionNotObject("meshes[i][primitives][j][attributes]");
                }

                for (nlohmann::json::iterator it = attributes.begin(); it != attributes.end(); ++it) {
                    asset.meshes[i].primitives[j].attributes[it.key()] = it.value();
                }
            }

        }

    }

    void localTransform(float T[3],float R[4],float S[3],float(&Matrix)[16]){
        std::vector<std::vector<float>> Tra(4,std::vector<float>(4)); 
        std::vector<std::vector<float>> Rot(4,std::vector<float>(4)); 
        std::vector<std::vector<float>> Sca(4,std::vector<float>(4)); 

        Tra[0][0]=1.0; Tra[0][1]=0.0; Tra[0][2]=0.0; Tra[0][3]=T[0];
        Tra[1][0]=0.0; Tra[1][1]=1.0; Tra[1][2]=0.0; Tra[1][3]=T[1];
        Tra[2][0]=0.0; Tra[2][1]=0.0; Tra[2][2]=1.0; Tra[2][3]=T[2];
        Tra[3][0]=0.0; Tra[3][1]=0.0; Tra[3][2]=0.0; Tra[3][3]=1.0;

        // Rot[0][0]=1-2*R[2]*R[2]-2*R[3]*R[3]; Rot[0][1]=2*R[1]*R[2]+2*R[0]*R[3];   Rot[0][2]=2*R[1]*R[3]-2*R[0]*R[2];   Rot[0][3]=0.0;
        // Rot[1][0]=2*R[1]*R[2]-2*R[0]*R[3];   Rot[1][1]=1-2*R[1]*R[1]-2*R[3]*R[3]; Rot[1][2]=2*R[2]*R[3]+2*R[0]*R[1];   Rot[1][3]=0.0;
        // Rot[2][0]=2*R[1]*R[3]+2*R[0]*R[2];   Rot[2][1]=2*R[2]*R[3]-2*R[0]*R[1];   Rot[2][2]=1-2*R[1]*R[1]-2*R[2]*R[2]; Rot[2][3]=0.0;
        // Rot[3][0]=0.0;                       Rot[3][1]=0.0;                       Rot[3][2]=0.0;                       Rot[3][3]=1.0;
        // Rot[0][0]=R[0]*R[0]+R[1]*R[1]-R[2]*R[2]-R[3]*R[3]; Rot[0][1]=2*R[1]*R[2]+2*R[0]*R[3];   Rot[0][2]=2*R[1]*R[3]-2*R[0]*R[2];   Rot[0][3]=0.0;
        // Rot[1][0]=2*R[1]*R[2]-2*R[0]*R[3];   Rot[1][1]=R[0]*R[0]-R[1]*R[1]+R[2]*R[2]-R[3]*R[3]; Rot[1][2]=2*R[2]*R[3]+2*R[0]*R[1];   Rot[1][3]=0.0;
        // Rot[2][0]=2*R[1]*R[3]+2*R[0]*R[2];   Rot[2][1]=2*R[2]*R[3]-2*R[0]*R[1];   Rot[2][2]=R[0]*R[0]-R[1]*R[1]-R[2]*R[2]+R[3]*R[3]; Rot[2][3]=0.0;
        // Rot[3][0]=0.0;                       Rot[3][1]=0.0;                       Rot[3][2]=0.0;                       Rot[3][3]=1.0;


        // Rot[0][0]=1-2*R[2]*R[2]-2*R[3]*R[3]; Rot[1][0]=2*R[1]*R[2]+2*R[0]*R[3];   Rot[2][0]=2*R[1]*R[3]-2*R[0]*R[2];   Rot[3][0]=0.0;
        // Rot[0][1]=2*R[1]*R[2]-2*R[0]*R[3];   Rot[1][1]=1-2*R[1]*R[1]-2*R[3]*R[3]; Rot[2][1]=2*R[2]*R[3]+2*R[0]*R[1];   Rot[3][1]=0.0;
        // Rot[0][2]=2*R[1]*R[3]+2*R[0]*R[2];   Rot[1][2]=2*R[2]*R[3]-2*R[0]*R[1];   Rot[2][2]=1-2*R[1]*R[1]-2*R[2]*R[2]; Rot[3][2]=0.0;
        // Rot[0][3]=0.0;                       Rot[1][3]=0.0;                       Rot[2][3]=0.0;                       Rot[3][3]=1.0;
        // Rot[0][0]=1.0;   Rot[1][0]=0.0;   Rot[2][0]=0.0;   Rot[3][0]=0.0;
        // Rot[0][1]=0.0;   Rot[1][1]=0.0;   Rot[2][1]=1.0;   Rot[3][1]=0.0;
        // Rot[0][2]=0.0;   Rot[1][2]=-1.0;   Rot[2][2]=0.0;   Rot[3][2]=0.0;
        // Rot[0][3]=0.0;   Rot[1][3]=0.0;   Rot[2][3]=0.0;   Rot[3][3]=1.0;

        //  new 0119 
        // Rot[0][0]=1-2*R[1]*R[1]-2*R[2]*R[2]; Rot[0][1]=2*R[0]*R[1]+2*R[3]*R[2];   Rot[0][2]=2*R[0]*R[2]-2*R[3]*R[1];   Rot[0][3]=0.0;
        // Rot[1][0]=2*R[0]*R[1]-2*R[3]*R[2];   Rot[1][1]=1-2*R[0]*R[0]-2*R[2]*R[2]; Rot[1][2]=2*R[1]*R[2]+2*R[3]*R[0];   Rot[1][3]=0.0;
        // Rot[2][0]=2*R[0]*R[2]+2*R[3]*R[1];   Rot[2][1]=2*R[1]*R[2]-2*R[3]*R[0];   Rot[2][2]=1-2*R[0]*R[0]-2*R[1]*R[1]; Rot[2][3]=0.0;
        // Rot[3][0]=0.0;                       Rot[3][1]=0.0;                       Rot[3][2]=0.0;                       Rot[3][3]=1.0;

        Rot[0][0]=1-2*R[1]*R[1]-2*R[2]*R[2]; Rot[1][0]=2*R[0]*R[1]+2*R[3]*R[2];   Rot[2][0]=2*R[0]*R[2]-2*R[3]*R[1];   Rot[3][0]=0.0;
        Rot[0][1]=2*R[0]*R[1]-2*R[3]*R[2];   Rot[1][1]=1-2*R[0]*R[0]-2*R[2]*R[2]; Rot[2][1]=2*R[1]*R[2]+2*R[3]*R[0];   Rot[3][1]=0.0;
        Rot[0][2]=2*R[0]*R[2]+2*R[3]*R[1];   Rot[1][2]=2*R[1]*R[2]-2*R[3]*R[0];   Rot[2][2]=1-2*R[0]*R[0]-2*R[1]*R[1]; Rot[3][2]=0.0;
        Rot[0][3]=0.0;                       Rot[1][3]=0.0;                       Rot[2][3]=0.0;                       Rot[3][3]=1.0;

        // Rot[0][0]=0.875;   Rot[1][0]=-0.21651;   Rot[2][0]=0.43301;   Rot[3][0]=0.0;
        // Rot[0][1]=0.43301;   Rot[1][1]=0.75;   Rot[2][1]=-0.5;   Rot[3][1]=0.0;
        // Rot[0][2]=-0.21651;   Rot[1][2]=0.625;   Rot[2][2]=0.75;   Rot[3][2]=0.0;
        // Rot[0][3]=0.0;   Rot[1][3]=0.0;   Rot[2][3]=0.0;   Rot[3][3]=1.0;


        std::cout<<"\nRot Test\n";
        for(int i=0;i<4;i++){
            for(int j=0;j<4;j++){
                std::cout<<Rot[i][j]<<" ";
            }
            std::cout<<"\n";
        }


        Sca[0][0]=S[0]; Sca[0][1]=0.0;  Sca[0][2]=0.0;  Sca[0][3]=0.0;
        Sca[1][0]=0.0;  Sca[1][1]=S[1]; Sca[1][2]=0.0;  Sca[1][3]=0.0;
        Sca[2][0]=0.0;  Sca[2][1]=0.0;  Sca[2][2]=S[2]; Sca[2][3]=0.0;
        Sca[3][0]=0.0;  Sca[3][1]=0.0;  Sca[3][2]=0.0;  Sca[3][3]=1.0;

        // tmp = T*R
        std::vector<std::vector<float>> tmp(4,std::vector<float>(4)); 
        tmp[0][0]=Tra[0][0]*Rot[0][0] + Tra[0][1]*Rot[1][0] + Tra[0][2]*Rot[2][0] + Tra[0][3]*Rot[3][0];  tmp[0][1]=Tra[0][0]*Rot[0][1] + Tra[0][1]*Rot[1][1] + Tra[0][2]*Rot[2][1] + Tra[0][3]*Rot[3][1];  
        tmp[0][2]=Tra[0][0]*Rot[0][2] + Tra[0][1]*Rot[1][2] + Tra[0][2]*Rot[2][2] + Tra[0][3]*Rot[3][2];  tmp[0][3]=Tra[0][0]*Rot[0][3] + Tra[0][1]*Rot[1][3] + Tra[0][2]*Rot[2][3] + Tra[0][3]*Rot[3][3];
        tmp[1][0]=Tra[1][0]*Rot[0][0] + Tra[1][1]*Rot[1][0] + Tra[1][2]*Rot[2][0] + Tra[1][3]*Rot[3][0];  tmp[1][1]=Tra[1][0]*Rot[0][1] + Tra[1][1]*Rot[1][1] + Tra[1][2]*Rot[2][1] + Tra[1][3]*Rot[3][1]; 
        tmp[1][2]=Tra[1][0]*Rot[0][2] + Tra[1][1]*Rot[1][2] + Tra[1][2]*Rot[2][2] + Tra[1][3]*Rot[3][2];  tmp[1][3]=Tra[1][0]*Rot[0][3] + Tra[1][1]*Rot[1][3] + Tra[1][2]*Rot[2][3] + Tra[1][3]*Rot[3][3];
        tmp[2][0]=Tra[2][0]*Rot[0][0] + Tra[2][1]*Rot[1][0] + Tra[2][2]*Rot[2][0] + Tra[2][3]*Rot[3][0];  tmp[2][1]=Tra[2][0]*Rot[0][1] + Tra[2][1]*Rot[1][1] + Tra[2][2]*Rot[2][1] + Tra[2][3]*Rot[3][1];  
        tmp[2][2]=Tra[2][0]*Rot[0][2] + Tra[2][1]*Rot[1][2] + Tra[2][2]*Rot[2][2] + Tra[2][3]*Rot[3][2];  tmp[2][3]=Tra[2][0]*Rot[0][3] + Tra[2][1]*Rot[1][3] + Tra[2][2]*Rot[2][3] + Tra[2][3]*Rot[3][3];
        tmp[3][0]=Tra[3][0]*Rot[0][0] + Tra[3][1]*Rot[1][0] + Tra[3][2]*Rot[2][0] + Tra[3][3]*Rot[3][0];  tmp[3][1]=Tra[3][0]*Rot[0][1] + Tra[3][1]*Rot[1][1] + Tra[3][2]*Rot[2][1] + Tra[3][3]*Rot[3][1];
        tmp[3][2]=Tra[3][0]*Rot[0][2] + Tra[3][1]*Rot[1][2] + Tra[3][2]*Rot[2][2] + Tra[3][3]*Rot[3][2];  tmp[3][3]=Tra[3][0]*Rot[0][3] + Tra[3][1]*Rot[1][3] + Tra[3][2]*Rot[2][3] + Tra[3][3]*Rot[3][3];

        // matrix = tmp*S
        std::vector<float>tmp2(16) ;
        tmp2[0]=tmp[0][0]*Sca[0][0] + tmp[0][1]*Sca[1][0] + tmp[0][2]*Sca[2][0] + tmp[0][3]*Sca[3][0];   tmp2[1]=tmp[0][0]*Sca[0][1] + tmp[0][1]*Sca[1][1] + tmp[0][2]*Sca[2][1] + tmp[0][3]*Sca[3][1];  
        tmp2[2]=tmp[0][0]*Sca[0][2] + tmp[0][1]*Sca[1][2] + tmp[0][2]*Sca[2][2] + tmp[0][3]*Sca[3][2];   tmp2[3]=tmp[0][0]*Sca[0][3] + tmp[0][1]*Sca[1][3] + tmp[0][2]*Sca[2][3] + tmp[0][3]*Sca[3][3];
        tmp2[4]=tmp[1][0]*Sca[0][0] + tmp[1][1]*Sca[1][0] + tmp[1][2]*Sca[2][0] + tmp[1][3]*Sca[3][0];   tmp2[5]=tmp[1][0]*Sca[0][1] + tmp[1][1]*Sca[1][1] + tmp[1][2]*Sca[2][1] + tmp[1][3]*Sca[3][1]; 
        tmp2[6]=tmp[1][0]*Sca[0][2] + tmp[1][1]*Sca[1][2] + tmp[1][2]*Sca[2][2] + tmp[1][3]*Sca[3][2];   tmp2[7]=tmp[1][0]*Sca[0][3] + tmp[1][1]*Sca[1][3] + tmp[1][2]*Sca[2][3] + tmp[1][3]*Sca[3][3];
        tmp2[8]=tmp[2][0]*Sca[0][0] + tmp[2][1]*Sca[1][0] + tmp[2][2]*Sca[2][0] + tmp[2][3]*Sca[3][0];   tmp2[9]=tmp[2][0]*Sca[0][1] + tmp[2][1]*Sca[1][1] + tmp[2][2]*Sca[2][1] + tmp[2][3]*Sca[3][1];  
        tmp2[10]=tmp[2][0]*Sca[0][2] + tmp[2][1]*Sca[1][2] + tmp[2][2]*Sca[2][2] + tmp[2][3]*Sca[3][2];  tmp2[11]=tmp[2][0]*Sca[0][3] + tmp[2][1]*Sca[1][3] + tmp[2][2]*Sca[2][3] + tmp[2][3]*Sca[3][3];
        tmp2[12]=tmp[3][0]*Sca[0][0] + tmp[3][1]*Sca[1][0] + tmp[3][2]*Sca[2][0] + tmp[3][3]*Sca[3][0];  tmp2[13]=tmp[3][0]*Sca[0][1] + tmp[3][1]*Sca[1][1] + tmp[3][2]*Sca[2][1] + tmp[3][3]*Sca[3][1];
        tmp2[14]=tmp[3][0]*Sca[0][2] + tmp[3][1]*Sca[1][2] + tmp[3][2]*Sca[2][2] + tmp[3][3]*Sca[3][2];  tmp2[15]=tmp[3][0]*Sca[0][3] + tmp[3][1]*Sca[1][3] + tmp[3][2]*Sca[2][3] + tmp[3][3]*Sca[3][3];

        std::vector<float>tmp3(16) ;
        tmp3[0] = Matrix[0]*tmp2[0] + Matrix[1]*tmp2[4] + Matrix[2]*tmp2[8] + Matrix[3]*tmp2[12];
        tmp3[1] = Matrix[0]*tmp2[1] + Matrix[1]*tmp2[5] + Matrix[2]*tmp2[9] + Matrix[3]*tmp2[13];
        tmp3[2] = Matrix[0]*tmp2[2] + Matrix[1]*tmp2[6] + Matrix[2]*tmp2[10] + Matrix[3]*tmp2[14];
        tmp3[3] = Matrix[0]*tmp2[3] + Matrix[1]*tmp2[7] + Matrix[2]*tmp2[11] + Matrix[3]*tmp2[15];

        tmp3[4] = Matrix[4]*tmp2[0] + Matrix[5]*tmp2[4] + Matrix[6]*tmp2[8] + Matrix[7]*tmp2[12];
        tmp3[5] = Matrix[4]*tmp2[1] + Matrix[5]*tmp2[5] + Matrix[6]*tmp2[9] + Matrix[7]*tmp2[13];
        tmp3[6] = Matrix[4]*tmp2[2] + Matrix[5]*tmp2[6] + Matrix[6]*tmp2[10] + Matrix[7]*tmp2[14];
        tmp3[7] = Matrix[4]*tmp2[3] + Matrix[5]*tmp2[7] + Matrix[6]*tmp2[11] + Matrix[7]*tmp2[15];

        tmp3[8] = Matrix[8]*tmp2[0] + Matrix[9]*tmp2[4] + Matrix[10]*tmp2[8] + Matrix[11]*tmp2[12];
        tmp3[9] = Matrix[8]*tmp2[1] + Matrix[9]*tmp2[5] + Matrix[10]*tmp2[9] + Matrix[11]*tmp2[13];
        tmp3[10] = Matrix[8]*tmp2[2] + Matrix[9]*tmp2[6] + Matrix[10]*tmp2[10] + Matrix[11]*tmp2[14];
        tmp3[11] = Matrix[8]*tmp2[3] + Matrix[9]*tmp2[7] + Matrix[10]*tmp2[11] + Matrix[11]*tmp2[15];

        tmp3[12] = Matrix[12]*tmp2[0] + Matrix[13]*tmp2[4] + Matrix[14]*tmp2[8] + Matrix[15]*tmp2[12];
        tmp3[13] = Matrix[12]*tmp2[1] + Matrix[13]*tmp2[5] + Matrix[14]*tmp2[9] + Matrix[15]*tmp2[13];
        tmp3[14] = Matrix[12]*tmp2[2] + Matrix[13]*tmp2[6] + Matrix[14]*tmp2[10] + Matrix[15]*tmp2[14];
        tmp3[15] = Matrix[12]*tmp2[3] + Matrix[13]*tmp2[7] + Matrix[14]*tmp2[11] + Matrix[15]*tmp2[15];


        for(int k=0;k<16;k++){
            Matrix[k] = tmp3[k]; 
        }

    }
    // 由多个children和多个mesh组成 
    // children是下面mesh的索引，可以定位mesh
    // 每个mesh都是由matrix(旋转缩放平移变换矩阵？)、mesh(meshes的索引)、name组成
    // 坐标转换localTransform：需要将T、R、S转换为Matrix
    static void loadNodes(Asset& asset, nlohmann::json& json){
        if (json.find("nodes") == json.end()) {
            return;
        }

        auto& nodes = json["nodes"];
        if (!nodes.is_array()) {
            throw MisformattedExceptionNotArray("nodes");
        }
       
        asset.nodes.resize(nodes.size());
        for (uint32_t i = 0; i < nodes.size(); i++) {
            // name
            if (nodes[i].find("name") != nodes[i].end()) {
                if (!nodes[i]["name"].is_string()) {
                    throw MisformattedExceptionNotString("nodes[i][name]");
                }

                std::string n = nodes[i]["name"];
                asset.nodes[i].name = n;
            }

            // camera
            // if (nodes[i].find("camera") != nodes[i].end()) {
            //     if (!nodes[i]["camera"].is_number()) {
            //         throw MisformattedExceptionNotNumber("nodes[i][camera]");
            //     }

            //     asset.nodes[i].camera = nodes[i]["camera"].get<int32_t>();
            // }

            // children
            std::cout<<"children";
            if (nodes[i].find("children") != nodes[i].end()) {
                auto& children = nodes[i]["children"];
                if (!children.is_array()) {
                    throw MisformattedExceptionNotArray("nodes[i][chidren]");
                }

                asset.nodes[i].children.resize(children.size());
                for (uint32_t j = 0; j < children.size(); ++j) {
                    asset.nodes[i].children[j] = children[j].get<uint32_t>();
                }
            }

            // skin
            if (nodes[i].find("skin") != nodes[i].end()) {
                if (!nodes[i]["skin"].is_number()) {
                    throw MisformattedExceptionNotNumber("nodes[i][skin]");
                }

                asset.nodes[i].skin = nodes[i]["skin"].get<int32_t>();
            }

            // TODO: nodes[i]["matrix"]
            // matrix 
            bool hasMatrix = false ;
            if (nodes[i].find("matrix") != nodes[i].end()) {
                if (!nodes[i]["matrix"].is_array()) {
                    throw MisformattedExceptionNotArray("nodes[i][matrix]");
                }

                if (nodes[i]["matrix"].size() != 16) {
                    throw MisformattedExceptionNotGoodSizeArray("nodes[i][matrix]");
                }
                std::vector<float> tmp (16) ;
                std::vector<float> tmp2 (16) ;
                for (uint32_t j = 0; j < 16; ++j) {
                    if (!nodes[i]["matrix"][j].is_number()) {
                        throw MisformattedExceptionNotNumber("nodes[i][matrix][j]");
                    }

                   // asset.nodes[i].matrix[j] = nodes[i]["matrix"][j].get<float>();
                    tmp[j]= nodes[i]["matrix"][j].get<float>();
                }
                tmp2[0] = tmp[0]*asset.nodes[i].matrix[0] + tmp[1]*asset.nodes[i].matrix[4] + tmp[2]*asset.nodes[i].matrix[8] + tmp[3]*asset.nodes[i].matrix[12];
                tmp2[1] = tmp[0]*asset.nodes[i].matrix[1] + tmp[1]*asset.nodes[i].matrix[5] + tmp[2]*asset.nodes[i].matrix[9] + tmp[3]*asset.nodes[i].matrix[13];
                tmp2[2] = tmp[0]*asset.nodes[i].matrix[2] + tmp[1]*asset.nodes[i].matrix[6] + tmp[2]*asset.nodes[i].matrix[10] + tmp[3]*asset.nodes[i].matrix[14];
                tmp2[3] = tmp[0]*asset.nodes[i].matrix[3] + tmp[1]*asset.nodes[i].matrix[7] + tmp[2]*asset.nodes[i].matrix[11] + tmp[3]*asset.nodes[i].matrix[15];

                tmp2[4] = tmp[4]*asset.nodes[i].matrix[0] + tmp[5]*asset.nodes[i].matrix[4] + tmp[6]*asset.nodes[i].matrix[8] + tmp[7]*asset.nodes[i].matrix[12];
                tmp2[5] = tmp[4]*asset.nodes[i].matrix[1] + tmp[5]*asset.nodes[i].matrix[5] + tmp[6]*asset.nodes[i].matrix[9] + tmp[7]*asset.nodes[i].matrix[13];
                tmp2[6] = tmp[4]*asset.nodes[i].matrix[2] + tmp[5]*asset.nodes[i].matrix[6] + tmp[6]*asset.nodes[i].matrix[10] + tmp[7]*asset.nodes[i].matrix[14];
                tmp2[7] = tmp[4]*asset.nodes[i].matrix[3] + tmp[5]*asset.nodes[i].matrix[7] + tmp[6]*asset.nodes[i].matrix[11] + tmp[7]*asset.nodes[i].matrix[15];

                tmp2[8] = tmp[8]*asset.nodes[i].matrix[0] + tmp[9]*asset.nodes[i].matrix[4] + tmp[10]*asset.nodes[i].matrix[8] + tmp[11]*asset.nodes[i].matrix[12];
                tmp2[9] = tmp[8]*asset.nodes[i].matrix[1] + tmp[9]*asset.nodes[i].matrix[5] + tmp[10]*asset.nodes[i].matrix[9] + tmp[11]*asset.nodes[i].matrix[13];
                tmp2[10] = tmp[8]*asset.nodes[i].matrix[2] + tmp[9]*asset.nodes[i].matrix[6] + tmp[10]*asset.nodes[i].matrix[10] + tmp[11]*asset.nodes[i].matrix[14];
                tmp2[11] = tmp[8]*asset.nodes[i].matrix[3] + tmp[9]*asset.nodes[i].matrix[7] + tmp[10]*asset.nodes[i].matrix[11] + tmp[11]*asset.nodes[i].matrix[15];

                tmp2[12] = tmp[12]*asset.nodes[i].matrix[0] + tmp[13]*asset.nodes[i].matrix[4] + tmp[14]*asset.nodes[i].matrix[8] + tmp[15]*asset.nodes[i].matrix[12];
                tmp2[13] = tmp[12]*asset.nodes[i].matrix[1] + tmp[13]*asset.nodes[i].matrix[5] + tmp[14]*asset.nodes[i].matrix[9] + tmp[15]*asset.nodes[i].matrix[13];
                tmp2[14] = tmp[12]*asset.nodes[i].matrix[2] + tmp[13]*asset.nodes[i].matrix[6] + tmp[14]*asset.nodes[i].matrix[10] + tmp[15]*asset.nodes[i].matrix[14];
                tmp2[15] = tmp[12]*asset.nodes[i].matrix[3] + tmp[13]*asset.nodes[i].matrix[7] + tmp[14]*asset.nodes[i].matrix[11] + tmp[15]*asset.nodes[i].matrix[15];

                for(int k=0;k<16;k++){
                    asset.nodes[i].matrix[k] = tmp2[k]; 
                }

                hasMatrix = true ;
            }
            // mesh
            if (nodes[i].find("mesh") != nodes[i].end()) {
                if (!nodes[i]["mesh"].is_number()) {
                    throw MisformattedExceptionNotNumber("nodes[i][mesh]");
                }

                asset.nodes[i].mesh = nodes[i]["mesh"].get<int32_t>();
            }
            // translation
            if (nodes[i].find("translation") != nodes[i].end()) {
                if (!nodes[i]["translation"].is_array()) {
                    throw MisformattedExceptionNotArray("nodes[i][translation]");
                }

                if (nodes[i]["translation"].size() != 3) {
                    throw MisformattedExceptionNotGoodSizeArray("nodes[i][translation]");
                }

                for (uint32_t j = 0; j < 3; ++j) {
                    if (!nodes[i]["translation"][j].is_number()) {
                        throw MisformattedExceptionNotNumber("nodes[i][translation][j]");
                    }

                    asset.nodes[i].translation[j] = nodes[i]["translation"][j].get<float>();
                }
            }

            // rotation
            if (nodes[i].find("rotation") != nodes[i].end()) {
                if (!nodes[i]["rotation"].is_array()) {
                    throw MisformattedExceptionNotArray("nodes[i][rotation]");
                }

                if (nodes[i]["rotation"].size() != 4) {
                    throw MisformattedExceptionNotGoodSizeArray("nodes[i][rotation]");
                }

                for (uint32_t j = 0; j < 4; ++j) {
                    if (!nodes[i]["rotation"][j].is_number()) {
                        throw MisformattedExceptionNotNumber("nodes[i][rotation][j]");
                    }

                    asset.nodes[i].rotation[j] = nodes[i]["rotation"][j].get<float>();
                }
            }
            // scale
            if (nodes[i].find("scale") != nodes[i].end()) {
                if (!nodes[i]["scale"].is_array()) {
                    throw MisformattedExceptionNotArray("nodes[i][scale]");
                }

                if (nodes[i]["scale"].size() != 3) {
                    throw MisformattedExceptionNotGoodSizeArray("nodes[i][scale]");
                }

                for (uint32_t j = 0; j < 3; ++j) {
                    if (!nodes[i]["scale"][j].is_number()) {
                        throw MisformattedExceptionNotNumber("nodes[i][scale][j]");
                    }

                    asset.nodes[i].scale[j] = nodes[i]["scale"][j].get<float>();
                }
            }

            if(!hasMatrix)
                localTransform(asset.nodes[i].translation,asset.nodes[i].rotation,asset.nodes[i].scale,asset.nodes[i].matrix) ;  // T * R * S
            

            // 20220213 add
            for(int k=0;k<asset.nodes[i].children.size();k++){
                std::cout<<"children ininin:"<<std::endl;
                for(int p=0;p<16;p++){
                    asset.nodes[asset.nodes[i].children[k]].matrix[p]=asset.nodes[i].matrix[p] ;
                }
            }


            std::cout<<"\nmatrix test:"<<std::endl;
            for(int k=0;k<16;k+=4){
                std::cout<<asset.nodes[i].matrix[k]<<" "<<asset.nodes[i].matrix[k+1]<<" "<<asset.nodes[i].matrix[k+2]<< " "<<asset.nodes[i].matrix[k+3]<<std::endl;
            }

            // TODO: nodes[i]["weights"]
        }

    }

    static void loadBuffers(Asset& asset, nlohmann::json& json) {
        if (json.find("buffers") == json.end()) {
            return;
        }

        auto& buffers = json["buffers"];
        if (!buffers.is_array()) {
            throw MisformattedExceptionNotArray("buffers");
        }

        asset.buffers.resize(buffers.size());
        for (uint32_t i = 0; i < buffers.size(); ++i) {

            // name
            if (buffers[i].find("name") != buffers[i].end()) {
                if (!buffers[i]["name"].is_string()) {
                    throw MisformattedExceptionNotString("buffers[i][name]");
                }
                std::string n = buffers[i]["name"];
                asset.buffers[i].name = n ;
            }

            // byteLength
            if (buffers[i].find("byteLength") == buffers[i].end()) {
                throw MisformattedExceptionIsRequired("buffers[i][byteLength]");
            } else if (!buffers[i]["byteLength"].is_number()) {
                throw MisformattedExceptionNotNumber("buffers[i][byteLength]");
            }
            asset.buffers[i].byteLength = buffers[i]["byteLength"].get<int32_t>();

            // uri
            if (buffers[i].find("uri") != buffers[i].end()) {
                if (!buffers[i]["uri"].is_string()) {
                    throw MisformattedExceptionNotString("buffers[i][uri]");
                }
                std::string URI =  buffers[i]["uri"]; ;
                asset.buffers[i].uri = URI;
            }

            loadBufferData(asset, asset.buffers[i]);
        }
    }

    /**
     * @brief 测试读取的数据
     *  将读取的数据中的索引indices和顶点坐标position输出为obj模型
     */
    void testPVData(std::vector<float> vV ,std::vector<float> vnV ,std::vector<int> iV){
        
        FILE* file = fopen("./scene.obj", "w");
        if (!file) {
            printf("write_obj: can't write data file \"%s\".\n", "out_culled.obj");
            system("PAUSE");
            exit(0);
        }
        for(int i=0;i<vV.size();i=i+3){
            fprintf(file,"v %lf %lf %lf\n",vV[i],vV[i+1],vV[i+2]); 
            fprintf(file,"vn %lf %lf %lf\n",vnV[i],vnV[i+1],vnV[i+2]); 
        }
        for(int i=0;i<iV.size();i=i+3){
            fprintf(file,"f %d//%d %d//%d %d//%d\n",iV[i]+1,iV[i]+1,iV[i+1]+1,iV[i+1]+1,iV[i+2]+1,iV[i+2]+1); 
        }
        fclose(file);
    }
    

    
    // 从二进制文件读取vertex和indices信息 
    static void loadBufferData(Asset& asset, Buffer& buffer){
        if (!buffer.uri.size() && buffer.byteLength > 0) {
            throw MisformattedException("buffers[i]", "is not empty but has no uri");
        }

        int vertexSize = 0 ;
        // std::vector<float> vV ;
        // std::vector<float> vnV ;
        int indicesSize = 0 ;
        // std::vector<int> iV ;
        int nodesOffset = 0 ;

        // 打开bin文件读取二进制信息
        std::ifstream ifs(buffer.uri , std::ios::binary | std::ios::in);
        // 获取顶点和索引数据的 bytelength
        // for(int i=0;i<asset.bufferViews.size();i++){
        //     if(asset.bufferViews[i].target == 34962)
        //         vertexSize += asset.bufferViews[i].byteLength / 4 ;
        //     else if(asset.bufferViews[i].target == 34963)
        //         indicesSize += asset.bufferViews[i].byteLength /2 ;  // ???? 问题：有些索引bytelength是2 有些是1 
        // }

        // float *position = new float[vertexSize];
        std::cout<<"loadbufferdata here!\n"<<asset.nodes.size() <<std::endl;
        for(int i=0;i<asset.nodes.size();i++){
            std::cout<<"loadbufferdata here 2!"<<std::endl;
            // 为了方便测试  从有效的meshes开始读
            if(asset.nodes[i].mesh != -1){
                std::cout<<"loadbufferdata here 3!"<<std::endl;
                int positionIndexOfAcc = asset.meshes[asset.nodes[i].mesh].primitives[0].attributes["POSITION"] ;
                int normalIndexOfAcc = asset.meshes[asset.nodes[i].mesh].primitives[0].attributes["NORMAL"] ;
                int indiceIndexOfAcc = asset.meshes[asset.nodes[i].mesh].primitives[0].indices ;
                asset.meshesName.push_back(asset.nodes[i].name);   // meshname
                // indicesCount是accessor里的count
                int indicesCount = asset.accessors[indiceIndexOfAcc].count ;
                int bvI = asset.accessors[indiceIndexOfAcc].bufferView ;
                int indicesOffset = asset.accessors[indiceIndexOfAcc].byteOffset + asset.bufferViews[bvI].byteOffset ;  // offset应该是accessor的byteoffset和bufferview的byteoffset之和 ( 应 该 是 吧 )
                // int indicesByteLength = asset.bufferViews[bvI].byteLength ;
                // int typeLength = indicesByteLength / indicesCount ;  // 索引bytelength 是 1或者2 
               


                int c = 0 ; // ifc.gcount : 每次读取的字节数
                if(asset.accessors[indiceIndexOfAcc].componentType == 5121){
                    int indicesByteLength = asset.accessors[indiceIndexOfAcc].count  ;
                    uint8_t *indices = new uint8_t[indicesCount];    
                    std::cout<<indicesOffset<<std::endl;
                    ifs.seekg(indicesOffset ,std::ios::beg) ;   // 读指针移动到offset处 
                    ifs.read((char*)indices,indicesByteLength)  ;  // 读bytelength个字符
                    c = ifs.gcount() ;
                    std::cout<<"read indices size : "<< c <<std::endl;
                    for(int j=0;j<c;j+=3){
                        
                        asset.iV.push_back(indices[j]+nodesOffset);
                        asset.iV.push_back(indices[j+1]+nodesOffset);
                        asset.iV.push_back(indices[j+2]+nodesOffset);
                        // std::cout<< indices[i]<<" "<<indices[i+1]<<" "<<indices[i+2]<<std::endl;
                    }
                    asset.meshesLength.push_back(c/3);    // 每个mesh的三角形的数量
                }             
                else if(asset.accessors[indiceIndexOfAcc].componentType == 5123){
                    int indicesByteLength = asset.accessors[indiceIndexOfAcc].count *2 ;
                    uint16_t *indices = new uint16_t[indicesCount];
                    ifs.seekg(indicesOffset ,std::ios::beg) ;   // 读指针移动到offset处 
                    ifs.read((char*)indices,indicesByteLength)  ;  //  读bytelength个字符
                    c = ifs.gcount() ;
                    std::cout<<"read indices size : "<< c <<std::endl;
                    for(int j=0;j<c/2;j+=3){
                        asset.iV.push_back(indices[j]+nodesOffset);
                        asset.iV.push_back(indices[j+1]+nodesOffset);
                        asset.iV.push_back(indices[j+2]+nodesOffset);
                        //std::cout<< indices[j]<<" "<<indices[j+1]<<" "<<indices[j+2]<<std::endl;
                    }
                    asset.meshesLength.push_back(c/2/3);    // 每个mesh的三角形的数量
                }else if(asset.accessors[indiceIndexOfAcc].componentType == 5125){
                    int indicesByteLength = asset.accessors[indiceIndexOfAcc].count *4 ;
                    uint32_t *indices = new uint32_t[indicesCount];
                    ifs.seekg(indicesOffset ,std::ios::beg) ;   // 读指针移动到offset处 
                    ifs.read((char*)indices,indicesByteLength)  ;  //  读bytelength个字符
                    c = ifs.gcount() ;
                    std::cout<<"read indices size : "<< c <<std::endl;
                    for(int j=0;j<c/4;j+=3){
                        asset.iV.push_back(indices[j]+nodesOffset);
                        asset.iV.push_back(indices[j+1]+nodesOffset);
                        asset.iV.push_back(indices[j+2]+nodesOffset);
                        //std::cout<< indices[j]<<" "<<indices[j+1]<<" "<<indices[j+2]<<std::endl;
                    }
                    asset.meshesLength.push_back(c/4/3);    // 每个mesh的三角形的数量
                }

                
                

                // position 的信息  v
                int positionsCount = asset.accessors[positionIndexOfAcc].count;
                int bvP = asset.accessors[positionIndexOfAcc].bufferView ;
                int positionsOffset = asset.accessors[positionIndexOfAcc].byteOffset + asset.bufferViews[bvP].byteOffset ;
                
                float *positions = new float[positionsCount*3] ;   // 一个position三个坐标，三个float 
                ifs.seekg(positionsOffset ,std::ios::beg) ;
                ifs.read((char*)positions,positionsCount*3*4) ; // 三个坐标，每个坐标长度4字节 ，
                //std::cout<<"read positions size : "<< ifs.gcount() <<std::endl;
                int d =  ifs.gcount() ;
                nodesOffset = nodesOffset + d/12 ;
                std::cout<<"read positions size : "<< d <<std::endl;
                for(int j=0;j<d/4;j+=3){
                    float f0=positions[j],f1=positions[j+1],f2=positions[j+2];
                    // positions[j]= asset.nodes[i].matrix[0]*f0 +asset.nodes[i].matrix[1]*f1+asset.nodes[i].matrix[2]*f2+asset.nodes[i].matrix[3];
                    // positions[j+1]= asset.nodes[i].matrix[4]*f0+asset.nodes[i].matrix[5]*f1+asset.nodes[i].matrix[6]*f2+asset.nodes[i].matrix[7];
                    // positions[j+2]= asset.nodes[i].matrix[8]*f0+asset.nodes[i].matrix[9]*f1+asset.nodes[i].matrix[10]*f2+asset.nodes[i].matrix[11];
                    positions[j]= asset.nodes[i].matrix[0]*f0 +asset.nodes[i].matrix[4]*f1+asset.nodes[i].matrix[8]*f2+asset.nodes[i].matrix[12];
                    positions[j+1]= asset.nodes[i].matrix[1]*f0+asset.nodes[i].matrix[5]*f1+asset.nodes[i].matrix[9]*f2+asset.nodes[i].matrix[13];
                    positions[j+2]= asset.nodes[i].matrix[2]*f0+asset.nodes[i].matrix[6]*f1+asset.nodes[i].matrix[10]*f2+asset.nodes[i].matrix[14];
                    asset.vV.push_back(positions[j]);
                    asset.vV.push_back(positions[j+1]);
                    asset.vV.push_back(positions[j+2]);
                    //std::cout<< positions[j]<<"  "<<positions[j+1]<<"  "<<positions[j+2]<<std::endl;
                }
                

                //  vn NORMAL
                int normalCount = asset.accessors[normalIndexOfAcc].count;
                int bvP_vn = asset.accessors[normalIndexOfAcc].bufferView ;
                int normalOffset = asset.accessors[normalIndexOfAcc].byteOffset + asset.bufferViews[bvP_vn].byteOffset ;
                float* normals = new float[normalCount*3];
                ifs.seekg(normalOffset ,std::ios::beg) ;
                ifs.read((char*)normals,normalCount*3*4) ; // 三个坐标，每个坐标长度4字节 ，
                int g = ifs.gcount();
                std::cout<<"read normals size : "<< d <<std::endl;
                for(int j=0;j<g/4;j+=3){
                    asset.vnV.push_back(normals[j]);
                    asset.vnV.push_back(normals[j+1]);
                    asset.vnV.push_back(normals[j+2]);
                    //std::cout<< positions[j]<<"  "<<positions[j+1]<<"  "<<positions[j+2]<<std::endl;
                }
               //  break ;   // 测试用 
            }


        }
        testPVData(asset.vV, asset.vnV, asset.iV);

        ifs.close();


    }

    

    // bufferview: 描述buffers中的数据（从哪到哪是顶点or索引）
    // 根据“bytelength”、“byteoffset”、“bytestride”将buffer中的二进制数据分为顶点、索引等等
    static void loadBufferViews(Asset& asset, nlohmann::json& json) {
        if (json.find("bufferViews") == json.end()) {
            return;
        }

        auto& bufferViews = json["bufferViews"];
        if (!bufferViews.is_array()) {
            throw MisformattedExceptionNotArray("bufferViews");
        }

        asset.bufferViews.resize(bufferViews.size()) ;

        for (uint32_t i = 0; i < bufferViews.size(); ++i) {
            // name
            if (bufferViews[i].find("name") != bufferViews[i].end()) {
                if (!bufferViews[i]["name"].is_string()) {
                    throw MisformattedExceptionNotString("bufferViews[i][name]");
                }

                std::string n = bufferViews[i]["name"];
                asset.bufferViews[i].name = n;
            }

            // buffer
            if (bufferViews[i].find("buffer") == bufferViews[i].end()) {
                throw MisformattedExceptionIsRequired("bufferViews[i][buffer]");
            }
            if (!bufferViews[i]["buffer"].is_number()) {
                throw MisformattedExceptionNotNumber("bufferViews[i][buffer]");
            }
            asset.bufferViews[i].buffer = bufferViews[i]["buffer"].get<int32_t>();

            // byteOffset (改)
            if (bufferViews[i].find("byteOffset") != bufferViews[i].end()) {
                ///throw MisformattedExceptionIsRequired("bufferViews[i][byteOffset]");
                if (!bufferViews[i]["byteOffset"].is_number()) {
                    throw MisformattedExceptionNotNumber("bufferViews[i][byteOffset]");
                }
                asset.bufferViews[i].byteOffset = bufferViews[i]["byteOffset"].get<int32_t>();
            }
            
            // byteLength
            if (bufferViews[i].find("byteLength") == bufferViews[i].end()) {
                throw MisformattedExceptionIsRequired("bufferViews[i][byteLength]");
            }
            if (!bufferViews[i]["byteLength"].is_number()) {
                throw MisformattedExceptionNotNumber("bufferViews[i][byteLength]");
            }
            asset.bufferViews[i].byteLength = bufferViews[i]["byteLength"].get<int32_t>();

            // byteStride
            if (bufferViews[i].find("byteStride") != bufferViews[i].end()) {
                if (!bufferViews[i]["byteStride"].is_number()) {
                    throw MisformattedExceptionNotNumber("bufferViews[i][byteStride]");
                }

                asset.bufferViews[i].byteStride = bufferViews[i]["byteStride"].get<int32_t>();
            }

            // target
            if (bufferViews[i].find("target") != bufferViews[i].end()) {
                if (!bufferViews[i]["target"].is_number()) {
                    throw MisformattedExceptionNotNumber("bufferViews[i][target]");
                }
                asset.bufferViews[i].target = bufferViews[i]["target"].get<uint16_t>();
            }

            // TODO: bufferViews[i]["extensions"]
            // TODO: bufferViews[i]["extras"]
        }
    }



    static void loadAccessors(Asset& asset, nlohmann::json& json) {
        if (json.find("accessors") == json.end()) {
            return;
        }

        auto& accessors = json["accessors"];

        if (!accessors.is_array()) {
            throw MisformattedExceptionNotArray("accessors");
        }
        std::cout<<"in 1"<<std::endl;
        asset.accessors.resize(accessors.size());
        for (uint32_t i = 0; i < accessors.size(); ++i) {
            // bufferView
            if (accessors[i].find("bufferView") != accessors[i].end()) {
                if (!accessors[i]["bufferView"].is_number()) {
                    throw MisformattedExceptionNotNumber("accessors[i][bufferView]");
                }

                asset.accessors[i].bufferView = accessors[i]["bufferView"].get<int32_t>();
            }

            // bufferView
            if (accessors[i].find("bufferView") != accessors[i].end()) {
                if (!accessors[i]["bufferView"].is_number()) {
                    throw MisformattedExceptionNotNumber("accessors[i][bufferView]");
                }

                asset.accessors[i].bufferView = accessors[i]["bufferView"].get<int32_t>();
            }

            // byteOffset
            if (accessors[i].find("byteOffset") != accessors[i].end()) {
                if (!accessors[i]["byteOffset"].is_number()) {
                    throw MisformattedExceptionNotNumber("accessors[i][byteOffset]");
                }

                asset.accessors[i].byteOffset = accessors[i]["byteOffset"].get<uint32_t>();
            }

            // componentType
            if (accessors[i].find("componentType") == accessors[i].end()) {
                throw MisformattedExceptionIsRequired("accessors[i][componentType]");
            } else if (!accessors[i]["componentType"].is_number()) {
                throw MisformattedExceptionNotNumber("accessors[i][componentType]");
            }

            asset.accessors[i].componentType = accessors[i]["componentType"].get<uint16_t>();

            // normalized
            if (accessors[i].find("normalized") != accessors[i].end()) {
                if (!accessors[i]["normalized"].is_boolean()) {
                    throw MisformattedExceptionNotBoolean("accessors[i][normalized]");
                }

                asset.accessors[i].normalized = accessors[i]["normalized"].get<bool>() ;
            }

            // count
            if (accessors[i].find("count") == accessors[i].end()) {
                throw MisformattedExceptionIsRequired("accessors[i][count]");
            } else if (!accessors[i]["count"].is_number()) {
                throw MisformattedExceptionNotNumber("accessors[i][count]");
            }

            asset.accessors[i].count = accessors[i]["count"].get<uint32_t>();

            // type
            if (accessors[i].find("type") == accessors[i].end()) {
                throw MisformattedExceptionIsRequired("accessors[i][type]");
            } else if (!accessors[i]["type"].is_string()) {
                throw MisformattedExceptionNotString("accessors[i][type]");
            }

            std::string type = accessors[i]["type"].get<std::string>();
            if (type == "SCALAR") {
                asset.accessors[i].type = Accessor::Type::Scalar;
            } else if (type == "VEC2") {
                asset.accessors[i].type = Accessor::Type::Vec2;
            } else if (type == "VEC3") {
                asset.accessors[i].type = Accessor::Type::Vec3;
            } else if (type == "VEC4") {
                asset.accessors[i].type = Accessor::Type::Vec4;
            } else if (type == "MAT2") {
                asset.accessors[i].type = Accessor::Type::Mat2;
            } else if (type == "MAT3") {
                asset.accessors[i].type = Accessor::Type::Mat3;
            } else if (type == "MAT4") {
                asset.accessors[i].type = Accessor::Type::Mat4;
            } else {
                throw MisformattedException("accessors[i][type]", "is not a valid type");
            }

            std::cout<<"in 2"<<std::endl;
            // TODO: accessors[i]["sparse"]
            // TODO: accessors[i]["extensions"]
            // TODO: accessors[i]["min"]
            // TODO: accessors[i]["max"]
        }
    }



    static std::string getDirectoryName(const std::string& path) {
        std::size_t found;

        found = path.find_last_of("/");
        if (found == std::string::npos) {
            return "";
        }

        return path.substr(0, found);
    }



    Asset load(std::string filename){

        nlohmann::json json;

        {
            std::ifstream file(filename);
            if(!file.is_open()){
                throw std::runtime_error("can't load file!");
            }
            file >> json;
        }

        Asset asset{} ;
        asset.dirName = getDirectoryName(filename);

        loadAsset(asset, json);
        loadScenes(asset,json);
        loadMeshes(asset,json);
        loadNodes(asset,json);

        // 应该是先读bufferview和accessor 
        loadBufferViews(asset,json);
        loadAccessors(asset,json);
        loadBuffers(asset,json);
        
        

        return asset;

    }



}
