cmake_minimum_required(VERSION 3.28)
project(idealGasSim VERSION 0.1.0 LANGUAGES CXX)

# enable tests support
include(CTest)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# set global CXX standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# warnings
string(APPEND CMAKE_CXX_FLAGS
      " -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion"
      " -Wshadow -Wimplicit-fallthrough -Wextra-semi -Wold-style-cast"
      " -fno-omit-frame-pointer")

# enable std lib exceptions
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  string(APPEND CMAKE_CXX_FLAGS " -D_GLIBCXX_ASSERTIONS")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  string(APPEND CMAKE_CXX_FLAGS " -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE")
endif()

# enable address and ub sanitizers in debug mode
string(APPEND CMAKE_CXX_FLAGS_DEBUG " -fsanitize=address,undefined")
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  string(APPEND CMAKE_CXX_FLAGS " -D_GLIBCXX_SANITIZE_STD_ALLOCATOR")
endif()
string(APPEND CMAKE_EXE_LINKER_FLAGS_DEBUG " -fsanitize=address,undefined")

# check for required libs 
find_package(SFML 2.6 COMPONENTS graphics REQUIRED)
find_package(ROOT CONFIG REQUIRED)

# gasSimLib library
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
target_include_directories(
	gasSimLib SYSTEM PUBLIC ${ROOT_INCLUDE_DIRS} gasSim/Libs
)
target_link_libraries(
	gasSimLib PUBLIC ${ROOT_LIBRARIES}
)

# idealGasSim main executable
add_executable(idealGasSim main.cpp)
target_include_directories(
	idealGasSim SYSTEM PRIVATE gasSim/Libs
)
target_link_libraries(idealGasSim PRIVATE gasSimLib
    sfml-graphics sfml-window sfml-system
		tbb
)

# TESTING SECTION
# to disable pass -DBUILD_TESTING=OFF to the configuration command
if (BUILD_TESTING)

	# Test executable (actual unit tests)
	add_executable(gasSimTests.t
			unitTesting/unitTests.cpp
			unitTesting/testingAddons.cpp
	)

	target_sources(gasSimTests.t PUBLIC
			FILE_SET HEADERS
			BASE_DIRS unitTesting
			FILES unitTesting/testingAddons.hpp
	)

	target_link_libraries(gasSimTests.t PRIVATE gasSimLib
		sfml-graphics sfml-window sfml-system ${ROOT_LIBRARIES} 
		tbb
	)

	add_test(NAME gasSimTests 
		COMMAND gasSimTests.t
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/unitTesting
	)

	# Extra interactive getVideo demo/stress test executable (not proper unit tests)
	add_executable(getVideoTest.t
			unitTesting/getVideoTest.cpp
			unitTesting/testingAddons.cpp
	)

	target_sources(getVideoTest.t PUBLIC
			FILE_SET HEADERS
			BASE_DIRS unitTesting
			FILES unitTesting/testingAddons.hpp
	)

	target_link_libraries(getVideoTest.t PRIVATE gasSimLib
		sfml-graphics sfml-window sfml-system ${ROOT_LIBRARIES} 
		tbb
	)

	set_target_properties(getVideoTest.t 
		PROPERTIES 
		RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/unitTesting
	)

endif()
