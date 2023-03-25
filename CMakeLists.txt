cmake_minimum_required(VERSION 3.24.2)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(eshywm VERSION 1.0)

option(BUILD_SHARED_LIBS "Build using shared libs" ON)

find_package(X11 REQUIRED)

set(BIN_NAME eshywm)
set(SOURCE_FILES main.cpp util.cpp config.cpp eshywm.cpp window.cpp window_manager.cpp menu_base.cpp context_menu.cpp taskbar.cpp switcher.cpp run_menu.cpp button.cpp)
list(TRANSFORM SOURCE_FILES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/source/)

add_executable(${BIN_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/source/main.cpp ${SOURCE_FILES})
target_include_directories(${BIN_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/source/includes)
target_link_libraries(${BIN_NAME} PUBLIC X11 ${CMAKE_CURRENT_SOURCE_DIR}/third_party/Imlib2/libImlib2.so /usr/lib/libXrandr.so)