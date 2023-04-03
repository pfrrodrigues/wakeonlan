cmake_minimum_required(VERSION 3.5.1)
project(wakeonlan-api)

set(CMAKE_CXX_STANDARD 14)
add_definitions(-DWAKEONLAN_API_VERSION="1.0.0")

add_subdirectory(modules/spdlog)

file(GLOB WAKEONLAN_API_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*")
file(GLOB_RECURSE WAKEONLAN_API_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*")

# SHARED LIBRARY
add_library(${PROJECT_NAME} SHARED ${WAKEONLAN_API_SOURCES} ${WAKEONLAN_API_HEADERS})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)
target_link_libraries(${PROJECT_NAME} spdlog pthread)

# STATIC LIBRARY
#add_library(${PROJECT_NAME}_static STATIC ${WAKEONLAN_API_SOURCES} ${WAKEONLAN_API_HEADERS})
#target_include_directories(${PROJECT_NAME}_static PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/)
#target_include_directories(${PROJECT_NAME}_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)
#target_link_libraries(${PROJECT_NAME}_static spdlog pthread)

file(GLOB APP_FILES examples/App.cpp ${WAKEONLAN_API_HEADERS})
add_executable(wolapp ${APP_FILES})
target_link_libraries(wolapp ${PROJECT_NAME})
