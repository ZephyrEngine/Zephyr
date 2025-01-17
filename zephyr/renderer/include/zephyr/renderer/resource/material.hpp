
#pragma once

#include <zephyr/renderer/resource/resource.hpp>
#include <zephyr/renderer/resource/texture_2d.hpp>
#include <memory>

namespace zephyr {

class Material : public Resource {
  public:
    std::shared_ptr<Texture2D> m_diffuse_map{};
};

} // namespace zephyr
