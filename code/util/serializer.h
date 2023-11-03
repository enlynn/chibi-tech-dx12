#pragma once 

#include <types.h>
#include "str8.h"
#include "str16.h"

using file_handle = void*;
using file_info   = void*;

//
// Serializable Interfaces for reading and writing binary data
// to files. A user should construct a file_reader or file_writer
// depending on the use case, and simply call "Serialize" or "Deserialze"
// on a type they want to read/write from the file.
//
// In order to serialize custom types, simply implement a version of
// Serialize and Deserialize:
//      template<your_type> constexpr void      Serialize(file_writer* Writer, your_type& Value);
//      template<your_type> constexpr your_type Deserialize(file_writer* Reader);
//
// Ideally, a user shouldn't have to directly call "Read", "ReadChunk", or
// "Write" functions from the file_reader/file_writer. A set of all basic types is provided
// so the user should be able to continue to call serializable on sub-types.
//
// For, example, let's say I had a struct I wanted to serialize:
//
// struct foo {
//      boo  Boo;
//      str8 Name;
//      f32  SomeFloat;
// }
// 
// template<foo> constexpr void Serialize(file_writer* Writer, foo& Value) {
//      Serialize(Writer, Value.Boo);
//      Serialize(Writer, Value.Name);
//      Serialize(Writer, Value.SomeFloat);
// }
//
// f32 is basic type so it's Serialize function is implemented by default. 
// str8 is an engine type, so Serialize will also be implemented by default. 
// boo is a user custom type, so will need to implement Serialize too. If 
// not implemented, then a compile error will occur.
//

struct file_reader
{
    u8*         mBasePtr;
    u8*         mOffsetPtr;
    u8*         mEndPtr;

    file_handle mFile;

    void Read(void* Data, u64 DataSize);
    // Might need to read the file in chunks.
    void ReadNextChunk(file_handle File);
};

struct file_writer
{
    u8*         mBasePtr;
    u8*         mOffsetPtr;
    u8*         mEndPtr;

    file_handle mFile;

    void Write(void* Data, u64 DataSize);
};

// Serializable Interface for types that get written to disc
template<typename T> constexpr void Serialize(file_writer* Writer, T Value) = delete;
// Deserializable Interface for types that get read from disc
template<typename T> constexpr T    Deserialize(file_reader* Reader)        = delete;

//
// Basic Serializable Types
//

template<s8>   constexpr void Serialize(file_writer* Writer, s8  Value);
template<s16>  constexpr void Serialize(file_writer* Writer, s16 Value);
template<s32>  constexpr void Serialize(file_writer* Writer, s32 Value);
template<s64>  constexpr void Serialize(file_writer* Writer, s64 Value);

template<u8>   constexpr void Serialize(file_writer* Writer, u8  Value);
template<u16>  constexpr void Serialize(file_writer* Writer, u16 Value);
template<u32>  constexpr void Serialize(file_writer* Writer, u32 Value);
template<u64>  constexpr void Serialize(file_writer* Writer, u64 Value);

template<f32>  constexpr void Serialize(file_writer* Writer, f32 Value);
template<f64>  constexpr void Serialize(file_writer* Writer, f64 Value);

template<bool> constexpr void Serialize(file_writer* Writer, bool Value);

// TODO: Move to string header.
// Null terminated UTF8 strings.  Format: { int, char8_t[] }
//template<istr8>  constexpr void Serialize(file_writer* Writer, istr8  Value);
// Null terminated UTF16 strings. Format: { int, char16_t[] }
//template<istr16> constexpr void Serialize(file_writer* Writer, istr16 Value);

//
// Basic Deserializable Types
//

template<s8>   constexpr s8   Deserialize(file_reader* Reader);
template<s16>  constexpr s16  Deserialize(file_reader* Reader);
template<s32>  constexpr s32  Deserialize(file_reader* Reader);
template<s64>  constexpr s64  Deserialize(file_reader* Reader);

template<u8>   constexpr u8   Deserialize(file_reader* Reader);
template<u16>  constexpr u16  Deserialize(file_reader* Reader);
template<u32>  constexpr u32  Deserialize(file_reader* Reader);
template<u64>  constexpr u64  Deserialize(file_reader* Reader);

template<f32>  constexpr f32  Deserialize(file_reader* Reader);
template<f64>  constexpr f64  Deserialize(file_reader* Reader);

template<bool> constexpr bool Deserialize(file_reader* Reader);