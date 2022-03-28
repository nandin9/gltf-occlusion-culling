# cmake_minimum_required (VERSION 3.18)
project (culling)

include_directories("jsoncpp")

add_library( # Sets the name of the library.
        myJNI

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        # native-lib.cpp
        jsoncpp/json_tool.h
        jsoncpp/json_reader.cpp
        jsoncpp/json_valueiterator.inl
        jsoncpp/json_value.cpp
        jsoncpp/json_writer.cpp
        jsoncpp/assertions.h
        jsoncpp/allocator.h
        jsoncpp/config.h
        jsoncpp/json_features.h
        jsoncpp/forwards.h
        jsoncpp/json.h
        jsoncpp/reader.h
        jsoncpp/value.h
        jsoncpp/version.h
        jsoncpp/writer.h
)

add_executable (demo ./src/main.cpp ./src/gltfLoader/gltf.cpp)
include_directories("src/include")
include_directories("extern")
add_subdirectory("src/include")
target_link_libraries(demo wheels)
target_link_libraries(demo myJNI)