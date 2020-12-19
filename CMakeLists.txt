cmake_minimum_required(VERSION 3.5)

project(Rebuilder LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(wxWidgets REQUIRED COMPONENTS core base xml propgrid adv)

add_executable(Rebuilder WIN32
  pkg/resources.rc
  src/app.cpp
  src/app.h
  src/mainwindow.cpp
  src/mainwindow.h
  src/patcher.cpp
  src/patcher.h
  src/sha1.hpp
  src/wxutils.h
)

target_link_libraries(Rebuilder PRIVATE ${wxWidgets_LIBRARIES} ddraw.lib dxguid.lib)

target_include_directories(Rebuilder PRIVATE ${wxWidgets_INCLUDE_DIRS})
