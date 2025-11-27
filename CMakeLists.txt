cmake_minimum_required(VERSION 3.28.3)
project(idealGasSim VERSION 0.1.0)

add_executable(idealGasSim main.cpp)

# LIBRARY

add_library(gasSimLib SHARED)
target_sources(gasSimLib
	PRIVATE
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

	PUBLIC
		FILE_SET HEADERS
		BASE_DIRS
			gasSim
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

target_link_libraries(idealGasSim
	PRIVATE
		gasSimLib
)

# Testing
add_executable(gasSimTests)
target_sources(gasSimTests
	PRIVATE
		unitTesting/unitTests.cpp
		unitTesting/testingAddons.cpp
	PUBLIC
		FILE_SET HEADERS
		BASE_DIRS unitTesting
		FILES unitTesting/testingAddons.hpp
)

target_link_libraries(gasSimTests
	PRIVATE
		gasSimLib
)

# COMPILER OPTIONS

target_compile_options(gasSimLib PRIVATE -Wall -Wextra)
target_compile_options(idealGasSim PRIVATE -Wall -Wextra)
target_compile_options(gasSimTests PRIVATE -Wall -Wextra)

target_compile_options(idealGasSim PRIVATE
    $<$<CONFIG:Debug>:-D_GLIBCXX_ASSERTIONS -D_LIBCPP_ENABLE_ASSERTIONS=1 -fsanitize=address,undefined -fno-omit-frame-pointer>
)
target_compile_options(gasSimTests PRIVATE
    $<$<CONFIG:Debug>:-D_GLIBCXX_ASSERTIONS -D_LIBCPP_ENABLE_ASSERTIONS=1 -fsanitize=address,undefined -fno-omit-frame-pointer>
)

target_link_options(idealGasSim PRIVATE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined -fno-omit-frame-pointer>
)
target_link_options(gasSimTests PRIVATE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined -fno-omit-frame-pointer>
)

target_link_libraries(idealGasSim PRIVATE
    sfml-graphics
    sfml-window
    sfml-system
    pthread
    Core Imt RIO Net Hist Graf Graf3d Gpad ROOTVecOps Tree TreePlayer Rint
    Postscript Matrix Physics MathCore Thread ROOTNTuple MultiProc
    ROOTDataFrame ROOTNTupleUtil
    tbb
)
target_link_libraries(gasSimTests PRIVATE
    sfml-graphics
    sfml-window
    sfml-system
    pthread
    Core Imt RIO Net Hist Graf Graf3d Gpad ROOTVecOps Tree TreePlayer Rint
    Postscript Matrix Physics MathCore Thread ROOTNTuple MultiProc
    ROOTDataFrame ROOTNTupleUtil
    tbb
)

target_include_directories(idealGasSim PRIVATE
    ./root/include
)
target_include_directories(gasSimTests PRIVATE
	./root/include
)

target_link_directories(idealGasSim PRIVATE
    ./root/lib
)
target_link_directories(gasSimTests PRIVATE
	./root/lib
)
