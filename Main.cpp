#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>
#include <GL/glew.h>
//#include <OpenGL/gl3.h>   // The GL Header File
#include <GLFW/glfw3.h> // The GLFW header
#include <glm/glm.hpp> // GL Math library header
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> 
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

using namespace std;
void printFps();


enum ShaderType {
    VERTEX,
    FRAGMENT,
    GEOMETRY,
};

struct Vertex
{
    Vertex(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) { }
    GLfloat x, y, z;
};

struct Texture
{
    Texture(GLfloat inU, GLfloat inV) : u(inU), v(inV) { }
    GLfloat u, v;
};

struct Normal
{
    Normal(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) { }
    GLfloat x, y, z;
};

struct Face
{
    Face(int v[], int t[], int n[]) {
        vIndex[0] = v[0];
        vIndex[1] = v[1];
        vIndex[2] = v[2];
        tIndex[0] = t[0];
        tIndex[1] = t[1];
        tIndex[2] = t[2];
        nIndex[0] = n[0];
        nIndex[1] = n[1];
        nIndex[2] = n[2];
    }
    GLuint vIndex[3], tIndex[3], nIndex[3];
};

void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void setShaderParams(int index);
void clearScreen();

int activeProgramIndex = 1;
GLuint gProgram[3];
GLint modelingMatrixLoc[3];
GLint viewingMatrixLoc[3];
GLint projectionMatrixLoc[3];
GLint eyePosLoc[3];
int gWidth = 640;
int gHeight = 480;

glm::mat4 projectionMatrix;
glm::mat4 viewingMatrix;
glm::mat4 modelingMatrix;

glm::vec3 cameraPos = glm::vec3(0.0f, 10.0f, 0.0f);
glm::vec3 cameraGaze = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float defaultFovyRad = (float)(45.0 / 180.0) * M_PI;

float deltaTime = 0.0f;	// Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame

float lastX, lastY;
bool firstTimeReceivingMouseInput = true;

float mouseYaw = -90.0f;
float mousePitch = 0.0f;

float centralObjectRotDeg = 0.0f;
float orbitObjectRotDef = 0.0;

vector<Vertex> gVertices;
vector<Texture> gTextures;
vector<Normal> gNormals;
vector<Face> gFaces;

// for drawing the Teapot
GLuint gVertexAttribBuffer, gIndexBuffer;
int gVertexDataSizeInBytes, gNormalDataSizeInBytes;
GLuint teapotVAO;

// for drawing the orbitObject(s)
GLuint orbitVertexAttribBuffer, orbitIndexBuffer;
int orbitVertexDataSizeInBytes, orbitNormalDataSizeInBytes;
GLuint orbitVAO;


GLsizei captureWidth = 512;
GLsizei captureHeight = captureWidth;
float captureFovy = (float)(90 / 180.0) * M_PI;

// for drawing the static cubemapped skybox
GLuint terrainVAO;
float terrainStartingPoint[] = {
    // positions          
    0.0f,  0.0f, 0.0f,
};

// Shader parameters
float heightFactor = 1.7f;
int widthParam = 30;
int sampleCount = 1000;


