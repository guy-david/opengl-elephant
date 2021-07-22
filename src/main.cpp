#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

int main(int argc, char *argv[])
{
    constexpr int window_width = 1366;
    constexpr int window_height = 768;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH |
                        GLUT_MULTISAMPLE | GLUT_STENCIL);

    glutInitWindowSize(window_width, window_height);
    glutInitWindowPosition(
        (glutGet(GLUT_SCREEN_WIDTH)  - window_width)  / 2,
        (glutGet(GLUT_SCREEN_HEIGHT) - window_height) / 2);

    glutCreateWindow("OpenGL Elephant");

    glutMainLoop();

    return 0;
}
