#include <iostream>

#include "graphics/colormap.hpp"
#include "graphics/components/reconstruction_component.hpp"
#include "graphics/scene_camera.hpp"

#include "modules/packets/reconstruction_packets.hpp"

namespace tomovis {

ReconstructionComponent::ReconstructionComponent(SceneObject& object,
                                                 int scene_id)
    : object_(object), volume_texture_(16, 16, 16), scene_id_(scene_id) {
    // FIXME move all this primitives stuff to a separate file
    static const GLfloat square[4][3] = {{0.0f, 0.0f, 1.0f},
                                         {0.0f, 1.0f, 1.0f},
                                         {1.0f, 1.0f, 1.0f},
                                         {1.0f, 0.0f, 1.0f}};

    glGenVertexArrays(1, &vao_handle_);
    glBindVertexArray(vao_handle_);
    glGenBuffers(1, &vbo_handle_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_handle_);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), square, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

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

    program_ = std::make_unique<ShaderProgram>("../src/shaders/simple_3d.vert",
                                               "../src/shaders/simple_3d.frag");

    cube_program_ =
        std::make_unique<ShaderProgram>("../src/shaders/wireframe_cube.vert",
                                        "../src/shaders/wireframe_cube.frag");

    slices_[0] = std::make_unique<slice>(0);
    slices_[1] = std::make_unique<slice>(1);
    slices_[2] = std::make_unique<slice>(2);

    // slice along axis 0 = x
    slices_[0]->set_orientation(glm::vec3(0.0f, -1.0f, -1.0f),
                                glm::vec3(0.0f, 2.0f, 0.0f),
                                glm::vec3(0.0f, 0.0f, 2.0f));

    // slice along axis 1 = y
    slices_[1]->set_orientation(glm::vec3(-1.0f, 0.0f, -1.0f),
                                glm::vec3(2.0f, 0.0f, 0.0f),
                                glm::vec3(0.0f, 0.0f, 2.0f));

    // slice along axis 2 = z
    slices_[2]->set_orientation(glm::vec3(-1.0f, -1.0f, 0.0f),
                                glm::vec3(2.0f, 0.0f, 0.0f),
                                glm::vec3(0.0f, 2.0f, 0.0f));

    box_origin_ = glm::vec3(-1.0f);
    box_size_ = glm::vec3(2.0f);

    colormap_texture_ = generate_colormap_texture("bone");
}

ReconstructionComponent::~ReconstructionComponent() {
    glDeleteVertexArrays(1, &cube_vao_handle_);
    glDeleteBuffers(1, &cube_vbo_handle_);
    glDeleteVertexArrays(1, &vao_handle_);
    glDeleteBuffers(1, &vbo_handle_);
    glDeleteTextures(1, &colormap_texture_);
}

void ReconstructionComponent::update_image_(int slice) {
    slices_[slice]->update_texture();
}

void ReconstructionComponent::draw(glm::mat4 world_to_screen) const {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    program_->use();

    glUniform1i(glGetUniformLocation(program_->handle(), "texture_sampler"), 0);
    glUniform1i(glGetUniformLocation(program_->handle(), "colormap_sampler"),
                1);
    glUniform1i(glGetUniformLocation(program_->handle(), "volume_data_sampler"),
                3);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, colormap_texture_);

    auto draw_slice = [&](slice& the_slice) {
        the_slice.get_texture().bind();

        GLint transform_loc =
            glGetUniformLocation(program_->handle(), "world_to_screen_matrix");
        GLint orientation_loc =
            glGetUniformLocation(program_->handle(), "orientation_matrix");
        glUniformMatrix4fv(transform_loc, 1, GL_FALSE, &world_to_screen[0][0]);
        glUniformMatrix4fv(orientation_loc, 1, GL_FALSE,
                           &the_slice.orientation[0][0]);

        GLint hovered_loc = glGetUniformLocation(program_->handle(), "hovered");
        glUniform1i(hovered_loc, (int)(the_slice.hovered));

        GLint has_data_loc =
            glGetUniformLocation(program_->handle(), "has_data");
        glUniform1i(has_data_loc, (int)(the_slice.has_data()));

        glBindVertexArray(vao_handle_);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        the_slice.get_texture().unbind();
    };

    std::vector<slice*> slices;
    for (auto& id_slice : slices_) {
        if (id_slice.second.get()->inactive) {
            continue;
        }
        slices.push_back(id_slice.second.get());
    }
    std::sort(slices.begin(), slices.end(), [](auto& lhs, auto& rhs) -> bool {
        if (rhs->transparent() == lhs->transparent()) {
            return rhs->id < lhs->id;
        }
        return rhs->transparent();
    });

    volume_texture_.bind();
    for (auto& slice : slices) {
        draw_slice(*slice);
    }
    volume_texture_.unbind();

    cube_program_->use();

    GLint matrix_loc =
        glGetUniformLocation(cube_program_->handle(), "transform_matrix");
    glUniformMatrix4fv(matrix_loc, 1, GL_FALSE, &world_to_screen[0][0]);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBindVertexArray(cube_vao_handle_);
    glDrawArrays(GL_TRIANGLES, 0, 12 * 3);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // TODO low-resolution transparent voxel volume if available?

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
}

