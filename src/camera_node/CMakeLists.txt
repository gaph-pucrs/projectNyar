cmake_minimum_required(VERSION 3.8)
project(camera_node)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 1. Buscar dependências do ROS 2 e OpenCV
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(image_transport REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)

set(dependencies
  rclcpp
  sensor_msgs
  image_transport
  cv_bridge
  OpenCV
)

# 2. Compilar o Nodo Publicador (Raspberry Pi)
add_executable(camera_publisher src/camera_publisher.cpp)
ament_target_dependencies(camera_publisher ${dependencies})

# 3. Compilar o Nodo Inscrito / Cérebro (Workstation)
add_executable(camera_subscriber src/camera_subscriber.cpp)
ament_target_dependencies(camera_subscriber ${dependencies})

# 4. Regras de Instalação (Essencial para o comando "ros2 run" achar os binários)
install(TARGETS
  camera_publisher
  camera_subscriber
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_export_dependencies(${dependencies})
ament_package()