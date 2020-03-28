cmake_minimum_required(VERSION 3.15)
project(Enet_Server)


set(CMAKE_CXX_STANDARD 14)

add_subdirectory(enet)
include_directories(enet/include)
link_directories("enet/")

add_executable(Enet_Server main.cpp)
target_link_libraries(Enet_Server enet.lib ws2_32.lib winmm.lib)