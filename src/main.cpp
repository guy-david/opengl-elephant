#include <unordered_map>
#include <vector>
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
static Camera camera = { { 0, 10, 20 }, { 0, 0, 0 } };
static GLfloat up[3] = { 0.0f, 1.0f, 0.0f };

static GLfloat ambient_intensity = 1.0f;

struct Material
{
    float shininess;
    vec3 ambience;
    vec3 diffuse;
    vec3 specular;
    vec3 emission;
};

struct Face
{
    Material material;
    std::vector<std::pair<vec3, vec3>> vertices;
};

struct Object
{
    vec3 pos;
    vec3 scale;
    std::vector<Face> faces;
};

std::vector<Object> objects;

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

Object load_obj(vec3 pos, const std::string &filename)
{
    std::ifstream in(filename, std::ios::in);
    if (!in)
    {
        std::cerr << "Cannot open " << filename << std::endl;
        exit(1);
    }

    std::unordered_map<std::string, Material> materials;
    std::string current_mat;

    Object obj;
    obj.pos = pos;
    obj.scale = { 1.0f, 1.0f, 1.0f };

    std::vector<vec3> vertices;
    std::vector<vec3> normals;

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
            vec3 v;
            s >> v.x;
            s >> v.y;
            s >> v.z;
            vertices.push_back(v);
        }
        else if (line.substr(0, 3) == "vn ")
        {
            std::istringstream s(line.substr(3));
            vec3 v;
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

            obj.faces.push_back(face);
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

            obj.faces.push_back(face);
        }
    }

    return obj;
}

static void draw_object(Object& obj)
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
            const vec3 vertex = face.vertices[j].first;
            const vec3 normal = face.vertices[j].second;

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

    for (Object &obj : objects)
        draw_object(obj);

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

    objects.push_back(load_obj({0, 0, 0}, "../models/Floor.obj"));
    objects[0].scale = { 1000.0f, 0.0f, 1000.0f };

    objects.push_back(load_obj({7, 0, 3}, "../models/Low Poly Elephant.obj"));
    objects[1].scale = { 4.0f, 4.0f, 4.0f };

    objects.push_back(load_obj({-8, 0, 5}, "../models/Low_Poly_Bush_001.obj"));
    objects.push_back(load_obj({-6, 0, 5}, "../models/Low_Poly_Bush_002.obj"));
    objects.push_back(load_obj({ 0, 0, 9}, "../models/Low_Poly_Grass_001.obj"));
    objects.push_back(load_obj({ 0, 0, 7}, "../models/Low_Poly_Grass_002.obj"));
    objects.push_back(load_obj({ 6, 0, 5}, "../models/Low_Poly_Tree_Stump_001.obj"));
    objects.push_back(load_obj({ 8, 0, 5}, "../models/Low_Poly_Tree_Stump_002.obj"));
    objects.push_back(load_obj({-8, 0, 0}, "../models/Low_Poly_Tree_001.obj"));
    objects.push_back(load_obj({-5, 0, 0}, "../models/Low_Poly_Tree_002.obj"));
    objects.push_back(load_obj({-2, 0, 0}, "../models/Low_Poly_Tree_003.obj"));
    objects.push_back(load_obj({ 1, 0, 0}, "../models/Low_Poly_Tree_004.obj"));
    objects.push_back(load_obj({ 4, 0, 0}, "../models/Low_Poly_Tree_005.obj"));
    objects.push_back(load_obj({ 7, 0, 0}, "../models/Low_Poly_Tree_006.obj"));

    g_window_id = glutCreateWindow("OpenGL Elephant");
    glutDisplayFunc(on_display);
    glutKeyboardFunc(on_key_down);
    glutTimerFunc(100, on_timer, 0);

    glutMainLoop();

    return 0;
}
