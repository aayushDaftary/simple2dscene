/**
* Author: Aayush Daftary
* Assignment: Simple 2D Scene
* Date due: 2025-02-15, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"                // 4x4 Matrix
#include "glm/gtc/matrix_transform.hpp"  // Matrix transformation methods
#include "ShaderProgram.h"               // We'll talk about these later in the course
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

enum AppStatus { RUNNING, TERMINATED };

// Our window dimensions
constexpr int WINDOW_WIDTH  = 640,
              WINDOW_HEIGHT = 480;

// Background color components
constexpr float BG_RED     = 0.1922f,
                BG_BLUE    = 0.549f,
                BG_GREEN   = 0.9059f,
                BG_OPACITY = 1.0f;

// Our viewport—or our "camera"'s—position and dimensions
constexpr int VIEWPORT_X      = 0,
              VIEWPORT_Y      = 0,
              VIEWPORT_WIDTH  = WINDOW_WIDTH,
              VIEWPORT_HEIGHT = WINDOW_HEIGHT;

// Our shader filepaths; these are necessary for a number of things
// Not least, to actually draw our shapes
// We'll have a whole lecture on these later
constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
               F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

// Our object's fill colour
//constexpr float TRIANGLE_RED     = 1.0,
//                TRIANGLE_BLUE    = 0.4,
//                TRIANGLE_GREEN   = 0.4,
//                TRIANGLE_OPACITY = 1.0;

constexpr float MILLISECONDS_IN_SECOND = 1000.0f;

AppStatus g_app_status = RUNNING;
SDL_Window* g_display_window;

ShaderProgram g_shader_program = ShaderProgram();

glm::mat4 g_view_matrix,        // Defines the position (location and orientation) of the camera
          g_model_matrixA,       // Defines every translation, rotation, and/or scaling applied to an object; we'll look at these next week
          g_model_matrixB,
          g_projection_matrix;  // Defines the characteristics of your camera, such as clip panes, field of view, projection method, etc.
float g_previous_ticks = 0.0f;
float g_rotationAngleA = 0.0f;
float g_rotationAngleB = 0.0f;
float g_scaleFactorB   = 1.0f;
float g_radiusA = 2;
float g_radiusB = 1;
float g_frames = 0;
constexpr float SPEED = 2.0f;
GLuint textureA;
GLuint textureB;
constexpr int NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL = 0;
constexpr GLint TEXTURE_BORDER = 0;
glm::vec3 g_positionA = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_positionB = glm::vec3(0.0f, 0.0f, 0.0f);


GLuint LoadTexture(const char* filePath) {
    int width, height, components;
    unsigned char* image = stbi_load(filePath, &width, &height, &components, STBI_rgb_alpha);
    if (image == nullptr) {
        std::cerr << "Unable to load image " << filePath << "\n";
        exit(1);
    }
    
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    stbi_image_free(image);
    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Simple 2d Scene",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    
    if (g_display_window == nullptr)
    {
        std::cerr << "ERROR: SDL Window could not be created.\n";
        g_app_status = TERMINATED;
        
        SDL_Quit();
        exit(1);
    }
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    // Initialise our camera
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    // Load up our shaders
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);
    
    // Initialise our view, model, and projection matrices
    g_view_matrix       = glm::mat4(1.0f);  // Defines the position (location and orientation) of the camera
    g_model_matrixA      = glm::mat4(1.0f);  // Defines every translation, rotations, or scaling applied to an object
    g_model_matrixB      = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);  // Defines the characteristics of your camera, such as clip planes, field of view, projection method etc.
    
    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);
    // Notice we haven't set our model matrix yet!
    
    //g_shader_program.set_colour(TRIANGLE_RED, TRIANGLE_BLUE, TRIANGLE_GREEN, TRIANGLE_OPACITY);
    
    // Each object has its own unique ID
    glUseProgram(g_shader_program.get_program_id());
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    textureA = LoadTexture("texture1.png");
    textureB = LoadTexture("texture2.png");
}

void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            g_app_status = TERMINATED;
        }
    }
}

void update() {
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    //g_frames++;
    //g_rotationAngleA += delta_time * SPEED;
    g_positionA.x = g_radiusA * cos(ticks);
    g_positionA.y = g_radiusA * sin(ticks);
    g_rotationAngleA += delta_time * glm::radians(90.0f);
    g_model_matrixA = glm::mat4(1.0f);
    g_model_matrixA = glm::translate(g_model_matrixA, g_positionA);
    //g_model_matrixA = glm::rotate(g_model_matrixA, g_rotationAngleA, glm::vec3(0.0f, 0.0f, 1.0f));
    
    
    g_positionB.x = g_radiusB * cos(ticks * 2.0);
    g_positionB.y = g_radiusB * sin(ticks * 2.0);
    g_rotationAngleB += delta_time * glm::radians(180.0f);
    g_scaleFactorB = 1.0f + 0.5f * sin(ticks * 2.0f);
        
    g_model_matrixB = g_model_matrixA;
    g_model_matrixB = glm::translate(g_model_matrixB, g_positionB);
    g_model_matrixB = glm::rotate(g_model_matrixB, g_rotationAngleB, glm::vec3(0.0f, 0.0f,  1.0f));
    g_model_matrixB = glm::scale(g_model_matrixB, glm::vec3(g_scaleFactorB, g_scaleFactorB, 1.0f));
}

void draw_object(glm::mat4 &object_g_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so use 6, not 3
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    float texCoords[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };
    //a
    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, GL_FALSE, 0, texCoords);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    //g_shader_program.set_model_matrix(g_model_matrixA);
    //glBindTexture(GL_TEXTURE_2D, textureA);
    draw_object(g_model_matrixA, textureA);
    draw_object(g_model_matrixB, textureB);
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }

/**
 Start here—we can see the general structure of a game loop without worrying too much about the details yet.
 */
int main(int argc, char* argv[])
{
    // Initialise our program—whatever that means
    initialise();
    
    while (g_app_status == RUNNING)
    {
        process_input();  // If the player did anything—press a button, move the joystick—process it
        update();         // Using the game's previous state, and whatever new input we have, update the game's state
        render();         // Once updated, render those changes onto the screen
    }
    
    shutdown();  // The game is over, so let's perform any shutdown protocols
    return 0;
}
