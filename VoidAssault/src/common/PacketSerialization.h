#pragma once
#include "raylib.h"
#include <vector>
#include <cstdint>


#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/vector.h>
#include <bitsery/traits/string.h>

using Buffer = std::vector<uint8_t>;

using OutputAdapter = bitsery::OutputBufferAdapter<Buffer>;
using InputAdapter = bitsery::InputBufferAdapter<Buffer>;


template <typename S>
void serialize(S& s, Vector2& v) {
    s.value4b(v.x);
    s.value4b(v.y);
}

template <typename S>
void serialize(S& s, Color& c) {
    s.value1b(c.r);
    s.value1b(c.g);
    s.value1b(c.b);
    s.value1b(c.a);
}