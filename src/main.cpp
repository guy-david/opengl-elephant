#include <cmath>
#include <vector>
#include <random>
#include <unordered_map>
#include <memory>
#include <fstream>
#include <sstream>
#include <iostream>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <glm/glm.hpp>

#define WINDOW_WIDTH 1366
#define WINDOW_HEIGHT 768

#define FOREST_SIZE 100
#define FOREST_AREA (FOREST_SIZE * FOREST_SIZE)

struct Material
{
    float shininess;
    glm::vec3 ambience;
    glm::vec3 diffuse;
    glm::vec3 specular;
    glm::vec3 emission;
};

struct Face
{
    Material material;
    std::vector<std::pair<glm::vec3, glm::vec3>> vertices;
};

struct Object
{
    glm::vec3 pos = { 0.0f, 0.0f, 0.0f };
    float yaw = -90.0f;
    float pitch = -90.0f;
    glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
    std::vector<Face> faces = {};
    float speed = 1.0f;
};
using ObjectPtr = std::shared_ptr<Object>;

static glm::vec3 up = { 0.0f, 1.0f, 0.0f };

static int g_window_id;
static GLfloat ambient_intensity = 0.5f;

static std::vector<ObjectPtr> objects;

static ObjectPtr camera;
static ObjectPtr free_camera;
static ObjectPtr elephant;

std::unordered_map<std::string, Material> load_mtl(const std::string &filename)
{
    std::ifstream in(filename, std::ios::in);
    if (!in)
    {
        std::cerr << "Cannot open " << filename << std::endl;
        exit(1);
    }

    std::unordered_map<std::string, Material> materials;
    std::string current_mat;

    std::string line;
    while (std::getline(in, line))
    {
        if (line.substr(0, 7) == "newmtl ")
        {
            current_mat = line.substr(7);
        }
        else if (line.substr(0, 3) == "Ns ")
        {
            std::istringstream ss(line.substr(3));
            ss >> materials[current_mat].shininess;
        }
        else if (line.substr(0, 3) == "Ka ")
        {
            std::istringstream ss(line.substr(3));
            ss >> materials[current_mat].ambience.x;
            ss >> materials[current_mat].ambience.y;
            ss >> materials[current_mat].ambience.z;
        }
        else if (line.substr(0, 3) == "Kd ")
        {
            std::istringstream ss(line.substr(3));
            ss >> materials[current_mat].diffuse.x;
            ss >> materials[current_mat].diffuse.y;
            ss >> materials[current_mat].diffuse.z;
        }
        else if (line.substr(0, 3) == "Ks ")
        {
            std::istringstream ss(line.substr(3));
            ss >> materials[current_mat].specular.x;
            ss >> materials[current_mat].specular.y;
            ss >> materials[current_mat].specular.z;
        }
        else if (line.substr(0, 3) == "Ke ")
        {
            std::istringstream ss(line.substr(3));
            ss >> materials[current_mat].emission.x;
            ss >> materials[current_mat].emission.y;
            ss >> materials[current_mat].emission.z;
        }
    }

    return materials;
}

ObjectPtr load_obj(const std::string &filename)
{
    std::ifstream in(filename, std::ios::in);
    if (!in)
    {
        std::cerr << "Cannot open " << filename << std::endl;
        exit(1);
    }

    std::unordered_map<std::string, Material> materials;
    std::string current_mat;

    auto obj = std::make_shared<Object>();

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;

    std::string line;
    while (std::getline(in, line))
    {
        if (line.substr(0, 7) == "mtllib ")
        {
            std::string mtl_path = line.substr(7);
            std::string dir_name = filename.substr(0, filename.find_last_of("\\/"));
            materials = load_mtl(dir_name + "/" + mtl_path);
        }
        else if (line.substr(0, 7) == "usemtl ")
        {
            current_mat = line.substr(7);
        }
        else if (line.substr(0, 2) == "v ")
        {
            std::istringstream s(line.substr(2));
            glm::vec3 v;
            s >> v.x;
            s >> v.y;
            s >> v.z;
            vertices.push_back(v);
        }
        else if (line.substr(0, 3) == "vn ")
        {
            std::istringstream s(line.substr(3));
            glm::vec3 v;
            s >> v.x;
            s >> v.y;
            s >> v.z;
            normals.push_back(v);
        }
        else if (line.substr(0, 2) == "f ")
        {
            std::istringstream ss(line.substr(2));
            std::string token;
            Face face;

            face.material = materials[current_mat];

            while (std::getline(ss, token, ' '))
            {
                std::istringstream ss2(token);
                int v_id, vn_id;
                ss2 >> v_id;
                ss2.ignore();
                ss2.ignore();
                ss2 >> vn_id;

                face.vertices.push_back({ vertices[v_id - 1], normals[vn_id - 1] });
            }

            obj->faces.push_back(face);
        }
        else if (line.substr(0, 2) == "l ")
        {
            std::istringstream ss(line.substr(2));
            std::string token;
            Face face;

            face.material = materials[current_mat];

            while (std::getline(ss, token, ' '))
            {
                std::istringstream ss2(token);
                int v_id;
                ss2 >> v_id;

                face.vertices.push_back({ vertices[v_id - 1], {} });
            }

            obj->faces.push_back(face);
        }
    }

    return obj;
}

