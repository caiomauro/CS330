#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>      // Image loading Utility functions

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnOpengl/camera.h> // Camera class

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Tutorial 6.3"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao;         // Handle for the vertex array object
        GLuint vbo;         // Handle for the vertex buffer object
        GLuint nVertices;    // Number of indices of the mesh
        GLuint vbos[2];
        GLuint nIndices;
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;
    // Triangle mesh data
    GLMesh gMesh;
    // Texture
    GLuint gTextureId;
    GLuint gTextureIdDesk;
    GLuint gTextureIdMonitor;
    GLuint gTextureIdStand;
    GLuint gTextureIdKeyboard;
    glm::vec2 gUVScale(5.0f, 5.0f);
    GLint gTexWrapMode = GL_REPEAT;

    // Shader programs
    GLuint gCubeProgramId;
    GLuint gLampProgramId;

    // camera
    Camera gCamera(glm::vec3(0.0f, 0.0f, 7.0f));
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

    // Subject position and scale
    glm::vec3 gCubePosition(0.0f, 0.0f, 0.0f);
    glm::vec3 gCubeScale(2.0f);

    // Cube and light color
    //m::vec3 gObjectColor(0.6f, 0.5f, 0.75f);
    glm::vec3 gObjectColor(1.f, 0.2f, 0.0f);
    glm::vec3 gLightColor(1.0f, 1.0f, 1.0f);

    // Light position and scale
    glm::vec3 gLightPosition(1.0f, 0.5f, 1.0f);
    glm::vec3 gLightScale(10.0f);

    // Lamp animation
    bool gIsLampOrbiting = true;
}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UCreateMesh(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);


/* Cube Vertex Shader Source Code*/
const GLchar* cubeVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
layout(location = 1) in vec3 normal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;
layout(location = 3) in float textureType;

out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate;
out float vertexTextureType;

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

    vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

    vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
    vertexTextureCoordinate = textureCoordinate;
    vertexTextureType = textureType;
}
);


/* Cube Fragment Shader Source Code*/
const GLchar* cubeFragmentShaderSource = GLSL(440,

    in vec3 vertexNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position
in vec2 vertexTextureCoordinate;
in float vertexTextureType;

out vec4 fragmentColor; // For outgoing cube color to the GPU

// Uniform / Global variables for object color, light color, light position, and camera/view position
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPosition;
uniform sampler2D uTexture; // Useful when working with multiple textures
uniform sampler2D uTextureDesk; // New texture sampler
uniform sampler2D uTextureMonitor; // New texture sampler
uniform sampler2D uTextureStand;
uniform sampler2D uTextureKeyboard;
uniform vec2 uvScale;

void main()
{
    /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

    //Calculate Ambient lighting*/
    float ambientStrength = 0.7f; // Set ambient or global lighting strength
    vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

    //Calculate Diffuse lighting*/
    vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
    vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    vec3 diffuse = impact * lightColor; // Generate diffuse light color

    //Calculate Specular lighting*/
    float specularIntensity = 1.0f; // Set specular light strength
    float highlightSize = 10.0f; // Set specular highlight size
    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
    //Calculate specular component
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    vec3 specular = specularIntensity * specularComponent * lightColor;

    // Texture holds the color to be used for all three components
    vec4 textureColor;

    if (vertexTextureType == 0)
    {
        textureColor = texture(uTextureDesk, vertexTextureCoordinate * uvScale);
    }
    else if (vertexTextureType == 0.1)
    {
        textureColor = texture(uTextureMonitor, vertexTextureCoordinate * uvScale);
    }
    else if (vertexTextureType == 0.2)
    {
        textureColor = texture(uTextureStand, vertexTextureCoordinate * uvScale);
    }
    else if (vertexTextureType == 0.3)
    {
        textureColor = texture(uTextureKeyboard, vertexTextureCoordinate * uvScale);
    }
    else {
        textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);
    }

    // Calculate phong result
    vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

    fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
}
);


/* Lamp Shader Source Code*/
const GLchar* lampVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);


/* Fragment Shader Source Code*/
const GLchar* lampFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

void main()
{
    fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);


// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}


