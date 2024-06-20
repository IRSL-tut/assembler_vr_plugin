cmake_minimum_required(VERSION 3.10)

set(target CnoidAssemblerVRPlugin)

### only for linux
set(sources
  src/AssemblerVRPlugin.cpp
  src/AssemblerVRProcess.cpp
  )
set(headers
  src/AssemblerVRPlugin.h
  src/AssemblerVRProcess.h
  )
if (NOT WIN32)
  add_definitions(-Wno-deprecated-declarations)
endif()

#add_definitions(-DIRSL_DEBUG)

if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
  # Build as a master project
  add_definitions(-DEXT_BUNDLE)
  set(CMAKE_CXX_STANDARD ${CHOREONOID_CXX_STANDARD})
  choreonoid_add_plugin(${target} ${sources} HEADERS ${headers})
  find_library(LIB_PYTHON_PLUGIN CnoidPythonPlugin HINTS ${CHOREONOID_PLUGIN_DIR})
  Message ("plugin: ${CHOREONOID_PLUGIN_DIR}, lib: ${LIB_PYTHON_PLUGIN}")
  target_link_libraries(${target} Choreonoid::CnoidUtil ${LIB_PYTHON_PLUGIN} ${xeuslib} Threads::Threads)
else()
  # Build as a bundled project
  Message ("Build : AssemblerVRPlugin / under choreonoid/ext")
  ## build
  ##add_definitions(-DEXT_BUNDLE)
  if (WIN32)
    add_definitions(-D_WIN32)
    set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} /W2 /DEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MP /INCREMENTAL:NO")
  endif()
  choreonoid_add_plugin(${target} ${sources} HEADERS ${headers})
  Message ("AssemblerVR: ${OPENVR_LIBRARIES}")
  target_link_libraries(${target} PUBLIC
    CnoidUtil
    RobotAssembler
    CnoidRobotAssemblerPlugin
    CnoidOpenVRPlugin
  )

endif()
