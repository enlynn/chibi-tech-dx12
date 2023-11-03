#pragma once

//
// Usage:
//
// - Define an Id type:
//      DEFINE_TYPE_ID(FooId)         // creates an Id that will be exported from its module
//
// bool IsValid(IdType id)
//     An id is valid if it does not equal its invalid mask
//
// IdType ToIndex(IdType id)
//     Decomposes an id to its index value
//
// IdType ToGeneration(IdType id)
//     Decomposes an id to its generational value
//
// IdType IncGeneration(IdType id)
//    Increments the generation of the id, returning the new id
//
// IdType SetGeneration(IdType id, GenerationType gen)
//    Sets the generation for the passed id, returning the new id
//
// IdType SetIndex(IdType id, IndexType index)
//    Sets the index for the passed id, returning the new id
//

#include <types.h>

#include <cassert>
#include <type_traits>

using id_type = u32;

constexpr u64 cU64InvalidId  = 0xffff'ffff'ffff'ffffui64;
constexpr u32 cU32InvalidId  = 0xffff'ffffui32;
constexpr u16 cU16InvalidId  = 0xffffui16;
constexpr u8  cU8InvalidId   = 0xffui8;

constexpr id_type cIdMask     = static_cast<id_type>(-1);

namespace id_type_internal 
{
	constexpr u32     cGenerationBits = 8;
	constexpr u32     cIndexBits      = sizeof(id_type) * 8 - cGenerationBits;
	constexpr id_type cIndexMask      = (id_type{1} << cIndexBits) - 1;
	constexpr id_type cGenerationMask = (id_type{1} << cGenerationBits) - 1;
} // end id_type_internal

using GenerationType = std::conditional_t<id_type_internal::cGenerationBits <= 16, std::conditional_t<id_type_internal::cGenerationBits <= 8, u8, u16>, u32>;
static_assert(sizeof(GenerationType) * 8 >= id_type_internal::cGenerationBits);
static_assert(sizeof(id_type) - sizeof(GenerationType) > 0);

using IndexType = std::conditional_t<id_type_internal::cGenerationBits <= 32, std::conditional_t<id_type_internal::cIndexBits <= 16, u16, u32>, u64>;
static_assert(sizeof(IndexType) * 8 >= id_type_internal::cIndexBits);
static_assert(sizeof(id_type) - sizeof(IndexType) >= 0);

constexpr bool 
IsValid(id_type id)
{
	return id != cIdMask;
}

constexpr id_type 
ToIndex(id_type id)
{
	return id & id_type_internal::cIndexMask;
}

constexpr id_type 
ToGeneration(id_type id)
{
	return (id >> id_type_internal::cIndexBits) & id_type_internal::cGenerationMask;
}

constexpr id_type 
IncGeneration(id_type id)
{
	const id_type generation = ToGeneration(id) + 1;
	assert(generation < 255); // NOTE(enlynn): comment to allow for overflow
	return ToIndex(id) | (generation << id_type_internal::cIndexBits);
}

constexpr id_type 
SetGeneration(id_type id, GenerationType gen)
{
	return ToIndex(id) | (gen << id_type_internal::cIndexBits);
}

constexpr id_type 
SetIndex(id_type id, IndexType index)
{
	return index | (ToGeneration(id) << id_type_internal::cIndexBits);
}

#if DEBUG_BUILD
namespace id_type_internal 
{
	struct id_base 
	{
		constexpr explicit id_base(id_type p_id) : id(p_id) {}
		constexpr operator id_type() const { return id; }
	private:
		id_type id;
	};
} //id_type_internal

#define DEFINE_TYPE_ID(name)                                 \
	struct name final : id_type_internal::id_base            \
	{                                                        \
		constexpr explicit name(id_type p_id)                \
			: id_base(p_id) {}                               \
		constexpr explicit name() : id_base(id::cIdMask) {}  \
	};

#else
	#define DEFINE_TYPE_ID(name)         using name = id_type;
#endif