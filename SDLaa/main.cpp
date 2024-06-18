/**
* Author: Dani Kim / dsk7781
* Assignment: Simple 2D Scene
* Date due: 2024-06-15, 11:59pm (extended to 2024-06-17, 11:59pm)
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum AppStatus { RUNNING, TERMINATED };

constexpr int WINDOW_WIDTH  = 640,
              WINDOW_HEIGHT = 480;

constexpr float BG_RED     = 0.830f,
                BG_GREEN   = 0.900f,
                BG_BLUE    = 0.820f,
                BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X      = 0,
              VIEWPORT_Y      = 0,
              VIEWPORT_WIDTH  = WINDOW_WIDTH,
              VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
               F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr GLint NUMBER_OF_TEXTURES = 1, // to be generated, that is
                LEVEL_OF_DETAIL    = 0, // mipmap reduction image level
                TEXTURE_BORDER     = 0; // this value MUST be zero

// sources: https://gamepress.gg/arknights/operator/silence, https://gamepress.gg/arknights/taxonomy/term/191
constexpr char SILENCE_SPRITE_FILEPATH[]   = "silence.png",
               DRONE_SPRITE_FILEPATH[] = "drone.png",
               LOGO_SPRITE_FILEPATH[] = "logo_rhine.png";


constexpr glm::vec3 INIT_SCALE_SILENCE      = glm::vec3(2.70f, 4.92f, 0.0f),
                    INIT_SCALE_DRONE      = glm::vec3(0.2f, 0.2f, 0.0f),
                    INIT_SCALE_LOGO      = glm::vec3(3.0f, 3.0f, 0.0f),
                    INIT_POS_SILENCE   = glm::vec3(0.0f, 0.0f, 0.0f),
                    INIT_POS_DRONE = glm::vec3(-2.0f, 0.0f, 0.0f),
                    INIT_POS_LOGO = glm::vec3(0.0f, 0.0f, 1.0f);

GLuint g_silence_texture_id;
GLuint g_drone_texture_id;
GLuint g_logo_texture_id;

float g_previous_ticks = 0.0f;

float g_rotation_drone_y = 0.0f;
float g_angle = 0.0f;
glm::vec3 g_translation_silence = glm::vec3(0.0f);
glm::vec3 g_translation_drone = glm::vec3(0.0f);

glm::mat4 g_view_matrix,
          g_silence_matrix,
          g_drone_matrix,
          g_logo_matrix,
          g_projection_matrix;

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();


GLuint load_texture(const char* filepath) // taking from Professor's stuff
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    
    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }
    
    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);
    
    return textureID;
}


void initialise()
{
    // Initialise video and joystick subsystems
    SDL_Init(SDL_INIT_VIDEO);
    
    g_display_window = SDL_CreateWindow("Silence's Incredible Growing/Shrinking Drone!",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);
    
    g_silence_matrix       = glm::mat4(1.0f);
    g_drone_matrix     = glm::mat4(1.0f);
    g_logo_matrix      = glm::mat4(1.0f);
    g_view_matrix       = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);
    
    glUseProgram(g_shader_program.get_program_id());
    
    glClearColor(BG_RED, BG_GREEN, BG_BLUE, BG_OPACITY);
    
    g_silence_texture_id   = load_texture(SILENCE_SPRITE_FILEPATH);
    g_drone_texture_id = load_texture(DRONE_SPRITE_FILEPATH);
    g_logo_texture_id  = load_texture(LOGO_SPRITE_FILEPATH);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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


void update()
{
    /* Delta time calculations */
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    
    /* Game logic */
    
    // for Silence/the character
    g_angle += 1.0f * delta_time;
    g_translation_silence.x = 1.0f * glm::cos(g_angle);
    g_translation_silence.y = 1.0f * glm::sin(g_angle);
    
    // for the drone
    g_rotation_drone_y += 50.0f * delta_time;
    g_translation_drone.x = 3.0f * glm::cos(g_angle);
    g_translation_drone.y = 0.6f * glm::sin(g_angle);

    float g_drone_scale = 1.0f - (sin(glm::radians(g_rotation_drone_y)) * 0.5f);
    
    // logo stays in the center as a background prop.
    
    /* Model matrix reset */
    g_silence_matrix = glm::mat4(1.0f);
    g_drone_matrix = glm::mat4(1.0f);
    g_logo_matrix = glm::mat4(1.0f);
    
    /* Transformations */
    g_silence_matrix = glm::translate(g_logo_matrix, g_translation_silence);
    g_silence_matrix = glm::scale(g_silence_matrix, INIT_SCALE_SILENCE);

    g_drone_matrix = glm::translate(g_drone_matrix, g_translation_drone);
    g_drone_matrix = glm::rotate(g_drone_matrix, glm::radians(g_rotation_drone_y), glm::vec3(0.0f, 0.0f, 1.0f));
    g_drone_matrix = glm::scale(g_drone_matrix, glm::vec3(g_drone_scale, g_drone_scale, 0.0f));
    
    // keep logo the same/in background
    g_logo_matrix = glm::translate(g_logo_matrix, INIT_POS_LOGO);
    g_logo_matrix = glm::scale(g_logo_matrix, INIT_SCALE_LOGO);
}


void draw_object(glm::mat4 &object_g_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so use 6, not 3
}


void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Vertices
    float vertices[] =
    {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] =
    {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };
    
    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false,
                          0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
    
    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT,
                          false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    
    // Bind texture
    draw_object(g_logo_matrix, g_logo_texture_id); // logo first for BG object
    draw_object(g_silence_matrix, g_silence_texture_id);
    
    
    // FANCY CODE - illusion of drone "disappearing" behind Silence in orbit
    if (g_translation_silence.y >= g_translation_drone.y) {

        draw_object(g_drone_matrix, g_drone_texture_id);
        draw_object(g_silence_matrix, g_silence_texture_id);
    } else {
        draw_object(g_silence_matrix, g_silence_texture_id);
        draw_object(g_drone_matrix, g_drone_texture_id);
    }
    
//    // NORMAL BEHAVIOR IN CASE THE OTHER ONE DOESN'T WORK/IS ILLEGAL - drone spins in front of Silence
//    draw_object(g_silence_matrix, g_silence_texture_id);
//    draw_object(g_drone_matrix, g_drone_texture_id);
    
    
    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    
    SDL_GL_SwapWindow(g_display_window);
}


void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    initialise();
    
    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }
    
    shutdown();
    return 0;
}
