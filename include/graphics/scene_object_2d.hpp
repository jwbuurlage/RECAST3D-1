#pragma once

#include "scene_object.hpp"


namespace tomovis {

class SceneObject2d : public SceneObject {
  public:
    SceneObject2d();
    ~SceneObject2d();

    void draw(glm::mat4 window_matrix) override;

  private:
    static constexpr int count_ = 20;
    GLuint texture_id_;
};

} // namespace tomovis