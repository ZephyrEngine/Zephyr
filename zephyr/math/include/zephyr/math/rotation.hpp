
#pragma once

#include <zephyr/math/quaternion.hpp>
#include <zephyr/float.hpp>

namespace zephyr {

  /**
   * Convert a unit quaternion to extrinsic x-y-z (intrinsic z-y'-x'') Tait-Bryan angles.
   * @param quaternion The unit quaternion
   * @returns a {@link #Vector3} storing the X, Y and Z angles.
   */
  inline Vector3 quaternion_to_extrinsic_xyz_angles(const Quaternion& quaternion) {
    Vector3 euler;

    const f32 wy = quaternion.W() * quaternion.Y();
    const f32 wz = quaternion.W() * quaternion.Z();
    const f32 xz = quaternion.X() * quaternion.Z();
    const f32 xy = quaternion.X() * quaternion.Y();
    const f32 yy = quaternion.Y() * quaternion.Y();
    const f32 zz = quaternion.Z() * quaternion.Z();

    const f32 m00 = 1 - 2 * (zz + yy);
    const f32 m01 = 2 * (xy + wz);
    const bool gimbal_lock = std::sqrt(m00 * m00 + m01 * m01) < 1e-6;

    euler.Y() = std::asin(std::clamp(-2.0f * (xz - wy), -1.0f, +1.0f));

    // Guard against gimbal lock when Y=-90°/+90° (X and Z rotate around the same axis).
    if(!gimbal_lock) {
      const f32 wx = quaternion.W() * quaternion.X();
      const f32 xx = quaternion.X() * quaternion.X();
      const f32 yz = quaternion.Y() * quaternion.Z();

      euler.X() = std::atan2(yz + wx, 0.5f - (xx + yy));
      euler.Z() = std::atan2(xy + wz, 0.5f - (zz + yy));
    } else {
      euler.X() = std::atan2(xy - wz, xz + wy);
      euler.Z() = 0.0f;
    }

    return euler;
  }

  /**
   * Convert extrinsic x-y-z (intrinsic z-y'-x'') Tait-Bryan angles into an unit quaternion.
   * @param euler A {@link #Vector3} storing the extrinsic x-y-z Tait-Bryan angles
   * @returns a {@link #Quaternion}
   */
  inline Quaternion extrinsic_xyz_angles_to_quaternion(const Vector3& euler) {
    const f32 half_x = euler.X() * 0.5f;
    const f32 cos_x = std::cos(half_x);
    const f32 sin_x = std::sin(half_x);

    const f32 half_y = euler.Y() * 0.5f;
    const f32 cos_y = std::cos(half_y);
    const f32 sin_y = std::sin(half_y);

    const f32 half_z = euler.Z() * 0.5f;
    const f32 cos_z = std::cos(half_z);
    const f32 sin_z = std::sin(half_z);

    const f32 cos_z_cos_y = cos_z * cos_y;
    const f32 sin_z_cos_y = sin_z * cos_y;
    const f32 sin_z_sin_y = sin_z * sin_y;
    const f32 cos_z_sin_y = cos_z * sin_y;

    return {
      cos_z_cos_y * cos_x + sin_z_sin_y * sin_x,
      cos_z_cos_y * sin_x - sin_z_sin_y * cos_x,
      sin_z_cos_y * sin_x + cos_z_sin_y * cos_x,
      sin_z_cos_y * cos_x - cos_z_sin_y * sin_x
    };
  }

} // namespace zephyr
