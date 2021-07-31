#include <cmath>
#include <thread>
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
    float model_yaw = 0.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;
    glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
    std::vector<Face> faces = {};
    float speed = 1.0f;
    float eyes_height = 0.0f;
    bool is_hidden = false;
};
using ObjectPtr = std::shared_ptr<Object>;

static glm::vec3 up = { 0.0f, 1.0f, 0.0f };

static int g_window_id;
static GLfloat ambient_intensity = 0.1f;

static std::vector<ObjectPtr> objects;

static ObjectPtr camera;
static float camera_back_distance = 10.0f;
static bool camera_3rd_person = false;
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
    if (obj.is_hidden)
        return;

	glPushMatrix();

    glTranslatef(obj.pos.x, obj.pos.y, obj.pos.z);
    glRotatef(obj.yaw + obj.model_yaw, 0.0f, -1.0f, 0.0f);
    glScalef(obj.scale.x, obj.scale.y, obj.scale.z);

    for (size_t i = 0; i < obj.faces.size(); i++)
    {
        glBegin(GL_POLYGON);

        const Face& face = obj.faces[i];

	    glMaterialfv(GL_FRONT, GL_AMBIENT, (const float *)&face.material.ambience);
	    glMaterialfv(GL_FRONT, GL_DIFFUSE, (const float *)&face.material.diffuse);
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
    constexpr float z_far = 200.0f;

    glClearColor(0.2, 0.5, 0.8, 1.0f * ambient_intensity);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(y_fov, aspect_ratio, z_near, z_far);

    float yaw = camera->yaw;
    float pitch = camera->pitch;

    if (camera_3rd_person)
        pitch = 0.0f;

    yaw = glm::radians(yaw);
    pitch = glm::radians(pitch);

    glm::vec3 direction;
    direction.x = cos(yaw) * cos(pitch);
    direction.y = sin(pitch);
    direction.z = sin(yaw) * cos(pitch);

    glm::vec3 pos = camera->pos;

    if (!camera_3rd_person) {
        pos.y += camera->eyes_height * camera->scale.y;
        gluLookAt(pos.x, pos.y, pos.z,
                  pos.x + direction.x,
                  pos.y + direction.y,
                  pos.z + direction.z,
                  up.x, up.y, up.z);
    } else {
        gluLookAt(pos.x - direction.x * camera_back_distance,
                  pos.y + camera_back_distance,
                  pos.z - direction.z * camera_back_distance,
                  pos.x + direction.x * camera_back_distance,
                  pos.y + direction.y * camera_back_distance,
                  pos.z + direction.z * camera_back_distance,
                  up.x, up.y, up.z);
    }

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
        float position[3] = { 0.0f, 30.0f, 0.0f };
        float target[3] = { 0.0f, 0.0f, 0.0f };
        glLightfv(id, GL_DIFFUSE, color);
        glLightfv(id, GL_SPECULAR, color);
        glLightfv(id, GL_POSITION, position);
        GLfloat direction[3] = { target[0] - position[0],
                                 target[1] - position[1],
                                 target[2] - position[2] };
        glLightfv(id, GL_SPOT_DIRECTION, direction);
        glLightf(id, GL_SPOT_CUTOFF, 90.0f);
        glLightf(id, GL_SPOT_EXPONENT, 3.0f);
    }

    {
        glEnable(GL_LIGHT1);
        int id = GL_LIGHT1;
        float color[3] = { 1.0f, 1.0f, 1.0f };
        float position[3] = { -15.0f, 0.0f, -15.0f };
        float target[3] = { 0.0f, 1.0f, 0.0f };
        glLightfv(id, GL_DIFFUSE, color);
        glLightfv(id, GL_SPECULAR, color);
        glLightfv(id, GL_POSITION, position);
        GLfloat direction[3] = { target[0] - position[0],
                                 target[1] - position[1],
                                 target[2] - position[2] };
        glLightfv(id, GL_SPOT_DIRECTION, direction);
        glLightf(id, GL_SPOT_CUTOFF, 180.0f);
        glLightf(id, GL_SPOT_EXPONENT, 0.0f);
    }

    GLfloat globalAmbientVec[4] = { ambient_intensity, ambient_intensity, ambient_intensity, 1.0f };
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbientVec);
	glLoadIdentity();

    {
        glPushMatrix();

        glTranslatef(-3, 6, 10);
        glutSolidSphere(3, 300, 300);

        glPopMatrix();
    };

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
                elephant->is_hidden = false;

                if (camera == free_camera) {
                    camera = elephant;
                    camera_3rd_person = true;
                } else if (camera == elephant && camera_3rd_person) {
                    elephant->is_hidden = true;
                    camera = elephant;
                    camera_3rd_person = false;
                } else {
                    free_camera->pos = camera->pos;
                    free_camera->pos.y += camera->eyes_height * camera->scale.y;
                    free_camera->yaw = camera->yaw;
                    free_camera->pitch = camera->pitch;
                    camera = free_camera;
                }

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
    camera->pitch = glm::clamp(camera->pitch, -89.0f, 89.0f);

    glutPostRedisplay();
}

