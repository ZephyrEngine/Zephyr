
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <zephyr/gpu/buffer.hpp>
#include <zephyr/gpu/texture.hpp>
#include <zephyr/integer.hpp>

namespace zephyr {

// subset of VkDescriptorType:
// https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#VkDescriptorType
enum class BindingType : u32 {
  Sampler = 0,
  ImageWithSampler = 1,
  SampledImage = 2,
  StorageImage = 3,
  UniformBuffer = 6,
  StorageBuffer = 7
};

// subset of VkPipelineBindPoint:
// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineBindPoint.html
enum class PipelineBindPoint : u32 {
  Graphics = 0,
  Compute = 1
};

// subset of VkIndexType:
// https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#VkIndexType
enum class IndexDataType {
  UInt16 = 0,
  UInt32 = 1
};

enum class VertexDataType {
  SInt8,
  UInt8,
  SInt16,
  UInt16,
  UInt32,
  Float16,
  Float32
};

// subset of VkPolygonMode:
// https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkPolygonMode.html
enum class PolygonMode {
  Fill = 0,
  Line = 1,
  Point = 2
};

// subset of VkCullModeFlagBits:
// https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkCullModeFlagBits.html
enum class PolygonFace {
  None = 0,
  Front = 1,
  Back = 2
};

// equivalent to VkCompareOp:
// https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkCompareOp.html
enum class CompareOp {
  Never = 0,
  Less = 1,
  Equal = 2,
  LessOrEqual = 3,
  Greater = 4,
  NotEqual = 5,
  GreaterOrEqual = 6,
  Always = 7
};

// equivalent to VkPrimitiveTopology
// https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkPrimitiveTopology.html
enum class PrimitiveTopology {
  PointList = 0,
  LineList = 1,
  LineStrip = 2,
  TriangleList = 3,
  TriangleStrip = 4,
  TriangleFan = 5,
  LineListWithAdjacency = 6,
  LineStripWithAdjacency = 7,
  TriangleListWithAdjacency = 8,
  TriangleStripWithAdjacency = 9,
  PatchList = 10
};

// equivalent to VkVertexInputRate
// https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkVertexInputRate.html
enum class VertexInputRate {
  Vertex = 0,
  Instance = 1
};

// equivalent to VkBlendFactor
// https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkBlendFactor.html
enum class BlendFactor {
  Zero = 0,
  One = 1,
  SrcColor = 2,
  OneMinusSrcColor = 3,
  DstColor = 4,
  OneMinusDstColor = 5,
  SrcAlpha = 6,
  OneMinusSrcAlpha = 7,
  DstAlpha = 8,
  OneMinusDstAlpha = 9,
  ConstantColor = 10,
  OneMinusConstantColor = 11,
  ConstantAlpha = 12,
  OneMinusConstantAlpha = 13,
  SrcAlphaSaturate = 14,
  Src1Color = 15,
  OneMinusSrc1Color = 16,
  Src1Alpha = 17,
  OneMinusSrc1Alpha = 18
};

// subset of VkBlendOp:
// https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkBlendOp.html
enum class BlendOp {
  Add = 0,
  Subtract = 1,
  ReverseSubtract = 2,
  Min = 3,
  Max = 4
};

// equivalent to VkColorComponentFlagBits:
// https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkColorComponentFlagBits.html
enum class ColorComponent {
  R = 1,
  G = 2,
  B = 4,
  A = 8,
  RGB = R | G | B,
  RGBA = R | G | B | A
};

constexpr auto operator|(ColorComponent lhs, ColorComponent rhs) -> ColorComponent {
  return static_cast<ColorComponent>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

// subset of VkShaderStageFlagBits:
// https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#VkShaderStageFlagBits
enum class ShaderStage : u32 {
  Vertex = 0x00000001,
  TessellationControl = 0x00000002,
  TessellationEvaluation = 0x00000004,
  Geometry = 0x00000008,
  Fragment = 0x00000010,
  Compute = 0x00000020,
  AllGraphics = Vertex | TessellationControl | TessellationEvaluation | Geometry | Fragment,
  All = AllGraphics | Compute
};

constexpr auto operator|(ShaderStage lhs, ShaderStage rhs) -> ShaderStage {
  return (ShaderStage)((u32)lhs | (u32)rhs);
}

// subset of VkPipelineStageFlagBits:
// https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkPipelineStageFlagBits.html
enum class PipelineStage : u32 {
  None = 0,
  TopOfPipe = 0x00000001,
  DrawIndirect = 0x00000002,
  VertexInput = 0x00000004,
  VertexShader = 0x00000008,
  TessellationControlShader = 0x00000010,
  TessellationEvaluationShader = 0x00000020,
  GeometryShader = 0x00000040,
  FragmentShader = 0x00000080,
  EarlyFragmentTests = 0x00000100,
  LateFragmentTests = 0x00000200,
  ColorAttachmentOutput = 0x00000400,
  ComputeShader = 0x00000800,
  Transfer = 0x00001000,
  BottomOfPipe = 0x00002000,
  Host = 0x00004000,
  AllGraphics = 0x00008000,
  AllCommands = 0x00010000
};

constexpr auto operator|(PipelineStage lhs, PipelineStage rhs) -> PipelineStage {
  return static_cast<PipelineStage>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

// subset of VkAccessFlagBits:
// https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkAccessFlagBits.html
enum class Access : u32 {
  None = 0,
  IndirectCommandRead = 0x0000'0001,
  IndexRead = 0x0000'0002,
  VertexAttributeRead = 0x0000'0004,
  UniformRead = 0x0000'0008,
  InputAttachmentRead = 0x0000'0010,
  ShaderRead = 0x0000'0020,
  ShaderWrite = 0x0000'0040,
  ColorAttachmentRead = 0x0000'0080,
  ColorAttachmentWrite = 0x0000'0100,
  DepthStencilAttachmentRead = 0x0000'0200,
  DepthStencilAttachmentWrite = 0x0000'0400,
  TransferRead = 0x0000'0800,
  TransferWrite = 0x0000'1000,
  HostRead = 0x0000'2000,
  HostWrite = 0x0000'4000,
  MemoryRead = 0x0000'8000,
  MemoryWrite = 0x0001'0000
};

constexpr auto operator|(Access lhs, Access rhs) -> Access {
  return static_cast<Access>(static_cast<int>(lhs) | static_cast<u32>(rhs));
}

struct Rect2D {
  int x;
  int y;
  int width;
  int height;
};

} // namespace zephyr
