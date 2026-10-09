cmake_minimum_required(VERSION 3.24)
project(SubdueWindows VERSION 0.12.0 LANGUAGES CXX RC)

add_executable(Subdue WIN32
    src/main.cpp
    resources/Subdue.rc
    resources/Subdue.manifest
)

target_compile_features(Subdue PRIVATE cxx_std_20)
target_compile_definitions(Subdue PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(Subdue PRIVATE resources)
target_link_libraries(Subdue PRIVATE comctl32 shell32 user32 gdi32 advapi32 ole32 uuid)

if(MSVC)
    target_compile_options(Subdue PRIVATE /W4 /permissive- /EHsc)
    target_link_options(Subdue PRIVATE /MANIFEST:NO)
endif()

set_target_properties(Subdue PROPERTIES
    OUTPUT_NAME Subdue
    VS_GLOBAL_ApplicationIcon "${CMAKE_CURRENT_SOURCE_DIR}/resources/Subdue.ico"
)
