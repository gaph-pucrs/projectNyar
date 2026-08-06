cmake_minimum_required(VERSION 3.8)
project(receptor)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# === O MAPA DO TESOURO: Avisando onde estão as bibliotecas ===
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)

# === A FORJA ===
add_executable(receptor_node src/receptor.cpp)

# === A UNIÃO ===
ament_target_dependencies(receptor_node
  rclcpp
  sensor_msgs
  cv_bridge
  OpenCV
)

# === O ARMAZENAMENTO ===
install(TARGETS receptor_node
  DESTINATION lib/${PROJECT_NAME})

ament_package()
