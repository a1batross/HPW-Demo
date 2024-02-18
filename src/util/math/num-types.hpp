#pragma once
#include <cstdint>

using ldouble = long double;
using real = float;
using uint = unsigned;
using byte = std::uint8_t;
using i64_t = std::int64_t;
using Uid = std::uint16_t;

static_assert(sizeof(byte) == 1);
static_assert(sizeof(i64_t) == 8);
static_assert(sizeof(Uid) == 2);
static_assert(sizeof(uint) >= 4 && sizeof(uint) <= 8);
