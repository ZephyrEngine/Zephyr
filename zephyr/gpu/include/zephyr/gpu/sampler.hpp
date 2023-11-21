
#pragma once

namespace zephyr {

  class Sampler {
    public:
      // subset of VkSamplerAddressMode:
      // https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#_vksampleraddressmode3
      enum class AddressMode {
        Repeat = 0,
        MirroredRepeat = 1,
        ClampToEdge = 2
      };

      // subset of VkFilter that is equivalent to VkSamplerMipmapMode:
      // https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#_vkfilter3
      // https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#VkSamplerMipmapMode
      enum class FilterMode {
        Nearest = 0,
        Linear = 1
      };

      // @todo: expose LOD, comparator and anisotropy configuration.
      struct Config {
        AddressMode address_mode_u = AddressMode::ClampToEdge;
        AddressMode address_mode_v = AddressMode::ClampToEdge;
        AddressMode address_mode_w = AddressMode::ClampToEdge;
        FilterMode mag_filter = FilterMode::Nearest;
        FilterMode min_filter = FilterMode::Nearest;
        FilterMode mip_filter = FilterMode::Nearest;
        bool anisotropy = false;
        int max_anisotropy = 1;
      };

      virtual ~Sampler() = default;

      virtual void* Handle() = 0;
  };

} // namespace zephyr