static void draw_object(const Object& obj)
{
	glPushMatrix();
	glNormal3d(0, 1, 0);

    glTranslatef(obj.pos.x, obj.pos.y, obj.pos.z);
    glScalef(obj.scale.x, obj.scale.y, obj.scale.z);
    for (size_t i = 0; i < obj.faces.size(); i++)
    {
        glBegin(GL_POLYGON);

        const Face& face = obj.faces[i];

	    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, (const float *)&face.material.diffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, (const float *)&face.material.specular);
        glMaterialfv(GL_FRONT, GL_EMISSION, (const float *)&face.material.emission);
        glMaterialf (GL_FRONT, GL_SHININESS, face.material.shininess);

        for (size_t j = 0; j < face.vertices.size(); j++)
        {
            const glm::vec3 vertex = face.vertices[j].first;
            const glm::vec3 normal = face.vertices[j].second;

            glNormal3f(normal.x, normal.y, normal.z);
            glVertex3f(vertex.x, vertex.y, vertex.z);
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

    glClearColor(0.2, 0.5, 0.8, 1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(y_fov, aspect_ratio, z_near, z_far);

    glm::vec3 direction;
    direction.x = cos(glm::radians(camera->yaw)) * cos(glm::radians(camera->pitch));
    direction.y = sin(glm::radians(camera->pitch));
    direction.z = sin(glm::radians(camera->yaw)) * cos(glm::radians(camera->pitch));

    gluLookAt(camera->pos.x, camera->pos.y, camera->pos.z,
              camera->pos.x + direction.x,
              camera->pos.y + direction.y,
              camera->pos.z + direction.z,
              up.x, up.y, up.z);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_FLAT);
	glEnable(GL_NORMALIZE);
	glEnable(GL_LIGHTING);

    {
        glEnable(GL_LIGHT0);
        int id = GL_LIGHT0;
        float color[3] = { 1.0f, 1.0f, 1.0f };
        float position[3] = { 7.0f, 0.0f, 3.0f };
        float target[3] = { 0.0f, 1.0f, 0.0f };
        glLightfv(id, GL_DIFFUSE, color);
        glLightfv(id, GL_SPECULAR, color);
        glLightfv(id, GL_POSITION, position);
        GLfloat direction[3] = { target[0] - position[0],
                                target[1] - position[1],
                                target[2] - position[2] };
        glLightfv(id, GL_SPOT_DIRECTION, direction);
        glLightf(id, GL_SPOT_CUTOFF, 120.0f);
        glLightf(id, GL_SPOT_EXPONENT, 0.0f);
    }

    GLfloat globalAmbientVec[4] = { ambient_intensity , ambient_intensity, ambient_intensity, 1.0f };
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbientVec);
	glLoadIdentity();

    for (ObjectPtr obj : objects)
        draw_object(*obj);

    glFlush();
    glutSwapBuffers();
}

static int w = 0;
static int s = 0;
static int a = 0;
static int d = 0;
static int spacebar = 0;

static void on_key_down(unsigned char key, int x, int y)
{
    constexpr unsigned char KEY_ESCAPE = 27;

    switch (key) {
        case KEY_ESCAPE:
            glutDestroyWindow(g_window_id);
            break;

        case ' ':
            if (!spacebar) {
                if (camera == free_camera)
                    camera = elephant;
                else
                    camera = free_camera;

                glutPostRedisplay();
            }

            spacebar = 1;
            break;

        case 'w': w = 1; break;
        case 's': s = 1; break;
        case 'a': a = 1; break;
        case 'd': d = 1; break;
    }
}

static void on_key_up(unsigned char key, int x, int y)
{
    switch (key) {
        case ' ': spacebar = 0; break;
        case 'w': w = 0; break;
        case 's': s = 0; break;
        case 'a': a = 0; break;
        case 'd': d = 0; break;
    }
}

