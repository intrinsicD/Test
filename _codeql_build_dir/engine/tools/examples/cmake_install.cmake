# Install script for directory: /home/runner/work/Test/Test/engine/tools/examples

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/examples/geometry_viewer" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/examples/geometry_viewer")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/examples/geometry_viewer"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/examples" TYPE EXECUTABLE FILES "/home/runner/work/Test/Test/_codeql_build_dir/engine/tools/examples/geometry_viewer")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/examples/geometry_viewer" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/examples/geometry_viewer")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/examples/geometry_viewer"
         OLD_RPATH "/home/runner/work/Test/Test/_codeql_build_dir/engine/rendering:/home/runner/work/Test/Test/_codeql_build_dir/engine/scene:/home/runner/work/Test/Test/_codeql_build_dir/engine/assets:/home/runner/work/Test/Test/_codeql_build_dir/engine/geometry:/home/runner/work/Test/Test/_codeql_build_dir/engine/core:/home/runner/work/Test/Test/_codeql_build_dir/engine/animation:/home/runner/work/Test/Test/_codeql_build_dir/engine/platform:/home/runner/work/Test/Test/_codeql_build_dir/_deps/glfw_external-build/src:/home/runner/work/Test/Test/_codeql_build_dir/third_party:"
         NEW_RPATH "")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/examples/geometry_viewer")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/examples/prototype_viewer" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/examples/prototype_viewer")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/examples/prototype_viewer"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/examples" TYPE EXECUTABLE FILES "/home/runner/work/Test/Test/_codeql_build_dir/engine/tools/examples/prototype_viewer")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/examples/prototype_viewer" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/examples/prototype_viewer")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/examples/prototype_viewer"
         OLD_RPATH "/home/runner/work/Test/Test/_codeql_build_dir/engine/tools:/home/runner/work/Test/Test/_codeql_build_dir/engine/runtime:/home/runner/work/Test/Test/_codeql_build_dir/engine/rendering:/home/runner/work/Test/Test/_codeql_build_dir/engine/scene:/home/runner/work/Test/Test/_codeql_build_dir/engine/assets:/home/runner/work/Test/Test/_codeql_build_dir/engine/platform:/home/runner/work/Test/Test/_codeql_build_dir/engine/compute:/home/runner/work/Test/Test/_codeql_build_dir/engine/physics:/home/runner/work/Test/Test/_codeql_build_dir/engine/geometry:/home/runner/work/Test/Test/_codeql_build_dir/engine/core:/home/runner/work/Test/Test/_codeql_build_dir/_deps/glfw_external-build/src:/home/runner/work/Test/Test/_codeql_build_dir/third_party:/home/runner/work/Test/Test/_codeql_build_dir/engine/animation:"
         NEW_RPATH "")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/examples/prototype_viewer")
    endif()
  endif()
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/home/runner/work/Test/Test/_codeql_build_dir/engine/tools/examples/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
