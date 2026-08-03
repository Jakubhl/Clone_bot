cmake_minimum_required(VERSION 3.14)
project(cloneBot VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/src/ui)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
# Qt
set(CMAKE_PREFIX_PATH $ENV{QTDIR})
find_package(Qt5 REQUIRED COMPONENTS Widgets)

# Sources
file(GLOB_RECURSE SRC_FILES
    src/*.cpp
    src/ui/*.ui
    include/*.h
)

add_executable(cloneBot ${SRC_FILES})

target_include_directories(cloneBot PRIVATE include)

target_link_libraries(cloneBot 
    Qt5::Widgets
)

# Windows: GUI app (bez konzole)
if (WIN32)
    set_target_properties(cloneBot PROPERTIES WIN32_EXECUTABLE TRUE)
endif()