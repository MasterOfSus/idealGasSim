cmake_minimum_required(VERSION 3.16)
project(idealGasSim VERSION 0.1.0)

# abilita il supporto per i test, tra cui l'opzione BUILD_TESTING usata sotto
include(CTest)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# richiedi l'uso di C++17, senza estensioni non-standard offerte dal compilatore usato 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# abilita warning
string(APPEND CMAKE_CXX_FLAGS " -Wall -Wextra ")
# sfml
string(APPEND CMAKE_CXX_FLAGS " -lsfml-graphics -lsfml-window -lsfml-system ")

# abilita:
# - l'address sanitizer e l'undefined-behaviour sanitizer
# - _LIBCPP_ENABLE_ASSERTIONS=1 (utilizzato da clang)
# - _GLIBCXX_ASSERTIONS (utilizzato da gcc)
#in debug mode
string(APPEND CMAKE_CXX_FLAGS_DEBUG " -D_GLIBCXX_ASSERTIONS -D_LIBCPP_ENABLE_ASSERTIONS=1 -fsanitize=address,undefined -fno-omit-frame-pointer")
string(APPEND CMAKE_EXE_LINKER_FLAGS_DEBUG " -fsanitize=address,undefined -fno-omit-frame-pointer ")

add_executable(demo gasSim/physicsEngine.cpp gasSim/graphics.cpp gasSim/statistics.cpp gasSim/output.cpp gasSim/input.cpp main.cpp)

# Find SFML
find_package(SFML 2.6 COMPONENTS graphics window system REQUIRED)

# Link SFML
target_link_libraries(demo PRIVATE sfml-graphics sfml-window sfml-system)

# se il testing e' abilitato...
#   per disabilitare il testing, passare -DBUILD_TESTING=OFF a cmake durante la fase di configurazione
if (BUILD_TESTING)

  # aggiungi l'eseguibile sample.t
	add_executable(unitTests gasSim/physicsEngine.cpp gasSim/graphics.cpp gasSim/statistics.cpp gasSim/output.cpp gasSim/input.cpp unitTesting/unitTests.cpp)
  # aggiungi l'eseguibile sample.t alla lista dei test
  add_test(NAME unitTests COMMAND unitTests)
	target_link_libraries(unitTests PRIVATE sfml-graphics sfml-window sfml-system)

endif()