int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Create the mesh
    UCreateMesh(gMesh); // Calls the function to create the Vertex Buffer Object

    // Create the shader programs
    if (!UCreateShaderProgram(cubeVertexShaderSource, cubeFragmentShaderSource, gCubeProgramId))
        return EXIT_FAILURE;

    if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLampProgramId))
        return EXIT_FAILURE;

    // Load texture
    const char* texFilename = "../../resources/textures/mouse.jpg";
    if (!UCreateTexture(texFilename, gTextureId))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }

    // Load the desk texture
    texFilename = "../../resources/textures/desk.jpg"; // Adjust the path as necessary
    if (!UCreateTexture(texFilename, gTextureIdDesk))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }

    // Load the desk texture
    texFilename = "../../resources/textures/display.png"; // Adjust the path as necessary
    if (!UCreateTexture(texFilename, gTextureIdMonitor))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }

    // Load the desk texture
    texFilename = "../../resources/textures/stand.jpg"; // Adjust the path as necessary
    if (!UCreateTexture(texFilename, gTextureIdStand))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }

    // Load the desk texture
    texFilename = "../../resources/textures/keyboard.jpg"; // Adjust the path as necessary
    if (!UCreateTexture(texFilename, gTextureIdKeyboard))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }


    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gCubeProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gCubeProgramId, "uTexture"), 0);
    // We set the desk texture as texture unit 1
    glUniform1i(glGetUniformLocation(gCubeProgramId, "uTextureDesk"), 1);
    // We set the desk texture as texture unit 2
    glUniform1i(glGetUniformLocation(gCubeProgramId, "uTextureMonitor"), 2);
    // We set the desk texture as texture unit 3
    glUniform1i(glGetUniformLocation(gCubeProgramId, "uTextureStand"), 3);
    // We set the desk texture as texture unit 4
    glUniform1i(glGetUniformLocation(gCubeProgramId, "uTextureKeyboard"), 4);

    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
        // --------------------
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        URender();

        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(gMesh);

    // Release texture
    UDestroyTexture(gTextureId);
    UDestroyTexture(gTextureIdDesk);
    UDestroyTexture(gTextureIdMonitor);
    UDestroyTexture(gTextureIdStand);
    UDestroyTexture(gTextureIdKeyboard);

    // Release shader programs
    UDestroyShaderProgram(gCubeProgramId);
    UDestroyShaderProgram(gLampProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a windowdddddddddddddddddddd
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);
    glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    static const float cameraSpeed = 2.5f;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS && gTexWrapMode != GL_REPEAT)
    {
        glBindTexture(GL_TEXTURE_2D, gTextureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glBindTexture(GL_TEXTURE_2D, 0);

        gTexWrapMode = GL_REPEAT;

        cout << "Current Texture Wrapping Mode: REPEAT" << endl;
    }
    else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS && gTexWrapMode != GL_MIRRORED_REPEAT)
    {
        glBindTexture(GL_TEXTURE_2D, gTextureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        glBindTexture(GL_TEXTURE_2D, 0);

        gTexWrapMode = GL_MIRRORED_REPEAT;

        cout << "Current Texture Wrapping Mode: MIRRORED REPEAT" << endl;
    }
    else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS && gTexWrapMode != GL_CLAMP_TO_EDGE)
    {
        glBindTexture(GL_TEXTURE_2D, gTextureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        gTexWrapMode = GL_CLAMP_TO_EDGE;

        cout << "Current Texture Wrapping Mode: CLAMP TO EDGE" << endl;
    }
    else if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS && gTexWrapMode != GL_CLAMP_TO_BORDER)
    {
        float color[] = { 1.0f, 0.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color);

        glBindTexture(GL_TEXTURE_2D, gTextureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glBindTexture(GL_TEXTURE_2D, 0);

        gTexWrapMode = GL_CLAMP_TO_BORDER;

        cout << "Current Texture Wrapping Mode: CLAMP TO BORDER" << endl;
    }

    if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS)
    {
        gUVScale += 0.1f;
        cout << "Current scale (" << gUVScale[0] << ", " << gUVScale[1] << ")" << endl;
    }
    else if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS)
    {
        gUVScale -= 0.1f;
        cout << "Current scale (" << gUVScale[0] << ", " << gUVScale[1] << ")" << endl;
    }

    // Pause and resume lamp orbiting
    static bool isLKeyDown = false;
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS && !gIsLampOrbiting)
        gIsLampOrbiting = true;
    else if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS && gIsLampOrbiting)
        gIsLampOrbiting = false;

    // Handle upward movement using Q key
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.ProcessKeyboard(UPWARD, gDeltaTime);

    // Handle downward movement using E key
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.ProcessKeyboard(DOWNWARD, gDeltaTime);

}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera.ProcessMouseScroll(yoffset);
}