bool ParseObj(const string& fileName, vector<Texture>& textures, vector<Normal>& normals, vector<Vertex>& vertices, vector<Face>& faces)
{
    fstream myfile;

    // Open the input 
    myfile.open(fileName.c_str(), std::ios::in);

    if (myfile.is_open())
    {
        string curLine;

        while (getline(myfile, curLine))
        {
            stringstream str(curLine);
            GLfloat c1, c2, c3;
            GLuint index[9];
            string tmp;

            if (curLine.length() >= 2)
            {
                if (curLine[0] == 'v')
                {
                    if (curLine[1] == 't') // texture
                    {
                        str >> tmp; // consume "vt"
                        str >> c1 >> c2;
                        textures.push_back(Texture(c1, c2));
                    }
                    else if (curLine[1] == 'n') // normal
                    {
                        str >> tmp; // consume "vn"
                        str >> c1 >> c2 >> c3;
                        normals.push_back(Normal(c1, c2, c3));
                    }
                    else // vertex
                    {
                        str >> tmp; // consume "v"
                        str >> c1 >> c2 >> c3;
                        vertices.push_back(Vertex(c1, c2, c3));
                    }
                }
                else if (curLine[0] == 'f') // face
                {
                    str >> tmp; // consume "f"
                    char c;
                    int vIndex[3], nIndex[3], tIndex[3];
                    str >> vIndex[0]; str >> c >> c; // consume "//"
                    str >> nIndex[0];
                    str >> vIndex[1]; str >> c >> c; // consume "//"
                    str >> nIndex[1];
                    str >> vIndex[2]; str >> c >> c; // consume "//"
                    str >> nIndex[2];

                    assert(vIndex[0] == nIndex[0] &&
                        vIndex[1] == nIndex[1] &&
                        vIndex[2] == nIndex[2]); // a limitation for now

                 // make indices start from 0
                    for (int c = 0; c < 3; ++c)
                    {
                        vIndex[c] -= 1;
                        nIndex[c] -= 1;
                        tIndex[c] -= 1;
                    }

                    faces.push_back(Face(vIndex, tIndex, nIndex));
                }
                else
                {
                    cout << "Ignoring unidentified line in obj file: " << curLine << endl;
                }
            }

            //data += curLine;
            if (!myfile.eof())
            {
                //data += "\n";
            }
        }

        myfile.close();
    }
    else
    {
        return false;
    }

    assert(gVertices.size() == gNormals.size());

    return true;
}

bool ReadDataFromFile(
    const string& fileName, ///< [in]  Name of the shader file
    string& data)     ///< [out] The contents of the file
{
    fstream myfile;

    // Open the input 
    myfile.open(fileName.c_str(), std::ios::in);

    if (myfile.is_open())
    {
        string curLine;

        while (getline(myfile, curLine))
        {
            data += curLine;
            if (!myfile.eof())
            {
                data += "\n";
            }
        }

        myfile.close();
    }
    else
    {
        return false;
    }

    return true;
}

GLuint createShader(const char* shaderName, ShaderType type) 
{
    string shaderSource;
    string filename(shaderName);
    if (!ReadDataFromFile(filename, shaderSource))
    {
        cout << "Cannot find file name: " + filename << endl;
        exit(-1);
    }

    GLint length = shaderSource.length();
    const GLchar* shader = (const GLchar*)shaderSource.c_str();

    GLuint vs;
    string debugName = "";

    if (type == VERTEX) 
    {
        vs = glCreateShader(GL_VERTEX_SHADER);
        debugName = "Vertex shader";
    }
    else if (type == FRAGMENT) 
    {
        vs = glCreateShader(GL_FRAGMENT_SHADER);
        debugName = "Fragment Shader";
    }
    else if (type == GEOMETRY) 
    {
        vs = glCreateShader(GL_GEOMETRY_SHADER);
        debugName = "Geometry shader";
    }
    glShaderSource(vs, 1, &shader, &length);
    glCompileShader(vs);

    char output[1024] = { 0 };
    glGetShaderInfoLog(vs, 1024, &length, output);
    printf("%s compile log for %s: %s\n", debugName.c_str(), shaderName, output);

    return vs;
}


