/**
* Author: Dani Kim
* Assignment: Lunar Lander
* Date due: 2024-07-13, 11:59pm (submitted 2024-07-18 w/ extension)
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
#define ENTITY_COUNT 4


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
    Entity* collidable;
    // Entity* cake;
    // Entity* toxic;
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
               TOXIC_FILEPATH[] = "assets/toxic.png",
               FONT_FILEPATH[] = "assets/font1.png";

constexpr int NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL  = 0;
constexpr GLint TEXTURE_BORDER   = 0;

const int FONTBANK_SIZE   = 16;

constexpr int CD_QUAL_FREQ = 44100,  // compact disk (CD) quality frequency
          AUDIO_CHAN_AMT  = 2,
          AUDIO_BUFF_SIZE = 4096;

// ––––– GLOBAL VARIABLES ––––– //
GameState g_game_state;

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING_STASIS;
int g_game_winner = 0; // 0 is no winner, 1 is Accomplished, 2 is Failed
// Reusing the Pong stuff because I can

float g_fuel = 1400.0f;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;
//float g_rotation_drone_y = 0.0f;
//float g_angle = 0.0f;

GLuint g_bbus_texture_id,
       g_cake_texture_id,
       g_toxic_texture_id,
       g_font_texture_id;


// ———— GENERAL FUNCTIONS ———— //
GLuint load_texture(const char* filepath);

void initialise();
void process_input();
void update();
void render();
void shutdown();

void draw_text(ShaderProgram *program, GLuint font_texture_id, std::string text, float screen_size, float spacing, glm::vec3 position)
{
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    // Instead of having a single pair of arrays, we'll have a series of pairs—one for each character
    // Don't forget to include <vector>!
    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    // For every character...
    for (int i = 0; i < text.size(); i++) {
        // 1. Get their index in the spritesheet, as well as their offset (i.e. their position
        //    relative to the whole sentence)
        int spritesheet_index = (int) text[i];  // ascii value of character
        float offset = (screen_size + spacing) * i;
        
        // 2. Using the spritesheet index, we can calculate our U- and V-coordinates
        float u_coordinate = (float) (spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float) (spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

        // 3. Inset the current pair in both vectors
        vertices.insert(vertices.end(), {
            offset + (-0.5f * screen_size), 0.5f * screen_size,
            offset + (-0.5f * screen_size), -0.5f * screen_size,
            offset + (0.5f * screen_size), 0.5f * screen_size,
            offset + (0.5f * screen_size), -0.5f * screen_size,
            offset + (0.5f * screen_size), 0.5f * screen_size,
            offset + (-0.5f * screen_size), -0.5f * screen_size,
        });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
        });
    }

    // 4. And render all of them using the pairs
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);
    
    program->set_model_matrix(model_matrix);
    glUseProgram(program->get_program_id());
    
    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices.data());
    glEnableVertexAttribArray(program->get_position_attribute());
    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates.data());
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());
    
    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int) (text.size() * 6));
    
    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}

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
    
    // ––––– TOXIC ––––– //
    g_game_state.collidable = new Entity[ENTITY_COUNT];
    GLuint toxic_texture_id = load_texture(TOXIC_FILEPATH);
    
    for (int i = 0; i < HAZARD_COUNT; i++) {
        g_game_state.collidable[i] = Entity(
                                            toxic_texture_id, // texture id
                                            0.0f,              // speed
                                            1.0f,              // width
                                            1.0f,               // height
                                            1 // type
                                            );
    };
    
    g_game_state.collidable[0].set_position(glm::vec3(2.9f, -1.0f, 0.0f));
    g_game_state.collidable[1].set_position(glm::vec3(0.1f, 0.7f, 0.0f));
    g_game_state.collidable[2].set_position(glm::vec3(-2.8f, -0.4f, 0.0f));
    
    //for (int i = 0; i < HAZARD_COUNT; i++) {
    
    // THIS IS A LITTLE INSANE BUT I CAN'T GET IT ANY OTHER WAY
        g_game_state.collidable[0].update(0.0f, NULL, 0, 1);
        g_game_state.collidable[1].update(0.0f, NULL, 0, 1);
        g_game_state.collidable[2].update(0.0f, NULL, 0, 1);
        g_game_state.collidable[0].set_scale(glm::vec3(1.05f, 1.5f, 0.0f));
        g_game_state.collidable[1].set_scale(glm::vec3(1.05f, 1.5f, 0.0f));
        g_game_state.collidable[2].set_scale(glm::vec3(1.05f, 1.5f, 0.0f));

    //};

    // ––––– CAKE ––––– //
    // g_game_state.cake = new Entity[CAKE_COUNT];
    GLuint cake_texture_id = load_texture(CAKE_FILEPATH);
    
    // for (int i = 0; i < CAKE_COUNT; i++)
        g_game_state.collidable[3] = Entity(
                                      cake_texture_id, // texture id
                                      0.0f,              // speed
                                      1.0f,              // width
                                      1.0f,               // height
                                      2   // type
                                      );
    
    g_game_state.collidable[3].set_position(glm::vec3(0.0f, -3.5f, 0.0f));
    g_game_state.collidable[3].update(0.0f, NULL, 0, 2);
    g_game_state.collidable[3].set_scale(glm::vec3(3.0f, 2.125f, 0.0f));
    
    
    // ––––– BBUS ––––– //
    GLuint bbus_texture_id = load_texture(BBUS_FILEPATH);
    
    g_game_state.bbus = new Entity(
                                   bbus_texture_id,         // texture id
                                   1.0f,                      // speed
                                   0.5f,                      // width
                                   0.5f,                       // height
                                   0 // type
                                   );
    
    g_game_state.bbus->set_position(glm::vec3(0.0f, 2.8f, 0.0f));
    
    // ––––– FONT ––––– //
    g_font_texture_id = load_texture(FONT_FILEPATH);
    
    
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
                g_app_status = TERMINATED;
                break;
                    
            case SDL_WINDOWEVENT_CLOSE:
                g_app_status = TERMINATED;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        // Quit the game with a keystroke
                        g_app_status = TERMINATED;
                        break;
                        
                    default:
                        break;
                }
                
            default:
                break;
        }
    }
    
    const Uint8* key_state = SDL_GetKeyboardState(NULL);
    if (g_fuel > 0 && g_game_winner == 0) { // if g_game_winner is any other number, game is over
        // I have wrestled w/ Entity enough to not want to do a restartable game this time. Apologies
        if (key_state[SDL_SCANCODE_LEFT]) {
            g_game_state.bbus->move_left();
            g_fuel -= 1;
        }
        else if (key_state[SDL_SCANCODE_RIGHT]) {
            g_game_state.bbus->move_right();
            g_fuel -= 1;
        }
        else if (key_state[SDL_SCANCODE_UP]) {
            g_game_state.bbus->move_up();
            g_fuel -= 2;
        }
        else if (key_state[SDL_SCANCODE_DOWN]) { // screw it, you can accelerate your fall too
            g_game_state.bbus->move_down();
            g_fuel -= 1;
        }
        
        else if (key_state[SDL_SCANCODE_SPACE]) g_app_status = RUNNING;
    }
    
    
    if (glm::length(g_game_state.bbus->get_movement()) > 1.0f)
            g_game_state.bbus->normalise_movement();
}


void update() {
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    
    delta_time += g_accumulator;
    
    if (delta_time < FIXED_TIMESTEP) {
        g_accumulator = delta_time;
        return;
    }
    
    while (delta_time >= FIXED_TIMESTEP) {
        if (g_app_status == RUNNING) {
            g_game_state.bbus->update(FIXED_TIMESTEP, g_game_state.collidable, 1, 0);
            
            // THIS IS SO INCREDIBLY STUPID BUT IT WORKS
            // I WISH THIS WAS SIMPLER BUT I DON'T THINK A FOR LOOP WOULD HELP ME IN THIS CASE

            // Check for collision with specific entities
            if (g_game_state.bbus->check_collision(&g_game_state.collidable[0])) {
                // Nuke barrel 1
                g_game_winner = 2; // Fail
                g_app_status = RUNNING_STASIS;
                break;
            }
            else if (g_game_state.bbus->check_collision(&g_game_state.collidable[1])) {
                // Nuke barrel 2
                g_game_winner = 2; // Fail
                g_app_status = RUNNING_STASIS;
                break;
            }
            else if (g_game_state.bbus->check_collision(&g_game_state.collidable[2])) {
                // Nuke barrel 3
                g_game_winner = 2; // Fail
                g_app_status = RUNNING_STASIS;
                break;
            }
            else if (g_game_state.bbus->check_collision(&g_game_state.collidable[3])) {
                // THE CAKE
                g_game_winner = 1; // WIN
                g_app_status = RUNNING_STASIS;
                break;
            }
            

            // Check for collision with floor/walls (which are failstates)
            if (g_game_state.bbus->get_position().y <= -3.75f) {
                g_game_winner = 2; // Fail
                g_app_status = RUNNING_STASIS;
                break;
            }
            else if (g_game_state.bbus->get_position().y >= 3.75f) {
                g_game_winner = 2; // Fail
                g_app_status = RUNNING_STASIS;
                break;
            }
            else if (g_game_state.bbus->get_position().x >= 5.0f) {
                g_game_winner = 2; // Fail
                g_app_status = RUNNING_STASIS;
                break;
            }
            else if (g_game_state.bbus->get_position().x <= -5.0f) {
                g_game_winner = 2; // Fail
                g_app_status = RUNNING_STASIS;
                break;
            }
            
            // Update entities only if the game is running
            for (int i = 0; i < ENTITY_COUNT; i++) {
                g_game_state.collidable[i].update(FIXED_TIMESTEP, NULL, 0, 0);
            }
        } else if (g_app_status == RUNNING_STASIS) {
            // Update entities with zero delta time to retain their positions
            g_game_state.bbus->update(0.0f, g_game_state.collidable, ENTITY_COUNT, 0);
            for (int i = 0; i < ENTITY_COUNT; ++i) {
                g_game_state.collidable[i].update(0.0f, NULL, 0, 0);
            }
        }

        delta_time -= FIXED_TIMESTEP;
    }

    g_accumulator = delta_time;
}



void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    
    
//    for (int i = 0; i < CAKE_COUNT; i++)
//        g_game_state.cake[i].render(&g_shader_program);
    
    for (int i = 0; i < ENTITY_COUNT; i++) {
        g_game_state.collidable[i].render(&g_shader_program);
    };
    g_game_state.bbus->render(&g_shader_program);
    
    if (g_app_status == RUNNING_STASIS) {
        //draw_text(ShaderProgram *program, GLuint font_texture_id, std::string text, float screen_size, float spacing, glm::vec3 position)
            if (g_game_winner == 1) { // win
                draw_text(&g_shader_program, g_font_texture_id, "Birthday Saved!", 0.4f, 0.05f,
                          glm::vec3(-3.25f, 1.75f, 0.0f));
                draw_text(&g_shader_program, g_font_texture_id, "Press Q to quit", 0.3f, 0.05f,
                          glm::vec3(-2.6f, 0.0f, 0.0f));
                // this text might not be perfectly centered but I tried
            }
            else if (g_game_winner == 2) { // fail
                draw_text(&g_shader_program, g_font_texture_id, "Birthday Ruined!", 0.4f, 0.05f,
                          glm::vec3(-3.45f, 1.75f, 0.0f));
                draw_text(&g_shader_program, g_font_texture_id, "Press Q to quit", 0.3f, 0.05f,
                          glm::vec3(-2.6f, 0.0f, 0.0f));
            }
            else {
                draw_text(&g_shader_program, g_font_texture_id, "Fortnite B-Day Lander!", 0.4f, 0.05f,
                          glm::vec3(-4.7f, 1.75f, 0.0f));
                draw_text(&g_shader_program, g_font_texture_id, "Press SPACE to start", 0.3f, 0.05f,
                          glm::vec3(-3.45f, 0.0f, 0.0f));
            }
        }
    else if (g_app_status == RUNNING) {
        // g_fuel = 240;
        if (g_fuel >= 800) { // illusion of half tank lol
            draw_text(&g_shader_program, g_font_texture_id, " HI-FUEL", 0.3f, 0.05f,
                      glm::vec3(-5.0f, -2.0f, 0.0f));
        }
        else if (g_fuel > 0) {
            draw_text(&g_shader_program, g_font_texture_id, " LO-FUEL", 0.3f, 0.05f,
                      glm::vec3(-5.0f, -2.0f, 0.0f));
        }
        else {
            draw_text(&g_shader_program, g_font_texture_id, " NO-FUEL", 0.3f, 0.05f,
                      glm::vec3(-5.0f, -2.0f, 0.0f));
        }
//        if (g_game_winner != 0) {
//            g_app_status == RUNNING_STASIS; 
//        }
    }
    
    SDL_GL_SwapWindow(g_display_window);
}
    

void shutdown()
{
    SDL_Quit();
    
    // delete [] g_game_state.cake;
    delete g_game_state.bbus;
   //  delete [] g_game_state.toxic;
    delete [] g_game_state.collidable;
}

// ––––– GAME LOOP ––––– //
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
