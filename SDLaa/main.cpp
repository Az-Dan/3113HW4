/**
* Author: Dani KIm
* Assignment: Rise of the AI
* Date due: 2024-07-27, 11:59pm (submitted 2024-07-31 ]w/ extension)
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
// #define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define ENEMY_COUNT 3
#define LEVEL1_WIDTH 14
#define LEVEL1_HEIGHT 5

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL_mixer.h>
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
#include "Map.hpp"
#include "Utility.hpp"

// ————— GAME STATE ————— //
struct GameState
{
    Entity *player;
    Entity *enemies;
    
    Map *map;
    
    Mix_Music *bgm;
    Mix_Chunk *jump_sfx;
    Mix_Chunk *death_sfx;
    Mix_Chunk *shoot_sfx;
};

enum AppStatus { RUNNING, RUNNING_STASIS, TERMINATED };

// ————— CONSTANTS ————— //
constexpr int WINDOW_WIDTH  = 640 * 1.5,
          WINDOW_HEIGHT = 480 * 1.5;

constexpr float BG_RED     = 0.1922f,
            BG_BLUE    = 0.549f,
            BG_GREEN   = 0.9059f,
            BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char GAME_WINDOW_NAME[] = "Roomba Murder Sim";

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr char SPRITESHEET_FILEPATH[] = "assets/fella.png",
           MAP_TILESET_FILEPATH[] = "assets/marble.png",
           ENEMY_FILEPATH[]       = "assets/roomba.png",
           BULLET_FILEPATH[]      = "assets/hadoblast.png",
           ENEMYBULLET_FILEPATH[] = "assets/darkhado.png",
           BGM_FILEPATH[]         = "assets/ska.mp3",
           JUMP_SFX_FILEPATH[]    = "assets/horridjump.mp3",
           DEATH_SFX_FILEPATH[]   = "assets/thwack.mp3",
           SHOOT_SFX_FILEPATH[]   = "assets/quack.mp3",
           FONT_FILEPATH[]        = "assets/font1.png";

// credit 2 https://timbojay.itch.io/lil-guy-base for spritesheet, the goat Kevin MacLeod for BGM

constexpr int NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL  = 0;
constexpr GLint TEXTURE_BORDER   = 0;


// SFX
constexpr int PLAY_ONCE   =  0,
          NEXT_CHNL   = -1,  // next available channel
          MUTE_VOL    =  0,
          MILS_IN_SEC =  1000,
          ALL_SFX_CHN = -1;

unsigned int LEVEL_1_DATA[] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
    2, 2, 1, 0, 0, 0, 1, 1, 1, 0, 0, 2, 2, 2,
    2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2
};

// ————— VARIABLES ————— //
GameState g_game_state;

int g_game_winner = 0; // 0 is no winner, 1 is Accomplished, 2 is Failed 

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f,
      g_accumulator    = 0.0f,
      g_textpos_x      = 0.0f,
      g_textpos_y      = 0.0f;

GLuint g_font_texture_id;
int g_enemies_left = ENEMY_COUNT;
bool g_coward_fell = false;

void initialise();
void process_input();
void update();
void render();
void shutdown();

// ————— GENERAL FUNCTIONS ————— //

void initialise()
{
    // ————— GENERAL ————— //
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    g_display_window = SDL_CreateWindow(GAME_WINDOW_NAME,
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
    
    // ————— VIDEO SETUP ————— //
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);
    
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());
    
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    // ————— MAP SET-UP ————— //
    GLuint map_texture_id = Utility::load_texture(MAP_TILESET_FILEPATH);
    g_game_state.map = new Map(LEVEL1_WIDTH, LEVEL1_HEIGHT, LEVEL_1_DATA, map_texture_id, 1.0f, 9, 9);
    
    // ————— GEORGE SET-UP ————— //

    GLuint player_texture_id = Utility::load_texture(SPRITESHEET_FILEPATH);

    int player_walking_animation[4][3] =
    {
        { 6, 7, 8 },  // for George to move to the left,
        { 9, 10, 11 }, // for George to move to the right,
        { 0, 1, 2 }, // for George to move upwards,
        { 3, 4, 5 }   // for George to move downwards
    };

    glm::vec3 acceleration = glm::vec3(0.0f,-4.905f, 0.0f);

    g_game_state.player = new Entity(
        player_texture_id,         // texture id
        5.0f,                      // speed
        acceleration,              // acceleration
        3.0f,                      // jumping power
        player_walking_animation,  // animation index sets
        0.0f,                      // animation time
        3,                         // animation frame amount
        0,                         // current animation index
        3,                         // animation column amount
        4,                         // animation row amount
        0.5f,                      // width
        1.0f,                       // height
        PLAYER
    );

    g_game_state.player->set_position(glm::vec3(1.0f, 1.0f, 0.0f));

    // Jumping
    g_game_state.player->set_jumping_power(5.0f);
    g_game_state.player->set_enemy_amt(ENEMY_COUNT);
    
    
    g_game_state.enemies = new Entity[ENEMY_COUNT];
    GLuint enemy_texture_id = Utility::load_texture(ENEMY_FILEPATH);
    for (int i = 0; i < ENEMY_COUNT; ++i) {
        //     Entity(GLuint texture_id, float speed, float width, float height, EntityType EntityType, AIType AIType, AIState AIState); // AI constructor
        g_game_state.enemies[i] =  Entity(enemy_texture_id, 1.0f, 1.0f, 1.0f, ENEMY, GUARD, IDLE);
        //g_game_state.enemies[i].set_scale(glm::vec3(0.5f));
     }
    
    // flyer
    //g_game_state.enemies[0].set_jumping_power(0.5f);
    g_game_state.enemies[0].set_ai_type(FLOATER);
    //g_game_state.enemies[0].set_ai_state(FLOATING);
    g_game_state.enemies[0].set_position(glm::vec3(10.0f, 2.0f, 0.0f));
    g_game_state.enemies[0].set_width(0.5f);
    g_game_state.enemies[0].set_height(0.5f);
    //g_game_state.enemies[0].set_movement(glm::vec3(0.0f));
    //g_game_state.enemies[0].set_texture_id(player_texture_id);
    g_game_state.enemies[0].set_acceleration(glm::vec3(0.09f, 0.0f, 0.0f));


    // coward
    g_game_state.enemies[1].set_ai_type(COWARD);
    g_game_state.enemies[1].set_position(glm::vec3(7.0f, 1.0f, 0.0f));
    g_game_state.enemies[1].set_width(0.5f);
    g_game_state.enemies[1].set_height(0.5f);
    g_game_state.enemies[1].set_movement(glm::vec3(0.0f));
    g_game_state.enemies[1].set_acceleration(glm::vec3(0.0f, -4.905f, 0.0f));


    // jumper - technically 2nd flyer??? something has gone wrong
    g_game_state.enemies[2].set_jumping_power(0.1f);
    g_game_state.enemies[2].set_ai_type(JUMPER);
    g_game_state.enemies[2].set_ai_state(JUMPING);
    g_game_state.enemies[2].set_position(glm::vec3(2.0f, -1.0f, 0.0f));
    g_game_state.enemies[2].set_width(0.5f);
    g_game_state.enemies[2].set_height(0.5f);
    g_game_state.enemies[2].set_movement(glm::vec3(0.0f));
    g_game_state.enemies[2].set_acceleration(glm::vec3(0.0f, -4.905f, 0.0f));

    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
    
    g_game_state.bgm = Mix_LoadMUS(BGM_FILEPATH);
    Mix_PlayMusic(g_game_state.bgm, -1);
    Mix_VolumeMusic(MIX_MAX_VOLUME / 16.0f);
    
    g_game_state.jump_sfx = Mix_LoadWAV(JUMP_SFX_FILEPATH);
    g_game_state.death_sfx = Mix_LoadWAV(DEATH_SFX_FILEPATH);
    g_game_state.shoot_sfx = Mix_LoadWAV(SHOOT_SFX_FILEPATH);
    
    // other stuff??
    g_font_texture_id = Utility::load_texture(FONT_FILEPATH);
    
    // ————— BLENDING ————— //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    g_game_state.player->set_movement(glm::vec3(0.0f));
    
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
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
                        
                    case SDLK_UP:
                        // Jump
                        if (g_game_state.player->get_map_collided_bottom() || g_game_state.player->get_collided_bottom() )
                        {
                            g_game_state.player->jump();
                            Mix_PlayChannel(-1, g_game_state.jump_sfx, 0);
                        }
                        break;
                    
                    case SDLK_SPACE:
                        // SHOOT
                        break;
                        
                    default:
                        break;
                }
                
            default:
                break;
        }
    }
    
    const Uint8 *key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_LEFT])       g_game_state.player->move_left();
    else if (key_state[SDL_SCANCODE_RIGHT]) g_game_state.player->move_right();
    else if (key_state[SDL_SCANCODE_Q]) g_app_status = TERMINATED;
    // plan B to fix fringe case where quitting takes a bit to work??

    
    if (glm::length(g_game_state.player->get_movement()) > 1.0f)
        g_game_state.player->normalise_movement();
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
        g_game_state.player->update(FIXED_TIMESTEP, g_game_state.player, g_game_state.enemies, ENEMY_COUNT,
                                    g_game_state.map);
        
        if (g_game_state.enemies[1].get_position().y <= -3.75f) { // necessary so that when coward falls over, he dies
            g_game_state.enemies[1].deactivate();
            g_coward_fell = true; // insane workarounds are needed. help
              //--g_enemies_left;
            //g_game_winner = 1;
            //break;
        }
        
        if (g_enemies_left >= g_game_state.player->get_enemy_amt()) {
            g_enemies_left = g_game_state.player->get_enemy_amt();
        }
        
        if (g_game_state.player->get_position().y <= -3.75f) { // off chance that bro falls into a pit
            //g_game_state.player->deactivate();
            g_game_winner = 2; // Fail
            g_app_status = RUNNING_STASIS;
           // bingus = true;
          // Mix_PlayChannel(-1, g_game_state.death_sfx, 0);
            //break;
        }
        

        
        for (int i = 0; i < ENEMY_COUNT; i++) {
            //    void update(float delta_time, Entity *player, Entity *collidable_entities, int collidable_entity_count, Map *map);
            g_game_state.enemies[i].update(FIXED_TIMESTEP, g_game_state.player, g_game_state.player, 1, g_game_state.map);
        }
        
        if (!g_game_state.player->get_activity()) { // player is dead
            g_game_winner = 2; // Fail
            g_app_status = RUNNING_STASIS;
        }
        
        if (g_enemies_left == 0 || (g_coward_fell && g_enemies_left == 1)) {
            g_game_winner = 1; // Win!
            g_app_status = RUNNING_STASIS;
            g_game_state.player->deactivate();
            g_game_state.player->set_movement(glm::vec3(0.0f));
            g_game_state.player->set_acceleration(glm::vec3(0.0f, 0.0f, 0.0f));            //break;
        }
        
        delta_time -= FIXED_TIMESTEP;
    }
    
    g_accumulator = delta_time;
    
    g_view_matrix = glm::mat4(1.0f);
    g_view_matrix = glm::translate(g_view_matrix, glm::vec3(-g_game_state.player->get_position().x, 0.0f, 0.0f));
}

void render()
{
    g_shader_program.set_view_matrix(g_view_matrix);
    
    glClear(GL_COLOR_BUFFER_BIT);

    
    g_game_state.player->render(&g_shader_program);
    
    for (int i = 0; i < ENEMY_COUNT; i++) {
        g_game_state.enemies[i].render(&g_shader_program);
    }
     
    g_game_state.map->render(&g_shader_program);
    
    if (g_app_status == RUNNING_STASIS) {
        if (g_game_winner == 2) {
            g_textpos_x = g_game_state.player->get_position().x - 2.0f;
            Utility::draw_text(&g_shader_program, g_font_texture_id, "YOU LOSE", 0.5f, -0.05f,
                glm::vec3(g_textpos_x, g_textpos_y, 0.0f));
            // Mix_PlayChannel(-1, g_game_state.death_sfx, 0);
        }
        else if (g_game_winner == 1) {
            g_textpos_x = g_game_state.player->get_position().x - 2.0f;
            Utility::draw_text(&g_shader_program, g_font_texture_id, "YOU WIN", 0.5f, -0.05f,
                glm::vec3(g_textpos_x, g_textpos_y, 0.0f));
        }
    }

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();
    
    delete [] g_game_state.enemies;
    delete    g_game_state.player;
    delete    g_game_state.map;
    Mix_FreeChunk(g_game_state.jump_sfx);
    Mix_FreeChunk(g_game_state.death_sfx);
    Mix_FreeChunk(g_game_state.shoot_sfx);
    Mix_FreeMusic(g_game_state.bgm);
}

// ————— GAME LOOP ————— //
int main(int argc, char* argv[])
{
    initialise();
    
    while (g_app_status == RUNNING || g_app_status == RUNNING_STASIS)
    {
        process_input();
        update();
        render();
    }
    
    shutdown();
    return 0;
}
