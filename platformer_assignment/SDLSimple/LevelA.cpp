#include "LevelA.h"
#include "Utility.h"
#include <vector>

#define LEVEL_HEIGHT 8

unsigned int LEVELA_DATA[] =
{
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 12, 2, 12, 2, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12
};

LevelA::~LevelA()
{
    delete m_game_state.player;
    delete m_game_state.map;
    delete m_game_state.enemies;
    delete m_game_state.attack;
}

void LevelA::initialise()
{
    m_game_state.next_scene_id = -1;
    
    GLuint map_texture_id = Utility::load_texture("assets/world_tileset.png");
    m_game_state.map = new Map(LEVEL_WIDTH, LEVEL_HEIGHT, LEVELA_DATA, map_texture_id,
                               1.0f, 8, 3);
    
    std::vector<std::vector<int>> animations =
    {
        {  },  // Rest
        { 0, 1, 2, 3, 4 },     // Charging
    };

    glm::vec3 acceleration = glm::vec3(0.0f, -4.81f, 0.0f);
    
    std::vector<GLuint> texture_ids =
    {
        Utility::load_texture("assets/spr_yamcha.png"),
        Utility::load_texture("assets/B_witch_charge.png")
    };
    
    m_game_state.player = new Entity(
        texture_ids,               // texture id
        2.0f,                      // speed
        acceleration,              // acceleration
        4.0f,                      // jumping power
        animations,                // animation index sets
        0.0f,                      // animation time
        4,                         // animation frame amount
        0,                         // current animation index
        1,                         // animation column amount
        1,                         // animation row amount
        0.7f,                      // width
        0.7f,                      // height
        PLAYER,                    // entity type
        REST                       // player state
    );
        
    m_game_state.player->set_position(glm::vec3(6.0f, 0.0f, 0.0f));

    std::vector<GLuint> enemy_texture_id = { Utility::load_texture("assets/spr_saibaman.png") };

    m_game_state.enemies = new Entity(
        enemy_texture_id,               // texture id
        0.0f,                      // speed
        acceleration,              // acceleration
        3.0f,                      // jumping power
        animations,                // animation index sets
        0.0f,                      // animation time
        4,                         // animation frame amount
        0,                         // current animation index
        1,                         // animation column amount
        1,                         // animation row amount
        0.7f,                      // width
        0.7f,                      // height
        ENEMY,                    // entity type
        REST                       // player state
    );
    //m_game_state.enemies->set_ai_state(IDLE);
    m_game_state.enemies->set_position(glm::vec3(8.0f, 0.0f, 0.0f));

    std::vector<GLuint> attack_texture_id = { Utility::load_texture("assets/spr_sokidan.png") };

    m_game_state.attack = new Entity(
        attack_texture_id,               // texture id
        0.0f,                      // speed
        glm::vec3(0.0f,0.0f,0.0f),              // acceleration
        3.0f,                      // jumping power
        animations,                // animation index sets
        0.0f,                      // animation time
        4,                         // animation frame amount
        0,                         // current animation index
        1,                         // animation column amount
        1,                         // animation row amount
        0.7f,                      // width
        0.7f,                      // height
        ENEMY,                    // entity type
        REST                       // player state
    );
    //m_game_state.enemies->set_ai_state(IDLE);
    m_game_state.attack->set_position(glm::vec3(0.0f, 0.0f, 0.0f));
    
}

void LevelA::update(float delta_time)
{
    m_game_state.player->update(delta_time, m_game_state.player, nullptr, 0,
                                m_game_state.map);
    m_game_state.enemies->update(delta_time, m_game_state.enemies, nullptr, 0, m_game_state.map);

    m_game_state.attack->update(delta_time, m_game_state.attack, nullptr, 0, m_game_state.map);

    
    
    if (m_game_state.player->get_position().y < -10.0f) m_game_state.next_scene_id = 1;
}

void LevelA::render(ShaderProgram *program)
{

    program->set_light_position_matrix(m_game_state.attack->get_position());
    //program->set_light_position_matrix(m_game_state.enemies->get_position());
    
    m_game_state.map->render(program);
    m_game_state.player->render(program);
    m_game_state.enemies->render(program);
    m_game_state.attack->render(program);
}