// glfw: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
    {
        if (action == GLFW_PRESS)
            cout << "Left mouse button pressed" << endl;
        else
            cout << "Left mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_MIDDLE:
    {
        if (action == GLFW_PRESS)
            cout << "Middle mouse button pressed" << endl;
        else
            cout << "Middle mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_RIGHT:
    {
        if (action == GLFW_PRESS)
            cout << "Right mouse button pressed" << endl;
        else
            cout << "Right mouse button released" << endl;
    }
    break;

    default:
        cout << "Unhandled mouse button event" << endl;
        break;
    }
}


// Functioned called to render a frame
void URender()
{
    // Lamp orbits around the origin
    const float angularVelocity = glm::radians(45.0f);
    if (gIsLampOrbiting)
    {
        glm::vec4 newPosition = glm::rotate(angularVelocity * gDeltaTime, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(gLightPosition, 3.0f);
        gLightPosition.x = newPosition.x;
        gLightPosition.y = newPosition.y;
        gLightPosition.z = newPosition.z;
    }

    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Activate the cube VAO (used by cube and lamp)
    glBindVertexArray(gMesh.vao);

    // CUBE: draw cube
    //----------------
    // Set the shader to be used
    glUseProgram(gCubeProgramId);

    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = glm::translate(gCubePosition) * glm::scale(gCubeScale);

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gCubeProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gCubeProgramId, "view");
    GLint projLoc = glGetUniformLocation(gCubeProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    GLint objectColorLoc = glGetUniformLocation(gCubeProgramId, "objectColor");
    GLint lightColorLoc = glGetUniformLocation(gCubeProgramId, "lightColor");
    GLint lightPositionLoc = glGetUniformLocation(gCubeProgramId, "lightPos");
    GLint viewPositionLoc = glGetUniformLocation(gCubeProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    GLint UVScaleLoc = glGetUniformLocation(gCubeProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gTextureIdDesk);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gTextureIdMonitor);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gTextureIdStand);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, gTextureIdKeyboard);

    // Draws the triangle
    glDrawElements(GL_TRIANGLES, gMesh.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}


