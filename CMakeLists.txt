cmake_minimum_required(VERSION 3.5)
project(message_synchronizer)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake_auto REQUIRED)
ament_auto_find_build_dependencies()

ament_auto_add_executable(example
  src/example.cpp
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  find_package(pcl_type_adapter REQUIRED)
  find_package(PCL REQUIRED)
  ament_auto_add_gtest(test_type_adapter test/test_type_adapter.cpp)
  target_link_libraries(test_type_adapter ${PCL_LIBRARIES})
  target_include_directories(test_type_adapter PRIVATE ${PCL_INCLUDE_DIRS})
  target_link_directories(test_type_adapter PRIVATE ${PCL_LIBRARY_DIRS})
  ament_target_dependencies(test_type_adapter pcl_type_adapter)
  ament_lint_auto_find_test_dependencies()
endif()

ament_auto_package()
