set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(Threads REQUIRED)

add_executable(elephant
    main.cpp
)

target_include_directories(elephant PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(elephant
    ${OPENGL_LIBRARIES}
    ${GLUT_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
)
