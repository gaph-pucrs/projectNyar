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
# Usando o nome correto do arquivo .cpp que nós descobrimos antes
add_executable(receptor_node src/camera_receptor.cpp)

# === A UNIÃO ===
# 1. Dependências exclusivas do ROS 2 (ament)
ament_target_dependencies(receptor_node
  rclcpp
  sensor_msgs
  cv_bridge
)

# 2. Dependências nativas de C++ (OpenCV)
target_include_directories(receptor_node PUBLIC ${OpenCV_INCLUDE_DIRS})
target_link_libraries(receptor_node ${OpenCV_LIBRARIES})

# === O ARMAZENAMENTO ===
install(TARGETS receptor_node
  DESTINATION lib/${PROJECT_NAME})

ament_package()
