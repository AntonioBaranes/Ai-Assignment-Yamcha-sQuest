/**
* Author: Antonio Baranes
* Assignment: Platformer
* Date due: 2023-11-23, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define LEVEL1_WIDTH 14
#define LEVEL1_HEIGHT 8
#define LEVEL1_LEFT_EDGE 5.0f
#define MAX_RGB 255

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL_mixer.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "cmath"
#include <ctime>
#include <vector>
#include "Entity.h"
#include "Map.h"
#include "Utility.h"
#include "Scene.h"
#include "Level0.h"
#include "LevelA.h"
#include "LevelB.h"
#include "LevelC.h"
#include "Effects.h"

// ––––– CONSTANTS ––––– //
constexpr int WINDOW_WIDTH = 1280 / 2, WINDOW_HEIGHT = 960 / 2;

constexpr float BG_RED = 27.0f / MAX_RGB, BG_BLUE = 43.0f / MAX_RGB,
BG_GREEN = 52.0f / MAX_RGB, BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0, VIEWPORT_Y = 0, VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_exercise.glsl",
F_SHADER_PATH[] = "shaders/fragment_exercise.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

enum AppStatus { RUNNING, TERMINATED };

enum current_level {Intro, Playing, Won, Died, Level_Beaten};
current_level curr_lev = Intro;

// ––––– GLOBAL VARIABLES ––––– //
Scene* g_curr_scene = nullptr;
Level0* g_level0 = nullptr;
LevelA* g_levelA = nullptr;
LevelB* g_levelB = nullptr;
LevelC* g_levelC = nullptr;

Effects* g_effects = nullptr;
Scene* g_levels[4];

SDL_Window* g_display_window = nullptr;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;

bool g_is_colliding_bottom = false;

AppStatus g_app_status = RUNNING;

bool g_attack_active = false;
GLuint g_font_texture_id;

int g_lives = 3;

int enemies_killed = 0;
bool level_beaten = false;

//Music Stuff
constexpr int CD_QUAL_FREQ = 44100,
AUDIO_CHAN_AMT = 2,     // stereo
AUDIO_BUFF_SIZE = 4096;

constexpr char BGM_FILEPATH[] = "assets/snd_bgm.mp3",
SFX_FILEPATH[] = "assets/snd_dash.wav";

constexpr int PLAY_ONCE = 0,    // play once, loop never
NEXT_CHNL = -1,   // next available channel
ALL_SFX_CHNL = -1;

Mix_Music* g_music;
Mix_Chunk* g_jump_sfx;

void switch_to_scene(Scene* scene);
void initialise();
void process_input();
void update();
void render();
void shutdown();

void switch_to_scene(Scene* scene) {
    g_curr_scene = scene;
    g_curr_scene->initialise();
}

void initialise() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    g_display_window = SDL_CreateWindow("Yamcha's Adventure in the Dark",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH,
        WINDOW_HEIGHT, SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    // Use lighting-related shaders
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix =
        glm::ortho(-3.75f, 3.75f, -2.8125f, 2.8125f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    // Set clear color to lighting style from Code 1
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Initialize levels
    g_level0 = new Level0();
    g_levelA = new LevelA();
    g_levelB = new LevelB();
    g_levelC = new LevelC();
    g_levels[0] = g_level0;
    g_levels[1] = g_levelA;
    g_levels[2] = g_levelB;
    g_levels[3] = g_levelC;

    switch_to_scene(g_levels[0]);

    g_effects = new Effects(g_projection_matrix, g_view_matrix);
    g_font_texture_id = Utility::load_texture("assets/font1.png");

    // ––––– BGM ––––– //
    Mix_OpenAudio(CD_QUAL_FREQ, MIX_DEFAULT_FORMAT, AUDIO_CHAN_AMT, AUDIO_BUFF_SIZE);

    // STEP 1: Have openGL generate a pointer to your music file
    g_music = Mix_LoadMUS(BGM_FILEPATH); // works only with mp3 files

    // STEP 2: Play music
    Mix_PlayMusic(
        g_music,  // music file
        -1        // -1 means loop forever; 0 means play once, look never
    );

    // STEP 3: Set initial volume
    Mix_VolumeMusic(MIX_MAX_VOLUME / 2.0);

    // ––––– SFX ––––– //
    g_jump_sfx = Mix_LoadWAV(SFX_FILEPATH);
}

void process_input() {
    g_curr_scene->get_state().player->set_movement(glm::vec3(0.0f));
    GameState curr_state = g_curr_scene->get_state();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_app_status = TERMINATED;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_q:
                g_app_status = TERMINATED;
                break;
            case SDLK_w:
                curr_state.player->jump();
                break;
            case SDLK_SPACE:
                if (!g_attack_active) {
                    //g_attack_active = true;
                    Mix_PlayChannel(NEXT_CHNL, g_jump_sfx, 0);
                    curr_state.attack->set_position(curr_state.player->get_position());
                    curr_state.attack->set_speed(-5.0f);
                }
                break;
            case SDLK_RETURN:
                curr_lev = Playing;
                switch_to_scene(g_levels[1]);
                break;
            case SDLK_p:
                Mix_PlayMusic(g_music, -1);
                break;
            default:
                break;
            }
        default:
            break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(nullptr);

    // Handle player states and effects
    if (key_state[SDL_SCANCODE_C] &&
        g_curr_scene->get_state().player->get_collided_bottom()) {
        g_curr_scene->get_state().player->set_player_state(CHARGING);
        g_effects->start(SHAKE);
    }
    else {
        g_curr_scene->get_state().player->set_player_state(REST);
        g_effects->start(NONE);
    }

    if (glm::length(g_curr_scene->get_state().player->get_movement()) > 1.0f)
        g_curr_scene->get_state().player->normalise_movement();

    if (key_state[SDL_SCANCODE_D]) {
        curr_state.player->move_right();
    }
    else if (key_state[SDL_SCANCODE_A]) {
        curr_state.player->move_left();
    }
}

void render() {
    g_shader_program.set_view_matrix(g_view_matrix);

    glClear(GL_COLOR_BUFFER_BIT);

    g_curr_scene->render(&g_shader_program);

    g_effects->render();
    if (curr_lev == Intro) {
        Utility::draw_text(&g_shader_program, g_font_texture_id, "Yamcha's Quest in", 0.3f,
            0.05f, glm::vec3(4.5f, -2.0f, 0.0f));
        Utility::draw_text(&g_shader_program, g_font_texture_id, "The Dark", 0.3f,
            0.05f, glm::vec3(6.0f, -2.5f, 0.0f));
        Utility::draw_text(&g_shader_program, g_font_texture_id, "Press Enter to", 0.3f,
            0.05f, glm::vec3(5.0f, -3.0f, 0.0f));
        Utility::draw_text(&g_shader_program, g_font_texture_id, "Start", 0.3f,
            0.05f, glm::vec3(6.5f, -3.5f, 0.0f));
     
       Utility::draw_text(&g_shader_program, g_font_texture_id, "Space to Shoot", 0.2f,
            0.05f, glm::vec3(5.50f, -4.0f, 0.0f));
       Utility::draw_text(&g_shader_program, g_font_texture_id, "Find and Defeat the Saibaman!", 0.25f,
           0.05f, glm::vec3(3.7f, -4.3f, 0.0f));
    }
    else if (curr_lev == Playing) {
        glm::vec3 player_pos = g_curr_scene->get_state().player->get_position();
        player_pos.y += 1;
        player_pos.x -= 1.5f;
        Utility::draw_text(&g_shader_program, g_font_texture_id, "Lives:" + std::to_string(g_lives), 0.5f,
            0.05f, player_pos);
        player_pos.y++;
        //Utility::draw_text(&g_shader_program, g_font_texture_id, "Enemies Killed:" + std::to_string(enemies_killed), 0.2f,
          //  0.05f, player_pos);
    }
    else if (curr_lev == Died) {
        glm::vec3 player_pos = g_curr_scene->get_state().player->get_position();
        player_pos.y += 1;
        player_pos.x -= 1.5f;
        Utility::draw_text(&g_shader_program, g_font_texture_id, "Yamcha Died!", 0.3f,
            0.05f, glm::vec3(5.0f, -3.5f, 0.0f));
    }
    else if (curr_lev == Level_Beaten) {
        glm::vec3 player_pos = g_curr_scene->get_state().player->get_position();
        player_pos.y += 1;
        player_pos.x -= 1.5f;
        Utility::draw_text(&g_shader_program, g_font_texture_id, "Saibaman Defeated!", 0.2f,
            0.05f,player_pos);
    }
    else if (curr_lev == Won) {
        glm::vec3 player_pos = g_curr_scene->get_state().player->get_position();
        player_pos.y += 1;
        player_pos.x -= 2.5f;
        Utility::draw_text(&g_shader_program, g_font_texture_id, "Yamcha Won!", 0.3f,
            0.05f, glm::vec3(5.0f, -3.5f, 0.0f));
    }

    SDL_GL_SwapWindow(g_display_window);
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
        g_curr_scene->update(FIXED_TIMESTEP);
        g_effects->update(FIXED_TIMESTEP);

        g_is_colliding_bottom =
            g_curr_scene->get_state().player->get_collided_bottom();

        delta_time -= FIXED_TIMESTEP;
    }

    g_accumulator = delta_time;

    g_view_matrix = glm::mat4(1.0f);

    glm::vec3 player_pos = g_curr_scene->get_state().player->get_position();

    // Define vertical and horizontal boundaries for the camera to follow the player smoothly
    float camera_x = player_pos.x > LEVEL1_LEFT_EDGE ? -player_pos.x : -5.0f;
    float camera_y = glm::clamp(-player_pos.y, -2.0f, 3.75f); // Adjust clamp values as needed for your level's design

    g_view_matrix = glm::translate(
        glm::mat4(1.0f),
        glm::vec3(camera_x, camera_y, 0.0f)
    );

    // Add any effects offsets
    g_view_matrix = glm::translate(g_view_matrix, g_effects->get_view_offset());


    //Allows attacking again
    if (g_curr_scene->get_state().attack->get_position().x > 15.0f) {
        g_attack_active = false;
    }

    //player and enemy collision
    GameState curr_state = g_curr_scene->get_state();
    Entity* curr_player = curr_state.player;
    Entity* curr_enemy = curr_state.enemies;
    Entity* curr_attack = curr_state.attack;
    if (curr_player->check_collision(curr_enemy)) {
        g_lives--;
        g_curr_scene->initialise();
        
    }
    if (curr_player->get_position().y < -7.0f) {
        g_lives--;
        g_curr_scene->initialise();
    }
    if (curr_attack->check_collision(curr_enemy)) {
        enemies_killed++;
        level_beaten = true;
        curr_state.enemies->deactivate();
        
    }
    if (g_lives <= 0) {
        
        curr_lev = Died;
        render();

        SDL_Delay(5000);
        g_lives = 3;
        enemies_killed = 0;
        curr_lev = Intro;
        switch_to_scene(g_levels[0]);
    }
    if (enemies_killed == 1 && level_beaten) {
        level_beaten = false;
        curr_lev = Level_Beaten;
        render();

        SDL_Delay(5000);
        curr_lev = Playing;
        switch_to_scene(g_levels[2]);
    }
    if (enemies_killed == 1 && level_beaten) {
        level_beaten = false;
        curr_lev = Level_Beaten;
        render();

        SDL_Delay(5000);
        curr_lev = Playing;
        switch_to_scene(g_levels[2]);
    }
    if (enemies_killed == 2 && level_beaten) {
        level_beaten = false;
        curr_lev = Level_Beaten;
        render();

        SDL_Delay(5000);
        curr_lev = Playing;
        switch_to_scene(g_levels[3]);
    }
    if (enemies_killed == 3) {
        curr_lev = Won;
        render();

        SDL_Delay(5000);
        g_lives = 3;
        enemies_killed = 0;
        curr_lev = Intro;
        switch_to_scene(g_levels[0]);
    }
}



void shutdown() {
    SDL_Quit();
    delete g_level0;
    delete g_levelA;
    delete g_levelB;
    delete g_levelC;
    delete g_effects;
}

// ––––– DRIVER GAME LOOP ––––– //
int main(int argc, char* argv[]) {
    initialise();

    while (g_app_status == RUNNING) {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