static void on_timer(int)
{
    if (w || s || a || d) {
        float yaw = glm::radians(camera->yaw);
        float pitch = glm::radians(camera->pitch);

        glm::vec3 direction;
        direction.x = cos(yaw) * cos(pitch);
        direction.y = sin(pitch);
        direction.z = sin(yaw) * cos(pitch);

        if (camera == elephant) {
            direction.y = 0;
        }

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

void setup_map_spawn(std::mt19937& rng, const std::vector<std::string> &kinds, size_t amount, float max_scale=1.0f, float min_scale=1.0f)
{
    std::uniform_real_distribution<float> location_dis(-FOREST_SIZE, FOREST_SIZE);
    std::uniform_real_distribution<float> yaw_dis(-180.0f, 180.f);
    std::uniform_real_distribution<float> scale_dis(min_scale, max_scale);
    std::uniform_int_distribution<size_t> kind_dis(0, kinds.size() - 1);

    for (size_t i = 0; i < amount; i++) {
        float x = location_dis(rng);
        float z = location_dis(rng);
        float yaw = yaw_dis(rng);
        float scale = scale_dis(rng);
        size_t kind_index = kind_dis(rng);
        const std::string &kind = kinds[kind_index];

        ObjectPtr object = load_obj(kind);
        object->pos = { x, 0, z };
        object->yaw = yaw;
        object->scale = { scale, scale, scale };
        objects.push_back(object);
    }

}

void setup_map()
{
    {
        int scale = 3;
        for (int x = -FOREST_SIZE; x < FOREST_SIZE; x += scale) {
            for (int z = -FOREST_SIZE; z < FOREST_SIZE; z += scale) {
                ObjectPtr floor = load_obj("../models/Floor.obj");
                floor->pos = { x, 0, z };
                floor->scale = { scale / 2.0f, 1.0f, scale / 2.0f };
                objects.push_back(floor);
            }
        }
    }

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
    setup_map_spawn(rng, tree_kinds,  FOREST_AREA / 64, 3.0f);
    setup_map_spawn(rng, stump_kinds, FOREST_AREA / 256, 3.0f);
    setup_map_spawn(rng, bush_kinds,  FOREST_AREA / 32);
    setup_map_spawn(rng, grass_kinds, FOREST_AREA / 8, 1.0f, 0.5f);

    elephant = load_obj("../models/Low Poly Elephant.obj");
    elephant->model_yaw = -90.f;
    elephant->eyes_height = 0.8f;
    elephant->scale = { 4, 4, 4 };
    objects.push_back(elephant);

    free_camera = std::make_shared<Object>();
    free_camera->pos = { 8, 8, 8 };
    free_camera->yaw = -135.0f;
    free_camera->pitch = -25.0f;
    objects.push_back(free_camera);

    camera = elephant;
    camera_3rd_person = true;
}

void game_thread(int argc, char *argv[])
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
}

int main(int argc, char *argv[])
{
    std::thread t1(game_thread, argc, argv);

    std::cout << "***************" << std::endl;
    std::cout << "OpenGL Elephant" << std::endl;
    std::cout << "***************" << std::endl;

    std::cout << std::endl;
    std::cout << "Type help to view the available commands." << std::endl;

    while (true) {
        std::cout << "\r> " << std::flush;

        std::string command;
        std::getline(std::cin, command);

        if (command == "help") {
            std::cout << "Available commands:" << std::endl;
            std::cout << "  help: view this list" << std::endl;
            std::cout << "  ambience: set the ambient intensity" << std::endl;
            std::cout << "  quit: exit the application" << std::endl;
        }

        else if (command == "ambience") {
            std::cout << "choose a value between 0-1: " << std::flush;
            std::cin >> ambient_intensity;

            glutPostRedisplay();
        }

        else if (command == "quit") {
            glutDestroyWindow(g_window_id);
            break;
        }
    }

    t1.join();

    return 0;
}
