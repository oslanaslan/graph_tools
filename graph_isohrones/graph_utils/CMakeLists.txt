file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
"./**/*.cpp"
)

add_library(${PROJECT_NAME} INTERFACE)

# target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)

target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
# target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

include(FetchContent)

FetchContent_Declare(
  graphmlpp
  URL https://github.com/vitrevance/graphmlpp/archive/7c298cbeb64e8fa4d062a668ba4e15149a547c50.zip
)
FetchContent_MakeAvailable(graphmlpp)

target_link_libraries(${PROJECT_NAME} INTERFACE graphmlpp)

FetchContent_Declare(
  pybind11
  URL https://github.com/pybind/pybind11/archive/869cc1ff085dd405635b00eb46e5c84f50f26099.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
FetchContent_MakeAvailable(pybind11)

set(PYBIND11_PYTHON_VERSION "3.12.0")
set(PYMODULE_NAME "${PROJECT_NAME}_pymodule")
pybind11_add_module(${PYMODULE_NAME} ${SRC_FILES})

target_include_directories(${PYMODULE_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(${PYMODULE_NAME} PUBLIC graphmlpp)

FetchContent_Declare(
  geos
  URL https://github.com/libgeos/geos/archive/0a8fc37a965bd29d61d14249ba22a2ae66fd58fa.zip
)
FetchContent_MakeAvailable(geos)

target_link_libraries(${PROJECT_NAME} INTERFACE geos)
