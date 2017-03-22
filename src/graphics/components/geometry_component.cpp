#include <iostream>

#include <glm/gtx/transform.hpp>

#include "graphics/components/geometry_component.hpp"
//#include "modules/packets/geometry_packets.hpp"

namespace tomovis {

GeometryComponent::GeometryComponent(SceneObject& object, int scene_id)
    : object_(object), scene_id_(scene_id), proj(0) {
    static const GLfloat square[4][3] = {{0.0f, 0.0f, 0.0f},
                                         {0.0f, 1.0f, 0.0f},
                                         {1.0f, 1.0f, 0.0f},
                                         {1.0f, 0.0f, 0.0f}};

    glGenVertexArrays(1, &vao_handle_);
    glBindVertexArray(vao_handle_);
    glGenBuffers(1, &vbo_handle_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_handle_);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), square, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    program_ = std::make_unique<ShaderProgram>(
        "../src/shaders/show_texture.vert", "../src/shaders/show_texture.frag");

    static const GLfloat cube[] = {
        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
        1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  -1.0f,
        1.0f,  -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f,
        1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,
        1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, 1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,
        1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,
        1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,
        1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  -1.0f,
        1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,
        1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  -1.0f, 1.0f};

    glGenVertexArrays(1, &cube_vao_handle_);
    glBindVertexArray(cube_vao_handle_);
    glGenBuffers(1, &cube_vbo_handle_);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo_handle_);
    glBufferData(GL_ARRAY_BUFFER, 9 * 12 * sizeof(GLfloat), cube,
                 GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    cube_program_ =
        std::make_unique<ShaderProgram>("../src/shaders/wireframe_cube.vert",
                                        "../src/shaders/wireframe_cube.frag");

    static const GLfloat lines[8][3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
                                        {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 1.0f},
                                        {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f},
                                        {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f}};

    glGenVertexArrays(1, &lines_vao_handle_);
    glBindVertexArray(lines_vao_handle_);
    glGenBuffers(1, &lines_vbo_handle_);
    glBindBuffer(GL_ARRAY_BUFFER, lines_vbo_handle_);
    glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(GLfloat), lines, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    lines_program_ = std::make_unique<ShaderProgram>(
        "../src/shaders/lines.vert", "../src/shaders/lines.frag");
}

GeometryComponent::~GeometryComponent() {}

void GeometryComponent::draw(glm::mat4 world_to_screen) const {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    cube_program_->use();

    // DRAW SOURCE
    glm::mat4 object_matrix =
        glm::translate(proj.source_position) * glm::scale(glm::vec3(0.1f));

    auto transform_matrix = world_to_screen * object_matrix;

    GLint matrix_loc =
        glGetUniformLocation(cube_program_->handle(), "transform_matrix");
    glUniformMatrix4fv(matrix_loc, 1, GL_FALSE, &transform_matrix[0][0]);

    glBindVertexArray(cube_vao_handle_);
    glDrawArrays(GL_TRIANGLES, 0, 12 * 3);

    // DRAW PROJECTION
    program_->use();

    proj.data_texture.bind();

    GLint or_matrix_loc =
        glGetUniformLocation(program_->handle(), "orientation_matrix");
    glUniformMatrix4fv(or_matrix_loc, 1, GL_FALSE,
                       &proj.detector_orientation[0][0]);
    GLint ws_matrix_loc =
        glGetUniformLocation(program_->handle(), "world_to_screen_matrix");
    glUniformMatrix4fv(ws_matrix_loc, 1, GL_FALSE, &world_to_screen[0][0]);

    glBindVertexArray(vao_handle_);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // DRAW LINES
    lines_program_->use();
    glm::mat4 source_to_det_matrix = proj.detector_orientation;
    source_to_det_matrix[2] += glm::vec4(-proj.source_position, 0.0f);
    auto line_transform_matrix = world_to_screen * glm::translate(proj.source_position) * source_to_det_matrix;

    GLint line_matrix_loc =
        glGetUniformLocation(lines_program_->handle(), "transform_matrix");
    glUniformMatrix4fv(matrix_loc, 1, GL_FALSE, &line_transform_matrix[0][0]);

    glLineWidth(2.0f);
    glBindVertexArray(lines_vao_handle_);
    glDrawArrays(GL_LINES, 0, 8 * 3);
    glLineWidth(1.0f);

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
}

}  // namespace tomovis