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
// IdType SetGeneration(IdType id, generation_type gen)
//    Sets the generation for the passed id, returning the new id
//
// IdType SetIndex(IdType id, index_type index)
//    Sets the index for the passed id, returning the new id
//

#include <types.h>

#include <cassert>
#include <type_traits>
#include <cstring>

using id_type = u32;

constexpr u64 cU64InvalidId  = 0xffff'ffff'ffff'ffffui64;
constexpr u32 cU32InvalidId  = 0xffff'ffffui32;
constexpr u16 cU16InvalidId  = 0xffffui16;
constexpr u8  cU8InvalidId   = 0xffui8;

constexpr id_type cIdMask     = static_cast<id_type>(-1);
constexpr id_type cInvalidId  = cIdMask;

namespace id_type_internal 
{
	constexpr u32     cGenerationBits = 8;
	constexpr u32     cIndexBits      = sizeof(id_type) * 8 - cGenerationBits;
	constexpr id_type cIndexMask      = (id_type{1} << cIndexBits) - 1;
	constexpr id_type cGenerationMask = (id_type{1} << cGenerationBits) - 1;
} // end id_type_internal

using generation_type = std::conditional_t<id_type_internal::cGenerationBits <= 16, std::conditional_t<id_type_internal::cGenerationBits <= 8, u8, u16>, u32>;
static_assert(sizeof(generation_type) * 8 >= id_type_internal::cGenerationBits);
static_assert(sizeof(id_type) - sizeof(generation_type) > 0);

using index_type = std::conditional_t<id_type_internal::cGenerationBits <= 32, std::conditional_t<id_type_internal::cIndexBits <= 16, u16, u32>, u64>;
static_assert(sizeof(index_type) * 8 >= id_type_internal::cIndexBits);
static_assert(sizeof(id_type) - sizeof(index_type) >= 0);

constexpr bool 
IsValid(id_type id)
{
	return id != cIdMask;
}

constexpr index_type
GetIndex(id_type id)
{
	return id & id_type_internal::cIndexMask;
}

constexpr generation_type
GetGeneration(id_type id)
{
	return (id >> id_type_internal::cIndexBits) & id_type_internal::cGenerationMask;
}

constexpr id_type 
IncGeneration(id_type id)
{
	const id_type generation = GetGeneration(id) + 1;
	assert(generation < 255); // NOTE(enlynn): comment to allow for overflow
	return GetIndex(id) | (generation << id_type_internal::cIndexBits);
}

constexpr id_type 
SetGeneration(id_type id, generation_type gen)
{
	return GetIndex(id) | (gen << id_type_internal::cIndexBits);
}

constexpr id_type 
SetIndex(id_type id, index_type index)
{
	return index | (GetGeneration(id) << id_type_internal::cIndexBits);
}

#if DEBUG_BUILD
namespace id_type_internal 
{
	struct id_base 
	{
		constexpr explicit id_base(const id_type p_id) : id(p_id) {}
		constexpr explicit operator id_type() const { return id; }
	protected:
		id_type id = cIdMask;
	};
} //id_type_internal

#define DEFINE_TYPE_ID(name)                                 \
	struct name final : public id_type_internal::id_base     \
	{                                                        \
		constexpr explicit name(const id_type p_id)          \
			: id_base(p_id) {}                               \
		constexpr explicit name() : id_base(cIdMask) {}      \
		constexpr operator id_type() const { return id; }    \
	};
#else
	#define DEFINE_TYPE_ID(name)         using name = id_type;
#endif

template<typename id_subtype, u32 cMaxIndices, u32 cMinFreeIndices = 10>
struct id_generator
{
    id_generator()
    {
        ForRange(u32, i, cMaxIndices)
            mGenerations[i] = 0;

        ForRange(u32, i, cMaxIndices)
            mFreeIndices[i] = 0;

        mFreeIndicesCount = 0;
        mNextIndex        = 0;
    }

    id_subtype AcquireId()
    {
        index_type      Index      = 0;
        generation_type Generation = 0;

        if (mFreeIndicesCount > cMinFreeIndices)
        {
            Index      = mFreeIndices[0];
            Generation = mGenerations[Index];

            mFreeIndicesCount -= 1;
            memmove(&mFreeIndices[0], &mFreeIndices[1], mFreeIndicesCount);
        }
        else
        {
            assert(mNextIndex < cMaxIndices);
            Index       = mNextIndex;
            mNextIndex += 1;
        }

        id_type Result = 0;
        SetIndex(Result, Index);
        SetGeneration(Result, Generation);

        return id_subtype(Result);
    }

    void ReleaseId(id_subtype Id)
    {
        if (IsIdValid(Id))
        {
            index_type      Index      = GetIndex(Id);

            mGenerations[Index]            += 1;     // Overflows are allowed, invalidates the id
            mFreeIndices[mFreeIndicesCount] = Index;
            mFreeIndicesCount              += 1;
        }
    }

    bool IsIdValid(id_subtype Id)
    {
        index_type Index = GetIndex(Id);

        bool IsValidId    = Id != cInvalidId;
        bool IsGenValid   = (Index < cMaxIndices) ? GetGeneration(Id) == mGenerations[Index] : false;

        return IsValidId && IsGenValid;
    }

private:
    generation_type mGenerations[cMaxIndices];
    id_type         mFreeIndices[cMaxIndices];
    u32             mNextIndex        = 0;
    u32             mFreeIndicesCount = 0;
};
