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
    Entity* player;
    Entity* platforms;
};

// ————— CONSTANTS ————— //

// window size
const int WINDOW_WIDTH = 640,
          WINDOW_HEIGHT = 480;

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
const char SPRITESHEET_FILEPATH[] = "assets/kerbal.png",
           PLATFORM_FILEPATH[] = "assets/default_platform.png";

// world constants
const float MILLISECONDS_IN_SECOND = 1000.0;
const float FIXED_TIMESTEP = 0.0166666f;
const float ACC_OF_GRAVITY = -9.81f;
const float GRAVITY_FACTOR = 0.01f;

// entity counts
const int PLATFORM_COUNT = 3;

// texture generation stuff
const int NUMBER_OF_TEXTURES = 1;  // to be generated, that is
const GLint LEVEL_OF_DETAIL = 0;  // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER = 0;  // this value MUST be zero

// custom
const float THRUSTER_FORCE_X = 0.3f;
const float THRUSTER_FORCE_Y = 0.5f;

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

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

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

    // ————— PLAYER ————— //
    // setup basic attributes
    g_gameState.player = new Entity();
    g_gameState.player->set_position(glm::vec3(-4.5f, 3.0f, 0.0f));
    g_gameState.player->set_acceleration(glm::vec3(0.0f, ACC_OF_GRAVITY*GRAVITY_FACTOR, 0.0f));
    g_gameState.player->m_texture_id = load_texture(SPRITESHEET_FILEPATH);
    g_gameState.player->m_jumping_power = 3.0f;

    // setup thruster animation
    //g_gameState.player->m_walking[g_gameState.player->DOWN]  = new int[4] { 0 };
    //g_gameState.player->m_walking[g_gameState.player->LEFT]  = new int[4] { 1, 5 };
    //g_gameState.player->m_walking[g_gameState.player->UP]    = new int[4] { 2, 6 };
    //g_gameState.player->m_walking[g_gameState.player->RIGHT] = new int[4] { 3, 7 };

    //g_gameState.player->m_animation_indices = g_gameState.player->m_walking[g_gameState.player->DOWN];
    g_gameState.player->m_animation_frames = 2;
    g_gameState.player->m_animation_index = 0;
    g_gameState.player->m_animation_time = 0.0f;
    g_gameState.player->m_animation_cols = 4;
    g_gameState.player->m_animation_rows = 2;
    g_gameState.player->set_height(0.75f);
    g_gameState.player->set_width(0.5f);

    // ————— PLATFORM ————— //
    g_gameState.platforms = new Entity[PLATFORM_COUNT];

    for (int i = 0; i < PLATFORM_COUNT; i++)
    {
        g_gameState.platforms[i].m_texture_id = load_texture(PLATFORM_FILEPATH);
        g_gameState.platforms[i].set_position(glm::vec3(i - 1.0f, -3.0f, 0.0f));
        g_gameState.platforms[i].update(0.0f, NULL, 0);
    }

    // ————— GENERAL ————— //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    // reset forced-movement if no player input
    g_gameState.player->set_acceleration(glm::vec3(0.0f, ACC_OF_GRAVITY * GRAVITY_FACTOR, 0.0f));

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

    const Uint8* key_state = SDL_GetKeyboardState(NULL);


    if (key_state[SDL_SCANCODE_LEFT])
    {
        add_acceleration(g_gameState.player, glm::vec3(-1*THRUSTER_FORCE_X, 0.0f, 0.0f));
        //g_gameState.player->m_animation_indices = g_gameState.player->m_walking[g_gameState.player->LEFT];
    }
    if (key_state[SDL_SCANCODE_RIGHT])
    {
        add_acceleration(g_gameState.player, glm::vec3(THRUSTER_FORCE_X, 0.0f, 0.0f));
        //g_gameState.player->m_animation_indices = g_gameState.player->m_walking[g_gameState.player->RIGHT];
    }
    if (key_state[SDL_SCANCODE_UP]) {
        add_acceleration(g_gameState.player, glm::vec3(0.0f, THRUSTER_FORCE_Y, 0.0f));
        //g_gameState.player->m_animation_indices = g_gameState.player->m_walking[g_gameState.player->UP];
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
        g_gameState.player->update(FIXED_TIMESTEP, g_gameState.platforms, 3);
        g_timeAccumulator -= FIXED_TIMESTEP;
    }
}

void render()
{
    // ————— GENERAL ————— //
    glClear(GL_COLOR_BUFFER_BIT);

    // ————— PLAYER ————— //
    g_gameState.player->render(&g_shaderProgram);

    // ————— PLATFORM ————— //
    for (int i = 0; i < PLATFORM_COUNT; i++) g_gameState.platforms[i].render(&g_shaderProgram);

    // ————— GENERAL ————— //
    SDL_GL_SwapWindow(g_displayWindow);
}

void shutdown() { SDL_Quit(); }

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