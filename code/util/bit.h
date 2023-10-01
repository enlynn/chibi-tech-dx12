#pragma once

#include <types.h>

// Retrieve the BitMask for Unsigned Ints only.
template<typename T> constexpr T   BitMask(T BitIndex)   = delete;
template<>           constexpr u8  BitMask(u8 BitIndex)  { assert(BitIndex < 8);  return (u8)((u8)1 << BitIndex); }
template<>           constexpr u16 BitMask(u16 BitIndex) { assert(BitIndex < 16); return (u16)((u16)1 << BitIndex); }
template<>           constexpr u32 BitMask(u32 BitIndex) { assert(BitIndex < 32); return (u32)((u32)1 << BitIndex); }
template<>           constexpr u64 BitMask(u64 BitIndex) { assert(BitIndex < 64); return (u64)((u64)1 << BitIndex); }

// Set the Bit for Unsigned Ints only.
template<typename T> constexpr T   BitSet(T   Value, T   BitIndex) = delete;
template<>           constexpr u8  BitSet(u8  Value, u8  BitIndex) { assert(BitIndex < 8);  return Value | BitMask(BitIndex); }
template<>           constexpr u16 BitSet(u16 Value, u16 BitIndex) { assert(BitIndex < 16); return Value | BitMask(BitIndex); }
template<>           constexpr u32 BitSet(u32 Value, u32 BitIndex) { assert(BitIndex < 32); return Value | BitMask(BitIndex); }
template<>           constexpr u64 BitSet(u64 Value, u64 BitIndex) { assert(BitIndex < 64); return Value | BitMask(BitIndex); }

// Clear the Bit for Unsigned Ints only.
template<typename T> constexpr T   BitClear(T   Value, T   BitIndex) = delete;
template<>           constexpr u8  BitClear(u8  Value, u8  BitIndex) { assert(BitIndex < 8);  return Value & ~BitMask(BitIndex); }
template<>           constexpr u16 BitClear(u16 Value, u16 BitIndex) { assert(BitIndex < 16); return Value & ~BitMask(BitIndex); }
template<>           constexpr u32 BitClear(u32 Value, u32 BitIndex) { assert(BitIndex < 32); return Value & ~BitMask(BitIndex); }
template<>           constexpr u64 BitClear(u64 Value, u64 BitIndex) { assert(BitIndex < 64); return Value & ~BitMask(BitIndex); }

// Flip the Bit for Unsigned Ints only.
template<typename T> constexpr T   BitFlip(T   Value, T   BitIndex) = delete;
template<>           constexpr u8  BitFlip(u8  Value, u8  BitIndex) { assert(BitIndex < 8);  return Value ^ BitMask(BitIndex); }
template<>           constexpr u16 BitFlip(u16 Value, u16 BitIndex) { assert(BitIndex < 16); return Value ^ BitMask(BitIndex); }
template<>           constexpr u32 BitFlip(u32 Value, u32 BitIndex) { assert(BitIndex < 32); return Value ^ BitMask(BitIndex); }
template<>           constexpr u64 BitFlip(u64 Value, u64 BitIndex) { assert(BitIndex < 64); return Value ^ BitMask(BitIndex); }

// Mask a bitfield for Unsigned Ints only.
template<typename T> constexpr T   BitMaskClear(T   Value, T   BitMask) = delete;
template<>           constexpr u8  BitMaskClear(u8  Value, u8  BitMask) { return Value & ~BitMask; }
template<>           constexpr u16 BitMaskClear(u16 Value, u16 BitMask) { return Value & ~BitMask; }
template<>           constexpr u32 BitMaskClear(u32 Value, u32 BitMask) { return Value & ~BitMask; }
template<>           constexpr u64 BitMaskClear(u64 Value, u64 BitMask) { return Value & ~BitMask; }

// Determine if a bit is set in a bitfield
template<typename T> constexpr T   IsBitSet(T   Value, T   BitIndex) = delete;
template<>           constexpr u8  IsBitSet(u8  Value, u8  BitIndex) { assert(BitIndex < 8);  return ((Value >> BitIndex) & ((u8) 1)) != 0; }
template<>           constexpr u16 IsBitSet(u16 Value, u16 BitIndex) { assert(BitIndex < 16); return ((Value >> BitIndex) & ((u16)1)) != 0; }
template<>           constexpr u32 IsBitSet(u32 Value, u32 BitIndex) { assert(BitIndex < 32); return ((Value >> BitIndex) & ((u32)1)) != 0; }
template<>           constexpr u64 IsBitSet(u64 Value, u64 BitIndex) { assert(BitIndex < 64); return ((Value >> BitIndex) & ((u64)1)) != 0; }

template<typename T, u32 MaxEnumValue>
class bitset
{
public:
	bitset()  = default;
	~bitset() = default;

	constexpr bitset& Set(T Enum)
	{
		u64 EnumValue = (u64)Enum;

		u64 Index = EnumValue / sizeof(mBits[0]);
		u64 Bit = EnumValue   % sizeof(mBits[0]);

		assert(Index < ArrayCount(mBits));

		mBits[Index] = BitSet(mBits[Index], Index);

		return *this;
	}

	constexpr bitset& Unset(T Enum)
	{
		u64 EnumValue = (u64)Enum;

		u64 Index = EnumValue / sizeof(mBits[0]);
		u64 Bit = EnumValue   % sizeof(mBits[0]);

		assert(Index < ArrayCount(mBits));

		mBits[Index] = BitClear(mBits[Index], Index);

		return *this;
	}

	constexpr bool IsSet(T Enum)
	{
		u64 EnumValue = (u64)Enum;

		u64 Index = EnumValue / sizeof(mBits[0]);
		u64 Bit = EnumValue   % sizeof(mBits[0]);

		assert(Index < ArrayCount(mBits));

		return IsBitSet(mBits[Index], Index);
	}

private:
	u64 mBits[DivideCeil(MaxEnumValue, 64u)] = {};
};