bool ReconstructionComponent::handle_mouse_button(int button, bool down) {
    if (down) {
        if (hovering_) {
            switch_if_necessary(recon_drag_machine_kind::translator);
            dragging_ = true;
            return true;
        }
    }

    if (!down) {
        if (dragged_slice_) {
            std::cout << "should set slice: " << dragged_slice_->id << "\n";
            // how do we know if the slice is new, or if it should be updated
            // we dont care, just send a set slice, with same id if it already
            // knows it.. clients responsibility
            auto packet = SetSlicePacket(scene_id_, dragged_slice_->id,
                                         dragged_slice_->packed_orientation());
            object_.send(packet);

            dragged_slice_ = nullptr;
            dragging_ = false;
            return true;
        }
        dragging_ = false;
    }

    return false;
}
bool ReconstructionComponent::handle_mouse_moved(float x, float y) {
    // update slices that is being hovered over
    y = -y;

    if (prev_y_ < -1.0) {
        prev_x_ = x;
        prev_y_ = y;
    }

    glm::vec2 delta(x - prev_x_, y - prev_y_);
    prev_x_ = x;
    prev_y_ = y;

    // TODO: fix for screen ratio ratio
    if (dragging_) {
        drag_machine_->on_drag(delta);
        return true;
    } else {
        check_hovered(x, y);
    }

    return false;
}

int ReconstructionComponent::index_hovering_over(float x, float y) {
    auto intersection_point = [](glm::mat4 inv_matrix, glm::mat4 orientation,
                                 glm::vec2 point) -> std::pair<bool, float> {
        auto intersect_ray_plane = [](glm::vec3 origin, glm::vec3 direction,
                                      glm::vec3 base, glm::vec3 normal,
                                      float& distance) -> bool {
            auto alpha = glm::dot(normal, direction);
            if (glm::abs(alpha) > 0.001f) {
                distance = glm::dot((base - origin), normal) / alpha;
                if (distance >= 0.001f) return true;
            }
            return false;
        };

        // how do we want to do this
        // end points of plane/line?
        // first see where the end
        // points of the square end up
        // within the box.
        // in world space:
        auto o = orientation;
        auto axis1 = glm::vec3(o[0][0], o[0][1], o[0][2]);
        auto axis2 = glm::vec3(o[1][0], o[1][1], o[1][2]);
        auto base = glm::vec3(o[2][0], o[2][1], o[2][2]);
        base += 0.5f * (axis1 + axis2);
        auto normal = glm::normalize(glm::cross(axis1, axis2));
        float distance = -1.0f;

        auto from = inv_matrix * glm::vec4(point.x, point.y, -1.0f, 1.0f);
        from /= from[3];
        auto to = inv_matrix * glm::vec4(point.x, point.y, 1.0f, 1.0f);
        to /= to[3];
        auto direction = glm::normalize(glm::vec3(to) - glm::vec3(from));

        bool does_intersect = intersect_ray_plane(glm::vec3(from), direction,
                                                  base, normal, distance);

        // now check if the actual point is inside the plane
        auto intersection_point = glm::vec3(from) + direction * distance;
        intersection_point -= base;
        auto along_1 = glm::dot(intersection_point, glm::normalize(axis1));
        auto along_2 = glm::dot(intersection_point, glm::normalize(axis2));
        if (glm::abs(along_1) > 0.5f * glm::length(axis1) ||
            glm::abs(along_2) > 0.5f * glm::length(axis2))
            does_intersect = false;

        return std::make_pair(does_intersect, distance);
    };

    auto inv_matrix = glm::inverse(object_.camera().matrix());
    int best_slice_index = -1;
    float best_z = std::numeric_limits<float>::max();
    for (auto& id_slice : slices_) {
        auto& slice = id_slice.second;
        if (slice->inactive) {
            continue;
        }
        slice->hovered = false;
        auto maybe_point =
            intersection_point(inv_matrix, slice->orientation, glm::vec2(x, y));
        if (maybe_point.first) {
            auto z = maybe_point.second;
            if (z < best_z) {
                best_z = z;
                best_slice_index = id_slice.first;
            }
        }
    }

    return best_slice_index;
}