static void on_mouse_click(int button, int state, int x, int y)
{
}

static void on_mouse_move(int x, int y)
{
    int dx = (x - WINDOW_WIDTH / 2);
    int dy = (y - WINDOW_HEIGHT / 2);

    if (dx || dy)
        glutWarpPointer(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);

    const float mouse_speed = 0.1f;

    float fdx = dx * mouse_speed;
    camera->yaw += fdx * mouse_speed;

    float fdy = dy * mouse_speed;
    camera->pitch -= fdy * mouse_speed;
    camera->pitch = glm::clamp(camera->pitch, -90.0f, 90.0f);
}

static void on_timer(int)
{
    if (w || s || a || d) {
        glm::vec3 direction;
        direction.x = cos(glm::radians(camera->yaw)) * cos(glm::radians(camera->pitch));
        direction.y = sin(glm::radians(camera->pitch));
        direction.z = sin(glm::radians(camera->yaw)) * cos(glm::radians(camera->pitch));

        if (w ^ s) {
            if (w)
                camera->pos += direction * 0.2f * camera->speed;
            if (s)
                camera->pos -= direction * 0.2f * camera->speed;
        }

        if (a ^ d) {
            if (a)
                camera->pos -= glm::normalize(glm::cross(direction, up)) * 0.2f * camera->speed;
            if (d)
                camera->pos += glm::normalize(glm::cross(direction, up)) * 0.2f * camera->speed;
        }

        glutPostRedisplay();
    }

    glutTimerFunc(40, on_timer, 0);
}

void setup_map_spawn(std::mt19937& rng, size_t amount, const std::vector<std::string> &kinds)
{
    std::uniform_real_distribution<float> location_dis(-FOREST_SIZE, FOREST_SIZE);
    std::uniform_int_distribution<size_t> kind_dis(0, kinds.size() - 1);

    for (size_t i = 0; i < amount; i++) {
        float x = location_dis(rng);
        float z = location_dis(rng);
        size_t kind_index = kind_dis(rng);
        const std::string &kind = kinds[kind_index];

        ObjectPtr object = load_obj(kind);
        object->pos = { x, 0, z };
        objects.push_back(object);
    }

}

void setup_map()
{
    ObjectPtr floor = load_obj("../models/Floor.obj");
    floor->scale = { FOREST_SIZE, 0.0f, FOREST_SIZE };
    objects.push_back(floor);

    std::vector<std::string> tree_kinds = {
        "../models/Low_Poly_Tree_001.obj",
        "../models/Low_Poly_Tree_002.obj",
        "../models/Low_Poly_Tree_003.obj",
        "../models/Low_Poly_Tree_004.obj",
        "../models/Low_Poly_Tree_005.obj",
        "../models/Low_Poly_Tree_006.obj",
    };

    std::vector<std::string> stump_kinds = {
        "../models/Low_Poly_Tree_Stump_001.obj",
        "../models/Low_Poly_Tree_Stump_002.obj",
    };

    std::vector<std::string> bush_kinds = {
        "../models/Low_Poly_Bush_001.obj",
        "../models/Low_Poly_Bush_002.obj",
    };

    std::vector<std::string> grass_kinds = {
        "../models/Low_Poly_Grass_001.obj",
        "../models/Low_Poly_Grass_002.obj",
    };

    std::mt19937 rng(1337);
    setup_map_spawn(rng, FOREST_AREA / 64, tree_kinds);
    setup_map_spawn(rng, FOREST_AREA / 256, stump_kinds);
    setup_map_spawn(rng, FOREST_AREA / 32, bush_kinds);
    setup_map_spawn(rng, FOREST_AREA / 8, grass_kinds);

    elephant = load_obj("../models/Low Poly Elephant.obj");
    elephant->scale = { 4, 4, 4 };
    objects.push_back(elephant);

    free_camera = std::make_shared<Object>();
    free_camera->pos = { 20, 20, 20 };
    free_camera->yaw = -45.0f;
    free_camera->pitch = 45.0f;
    objects.push_back(free_camera);

    camera = elephant;
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

    setup_map();

    g_window_id = glutCreateWindow("OpenGL Elephant");

    glutDisplayFunc(on_display);

    glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);
    glutKeyboardFunc(on_key_down);
    glutKeyboardUpFunc(on_key_up);

	glutMouseFunc(on_mouse_click);
	glutPassiveMotionFunc(on_mouse_move);
    glutSetCursor(GLUT_CURSOR_NONE);
    glutWarpPointer(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);

    glutTimerFunc(40, on_timer, 0);

    glutMainLoop();

    return 0;
}
