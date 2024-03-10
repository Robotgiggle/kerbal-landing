class Entity
{
private:
    bool m_is_active = true;

    // ––––– PHYSICS (GRAVITY) ––––– //
    float     m_angle;
    glm::vec3 m_position;
    glm::vec3 m_velocity;
    glm::vec3 m_acceleration;

    // ————— TRANSFORMATIONS ————— //
    float     m_speed;
    float     m_rot_speed;
    float     m_rotation;
    glm::vec3 m_movement;
    glm::vec3 m_scale;
    glm::mat4 m_model_matrix;

public:
    // ————— STATIC VARIABLES ————— //
    static const int SECONDS_PER_FRAME = 4;
    static const int LEFT = 0,
        RIGHT = 1,
        UP = 2,
        DOWN = 3;

    // ————— ANIMATION ————— //
    int** m_walking = new int* [4];

    int m_animation_frames = 0,
        m_animation_index = 0,
        m_animation_cols = 0,
        m_animation_rows = 0;

    int* m_animation_indices = nullptr;
    float m_animation_time = 0.0f;

    // ––––– PHYSICS (JUMPING) ––––– //
    bool  m_is_jumping = false;
    float m_jumping_power = 0;

    // ––––– PHYSICS (COLLISIONS) ––––– //
    bool m_collided_top = false;
    bool m_collided_bottom = false;
    bool m_collided_left = false;
    bool m_collided_right = false;

    int m_control_mode = 1;
    GLuint m_texture_id;

    // ————— METHODS ————— //
    Entity();
    ~Entity();

    void draw_sprite_from_texture_atlas(ShaderProgram* program, GLuint texture_id, int index);
    bool const check_collision(Entity* other) const;
    void const check_collision_y(Entity* collidable_entities, int collidable_entity_count);
    void const check_collision_x(Entity* collidable_entities, int collidable_entity_count);

    void update(float delta_time, Entity* collidable_entities, int collidable_entity_count);
    void render(ShaderProgram* program);

    void move_left() { m_movement.x = -1.0f; };
    void move_right() { m_movement.x = 1.0f; };
    void move_up() { m_movement.y = 1.0f; };
    void move_down() { m_movement.y = -1.0f; };

    void rotate_clockwise() { m_rotation = -1.0f; };
    void rotate_anticlockwise() { m_rotation = 1.0f; };

    void activate() { m_is_active = true; };
    void deactivate() { m_is_active = false; };

    // ————— GETTERS ————— //
    glm::vec3 const get_position()     const { return m_position; };
    glm::vec3 const get_velocity()     const { return m_velocity; };
    glm::vec3 const get_acceleration() const { return m_acceleration; };
    glm::vec3 const get_movement()     const { return m_movement; };
    float     const get_rotation()     const { return m_rotation; };
    float     const get_angle()        const { return m_angle; };
    float     const get_rot_speed()    const { return m_rot_speed; };
    float     const get_speed()        const { return m_speed; };
    float     const get_width()        const { return m_scale.x; };
    float     const get_height()       const { return m_scale.y; };

    // ————— SETTERS ————— //
    void const set_position(glm::vec3 new_position) { m_position = new_position; };
    void const set_velocity(glm::vec3 new_velocity) { m_velocity = new_velocity; };
    void const set_acceleration(glm::vec3 new_position) { m_acceleration = new_position; };
    void const set_movement(glm::vec3 new_movement) { m_movement = new_movement; };
    void const set_rotation(float new_rotation) { m_rotation = new_rotation; };
    void const set_angle(float new_angle) { m_angle = new_angle; };
    void const set_rot_speed(float new_rot_speed) { m_rot_speed = new_rot_speed; };
    void const set_speed(float new_speed) { m_speed = new_speed; };
    void const set_width(float new_width) { m_scale.x = new_width; };
    void const set_height(float new_height) { m_scale.y = new_height; };
};
