#pragma once
#include <iostream>
#include <fstream>
#include "gltfLoader/gltf.h"
#include "json/json.h"

using namespace std;

namespace JSON{

    void exportGLTF(gltf::Asset asset){ 
        Json::Value root ;

        // asset
        Json::Value ass ;
        ass["generator"] = asset.metadata.generator;
        ass["version"] = asset.metadata.version ; 
        root["asset"] = Json::Value(ass);

        // buffers
        Json::Value buffers(Json::arrayValue);
        Json::Value child;
        child["byteLength"] = asset.newBufferLength;
        child["uri"] = "sceneTest.bin";
        buffers[0]=child;
        root["buffers"] = Json::Value(buffers);

        // bufferviews
        Json::Value bufferviews(Json::arrayValue);
        for(int i=0;i<asset.newBufferViews.size();i++){
            Json::Value child;
            child["buffer"] = 0 ;
            child["byteOffset"] = asset.newBufferViews[i].byteOffset ;
            child["byteLength"] = asset.newBufferViews[i].byteLength ;
            if(i == 0)
                child["byteStride"] = asset.newBufferViews[i].byteStride ;
            child["target"] = asset.newBufferViews[i].target ;
            bufferviews[i] = child ;
        }
        root["bufferViews"] = Json::Value(bufferviews);
        cout<<"\nin here" ;
        // accessors
        Json::Value accessors(Json::arrayValue) ;
        for(int i=0;i<asset.newAccessors.size();i++){
            Json::Value child;
            child["bufferView"] = asset.newAccessors[i].bufferView ;
            child["byteOffset"] = asset.newAccessors[i].byteOffset ;
            child["componentType"] = asset.newAccessors[i].componentType ;
            child["count"] = asset.newAccessors[i].count ;
            if(i%2 == 0)
                child["type"] = "SCALAR" ;
            else
                child["type"] = "VEC3" ;
            accessors[i] = child ;
        }
        root["accessors"] = Json::Value(accessors);

        cout<<"\nin here" ;
        // scene  (注意scene与scenes的区别)
        root["scene"] = 0 ;

        // scenes
        Json::Value scenes(Json::arrayValue);
        Json::Value child2;
        child2["nodes"].append(0);
        scenes[0] = child2;
        root["scenes"] = Json::Value(scenes);
        
        // nodes
        Json::Value nodes(Json::arrayValue);
        Json::Value child3;
        for(int i=0;i<asset.newmeshesLength.size();i++)
            child3["children"].append(i+1);
        nodes[0] = child3 ;

        for(int i=0;i<asset.newmeshesLength.size();i++){
            Json::Value child;
            child["mesh"] = i ;
            child["name"] = asset.meshesName[i] ;
            nodes[i+1] = child ;
        }

        root["nodes"] = Json::Value(nodes) ;
        cout<<"\nin here" ;
        // meshes
        Json::Value meshes(Json::arrayValue);
        for(int i=0;i<asset.newmeshesLength.size();i++){
            cout<<"\nin here "<<i ;
            Json::Value child;
            Json::Value child2 ;
            Json::Value child3;
            child3["POSITION"] = asset.newMeshes[i].primitives[0].attributes["POSITION"] ;
            child2["indices"] = asset.newMeshes[i].primitives[0].indices ;
            child2["attributes"] = Json::Value(child3) ;

            child["primitives"].append(child2) ;
            child["name"] = asset.meshesName[i] ;

            meshes[i] = child ;
             cout<<"\nin here   "<< i ;
        }
        cout<<"\nin here " ;
        root["meshes"] = Json::Value(meshes) ;

        // cameras
        // materials
        // textures
        // images
        // samplers


        Json::StyledWriter sw;
	    // cout << sw.write(root) << endl;

        // 输出到.gltf文件
        ofstream os;
    	os.open("sceneTest.gltf", std::ios::out );
        os << sw.write(root);
	    os.close();
    }
}