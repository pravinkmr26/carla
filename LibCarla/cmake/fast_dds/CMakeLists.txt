#set(CMAKE_C_COMPILER "/usr/bin/gcc-7")
#set(CMAKE_CXX_COMPILER "/usr/bin/g++-7")

cmake_minimum_required(VERSION 3.5.1)
project(libcarla_fastdds)

# Install headers.
find_package(fastrtps REQUIRED)
find_package(fastcdr REQUIRED)
message("-- Loaded FasdDDS and CDR")

file(GLOB libcarla_carla_fastdds_headers
  "${libcarla_source_path}/carla/ros2/publishers/*.h"
  "${libcarla_source_path}/carla/ros2/subscribers/*.h"
  "${libcarla_source_path}/carla/ros2/listeners/*.h"
  "${libcarla_source_path}/carla/ros2/types/*.h"
  )
install(FILES ${libcarla_carla_fastdds_headers} DESTINATION include/carla/ros2)

file(GLOB fast_dds_dependencies "${FASTDDS_LIB_PATH}/*.so*")
install(FILES ${fast_dds_dependencies} DESTINATION lib)


file(GLOB libcarla_fastdds_sources
  "${libcarla_source_path}/carla/ros2/publishers/*.cpp"
  "${libcarla_source_path}/carla/ros2/subscribers/*.cpp"
  "${libcarla_source_path}/carla/ros2/listeners/*.cpp"
  "${libcarla_source_path}/carla/ros2/types/*.cpp")

set(FASTDDS_CPP_STD_INCLUDES "/usr/include/c++/7")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -fPIC -D_GLIBCXX_USE_CXX11_ABI=0 -I${FASTDDS_CPP_STD_INCLUDES} -I/usr/include/x86_64-linux-gnu/c++/7" CACHE STRING "" FORCE)
#set(CMAKE_CXX_FLAGS_RELEASE "-O3 -fPIC -D_GLIBCXX_USE_CXX11_ABI=0 -I${FASTDDS_CPP_STD_INCLUDES}" CACHE STRING "" FORCE)

# ==============================================================================
# Create targets for debug and release in the same build type.
# ==============================================================================

message("-- include paths for fast dds " ${BOOST_INCLUDE_PATH} "-- " ${RPCLIB_INCLUDE_PATH})
if (LIBCARLA_BUILD_RELEASE)
  add_library(carla_fastdds STATIC ${libcarla_fastdds_sources})
  message("-- release build ")
  target_include_directories(carla_fastdds SYSTEM PRIVATE
      "${BOOST_INCLUDE_PATH}"
      "${RPCLIB_INCLUDE_PATH}")

  target_include_directories(carla_fastdds PRIVATE "${FASTDDS_INCLUDE_PATH}")
  target_include_directories(carla_fastdds PRIVATE "${libcarla_source_path}/carla/ros2")
  target_link_libraries(carla_fastdds fastrtps fastcdr "${FAST_DDS_LIBRARIES}")
  install(TARGETS carla_fastdds DESTINATION lib)
  set_target_properties(carla_fastdds PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS_RELEASE}")

endif()

if (LIBCARLA_BUILD_DEBUG)
message("-- debug build ")
  add_library(carla_fastdds_debug STATIC ${libcarla_fastdds_sources})
 target_include_directories(carla_fastdds_debug SYSTEM PRIVATE
      "${BOOST_INCLUDE_PATH}"
      "${RPCLIB_INCLUDE_PATH}")
  install(TARGETS carla_fastdds_debug DESTINATION lib)
  set_target_properties(carla_fastdds_debug PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS_DEBUG}")
  target_compile_definitions(carla_fastdds_debug PUBLIC -DBOOST_ASIO_ENABLE_BUFFER_DEBUGGING)

endif()
