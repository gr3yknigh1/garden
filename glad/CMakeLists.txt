project(glad C)

add_library(glad)

target_include_directories(glad
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

if (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
  target_sources(glad
    PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}/include/glad/glad.h
      ${CMAKE_CURRENT_LIST_DIR}/include/glad/wgl.h
      ${CMAKE_CURRENT_LIST_DIR}/include/KHR/khrplatform.h
      ${CMAKE_CURRENT_LIST_DIR}/src/glad.c
      ${CMAKE_CURRENT_LIST_DIR}/src/glad_wgl.c
  )

  set_target_properties(glad
    PROPERTIES
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
  )

else()
  target_sources(glad
    PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}/include/glad/glad.h
      ${CMAKE_CURRENT_LIST_DIR}/include/KHR/khrplatform.h
      ${CMAKE_CURRENT_LIST_DIR}/src/glad.c
  )
endif()