// Implements the UCreateMesh function
void UCreateMesh(GLMesh& mesh)
{
    // Calculate normals for the cube vertices
    glm::vec3 cubeFrontNormal = glm::cross(glm::vec3(0.5f, 0.2f, -0.3f) - glm::vec3(-0.5f, -0.2f, -0.3f), glm::vec3(-0.5f, 0.2f, -0.3f) - glm::vec3(-0.5f, -0.2f, -0.3f));
    glm::vec3 cubeBackNormal = glm::cross(glm::vec3(-0.5f, -0.2f, 0.3f) - glm::vec3(0.5f, 0.2f, 0.3f), glm::vec3(0.5f, -0.2f, 0.3f) - glm::vec3(0.5f, 0.2f, 0.3f));

    // Calculate normals for the cylinder vertices
    glm::vec3 cylinderTopNormal = glm::cross(glm::vec3(0.1f, 0.15f, 0.1f) - glm::vec3(0.3f, 0.25f, 0.0f), glm::vec3(0.5f, 0.15f, 0.1f) - glm::vec3(0.3f, 0.25f, 0.0f));
    glm::vec3 cylinderBottomNormal = glm::cross(glm::vec3(0.3f, 0.25f, 0.0f) - glm::vec3(0.1f, 0.15f, 0.1f), glm::vec3(0.5f, 0.15f, 0.1f) - glm::vec3(0.1f, 0.15f, 0.1f));

    // Calculate normals for the plane vertices
    glm::vec3 planeNormal = glm::cross(glm::vec3(1.5f, -0.25f, -1.3f) - glm::vec3(-1.5f, -0.25f, -1.3f), glm::vec3(-1.5f, -0.25f, 1.3f) - glm::vec3(-1.5f, -0.25f, -1.3f));

    // Normalize the normals
    cubeFrontNormal = glm::normalize(cubeFrontNormal);
    cubeBackNormal = glm::normalize(cubeBackNormal);
    cylinderTopNormal = glm::normalize(cylinderTopNormal);
    cylinderBottomNormal = glm::normalize(cylinderBottomNormal);
    planeNormal = glm::normalize(planeNormal);

    GLfloat verts[] = {
        // Cube vertices (Mouse Body)
        -0.5f, -0.2f, -0.3f, planeNormal.x, planeNormal.y, planeNormal.z, 1.0f, 0.0f, 2.0f, // Vertex 0
        0.5f, -0.2f, -0.3f, planeNormal.x, planeNormal.y, planeNormal.z, 0.0f, 0.0f, 2.0f, // Vertex 1
        0.5f, 0.2f, -0.3f, planeNormal.x, planeNormal.y, planeNormal.z, 0.0f, 1.0f, 2.0f, // Vertex 2
        -0.5f, 0.2f, -0.3f, planeNormal.x, planeNormal.y, planeNormal.z, 1.0f, 1.0f, 2.0f, // Vertex 3

        -0.5f, -0.2f, 0.3f, planeNormal.x, planeNormal.y, planeNormal.z, 0.0f, 0.0f, 2.0f, // Vertex 4
        0.5f, -0.2f, 0.3f, planeNormal.x, planeNormal.y, planeNormal.z, 1.0f, 0.0f, 2.0f, // Vertex 5
        0.5f, 0.2f, 0.3f, planeNormal.x, planeNormal.y, planeNormal.z, 1.0f, 1.0f, 2.0f, // Vertex 6
        -0.5f, 0.2f, 0.3f, planeNormal.x, planeNormal.y, planeNormal.z, 0.0f, 1.0f, 2.0f, // Vertex 7

        // Cylinder vertices (Scroll Wheel)
        0.3f, 0.25f, 0.0f, planeNormal.x, planeNormal.y, planeNormal.z, 0.5f, 1.0f, 2.0f, // Vertex 8 (Top)
        0.1f, 0.15f, 0.1f, planeNormal.x, planeNormal.y, planeNormal.z, 0.0f, 0.0f, 2.0f, // Vertex 9 (Bottom Left)
        0.5f, 0.15f, 0.1f, planeNormal.x, planeNormal.y, planeNormal.z, 1.0f, 0.0f, 2.0f, // Vertex 10 (Bottom Right)

        // Cube vertices (Left Button)
        0.05f, 0.15f, 0.35f, planeNormal.x, planeNormal.y, planeNormal.z, 0.0f, 0.0f, 2.0f, // Vertex 11
        -0.15f, 0.15f, 0.35f, planeNormal.x, planeNormal.y, planeNormal.z, 1.0f, 0.0f, 2.0f, // Vertex 12
        -0.15f, 0.0f, 0.35f, planeNormal.x, planeNormal.y, planeNormal.z, 1.0f, 1.0f, 2.0f, // Vertex 13
        0.05f, 0.0f, 0.35f, planeNormal.x, planeNormal.y, planeNormal.z, 0.0f, 1.0f, 2.0f, // Vertex 14

        // Cube vertices (Right Button)
        0.1f, 0.15f, 0.35f,planeNormal.x, planeNormal.y, planeNormal.z, 0.0f, 0.0f, 2.0f, // Vertex 15
        0.3f, 0.15f, 0.35f,planeNormal.x, planeNormal.y, planeNormal.z, 1.0f, 0.0f, 2.0f, // Vertex 16
        0.3f, 0.0f, 0.35f, planeNormal.x, planeNormal.y, planeNormal.z,  1.0f, 1.0f, 2.0f, // Vertex 17
        0.1f, 0.0f, 0.35f, planeNormal.x, planeNormal.y, planeNormal.z,  0.0f, 1.0f, 2.0f, // Vertex 18

        //Plane
        -3.5f, -0.25f, -3.3f, planeNormal.x, planeNormal.y, planeNormal.z, 0.0f, -0.01f, 0.0f, // Vertex 19
        3.5f, -0.25f, -3.3f, planeNormal.x, planeNormal.y, planeNormal.z, 1.0f, -0.01f, 0.0f, // Vertex 20
        -3.5f, -0.25f, 3.3f, planeNormal.x, planeNormal.y, planeNormal.z, 1.0f, -1.0f, 0.0f, // Vertex 21
        3.5f, -0.25f, 3.3f, planeNormal.x, planeNormal.y, planeNormal.z, 0.0f, -1.0f, 0.0f, // Vertex 22

        //Monitor
        -2.5f, 0.3f, -1.5f, planeNormal.x, planeNormal.y, planeNormal.z, 0.0f, 0.0f, 0.1f, // Vertex 23
        2.5f, 0.3f, -1.5f, planeNormal.x, planeNormal.y, planeNormal.z, 1.0f, 1.0f, 0.1f, // Vertex 24
        2.5f, 2.6f, -1.5f, planeNormal.x, planeNormal.y, planeNormal.z, 1.0f, 0.0f, 0.1f, // Vertex 25
        -2.5f, 2.6f, -1.5f, planeNormal.x, planeNormal.y, planeNormal.z, 0.0f, 1.0f, 0.1f, // Vertex 26

        -2.5f, 0.3f, -1.0f, planeNormal.x, planeNormal.y, planeNormal.z, 0.0f, 0.0, 0.1f, // Vertex 27
        2.5f, 0.3f, -1.0f, planeNormal.x, planeNormal.y, planeNormal.z, 1.0f, 1.0f, 0.1f, // Vertex 28
        2.5f, 2.6f, -1.0f, planeNormal.x, planeNormal.y, planeNormal.z, 1.0f, 0.0f, 0.1f, // Vertex 29
        -2.5f, 2.6f, -1.0f, planeNormal.x, planeNormal.y, planeNormal.z, 0.0f, 1.0f, 0.1f, // Vertex 30

        //Stand
        -0.5f, -0.3f, -1.5f, planeNormal.x, planeNormal.y, planeNormal.z, 0.0f, -2.5f, 0.2f, // Vertex 31
        0.5f, -0.3f, -1.5f, planeNormal.x, planeNormal.y, planeNormal.z, 1.0f, -2.5f, 0.2f, // Vertex 32
        0.5f, 0.3f, -1.5f, planeNormal.x, planeNormal.y, planeNormal.z, 1.0f, -2.5f, 0.2f, // Vertex 33
        -0.5f, 0.3f, -1.5f, planeNormal.x, planeNormal.y, planeNormal.z, 0.0f, -2.5f, 0.2f, // Vertex 34

        -0.5f, -0.3f, -1.0f, planeNormal.x, planeNormal.y, planeNormal.z, 0.0f, -2.5f, 0.2f, // Vertex 35
        0.5f, -0.3f, -1.0f, planeNormal.x, planeNormal.y, planeNormal.z, 1.0f, -2.5f, 0.2f, // Vertex 36
        0.5f, 0.3f, -1.0f, planeNormal.x, planeNormal.y, planeNormal.z, 1.0f, -2.5f, 0.2f, // Vertex 37
        -0.5f, 0.3f, -1.0f, planeNormal.x, planeNormal.y, planeNormal.z, 0.0f, -2.5f, 0.2f, // Vertex 38

        //Keyboard
        -1.0f, -0.3f, 2.0f, planeNormal.x, planeNormal.y, planeNormal.z, 1.0f, 0.0f, 0.3f, // Vertex 39
        1.0f, -0.3f, 2.0f, planeNormal.x, planeNormal.y, planeNormal.z, 0.0f, 1.0f, 0.3f, // Vertex 40
        1.0f, 0.0f, 2.0f, planeNormal.x, planeNormal.y, planeNormal.z, 0.0f, 0.0f, 0.3f, // Vertex 41
        -1.0f, 0.0f, 2.0f, planeNormal.x, planeNormal.y, planeNormal.z, 1.0f, 0.0f, 0.3f, // Vertex 42

        -1.0f, -0.3f, 1.0f, planeNormal.x, planeNormal.y, planeNormal.z, 1.0f, 0.0f, 0.3f, // Vertex 43
        1.0f, -0.3f, 1.0f, planeNormal.x, planeNormal.y, planeNormal.z, 0.0f, 0.0f, 0.3f, // Vertex 44
        1.0f, 0.0f, 1.0f, planeNormal.x, planeNormal.y, planeNormal.z, 0.0f, 1.0f, 0.3f, // Vertex 45
        -1.0f, 0.0f, 1.0f, planeNormal.x, planeNormal.y, planeNormal.z, 1.0f, 0.0f, 0.3f, // Vertex 46

        // Lightbar
        -1.8f, 2.5f, -1.0f, planeNormal.x, planeNormal.y, planeNormal.z, 0.0f, -2.5f, 0.2f, // Vertex 47
        1.8f, 2.5f, -1.0f, planeNormal.x, planeNormal.y, planeNormal.z, 1.0f, 3.5f, 0.2f, // Vertex 48
        1.8f, 2.7f, -1.0f, planeNormal.x, planeNormal.y, planeNormal.z, 1.0f, 3.5f, 0.2f, // Vertex 49
        -1.8f, 2.7f, -1.0f, planeNormal.x, planeNormal.y, planeNormal.z, 0.0f, 3.5f, 0.2f, // Vertex 50

        -1.8f, 2.5f, -0.8f, planeNormal.x, planeNormal.y, planeNormal.z, 0.0f, 3.5f, 0.2f, // Vertex 51
        1.8f, 2.5f, -0.8f, planeNormal.x, planeNormal.y, planeNormal.z, 1.0f, 3.5f, 0.2f, // Vertex 52
        1.8f, 2.7f, -0.8f, planeNormal.x, planeNormal.y, planeNormal.z, 1.0f, 3.5f, 0.2f, // Vertex 53
        -1.8f, 2.7f, -0.8f, planeNormal.x, planeNormal.y, planeNormal.z, 0.0f, 3.5f, 0.2f, // Vertex 54
    };


    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;
    const GLuint floatsPerTextureType = 1;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV + floatsPerTextureType));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Data for the indices
    // Index data to share position data
    GLushort indices[] = {
        // Mouse Body (Cube) indices
        0, 1, 2,  // Front Face
        0, 2, 3,  // Front Face
        4, 5, 6,  // Back Face
        4, 6, 7,  // Back Face
        0, 4, 5,  // Left Face
        0, 5, 1,  // Left Face
        3, 7, 6,  // Right Face
        3, 6, 2,  // Right Face
        0, 3, 7,  // Top Face
        0, 7, 4,  // Top Face
        1, 2, 6,  // Bottom Face
        1, 6, 5,  // Bottom Face

        // Scroll Wheel (Cylinder) indices
        8, 9, 10, // Scroll Wheel Body

        // Left Button (Cube) indices
        11, 12, 13,  // Front Face
        11, 13, 14,  // Front Face

        // Right Button (Cube) indices
        15, 16, 17,  // Front Face
        15, 17, 18,  // Front Face

        //Plane
        19,20,21,
        20,21,22,

        //monitor
        23, 24, 25, // Front Face
        23, 25, 26, // Front Face
        27, 28, 29, // Back Face
        27, 29, 30, // Back Face
        23, 27, 28, // Left Face
        23, 28, 24, // Left Face
        26, 30, 29, // Right Face
        26, 29, 25, // Right Face
        23, 26, 30, // Top Face
        23, 30, 27, // Top Face
        24, 25, 29, // Bottom Face
        24, 29, 28, // Bottom Face

        //stand
        31, 32, 33,  // Front Face
        31, 33, 34,  // Front Face
        35, 36, 37,  // Back Face
        35, 37, 38,  // Back Face
        31, 35, 36,  // Left Face
        31, 36, 32,  // Left Face
        34, 38, 37,  // Right Face
        34, 37, 33,  // Right Face
        31, 34, 38,  // Top Face
        31, 38, 35,  // Top Face
        32, 33, 37,  // Bottom Face
        32, 37, 36,  // Bottom Face

        //Keyboard
        39, 40, 41,  // Front Face
        39, 41, 42,  // Front Face
        43, 44, 45,  // Back Face
        43, 45, 46,  // Back Face
        39, 43, 44,  // Left Face
        39, 44, 40,  // Left Face
        42, 46, 45,  // Right Face
        42, 45, 41,  // Right Face
        39, 42, 46,  // Top Face
        39, 46, 43,  // Top Face
        40, 41, 45,  // Bottom Face
        40, 45, 44,   // Bottom Face

        // Lightbar
        47, 48, 49, // Front Face
        47, 49, 50, // Front Face
        51, 52, 53, // Back Face
        51, 53, 54, // Back Face
        47, 51, 52, // Left Face
        47, 52, 48, // Left Face
        50, 54, 53, // Right Face
        50, 53, 49, // Right Face
        47, 50, 54, // Top Face
        47, 54, 51, // Top Face
        48, 49, 53, // Bottom Face
        48, 53, 52 // Bottom Face
    };

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Strides between vertex coordinates
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV + floatsPerTextureType);

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerNormal));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, floatsPerTextureType, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV)));
    glEnableVertexAttribArray(3);
}


void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(1, &mesh.vbo);
}


/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}


void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}


// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}
