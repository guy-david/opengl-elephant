cmake_minimum_required(VERSION 2.8)

project(elephant)

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

add_executable(elephant
    main.cpp
)

target_link_libraries(elephant
    ${OPENGL_LIBRARIES}
    ${GLUT_LIBRARY}
)