void initShaders()
{
    gProgram[0] = glCreateProgram();

    GLuint vs1 = createShader("vertex.glsl", VERTEX);
    GLuint fs1 = createShader("fragment.glsl", FRAGMENT);
    GLuint geo = createShader("geo.glsl", GEOMETRY);

    // Attach the shaders to the programs
    glAttachShader(gProgram[0], vs1);
    glAttachShader(gProgram[0], geo);
    glAttachShader(gProgram[0], fs1);
    assert(glGetError() == GL_NONE);

    // Link the programs
    GLint status;
    glLinkProgram(gProgram[0]);
    glGetProgramiv(gProgram[0], GL_LINK_STATUS, &status);
    if (status != GL_TRUE)
    {
        cout << "Program link failed. status:" << status<< endl;
        exit(-1);
    }

    // Get the locations of the uniform variables from both programs

    for (int i = 0; i < 1; ++i)
    {
        modelingMatrixLoc[i] = glGetUniformLocation(gProgram[i], "modelMat");
        cout << "modeling loc: " << modelingMatrixLoc[i] << endl;

        viewingMatrixLoc[i] = glGetUniformLocation(gProgram[i], "viewingMatrix");
        cout << "viewing loc: " << viewingMatrixLoc[i] << endl;

        projectionMatrixLoc[i] = glGetUniformLocation(gProgram[i], "projectionMatrix");
        cout << "projectionMatrix loc: " << projectionMatrixLoc[i] << endl;

        //eyePosLoc[i] = glGetUniformLocation(gProgram[i], "eyePos");
    }
}

GLuint hloc, wloc, sloc;

void init()
{
    glEnable(GL_DEPTH_TEST);
    initShaders();
    assert(glGetError() == GL_NONE);
    cout << "Shaders are initialized correctly.";

    activeProgramIndex = 0;
    setShaderParams(0);


    hloc = glGetUniformLocation(gProgram[0], "height");
    wloc = glGetUniformLocation(gProgram[0], "widthParam");
    sloc = glGetUniformLocation(gProgram[0], "sampleCount");

    // use actual camera setup determined via user interaction.
    viewingMatrix = glm::lookAt(cameraPos, cameraPos + cameraGaze, cameraUp);
    glViewport(0, 0, gWidth, gHeight); // reset viewport and projection to actual values.

    float aspect = (float)gWidth / (float)gHeight;
    projectionMatrix = glm::perspective(defaultFovyRad, aspect, 0.1f, 100.0f);
}

void display()
{
    // main render loop

    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    printFps();
    clearScreen();

    viewingMatrix = glm::lookAt(cameraPos, cameraPos + cameraGaze, cameraUp);
    modelingMatrix = glm::mat4(1.0f);
    modelingMatrix[0][0] = 1.0f;
    modelingMatrix[1][1] = 1.0f;
    modelingMatrix[2][2] = 1.0f;
    modelingMatrix[3][3] = 1.0f;

    setShaderParams(0);

    // send heightFactor
    glUniform1f(hloc, heightFactor);
    glUniform1i(wloc, widthParam);
    glUniform1i(sloc, sampleCount);


    GLuint terrainVBO;
    glGenVertexArrays(1, &terrainVAO);
    glGenBuffers(1, &terrainVBO);
    glBindVertexArray(terrainVAO);
    glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(terrainStartingPoint), &terrainStartingPoint, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    //glDrawArrays(GL_POINTS, 0, 3);
    glDrawArraysInstanced(GL_POINTS, 0, 1, sampleCount* sampleCount);
}

