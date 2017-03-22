#pragma once

#include <memory>

#include "scene_camera_3d.hpp"
#include "scene_object.hpp"
#include "slice.hpp"

namespace tomovis {

class SceneObject3d : public SceneObject {
   public:
    SceneObject3d(int scene_id);
    ~SceneObject3d();

    void draw(glm::mat4 window_matrix) override;

   private:
    // overwrite specific (3d) camera to use
    std::map<int, std::unique_ptr<slice>> slices_;
};

}  // namespace tomovis