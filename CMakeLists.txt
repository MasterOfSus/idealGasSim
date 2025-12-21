
# Hi, for the sake of my sanity this entire file was written by chatGPT

cmake_minimum_required(VERSION 3.28)
project(idealGasSim VERSION 0.1.0 LANGUAGES CXX)

# Set global C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build types and optimization / sanitizers
# Default to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Library target
add_library(gasSimLib SHARED
    # Source files
    gasSim/PhysicsEngine/Particle.cpp 
    gasSim/PhysicsEngine/Collision.cpp 
    gasSim/PhysicsEngine/Gas.cpp
    gasSim/Graphics/RenderStyle.cpp 
    gasSim/Graphics/Camera.cpp
    gasSim/DataProcessing/GasData.cpp 
    gasSim/DataProcessing/TdStats.cpp 
    gasSim/DataProcessing/SimDataPipeline.cpp
    gasSim/DataProcessing/SDPgetVideo.cpp
    gasSim/Input/Input.cpp
)

# Header files
target_sources(gasSimLib PUBLIC
    FILE_SET HEADERS
    BASE_DIRS gasSim
    FILES
        gasSim/PhysicsEngine/GSVector.hpp 
        gasSim/PhysicsEngine/Particle.hpp 
        gasSim/PhysicsEngine/Collision.hpp 
        gasSim/PhysicsEngine/Gas.hpp
        gasSim/Graphics/RenderStyle.hpp 
        gasSim/Graphics/Camera.hpp
        gasSim/DataProcessing/GasData.hpp 
        gasSim/DataProcessing/TdStats.hpp 
        gasSim/DataProcessing/SimDataPipeline.hpp
        gasSim/Input/Input.hpp
)

# Compile options for library
target_compile_options(gasSimLib PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
		-Wimplicit-fallthrough -Wextra-semi -Wold-style-cast
		-fno-omit-frame-pointer
		$<$<CONFIG:Debug>:-g -D_GLIBCXX_ASSERTIONS -D_LIBCPP_ENABLE_ASSERTIONS=1 -fsanitize=thread,undefined>
    $<$<CONFIG:Release>:-O3 -march=native -DNDEBUG>
)

# Main executable
add_executable(idealGasSim main.cpp)
target_link_libraries(idealGasSim PRIVATE gasSimLib
    sfml-graphics sfml-window sfml-system pthread
    Core Imt RIO Net Hist Graf Graf3d Gpad ROOTVecOps Tree TreePlayer Rint
    Postscript Matrix Physics MathCore Thread ROOTNTuple MultiProc
    ROOTDataFrame ROOTNTupleUtil
    tbb
)

target_compile_options(idealGasSim PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
		-Wimplicit-fallthrough -Wextra-semi -Wold-style-cast
		-fno-omit-frame-pointer
		$<$<CONFIG:Debug>:-g -D_GLIBCXX_ASSERTIONS -D_LIBCPP_ENABLE_ASSERTIONS=1 -fsanitize=thread,undefined>
    $<$<CONFIG:Release>:-O3 -march=native -DNDEBUG>
)

target_link_options(idealGasSim PRIVATE
    $<$<CONFIG:Debug>:-fsanitize=thread,undefined -fno-omit-frame-pointer>
)

target_include_directories(idealGasSim PRIVATE ./root/include)
target_link_directories(idealGasSim PRIVATE ./root/lib)

# Test executable
add_executable(gasSimTests
    unitTesting/unitTests.cpp
    unitTesting/testingAddons.cpp
)

target_sources(gasSimTests PUBLIC
    FILE_SET HEADERS
    BASE_DIRS unitTesting
    FILES unitTesting/testingAddons.hpp
)

target_link_libraries(gasSimTests PRIVATE gasSimLib
    sfml-graphics sfml-window sfml-system pthread
    Core Imt RIO Net Hist Graf Graf3d Gpad ROOTVecOps Tree TreePlayer Rint
    Postscript Matrix Physics MathCore Thread ROOTNTuple MultiProc
    ROOTDataFrame ROOTNTupleUtil
    tbb
)

target_compile_options(gasSimTests PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
		-Wimplicit-fallthrough -Wextra-semi -Wold-style-cast
		-fno-omit-frame-pointer
		$<$<CONFIG:Debug>:-g -D_GLIBCXX_ASSERTIONS -D_LIBCPP_ENABLE_ASSERTIONS=1 -fsanitize=thread,undefined>
    $<$<CONFIG:Release>:-O3 -march=native -DNDEBUG>
)

target_link_options(gasSimTests PRIVATE
    $<$<CONFIG:Debug>:-fsanitize=thread,undefined -fno-omit-frame-pointer>
)

target_include_directories(gasSimTests PRIVATE ./root/include)
target_link_directories(gasSimTests PRIVATE ./root/lib)