void clearScreen()
{
    glClearColor(0, 0.0, 0.05, 1);
    glClearDepth(1.0f);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void printFps()
{
    cout << "Fps:" << 1000 / (deltaTime * 1000) << endl;
}


/*
void drawTeapot()
{
    glm::mat4 teapotTranslation = glm::translate(glm::mat4(1.0), teapotPosition);
    float rotRad = (float)(centralObjectRotDeg / 180.f) * M_PI;
    glm::quat rotQuat(cos(rotRad / 2), 0, 1 * sin(rotRad / 2), 0);

    float scalef = 0.5f;
    glm::mat4 scale = glm::scale(glm::mat4(1.0), glm::vec3(scalef, scalef, scalef));
    modelingMatrix = teapotTranslation * glm::toMat4(rotQuat) * scale * glm::mat4(1.0);

    activeProgramIndex = 2;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, dynamicCubemapTexId);

    setShaderParams(activeProgramIndex);
    drawModel(teapotVAO, gFaces.size());
}*/

void setShaderParams(int index)
{
    glUseProgram(gProgram[index]);
    glUniformMatrix4fv(projectionMatrixLoc[index], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    glUniformMatrix4fv(viewingMatrixLoc[index], 1, GL_FALSE, glm::value_ptr(viewingMatrix));
    glUniformMatrix4fv(modelingMatrixLoc[index], 1, GL_FALSE, glm::value_ptr(modelingMatrix));
    //glUniform3fv(eyePosLoc[index], 1, glm::value_ptr(cameraPos));
}
void reshape(GLFWwindow* window, int w, int h)
{
    w = w < 1 ? 1 : w;
    h = h < 1 ? 1 : h;

    gWidth = w;
    gHeight = h;

    glViewport(0, 0, w, h);

    // Use perspective projection

    float aspect = (float)w / (float)h;
    projectionMatrix = glm::perspective(defaultFovyRad, aspect, 0.1f, 100.0f);

    // Assume default camera position and orientation (camera is at
    // (0, 0, 0) with looking at -z direction and its up vector pointing
    // at +y direction)

    lastX = w / 2;
    lastY = h / 2;

    viewingMatrix = glm::mat4(1);
}

void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        const float cameraSpeed = 150. * deltaTime;
        switch (key)
        {
        case GLFW_KEY_W:
            cameraPos += cameraSpeed * cameraGaze;
            break;
        case GLFW_KEY_S:
            cameraPos -= cameraSpeed * cameraGaze;
            break;
        case GLFW_KEY_A:
            cameraPos -= glm::normalize(glm::cross(cameraGaze, cameraUp)) * cameraSpeed;
            break;
        case GLFW_KEY_D:
            cameraPos += glm::normalize(glm::cross(cameraGaze, cameraUp)) * cameraSpeed;
            break;
        case GLFW_KEY_Q:
            sampleCount -= 100;
            if (sampleCount < 100) sampleCount = 100;

            //cameraPos -= cameraUp * cameraSpeed;
            break;
        case GLFW_KEY_E: // increase sample count
            sampleCount += 100;
            //cameraPos += cameraUp * cameraSpeed;
            break;
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        default:
            break;
        }
    }
}

void mainLoop(GLFWwindow* window)
{
    while (!glfwWindowShouldClose(window))
    {
        display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

int main(int argc, char** argv)   // Create Main Function For Bringing It All Together
{
    GLFWwindow* window;
    if (!glfwInit())
    {
        exit(-1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    int width = 800, height = 600;
    window = glfwCreateWindow(width, height, "HW3 Terrain Generation!", NULL, NULL);

    if (!window)
    {
        glfwTerminate();
        exit(-1);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Initialize GLEW to setup the OpenGL Function pointers
    if (GLEW_OK != glewInit())
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return EXIT_FAILURE;
    }

    char rendererInfo[512] = { 0 };
    strcpy_s(rendererInfo, (const char*)glGetString(GL_RENDERER));
    strcat_s(rendererInfo, " - ");
    strcat_s(rendererInfo, (const char*)glGetString(GL_VERSION));
    glfwSetWindowTitle(window, rendererInfo);

    init();

    glfwSetKeyCallback(window, keyboard);
    glfwSetWindowSizeCallback(window, reshape);
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    reshape(window, width, height); // need to call this once ourselves
    mainLoop(window); // this does not return unless the window is closed

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstTimeReceivingMouseInput)
    {
        lastX = xpos;
        lastY = ypos;
        firstTimeReceivingMouseInput = false;
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;
        lastX = xpos;
        lastY = ypos;

        float sensitivity = 0.1f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        mouseYaw += xoffset;
        mousePitch += yoffset;

        if (mousePitch > 89.0f)
            mousePitch = 89.0f;
        if (mousePitch < -89.0f)
            mousePitch = -89.0f;

        glm::vec3 direction;
        direction.x = cos(glm::radians(mouseYaw)) * cos(glm::radians(mousePitch));
        direction.y = sin(glm::radians(mousePitch));
        direction.z = sin(glm::radians(mouseYaw)) * cos(glm::radians(mousePitch));
        cameraGaze = glm::normalize(direction);
    }
    else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE) {
        firstTimeReceivingMouseInput = true;
    }

}
