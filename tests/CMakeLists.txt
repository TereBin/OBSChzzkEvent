add_executable(alert-model-tests model-tests.cpp)
target_include_directories(alert-model-tests PRIVATE ../src)
target_compile_features(alert-model-tests PRIVATE cxx_std_17)
add_test(NAME alert-model COMMAND alert-model-tests)

# A small OBS API double exercises the production audio controller without launching OBS.
add_executable(alert-audio-tests audio-tests.cpp ../src/chzzk-alert-audio.cpp)
target_include_directories(alert-audio-tests PRIVATE stubs ../src)
target_compile_features(alert-audio-tests PRIVATE cxx_std_17)
add_test(NAME alert-audio COMMAND alert-audio-tests)

add_executable(alert-http-tests http-tests.cpp ../src/chzzk-alert-request.cpp
               ../src/chzzk-alert-server.cpp ../src/chzzk-alert-server.hpp ../src/chzzk-alert-connection.cpp)
target_include_directories(alert-http-tests PRIVATE ../src)
target_link_libraries(alert-http-tests PRIVATE Qt6::Core Qt6::Network)
set_target_properties(alert-http-tests PROPERTIES AUTOMOC ON)
add_test(NAME alert-http COMMAND alert-http-tests)
set_tests_properties(alert-http PROPERTIES
  ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

add_executable(alert-connection-tests connection-tests.cpp ../src/chzzk-alert-connection.cpp)
target_include_directories(alert-connection-tests PRIVATE ../src)
target_link_libraries(alert-connection-tests PRIVATE Qt6::Core)
target_compile_features(alert-connection-tests PRIVATE cxx_std_17)
add_test(NAME alert-connection COMMAND alert-connection-tests)
set_tests_properties(alert-connection PROPERTIES
  ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

if(MSVC)
  foreach(test_target alert-model-tests alert-audio-tests alert-http-tests alert-connection-tests)
    target_compile_options(${test_target} PRIVATE /Z7)
  endforeach()
endif()

find_program(NODE_EXECUTABLE node)
if(NODE_EXECUTABLE)
  add_test(NAME browser-relay COMMAND "${NODE_EXECUTABLE}" --test "${CMAKE_CURRENT_SOURCE_DIR}/browser-relay.test.cjs")
endif()
