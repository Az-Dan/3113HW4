/**
* Author: Dani Kim
* Assignment: Pong Clone
* Date due: 2024-06-29, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

// Saria is Player1/left bumper, Silence is Player2/right bumper

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
#include <iostream>
#include <ctime>
#include "cmath"

enum AppStatus {RUNNING, RUNNING_STASIS, TERMINATED};
// RUNNING_STASIS is for the start screen/the Game Over "X/Y won!" screen

constexpr int WINDOW_WIDTH = 640,
              WINDOW_HEIGHT = 480;

constexpr float WIDTH_BOUND = 4.5f;

constexpr float BG_RED     = 0.830f,
                BG_GREEN   = 0.900f,
                BG_BLUE    = 0.820f,
                BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
              VIEWPORT_Y = 0,
              VIEWPORT_WIDTH = WINDOW_WIDTH,
              VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr GLint NUMBER_OF_TEXTURES = 1, // to be generated, that is
                LEVEL_OF_DETAIL    = 0, // mipmap reduction image level
                TEXTURE_BORDER     = 0; // this value MUST be zero

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl";
constexpr char F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr char SARIA_SPRITE_FILEPATH[] = "assets/saria.png";
constexpr char SILENCE_SPRITE_FILEPATH[] = "assets/silence.png";
constexpr char LAWSUIT_SPRITE_FILEPATH[] = "assets/lawsuit.png";

constexpr float PLAYER_WIDTH = 1.0f;
constexpr float PLAYER_HEIGHT = 984 / 542.0f * PLAYER_WIDTH;
constexpr glm::vec3 PLAYER_INIT_SCALE = glm::vec3(PLAYER_WIDTH, PLAYER_HEIGHT, 0.0f);
constexpr glm::vec3 SARIA_INIT_POS = glm::vec3(-4.5f, 0.0f, 0.0f);
constexpr glm::vec3 SILENCE_INIT_POS = glm::vec3(4.5f, 0.0f, 0.0f);

constexpr float LAWSUIT_WIDTH[3] = { 0.5f, 0.5f, 0.5f };
constexpr float LAWSUIT_HEIGHT[3] = { LAWSUIT_WIDTH[0], LAWSUIT_WIDTH[1], LAWSUIT_WIDTH[2] };
constexpr glm::vec3 LAWSUIT_INIT_SCALE[3] = {
    glm::vec3(LAWSUIT_WIDTH[0], LAWSUIT_HEIGHT[0], 0.0f),
    glm::vec3(LAWSUIT_WIDTH[1], LAWSUIT_HEIGHT[1], 0.0f),
    glm::vec3(LAWSUIT_WIDTH[2], LAWSUIT_HEIGHT[2], 0.0f)
};

constexpr char WINNER_SARIA_SPRITE_FILEPATH[] = "assets/win_saria.png";
constexpr char WINNER_P2_SPRITE_FILEPATH[] = "assets/win_silence.png";
constexpr char START_SPRITE_FILEPATH[] = "assets/startscreen.png";
constexpr float FULLSCREEN_TEXT_SIZE = 6.0f;
constexpr glm::vec3 FULLSCREEN_TEXT_INIT_SCALE = glm::vec3(FULLSCREEN_TEXT_SIZE, FULLSCREEN_TEXT_SIZE, 0.0f);

constexpr char PLAYERMODE_SPRITE_FILEPATH[] = "assets/human.png";
constexpr char CPUMODE_SPRITE_FILEPATH[] = "assets/cpu.png";
constexpr float SIGN_DIM = 2.0f;
constexpr glm::vec3 PLAYERMODE_INIT_POS = glm::vec3(0.0f, 2.0f, 0.0f);
constexpr glm::vec3 PLAYERMODE_INIT_SCALE = glm::vec3(SIGN_DIM, SIGN_DIM, 0.0f);
constexpr glm::vec3 CPUMODE_INIT_POS = glm::vec3(0.0f, 2.0f, 0.0f);
constexpr glm::vec3 CPUMODE_INIT_SCALE = glm::vec3(SIGN_DIM, SIGN_DIM, 0.0f);


SDL_Window* g_display_window = nullptr;
AppStatus g_game_status = RUNNING_STASIS;
int g_game_mode = 0; // 2 is two players, 1 is one player
int g_game_winner = 0; // 0 is no winner, 1 is Player 1/Saria wins, 2 is Player 2/Silence wins
ShaderProgram g_shader_program = ShaderProgram();
const Uint8* key_state = SDL_GetKeyboardState(NULL);

float g_previous_ticks = 0.0f;
int g_lawsuit_ball_number = 1;

GLuint g_saria_texture_id,
       g_silence_texture_id,
       g_ball_suits_texture_id[3],
       g_playermode_texture_id,
       g_cpumode_texture_id,
       g_start_texture_id,
       g_winner_saria_texture_id,
       g_winner_silence_texture_id;

float g_player_speed = 5.0f;
float g_ball_speed = 1.0f;

glm::mat4 g_view_matrix,
          g_saria_matrix,
          g_silence_matrix,
          g_ball_suits_matrix[3],
          g_playermode_matrix,
          g_cpumode_matrix,
          g_start_matrix,
          g_winner_saria_matrix,
          g_winner_silence_matrix,
          g_projection_matrix;

glm::vec3 g_saria_position = glm::vec3(0.0f);
glm::vec3 g_silence_position = glm::vec3(0.0f);
glm::vec3 g_saria_movement = glm::vec3(0.0f);
glm::vec3 g_CPUsil_movement = glm::vec3(0.0f, 1.0f, 0.0f); // for when 't' key is pressed and Silence is moving by itself
glm::vec3 g_silence_movement = glm::vec3(0.0f);
glm::vec3 g_ball_suits_position[3] = { glm::vec3(0.0f) };
glm::vec3 g_ball_suits_movement[3] = { glm::vec3(0.0f) };


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

void initialize() 
{
    SDL_Init(SDL_INIT_VIDEO);

    g_display_window = SDL_CreateWindow("Saria v. Silence - Custody Lawsuit Pong!",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    for (int i = 0; i < 3; i++) {
            g_ball_suits_movement[i] = glm::vec3(3.0f, 1.0f, 0.0f); // controls for all 3 balls
    }
    g_view_matrix = glm::mat4(1.0f);
    g_saria_matrix = glm::mat4(1.0f);
    g_silence_matrix = glm::mat4(1.0f);
    for (int i = 0; i < g_lawsuit_ball_number; i++) {
        g_ball_suits_matrix[i] = glm::mat4(1.0f);
    }
    g_playermode_matrix = glm::mat4(1.0f);
    g_cpumode_matrix = glm::mat4(1.0f);
    g_start_matrix = glm::mat4(1.0f);
    g_winner_saria_matrix = glm::mat4(1.0f);
    g_winner_silence_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_GREEN, BG_BLUE, BG_OPACITY);

    g_saria_texture_id = load_texture(SARIA_SPRITE_FILEPATH);
    g_silence_texture_id = load_texture(SILENCE_SPRITE_FILEPATH);
    for (int i = 0; i < 3; i++) {
        g_ball_suits_texture_id[i] = load_texture(LAWSUIT_SPRITE_FILEPATH);
    }
    g_playermode_texture_id = load_texture(PLAYERMODE_SPRITE_FILEPATH);
    g_cpumode_texture_id = load_texture(CPUMODE_SPRITE_FILEPATH);
    g_start_texture_id = load_texture(START_SPRITE_FILEPATH);
    g_winner_saria_texture_id = load_texture(WINNER_SARIA_SPRITE_FILEPATH);
    g_winner_silence_texture_id = load_texture(WINNER_P2_SPRITE_FILEPATH);

    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input() {
    g_saria_movement = glm::vec3(0.0f);
    g_silence_movement = glm::vec3(0.0f);

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        { 
        // when you hit the X, you should expect the window to close
        case SDL_QUIT:
            g_game_status = TERMINATED;
            break;
                
        case SDL_WINDOWEVENT_CLOSE:
            g_game_status = TERMINATED;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
                case SDLK_t:
                    // switch between one and two player mode
                    if (g_game_mode == 1) {
                        g_game_mode = 2;
                    }
                    else {
                        g_game_mode = 1;
                    }
                    break;
                    
                case SDLK_1:
                    g_lawsuit_ball_number = 1;
                    break;
                    
                case SDLK_2:
                    if (g_lawsuit_ball_number == 1) {
                        // stops ball 2 from respawning annoyingly w/ keypresses/after switching from 3 balls
                        g_ball_suits_position[1] = glm::vec3(0.0f, 0.0f, 0.0f);
                    }
                    g_lawsuit_ball_number = 2;
                    break;
                    
                case SDLK_3:
                    if (g_lawsuit_ball_number == 2) {
                        // for whatever reason above IF also stops 2 balls from resetting when I press 3 w/ 2 balls on screen
                        g_ball_suits_position[2] = glm::vec3(0.0f, 0.0f, 0.0f);
                    }
                    else if (g_lawsuit_ball_number == 1) {
                        g_ball_suits_position[1] = glm::vec3(0.0f, 0.0f, 0.0f);
                        g_ball_suits_position[2] = glm::vec3(-1.0f, 0.0f, 0.0f);
                    }
                    g_lawsuit_ball_number = 3;
                    break;
                    
                default:
                    break;
            }

        default:
            break;
        }
    }
    
    if (key_state[SDL_SCANCODE_W])
    {
        g_saria_movement.y = 1.0f;
    }
    else if (key_state[SDL_SCANCODE_S])
    {
        g_saria_movement.y = -1.0f;
    }
    
    if (g_game_mode == 2) {
        if (key_state[SDL_SCANCODE_UP])
        {
            g_silence_movement.y = 1.0f;
        }
        else if (key_state[SDL_SCANCODE_DOWN])
        {
            g_silence_movement.y = -1.0f;
        }
    }

    // this is needed to avoid more jank!!
    if (glm::length(g_saria_movement) > 1.0f)
    {
        g_saria_movement = glm::normalize(g_saria_movement);
    }

    if (glm::length(g_silence_movement) > 1.0f)
    {
        g_silence_movement = glm::normalize(g_silence_movement);
    }
}

void update() {
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND; // get the current number of ticks
    float delta_time = ticks - g_previous_ticks; // the delta time is the difference from the last frame
    g_previous_ticks = ticks;

    if (g_game_status == RUNNING_STASIS) {
        g_winner_saria_matrix = glm::mat4(1.0f);
        g_winner_silence_matrix = glm::mat4(1.0f);
        g_start_matrix = glm::mat4(1.0f);
        
        if (g_game_winner == 1) {
            g_winner_saria_matrix = glm::scale(g_winner_saria_matrix, FULLSCREEN_TEXT_INIT_SCALE);
        }
        else if (g_game_winner == 2) {
            g_winner_silence_matrix = glm::scale(g_winner_silence_matrix, FULLSCREEN_TEXT_INIT_SCALE);
        }
        else {
            g_start_matrix = glm::scale(g_start_matrix, FULLSCREEN_TEXT_INIT_SCALE);
        }
        if (key_state[SDL_SCANCODE_SPACE]) {
            g_game_status = RUNNING;
            g_game_mode = 2;
            g_game_winner = 0;
            g_lawsuit_ball_number = 1;
            g_ball_suits_position[0] = glm::vec3(0.0f, 0.0f, 0.0f);
        }
    }
    else if (g_game_status == RUNNING) {
        if (g_game_mode == 1) {
            if (g_silence_position.y > 3.6f - PLAYER_HEIGHT / 2) {
                g_silence_position.y = 3.6f - PLAYER_HEIGHT / 2;
                g_CPUsil_movement.y *= -1;
            }
            else if (g_silence_position.y < -3.6f + PLAYER_HEIGHT / 2) {
                g_silence_position.y = -3.6f + PLAYER_HEIGHT / 2;
                g_CPUsil_movement.y *= -1;
            }
            g_silence_position += g_CPUsil_movement * 5.0f * delta_time;
        }
        else {
            if (g_silence_position.y >= 3.6f - PLAYER_HEIGHT / 2) {
                g_silence_position.y = 3.6f - PLAYER_HEIGHT / 2 - 0.005f;
            }
            else if (g_silence_position.y <= -3.6f + PLAYER_HEIGHT / 2) {
                g_silence_position.y = -3.6f + PLAYER_HEIGHT / 2 + 0.005f;
            }
            else {
                g_silence_position += g_silence_movement * g_player_speed * delta_time;
            }
        }
        if (g_saria_position.y > 3.6f - PLAYER_HEIGHT / 2) {
            g_saria_position.y = 3.6f - PLAYER_HEIGHT / 2;
        }
        else if (g_saria_position.y < -3.6f + PLAYER_HEIGHT / 2) {
            g_saria_position.y = -3.6f + PLAYER_HEIGHT / 2;
        }
        else {
            g_saria_position += g_saria_movement * g_player_speed * delta_time;
        }
        // ball
        for (int i = 0; i < g_lawsuit_ball_number; i++) {
            // Update ball position
            g_ball_suits_position[i] += g_ball_suits_movement[i] * g_ball_speed * delta_time;
            
            // Check for collision with top and bottom boundaries
            if (g_ball_suits_position[i].y + LAWSUIT_HEIGHT[i] / 2 >= 3.6f) {
                g_ball_suits_position[i].y = 3.6f - LAWSUIT_HEIGHT[i] / 2;
                g_ball_suits_movement[i].y *= -1;
            } else if (g_ball_suits_position[i].y - LAWSUIT_HEIGHT[i] / 2 <= -3.6f) {
                g_ball_suits_position[i].y = -3.6f + LAWSUIT_HEIGHT[i] / 2;
                g_ball_suits_movement[i].y *= -1;
            }
            
            // Check for collisions
            
            // SILENCE'S STUFF
            if (g_ball_suits_position[i].x + LAWSUIT_WIDTH[i] / 2 >= SILENCE_INIT_POS.x - PLAYER_WIDTH / 2) {
                if (g_ball_suits_position[i].y + LAWSUIT_HEIGHT[i] / 2 >= g_silence_position.y - PLAYER_HEIGHT / 2 &&
                    g_ball_suits_position[i].y - LAWSUIT_HEIGHT[i] / 2 <= g_silence_position.y + PLAYER_HEIGHT / 2) {
                    g_ball_suits_position[i].x = SILENCE_INIT_POS.x - PLAYER_WIDTH / 2 - LAWSUIT_WIDTH[i] / 2;
                    g_ball_suits_movement[i].x *= -1;
                } else if (g_ball_suits_position[i].x - LAWSUIT_WIDTH[i] / 2 > 4.5f) {
                    // Silence loses, Saria wins
                    g_game_status = RUNNING_STASIS;
                    g_game_winner = 1;
                }
            }
            
            // SARIA'S STUFF
            if (g_ball_suits_position[i].x - LAWSUIT_WIDTH[i] / 2 <= SARIA_INIT_POS.x + PLAYER_WIDTH / 2) {
                if (g_ball_suits_position[i].y + LAWSUIT_HEIGHT[i] / 2 >= g_saria_position.y - PLAYER_HEIGHT / 2 &&
                    g_ball_suits_position[i].y - LAWSUIT_HEIGHT[i] / 2 <= g_saria_position.y + PLAYER_HEIGHT / 2) {
                    g_ball_suits_position[i].x = SARIA_INIT_POS.x + PLAYER_WIDTH / 2 + LAWSUIT_WIDTH[i] / 2;
                    g_ball_suits_movement[i].x *= -1;
                } else if (g_ball_suits_position[i].x + LAWSUIT_WIDTH[i] / 2 < -4.5f) {
                    // Saria loses, Silence wins
                    g_game_status = RUNNING_STASIS;
                    g_game_winner = 2;
                }
            }
        }
        
        /* Model matrix reset */
        g_saria_matrix = glm::mat4(1.0f);
        g_silence_matrix = glm::mat4(1.0f);
        for (int i = 0; i < g_lawsuit_ball_number; i++) {
            g_ball_suits_matrix[i] = glm::mat4(1.0f);
        }
        g_playermode_matrix = glm::mat4(1.0f);
        g_cpumode_matrix = glm::mat4(1.0f);
        
        /* Transformations */
        for (int i = 0; i < g_lawsuit_ball_number; i++) {
            g_ball_suits_matrix[i] = glm::translate(g_ball_suits_matrix[i], g_ball_suits_position[i]);
            g_ball_suits_matrix[i] = glm::scale(g_ball_suits_matrix[i], LAWSUIT_INIT_SCALE[i]);
        }
        g_saria_matrix = glm::translate(g_saria_matrix, SARIA_INIT_POS);
        g_saria_matrix = glm::translate(g_saria_matrix, g_saria_position);
        g_saria_matrix = glm::scale(g_saria_matrix, PLAYER_INIT_SCALE);
        g_silence_matrix = glm::translate(g_silence_matrix, SILENCE_INIT_POS);
        g_silence_matrix = glm::translate(g_silence_matrix, g_silence_position);
        g_silence_matrix = glm::scale(g_silence_matrix, PLAYER_INIT_SCALE);
        
        g_playermode_matrix = glm::translate(g_playermode_matrix, PLAYERMODE_INIT_POS);
        g_playermode_matrix = glm::scale(g_playermode_matrix, PLAYERMODE_INIT_SCALE);
        
        g_cpumode_matrix = glm::translate(g_cpumode_matrix, CPUMODE_INIT_POS);
        g_cpumode_matrix = glm::scale(g_cpumode_matrix, CPUMODE_INIT_SCALE);

    }
}


void draw_object(glm::mat4& object_g_model_matrix, GLuint& object_texture_id) {
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void render() {
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
    if (g_game_status == RUNNING) {
        draw_object(g_saria_matrix, g_saria_texture_id);
        draw_object(g_silence_matrix, g_silence_texture_id);
        for (int i = 0; i < g_lawsuit_ball_number; i++) {
            draw_object(g_ball_suits_matrix[i], g_ball_suits_texture_id[i]);

        }
        if (g_game_mode == 2) {
            draw_object(g_playermode_matrix, g_playermode_texture_id);
        }
        else {
            draw_object(g_cpumode_matrix, g_cpumode_texture_id);
        }

    }
    else if (g_game_status == RUNNING_STASIS) {
        if (g_game_winner == 1) {
            draw_object(g_winner_saria_matrix, g_winner_saria_texture_id);
        }
        else if (g_game_winner == 2) {
            draw_object(g_winner_silence_matrix, g_winner_silence_texture_id);
        }
        else {
            draw_object(g_start_matrix, g_start_texture_id);
        }
    }
    
    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }

int main(int argc, char* argv[]) {
    initialize();

    while (g_game_status == RUNNING || g_game_status == RUNNING_STASIS)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
