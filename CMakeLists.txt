cmake_minimum_required(VERSION 3.2)
project(AlphaServer)
add_executable(AlphaServer AlphaServer.cpp Chunk.cpp ChunkManager.cpp Client.cpp PlayerManager.cpp)

find_package(
        Boost
        REQUIRED
        COMPONENTS iostreams
)

target_link_libraries(AlphaServer boost_iostreams)