#include <vector>
#include <array>
#include <iostream>
#include <fstream>
#include <sstream>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#define WINDOW_WIDTH 1366
#define WINDOW_HEIGHT 768

struct vec2
{
    float x, y;
};

struct vec3
{
    float x, y, z;
};

struct vec4
{
    float x, y, z, w;
};

struct Camera {
    vec3 pos;
    vec3 target;
};

static int g_window_id;
static Camera camera = { { 3, 10, 20 }, { 0, 0, 0 } };
static GLfloat up[3] = { 0.0f, 1.0f, 0.0f };

static GLfloat ambient_intensity = 0.2f;

struct Object
{
    vec3 pos;
    std::vector<std::vector<vec3>> faces;
};

Object tree;

void load_obj(const char *filename, std::vector<std::vector<vec3>> &faces)
{
    std::ifstream in(filename, std::ios::in);
    if (!in)
    {
        std::cerr << "Cannot open " << filename << std::endl;
        exit(1);
    }

    std::vector<vec3> vertices;

    std::string line;
    while (std::getline(in, line))
    {
        if (line.substr(0, 2) == "v ")
        {
            std::istringstream s(line.substr(2));
            vec3 v;
            s >> v.x;
            s >> v.y;
            s >> v.z;
            vertices.push_back(v);
        }
        else if (line.substr(0, 2) == "f ")
        {
            std::istringstream ss(line.substr(2));
            std::string token;
            std::vector<vec3> face;

            while(std::getline(ss, token, ' '))
            {
                std::istringstream ss2(token);
                int a, b, c;
                ss2 >> a; ss2 >> b; ss2 >> c;

                face.push_back(vertices[a - 1]);
            }

            faces.push_back(face);
        }
    }
}

static void draw_floor()
{
    GLfloat color1[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
	GLfloat color2[4] = { 0.7f, 0.7f, 0.7f, 1.0f };

    int columns = 10;
    int rows = 10;
    int xMin = -10;
    int xMax = 10;
    int yMin = -10;
    int yMax = 10;

	glPushMatrix();
	glNormal3d(0, 1, 0);

	float width = xMax - xMin;
	float height = yMax - yMin;
	float row_step = height / (float)columns; // calc rows height
	float column_step = width / (float)columns; // calc column width

	GLfloat specular[] = { 1.0f, 1.0f, 1.0f };
	GLfloat shininess = 128.0f;
	glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
	glMaterialf(GL_FRONT, GL_SHININESS, shininess); // make the floor shiny

	glBegin(GL_QUADS);
		// for each row
		for (int row = 0; row < rows; row++)
		{
			// for each column
			for (int column = 0; column < columns; column++)
			{
				glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, (row + column) % 2 == 0 ? color1 : color2); // choose color
				glVertex3f(xMin + column * column_step, 0, yMin + row * row_step);
				glVertex3f(xMin + (column + 1) * column_step, 0, yMin + row * row_step);
				glVertex3f(xMin + (column + 1) * column_step, 0, yMin + (row + 1) * row_step);
				glVertex3f(xMin + column * column_step, 0, yMin + (row + 1) * row_step);
			}
		}
	glEnd();

	glPopMatrix();
}

static void draw_bush()
{
	glPushMatrix();
	glNormal3d(0, 1, 0);

    GLfloat color1[4] = { 0.318f, 0.212f, 0.102f, 1.0f };
	GLfloat color2[4] = { 1.0f, 1.0f, 0.6f, 1.0f };

    glScalef(4.0f, 1.0f, 4.0f);
    for (size_t i = 0; i < tree.faces.size(); i++)
    {
        glBegin(GL_POLYGON);
	    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, (i < 6) ? color1 : color2);
        for (size_t j = 0; j < tree.faces[i].size(); j++)
        {
            // std::cerr << tree.faces[i][j].x << ", " << tree.faces[i][j].y << ", " << tree.faces[i][j].z << "\n";
            glVertex3f(tree.faces[i][j].x, tree.faces[i][j].y, tree.faces[i][j].z);
        }
	    glEnd();
    }

	glPopMatrix();
}

static void on_display()
{
    constexpr float y_fov = 75.0f;
    constexpr float aspect_ratio = static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT;
    constexpr float z_near = 1.0f;
    constexpr float z_far = 90.0f;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(y_fov, aspect_ratio, z_near, z_far);

    gluLookAt(camera.pos.x, camera.pos.y, camera.pos.z,
              camera.target.x, camera.target.y, camera.target.z,
              up[0], up[1], up[2]);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_NORMALIZE);
	glEnable(GL_LIGHTING);

    GLfloat globalAmbientVec[4] = { ambient_intensity , ambient_intensity, ambient_intensity, 1.0f };
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbientVec);
	glLoadIdentity();

    draw_floor();

    draw_bush();

    glFlush();
    glutSwapBuffers();
    glutPostRedisplay();
}

static void on_key_down(unsigned char key, int x, int y)
{
    constexpr int KEY_ESCAPE = 27;

    switch (key) {
        case KEY_ESCAPE:
            glutDestroyWindow(g_window_id);
            break;

        case GLUT_KEY_UP:
        case 'w':
            camera.pos.x += 0.2f * (camera.target.x - camera.pos.x);
            camera.pos.y += 0.2f * (camera.target.y - camera.pos.y);
            camera.pos.z += 0.2f * (camera.target.z - camera.pos.z);
            break;

        case GLUT_KEY_DOWN:
        case 's':
            camera.pos.x -= 0.2f * (camera.target.x - camera.pos.x);
            camera.pos.y -= 0.2f * (camera.target.y - camera.pos.y);
            camera.pos.z -= 0.2f * (camera.target.z - camera.pos.z);
            break;
    }
}

static void on_timer(int)
{

}

int main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH |
                        GLUT_MULTISAMPLE | GLUT_STENCIL);

    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(
        (glutGet(GLUT_SCREEN_WIDTH)  - WINDOW_WIDTH)  / 2,
        (glutGet(GLUT_SCREEN_HEIGHT) - WINDOW_HEIGHT) / 2);
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    load_obj("/home/guy/projects/opengl-elephant/Low_Poly_Foliage_Pack_001/OBJ Files/Low_Poly_Tree_Stump_002.obj",
             tree.faces);
    tree.pos = { 0, 0, 0 };

    g_window_id = glutCreateWindow("OpenGL Elephant");
    glutDisplayFunc(on_display);
    glutKeyboardFunc(on_key_down);
    glutTimerFunc(100, on_timer, 0);

    glutMainLoop();

    return 0;
}
