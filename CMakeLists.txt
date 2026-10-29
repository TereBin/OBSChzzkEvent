cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" ON)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
find_package(obs-frontend-api REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs OBS::obs-frontend-api)

find_package(Qt6 REQUIRED COMPONENTS Core Network Widgets)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Network Qt6::Widgets)
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES AUTOMOC ON)

target_compile_features(${CMAKE_PROJECT_NAME} PRIVATE cxx_std_17)
if(MSVC)
  target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE /Z7)
endif()

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE
    src/chzzk-alert-server.cpp
    src/chzzk-alert-server.hpp
    src/chzzk-alert-protocol.hpp
    src/chzzk-alert-event-history.hpp
    src/chzzk-alert-connection.cpp
    src/chzzk-alert-connection.hpp
    src/chzzk-alert-status-dock.cpp
    src/chzzk-alert-status-dock.hpp
    src/chzzk-alert-request.cpp
    src/chzzk-alert-request.hpp
    src/chzzk-alert-audio.cpp
    src/chzzk-alert-audio.hpp
    src/chzzk-alert-settings.cpp
    src/chzzk-alert-settings.hpp
    src/chzzk-obs-resource.hpp
    src/chzzk-alert-visual.hpp
    src/chzzk-alert-model.hpp
    src/chzzk-alert-layout.hpp
    src/chzzk-alert-source.cpp
    src/chzzk-alert-source.hpp
    src/plugin-main.cpp
)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

option(ENABLE_ALERT_TESTS "Build alert state, audio lifecycle, HTTP and browser relay regression tests" OFF)
if(ENABLE_ALERT_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