void ReconstructionComponent::check_hovered(float x, float y) {
    int best_slice_index = index_hovering_over(x, y);

    if (best_slice_index >= 0) {
        slices_[best_slice_index]->hovered = true;
        hovering_ = true;
    } else {
        hovering_ = false;
    }
}

void ReconstructionComponent::switch_if_necessary(
    recon_drag_machine_kind kind) {
    if (!drag_machine_ || drag_machine_->kind() != kind) {
        switch (kind) {
            case recon_drag_machine_kind::translator:
                drag_machine_ = std::make_unique<SliceTranslator>(*this);
                break;
            default:
                break;
        }
    }
}

void SliceTranslator::on_drag(glm::vec2 delta) {
    // 1) what are we dragging, and does it have data?
    // if it does then we need to make a new slice
    // else we drag the current slice along the normal
    if (!comp_.dragged_slice()) {
        std::unique_ptr<slice> new_slice;
        int id = (*(comp_.get_slices().rbegin())).first + 1;
        int to_remove = -1;
        for (auto& id_the_slice : comp_.get_slices()) {
            auto& the_slice = id_the_slice.second;
            if (the_slice->hovered) {
                if (the_slice->has_data()) {
                    new_slice = std::make_unique<slice>(id);
                    new_slice->orientation = the_slice->orientation;
                    to_remove = the_slice->id;
                    // FIXME need to generate a new id and upon 'popping'
                    // send a UpdateSlice packet
                    comp_.dragged_slice() = new_slice.get();
                } else {
                    comp_.dragged_slice() = the_slice.get();
                }
                break;
            }
        }
        if (new_slice) {
            comp_.get_slices()[new_slice->id] = std::move(new_slice);
        }
        if (to_remove >= 0) {
            comp_.get_slices().erase(to_remove);
            std::cout << "remove slice: " << to_remove << "\n";
        }
        assert(comp_.dragged_slice());
    }

    auto slice = comp_.dragged_slice();
    auto& o = slice->orientation;

    auto axis1 = glm::vec3(o[0][0], o[0][1], o[0][2]);
    auto axis2 = glm::vec3(o[1][0], o[1][1], o[1][2]);
    auto normal = glm::normalize(glm::cross(axis1, axis2));

    // project the normal vector to screen coordinates
    // FIXME maybe need window matrix here too which would be kind of
    // painful maybe
    auto base_point_normal =
        glm::vec3(o[2][0], o[2][1], o[2][2]) + 0.5f * (axis1 + axis2);
    auto end_point_normal = base_point_normal + normal;

    auto a =
        comp_.object().camera().matrix() * glm::vec4(base_point_normal, 1.0f);
    auto b =
        comp_.object().camera().matrix() * glm::vec4(end_point_normal, 1.0f);
    auto normal_delta = b - a;
    float difference =
        glm::dot(glm::vec2(normal_delta.x, normal_delta.y), delta);

    // take the inner product of delta x and this normal vector

    auto dx = difference * normal;
    // FIXME check if it is still inside the bounding box of the volume
    // probably by checking all four corners are inside bounding box, should
    // define this box somewhere
    o[2][0] += dx[0];
    o[2][1] += dx[1];
    o[2][2] += dx[2];
}

}  // namespace tomovis