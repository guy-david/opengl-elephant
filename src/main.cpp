#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

static int g_window_id;

void on_display()
{
    glClearColor( 0.0, 0.0, 0.0, 1.0 );
    glClear( GL_COLOR_BUFFER_BIT );

    glFlush();
}

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

    g_window_id = glutCreateWindow("OpenGL Elephant");
    glutDisplayFunc(on_display);

    glutMainLoop();

    return 0;
}
