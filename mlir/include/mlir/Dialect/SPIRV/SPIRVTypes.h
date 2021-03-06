//===- SPIRVTypes.h - MLIR SPIR-V Types -------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares the types in the SPIR-V dialect.
//
//===----------------------------------------------------------------------===//

#ifndef MLIR_DIALECT_SPIRV_SPIRVTYPES_H_
#define MLIR_DIALECT_SPIRV_SPIRVTYPES_H_

#include "mlir/IR/StandardTypes.h"
#include "mlir/IR/TypeSupport.h"
#include "mlir/IR/Types.h"

#include <tuple>

// Forward declare enum classes related to op availability. Their definitions
// are in the TableGen'erated SPIRVEnums.h.inc and can be referenced by other
// declarations in SPIRVEnums.h.inc.
namespace mlir {
namespace spirv {
enum class Version : uint32_t;
enum class Extension;
enum class Capability : uint32_t;
} // namespace spirv
} // namespace mlir

// Pull in all enum type definitions and utility function declarations
#include "mlir/Dialect/SPIRV/SPIRVEnums.h.inc"
// Pull in all enum type availability query function declarations
#include "mlir/Dialect/SPIRV/SPIRVEnumAvailability.h.inc"

namespace mlir {
namespace spirv {
/// Returns the implied extensions for the given version. These extensions are
/// incorporated into the current version so they are implicitly declared when
/// targeting the given version.
ArrayRef<Extension> getImpliedExtensions(Version version);

/// Returns the directly implied capabilities for the given capability. These
/// capabilities are implicitly declared by the given capability.
ArrayRef<Capability> getDirectImpliedCapabilities(Capability cap);
/// Returns the recursively implied capabilities for the given capability. These
/// capabilities are implicitly declared by the given capability. Compared to
/// the above function, this function collects implied capabilities recursively:
/// if an implicitly declared capability implicitly declares a third one, the
/// third one will also be returned.
SmallVector<Capability, 0> getRecursiveImpliedCapabilities(Capability cap);

namespace detail {
struct ArrayTypeStorage;
struct ImageTypeStorage;
struct PointerTypeStorage;
struct RuntimeArrayTypeStorage;
struct StructTypeStorage;
} // namespace detail

namespace TypeKind {
enum Kind {
  Array = Type::FIRST_SPIRV_TYPE,
  Image,
  Pointer,
  RuntimeArray,
  Struct,
  LAST_SPIRV_TYPE = Struct,
};
}

// SPIR-V composite type: VectorType, SPIR-V ArrayType, or SPIR-V StructType.
class CompositeType : public Type {
public:
  using Type::Type;

  static bool classof(Type type);

  unsigned getNumElements() const;

  Type getElementType(unsigned) const;
};

// SPIR-V array type
class ArrayType : public Type::TypeBase<ArrayType, CompositeType,
                                        detail::ArrayTypeStorage> {
public:
  using Base::Base;
  // Zero layout specifies that is no layout
  using LayoutInfo = uint64_t;

  static bool kindof(unsigned kind) { return kind == TypeKind::Array; }

  static ArrayType get(Type elementType, unsigned elementCount);

  static ArrayType get(Type elementType, unsigned elementCount,
                       LayoutInfo layoutInfo);

  unsigned getNumElements() const;

  Type getElementType() const;

  bool hasLayout() const;

  uint64_t getArrayStride() const;
};

// SPIR-V image type
class ImageType
    : public Type::TypeBase<ImageType, Type, detail::ImageTypeStorage> {
public:
  using Base::Base;

  static bool kindof(unsigned kind) { return kind == TypeKind::Image; }

  static ImageType
  get(Type elementType, Dim dim,
      ImageDepthInfo depth = ImageDepthInfo::DepthUnknown,
      ImageArrayedInfo arrayed = ImageArrayedInfo::NonArrayed,
      ImageSamplingInfo samplingInfo = ImageSamplingInfo::SingleSampled,
      ImageSamplerUseInfo samplerUse = ImageSamplerUseInfo::SamplerUnknown,
      ImageFormat format = ImageFormat::Unknown) {
    return ImageType::get(
        std::tuple<Type, Dim, ImageDepthInfo, ImageArrayedInfo,
                   ImageSamplingInfo, ImageSamplerUseInfo, ImageFormat>(
            elementType, dim, depth, arrayed, samplingInfo, samplerUse,
            format));
  }

  static ImageType
      get(std::tuple<Type, Dim, ImageDepthInfo, ImageArrayedInfo,
                     ImageSamplingInfo, ImageSamplerUseInfo, ImageFormat>);

  Type getElementType() const;
  Dim getDim() const;
  ImageDepthInfo getDepthInfo() const;
  ImageArrayedInfo getArrayedInfo() const;
  ImageSamplingInfo getSamplingInfo() const;
  ImageSamplerUseInfo getSamplerUseInfo() const;
  ImageFormat getImageFormat() const;
  // TODO(ravishankarm): Add support for Access qualifier
};

// SPIR-V pointer type
class PointerType
    : public Type::TypeBase<PointerType, Type, detail::PointerTypeStorage> {
public:
  using Base::Base;

  static bool kindof(unsigned kind) { return kind == TypeKind::Pointer; }

  static PointerType get(Type pointeeType, StorageClass storageClass);

  Type getPointeeType() const;

  StorageClass getStorageClass() const;
};

// SPIR-V run-time array type
class RuntimeArrayType
    : public Type::TypeBase<RuntimeArrayType, Type,
                            detail::RuntimeArrayTypeStorage> {
public:
  using Base::Base;

  static bool kindof(unsigned kind) { return kind == TypeKind::RuntimeArray; }

  static RuntimeArrayType get(Type elementType);

  Type getElementType() const;
};

// SPIR-V struct type
class StructType : public Type::TypeBase<StructType, CompositeType,
                                         detail::StructTypeStorage> {
public:
  using Base::Base;

  // Layout information used for members in a struct in SPIR-V
  //
  // TODO(ravishankarm) : For now this only supports the offset type, so uses
  // uint64_t value to represent the offset, with
  // std::numeric_limit<uint64_t>::max indicating no offset. Change this to
  // something that can hold all the information needed for different member
  // types
  using LayoutInfo = uint64_t;

  using MemberDecorationInfo = std::pair<uint32_t, spirv::Decoration>;

  static bool kindof(unsigned kind) { return kind == TypeKind::Struct; }

  /// Construct a StructType with at least one member.
  static StructType get(ArrayRef<Type> memberTypes,
                        ArrayRef<LayoutInfo> layoutInfo = {},
                        ArrayRef<MemberDecorationInfo> memberDecorations = {});

  /// Construct a struct with no members.
  static StructType getEmpty(MLIRContext *context);

  unsigned getNumElements() const;

  Type getElementType(unsigned) const;

  bool hasLayout() const;

  uint64_t getOffset(unsigned) const;

  // Returns in `allMemberDecorations` the spirv::Decorations (apart from
  // Offset) associated with all members of the StructType.
  void getMemberDecorations(SmallVectorImpl<StructType::MemberDecorationInfo>
                                &allMemberDecorations) const;

  // Returns in `memberDecorations` all the spirv::Decorations (apart from
  // Offset) associated with the `i`-th member of the StructType.
  void getMemberDecorations(
      unsigned i, SmallVectorImpl<spirv::Decoration> &memberDecorations) const;
};

} // end namespace spirv
} // end namespace mlir

#endif // MLIR_DIALECT_SPIRV_SPIRVTYPES_H_
