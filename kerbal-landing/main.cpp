/**
* Author: Ben Miller
* Assignment: Lunar Lander
* Date due: 2024-09-03, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define LOG(argument) std::cout << argument << '\n'
#define STB_IMAGE_IMPLEMENTATION
#define GL_SILENCE_DEPRECATION
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
#include "cmath"
#include <ctime>
#include <vector>
#include "Entity.h"

// ————— STRUCTS AND ENUMS —————//
struct GameState
{
    Entity* background;
    Entity* terrain;
    Entity* player;
    Entity* flame;
    Entity* landingPads;
    Entity* letters;
    Entity* endText;
};

// ————— CONSTANTS ————— //

// window size
const int WINDOW_WIDTH = 800,
          WINDOW_HEIGHT = 600;

// background color
const float BG_RED = 0.1922f,
            BG_BLUE = 0.549f,
            BG_GREEN = 0.9059f,
            BG_OPACITY = 1.0f;

// viewport position & size
const int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

// shader filepaths
const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

// sprite filepaths
const char BACKGROUND_FILEPATH[] = "assets/background.png",
           TERRAIN_FILEPATH[] = "assets/terrain.png",
           PLAYER_FILEPATH[] = "assets/kerbal_head.png",
           FLAME_FILEPATH[] = "assets/flame.png",
           LANDINGPAD_FILEPATH[] = "assets/landing_pad.png",
           LETTERSHEET_FILEPATH[] = "assets/default_font.png",
           VICTORY_FILEPATH[] = "assets/you_win.png",
           CRASHED_FILEPATH[] = "assets/you_lose.png";

// world constants
const float MILLISECONDS_IN_SECOND = 1000.0;
const float FIXED_TIMESTEP = 0.0166666f;
const float ACC_OF_GRAVITY = -0.08f;

// texture generation stuff
const int NUMBER_OF_TEXTURES = 1;  // to be generated, that is
const GLint LEVEL_OF_DETAIL = 0;  // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER = 0;  // this value MUST be zero

// custom
const float THRUSTER_FORCE = 0.3f;
const float GROUND_OFFSET = 0.8f;
const float SAFE_SPEED = 0.35f;
const int LETTER_COUNT = 9;
const int LANDINGPAD_COUNT = 4;
const glm::vec3 PAD_COORDINATES[] = {
    glm::vec3(-3.9f,-2.4f,0.0f),
    glm::vec3(1.55f,-2.35f,0.0f),
    glm::vec3(-1.9f,-0.95f,0.0f),
    glm::vec3(4.05f,-1.2f,0.0f),
};

// ————— VARIABLES ————— //

// game state container
GameState g_gameState;

// core globals
SDL_Window* g_displayWindow;
ShaderProgram g_shaderProgram;
glm::mat4 g_viewMatrix, g_projectionMatrix;
bool g_gameIsRunning = true;

// times
float g_previousTicks = 0.0f;
float g_timeAccumulator = 0.0f;

// custom
bool g_tooFast = false;
bool g_thrusterOn = false;
bool g_showEndText = false;
float g_endingTimer = 4.0f;
float g_fuel = 3000;

// ———— GENERAL FUNCTIONS ———— //
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

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);

    return textureID;
}

void add_acceleration(Entity* entity, glm::vec3 force) {
    glm::vec3 acc = entity->get_acceleration();
    acc.x += force.x;
    acc.y += force.y;
    acc.z += force.z;
    entity->set_acceleration(acc);
}

float get_ground_level(float xPos) {
    // determines where the ground is based on a really big piecewise function
    // the function is available here: https://www.desmos.com/calculator/gs3nqgoldy
    float output = -3.75f;
    if (-5.0f < xPos and xPos < -4.143f) output = -0.1f * xPos - 3.0f;
    else if (xPos < -3.918f) output = -3.6f * xPos - 17.5f;
    else if (xPos < -3.533f) output = 2.5f * xPos + 6.4f;
    else if (xPos < -2.727f) output = -0.5f * xPos - 4.2f;
    else if (xPos < -1.926f) output = 1.7f * xPos + 1.8f;
    else if (xPos < -0.643f) output = -1.0f * xPos - 3.4f;
    else if (xPos < 0.125f) output = 0.4f * xPos - 2.5f;
    else if (xPos < 1.5f) output = -0.4f * xPos - 2.4f;
    else if (xPos < 2.813f) output = 0.2f * xPos - 3.3f;
    else if (xPos < 3.741f) output = 1.8f * xPos - 7.8f;
    else if (xPos < 4.143f) output = -3.6f * xPos + 12.4f;
    else if (xPos < 5.0f) output = -0.1f * xPos - 2.1f;
    else {
        LOG("Invalid X position!");
        assert(false);
    }
    return output;
}

void end_game(bool success) {
    g_gameState.endText = new Entity();
    if (success) g_gameState.endText->m_texture_id = load_texture(VICTORY_FILEPATH);
    else g_gameState.endText->m_texture_id = load_texture(CRASHED_FILEPATH);
    g_gameState.endText->set_width(10.0f);
    g_gameState.endText->set_height(7.5f);
    g_gameState.endText->update(0.0f, NULL, 0);
    g_showEndText = true;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_displayWindow = SDL_CreateWindow("Kerbal Landing",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_displayWindow);
    SDL_GL_MakeCurrent(g_displayWindow, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shaderProgram.load(V_SHADER_PATH, F_SHADER_PATH);

    g_viewMatrix = glm::mat4(1.0f);
    g_projectionMatrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shaderProgram.set_projection_matrix(g_projectionMatrix);
    g_shaderProgram.set_view_matrix(g_viewMatrix);

    glUseProgram(g_shaderProgram.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    // ————— BACKGROUND ————— //
    g_gameState.background = new Entity();
    g_gameState.background->m_texture_id = load_texture(BACKGROUND_FILEPATH);
    g_gameState.background->set_width(10.0f);
    g_gameState.background->set_height(7.5f);
    g_gameState.background->update(0.0f, NULL, 0);

    // ————— TERRAIN ————— //
    g_gameState.terrain = new Entity();
    g_gameState.terrain->m_texture_id = load_texture(TERRAIN_FILEPATH);
    g_gameState.terrain->set_width(10.0f);
    g_gameState.terrain->set_height(7.5f);
    g_gameState.terrain->update(0.0f, NULL, 0);

    // ————— PLAYER ————— //
    // setup basic attributes
    g_gameState.player = new Entity();
    g_gameState.player->set_angle(-90.0f);
    g_gameState.player->set_position(glm::vec3(-4.6f, 3.4f, 0.0f));
    g_gameState.player->set_velocity(glm::vec3(0.4f, 0.0f, 0.0f));
    g_gameState.player->set_acceleration(glm::vec3(0.0f, ACC_OF_GRAVITY, 0.0f));
    g_gameState.player->m_texture_id = load_texture(PLAYER_FILEPATH);
    g_gameState.player->set_rot_speed(1.0f);
    g_gameState.player->m_jumping_power = 3.0f;
    g_gameState.player->m_control_mode = 2;

    // setup visuals
    g_gameState.player->set_height(0.35f);
    g_gameState.player->set_width(0.4f);

    // ————— FLAME ————— //
    g_gameState.flame = new Entity();
    g_gameState.flame->m_texture_id = load_texture(FLAME_FILEPATH);
    g_gameState.flame->set_width(0.25f);
    g_gameState.flame->set_height(0.6f);

    // ————— LANDING PADS ————— //
    g_gameState.landingPads = new Entity[LANDINGPAD_COUNT];

    for (int i = 0; i < LANDINGPAD_COUNT; i++)
    {
        g_gameState.landingPads[i].m_texture_id = load_texture(LANDINGPAD_FILEPATH);
        g_gameState.landingPads[i].set_position(PAD_COORDINATES[i]);
        g_gameState.landingPads[i].set_width(0.35f);
        g_gameState.landingPads[i].set_height(0.7f);
        g_gameState.landingPads[i].update(0.0f, NULL, 0);
    }

    // ————— DISPLAY LETTERS ————— //
    g_gameState.letters = new Entity[LETTER_COUNT];
    char message[] = "FUEL 0000";

    for (int i = 0; i < LETTER_COUNT; i++) {
        g_gameState.letters[i].m_texture_id = load_texture(LETTERSHEET_FILEPATH);
        g_gameState.letters[i].m_animation_indices = new int[256];
        for (int j = 0; j < 256; j++) g_gameState.letters[i].m_animation_indices[j] = j;
        g_gameState.letters[i].m_animation_index = message[i];
        g_gameState.letters[i].m_animation_cols = 16;
        g_gameState.letters[i].m_animation_rows = 16;
        g_gameState.letters[i].set_width(0.4f);
        g_gameState.letters[i].set_height(0.4f);
        g_gameState.letters[i].set_position(glm::vec3(-4.6f + i*0.2f, -3.3f, 0.0f));
        g_gameState.letters[i].update(0.0f, NULL, 0);
    }
    
    // ————— GENERAL ————— //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    // reset forced-movement if no player input
    g_gameState.player->set_acceleration(glm::vec3(0.0f, ACC_OF_GRAVITY, 0.0f));
    g_gameState.player->set_rotation(0.0f);

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_gameIsRunning = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_q:
                g_gameIsRunning = false;
                break;

            default:
                break;
            }

        default:
            break;
        }
    }

    float angle = g_gameState.player->get_angle();
    const Uint8* key_state = SDL_GetKeyboardState(NULL);
    if (not g_showEndText) {
        if (key_state[SDL_SCANCODE_LEFT])
        {
            if (angle < 90.0f) {
                g_gameState.player->rotate_anticlockwise();
            }
        }
        if (key_state[SDL_SCANCODE_RIGHT])
        {
            if (angle > -90.0f) {
                g_gameState.player->rotate_clockwise();
            }
        }
        if (key_state[SDL_SCANCODE_UP] and g_fuel > 0) {
            g_thrusterOn = true;
            glm::vec3 thrustVec = glm::vec3(
                THRUSTER_FORCE * cos(glm::radians(angle + 90)),
                THRUSTER_FORCE * sin(glm::radians(angle + 90)),
                0.0f);
            add_acceleration(g_gameState.player, thrustVec);
            g_fuel -= 0.1f;
        }
        else {
            g_thrusterOn = false;
        }
    }
    else {
        g_thrusterOn = false;
    }

    // normalize forced-movement vector so you don't go faster diagonally
    if (glm::length(g_gameState.player->get_movement()) > 1.0f)
    {
        g_gameState.player->set_movement(glm::normalize(g_gameState.player->get_movement()));
    }
}

void update()
{
    // ————— DELTA TIME ————— //
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND; // get the current number of ticks
    float delta_time = ticks - g_previousTicks; // the delta time is the difference from the last frame
    g_previousTicks = ticks;

    // ————— FIXED TIMESTEP ————— //
    g_timeAccumulator += delta_time;
    if (g_timeAccumulator < FIXED_TIMESTEP) return;
    while (g_timeAccumulator >= FIXED_TIMESTEP)
    {
        // handle game ending
        if (g_showEndText) {
            if ((g_endingTimer -= FIXED_TIMESTEP) <= 0) {
                g_gameIsRunning = false;
            }
        }
        
        // get player info
        glm::vec3 pos = g_gameState.player->get_position();
        glm::vec3 vel = g_gameState.player->get_velocity();
        float xOffset = g_gameState.player->get_width() / 2;
        float yOffset = g_gameState.player->get_height() / 2;
        float angle = g_gameState.player->get_angle();
        
        // check for wall collision
        if (abs(pos.x) >= 5.0f - xOffset) {
            vel.x = 0.0f;
            pos.x += (pos.x > 0)? -0.01f : 0.01f;
        }
        if (pos.y >= 3.75f - yOffset) {
            vel.y = 0.0f;
            pos.y -= 0.01f;
        }

        // check for terrain collision
        glm::vec3 collisionPoints[] = {
            pos + glm::vec3(0.0f,0.0f-yOffset,0.0f),
            pos + glm::vec3(-0.19f,0.1f-yOffset,0.0f),
            pos + glm::vec3(0.19f,0.1f-yOffset,0.0f),
        };
        for (int i = 0; i < 3; i++) {
            if (collisionPoints[i].y <= get_ground_level(collisionPoints[i].x) + GROUND_OFFSET) {
                vel = glm::vec3(0.0f);
                end_game(false);
            }
        }

        // check for successful landing
        if (g_gameState.player->m_collided_bottom) {
            vel = glm::vec3(0.0f);
            if (angle > 25 or angle < -25 or g_tooFast) {
                end_game(false);
            } else {
                end_game(true);
            }
        }

        // check if player is moving slow enough to land
        float currentSpeed = glm::length(vel);
        g_tooFast = currentSpeed >= SAFE_SPEED;

        // move the player
        g_gameState.player->set_position(pos);
        g_gameState.player->set_velocity(vel);
        g_gameState.player->update(FIXED_TIMESTEP, g_gameState.landingPads, LANDINGPAD_COUNT);

        // reposition the flame
        glm::vec3 flameOffset = glm::vec3(
            0.4f * cos(glm::radians(angle - 90)),
            0.4f * sin(glm::radians(angle - 90)),
            0.0f);
        g_gameState.flame->set_position(g_gameState.player->get_position() + flameOffset);
        g_gameState.flame->set_angle(angle);
        g_gameState.flame->update(FIXED_TIMESTEP, NULL, 0);

        // update the fuel counter
        for (int i = 0; i < 4; i++) {
            g_gameState.letters[8-i].m_animation_index = ( int(g_fuel) % int(pow(10,i+1)) ) / pow(10,i) + 48;
        }
 
        // update time accumulator
        g_timeAccumulator -= FIXED_TIMESTEP;
    }
}

void render()
{
    // ————— GENERAL ————— //
    glClear(GL_COLOR_BUFFER_BIT);

    // ————— BACKGROUND ————— //
    g_gameState.background->render(&g_shaderProgram);

    // ————— FLAME ————— //
    if (g_thrusterOn) g_gameState.flame->render(&g_shaderProgram);

    // ————— PLAYER ————— //
    g_gameState.player->render(&g_shaderProgram);

    // ————— LANDING PADS ————— //
    for (int i = 0; i < LANDINGPAD_COUNT; i++) g_gameState.landingPads[i].render(&g_shaderProgram);

    // ————— TERRAIN ————— //
    g_gameState.terrain->render(&g_shaderProgram);

    // ————— DISPLAY LETTERS ————— //
    for (int i = 0; i < LETTER_COUNT; i++) g_gameState.letters[i].render(&g_shaderProgram);

    // ————— ENDING TEXT ————— //
    if (g_showEndText) g_gameState.endText->render(&g_shaderProgram);

    // ————— GENERAL ————— //
    SDL_GL_SwapWindow(g_displayWindow);
}

void shutdown() { 
    SDL_Quit();
    delete[] g_gameState.background;
    delete[] g_gameState.terrain;
    delete[] g_gameState.player;
    delete[] g_gameState.flame;
    delete[] g_gameState.landingPads;
    delete[] g_gameState.letters;
    delete[] g_gameState.endText;
}

// ————— DRIVER GAME LOOP ————— /
int main(int argc, char* argv[])
{
    initialise();

    while (g_gameIsRunning)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}