/**
* Author: Dani Kim
* Assignment: Lunar Lander
* Date due: 2024-07-13, 11:59pm (submitted 2024-07-16 w/ extension)
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define CAKE_COUNT 1
#define HAZARD_COUNT 3

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include "Entity.hpp"
// Including the audio library
#include <SDL_mixer.h>

// ––––– STRUCTS AND ENUMS ––––– //
enum AppStatus {RUNNING, RUNNING_STASIS, TERMINATED};
// RUNNING_STASIS is for the start screen/the Game Over "Mission Accomplished/Failed" screen

struct GameState
{
    Entity* bbus;
    Entity* cake;
    Entity* toxin;
};

// ––––– CONSTANTS ––––– //
constexpr int WINDOW_WIDTH  = 640 * 1.5,
          WINDOW_HEIGHT = 480 * 1.5;

constexpr float BG_RED     = 0.830f,
                BG_GREEN   = 0.900f,
                BG_BLUE    = 0.820f,
                BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;
constexpr char BBUS_FILEPATH[] = "assets/battlebus.png",
               CAKE_FILEPATH[]    = "assets/cake.png",
               TOXIC_FILEPATH[] = "assets/toxic.png";

constexpr int NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL  = 0;
constexpr GLint TEXTURE_BORDER   = 0;

const int FONTBANK_SIZE   = 16;

// THIS IS THE STUFF THAT APPLIES FOR THE LANDER SPECIFICALLY
constexpr float GRAVITY = 0.0005f;

constexpr int CD_QUAL_FREQ = 44100,  // compact disk (CD) quality frequency
          AUDIO_CHAN_AMT  = 2,
          AUDIO_BUFF_SIZE = 4096;

// ––––– GLOBAL VARIABLES ––––– //
GameState g_game_state;

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING_STASIS;
int g_game_winner = 0; // 0 is no winner, 1 is Accomplished, 2 is Failed
// Reusing the Pong stuff because I can

int g_fuel = 500;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;
//float g_rotation_drone_y = 0.0f;
//float g_angle = 0.0f;

// ———— GENERAL FUNCTIONS ———— //
GLuint load_texture(const char* filepath);

void initialise();
void process_input();
void update();
void render();
void shutdown();

// ––––– GENERAL FUNCTIONS ––––– //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    
    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }
    
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    stbi_image_free(image);
    
    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    
    // Start Audio
    Mix_OpenAudio(
        CD_QUAL_FREQ,        // the frequency to playback audio at (in Hz)
        MIX_DEFAULT_FORMAT,  // audio format
        AUDIO_CHAN_AMT,      // number of channels (1 is mono, 2 is stereo, etc).
        AUDIO_BUFF_SIZE      // audio buffer size in sample FRAMES (total samples divided by channel count)
        );
    
    g_display_window = SDL_CreateWindow("Save Timmy's Birthday!",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (context == nullptr)
    {
        LOG("ERROR: Could not create OpenGL context.\n");
        shutdown();
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    // ––––– CAKE ––––– //
    g_game_state.cake = new Entity[CAKE_COUNT];
    GLuint teacup_texture_id = load_texture(CAKE_FILEPATH);
    
    for (int i = 0; i < CAKE_COUNT; i++)
        g_game_state.cake[i] = Entity(
            teacup_texture_id, // texture id
            0.0f,              // speed
            1.0f,              // width
            1.0f               // height
        );
    
    g_game_state.cake[0].set_position(glm::vec3(0.0f, -2.5f, 0.0f));
    g_game_state.cake[0].update(0.0f, NULL, 0);
    
    g_game_state.cake[0].set_scale(glm::vec3(3.0f, 2.125f, 0.0f));

    // ––––– BBUS ––––– //
    GLuint bbus_texture_id = load_texture(BBUS_FILEPATH);

    g_game_state.bbus = new Entity(
        bbus_texture_id,         // texture id
        1.0f,                      // speed
        0.5f,                      // width
        0.5f                       // height
    );
    
    g_game_state.bbus->set_position(glm::vec3(0.0f, 2.5f, 0.0f));
    
    // ––––– GENERAL ––––– //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    g_game_state.bbus->set_movement(glm::vec3(0.0f));
    
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            // End game
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_app_status = TERMINATED;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        // Quit the game with a keystroke
                        g_app_status = TERMINATED;
                        break;
                        
                    case SDLK_SPACE:
                        g_game_state.bbus->move_up();
                        /** PART I: Let go of the bbus */
                        break;
                        
                    default:
                        break;
                }
                
            default:
                break;
        }
    }
    
    const Uint8* key_state = SDL_GetKeyboardState(NULL);

//    if (key_state[SDL_SCANCODE_LEFT])       g_game_state.bbus->move_left();
//    else if (key_state[SDL_SCANCODE_RIGHT]) g_game_state.bbus->move_right();
//    else if (key_state[SDL_SCANCODE_UP]) g_game_state.bbus->move_up();
    
    if (glm::length(g_game_state.bbus->get_movement()) > 1.0f)
            g_game_state.bbus->normalise_movement();
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    
    delta_time += g_accumulator;
    
    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }
    
    while (delta_time >= FIXED_TIMESTEP)
    {
        g_game_state.bbus->update(FIXED_TIMESTEP, g_game_state.cake, CAKE_COUNT);
        g_game_state.cake[0].update(0.0f, NULL, 0);
        delta_time -= FIXED_TIMESTEP;
    }
    
    g_accumulator = delta_time;
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    
    g_game_state.bbus->render(&g_shader_program);
    
    for (int i = 0; i < CAKE_COUNT; i++)
        g_game_state.cake[i].render(&g_shader_program);
    
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();
    
    delete [] g_game_state.cake;
    delete g_game_state.bbus;
}

// ––––– GAME LOOP ––––– //
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
