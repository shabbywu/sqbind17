# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置 并包含子项目。
#
cmake_minimum_required(VERSION 3.13)

project("sqbind17")
set(CMAKE_CXX_STANDARD 17)

# 设置变量
if(SQUIRREL305)
  set(squirrel_dir ${CMAKE_CURRENT_LIST_DIR}/externals/libs/SQUIRREL305)
  set(SQUIRRELLIB SQUIRREL305)
else()
  set(squirrel_dir ${CMAKE_CURRENT_LIST_DIR}/externals/libs/SQUIRREL32)
  set(SQUIRRELLIB SQUIRREL32)
endif()

if(NOT TARGET sqbind17_header)
  add_library(sqbind17_header INTERFACE)
  target_include_directories(
    sqbind17_header INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include
    ${squirrel_dir}/include ${squirrel_dir}/squirrel)
  add_library(sqbind17::header ALIAS sqbind17_header)
endif()

option(BUILD_SQUIRREL "Build squirrel?" OFF)
option(BUILD_CPP_TESTCASE "Build cpp test?" OFF)

if(NOT TARGET sqbind17)
  add_library(sqbind17 INTERFACE)
  target_link_libraries(sqbind17 INTERFACE sqbind17_header)
  add_library(sqbind17::sqbind17 ALIAS sqbind17)
endif()

if(${BUILD_SQUIRREL} OR ${BUILD_CPP_TESTCASE})
  set(DISABLE_DYNAMIC TRUE)
  set(SQ_DISABLE_INTERPRETER TRUE)
  add_subdirectory(${squirrel_dir} squirrel)
  target_compile_options(squirrel_static PRIVATE -fPIC /MT)
  target_compile_options(sqstdlib_static PRIVATE -fPIC /MT)

  target_link_libraries(sqbind17 INTERFACE squirrel_static sqstdlib_static)
endif()

if(${BUILD_CPP_TESTCASE})
  add_subdirectory(tests_cpp)
endif()
