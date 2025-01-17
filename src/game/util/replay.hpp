#pragma once
#include <optional>
#include "util/str.hpp"
#include "util/macro.hpp"
#include "util/mem-types.hpp"
#include "util/math/num-types.hpp"
#include "util/vector-types.hpp"
#include "game/util/keybits.hpp"
#include "game/core/difficulty.hpp"

using Key_packet = Vector<hpw::keycode>;

/// Game replay (keylogger)
class Replay final {
  nocopy(Replay);
  struct Impl;
  Unique<Impl> impl {};

public:
  struct Info;

  explicit Replay(CN<Str> path, bool write_mode);
  ~Replay();
  void close();
  void push(CN<Key_packet> key_packet);
  std::optional<Key_packet> pop(); /// будет возвращать нажатые клавиши, пока не кончатся
  CP<Impl> get_impl() const;
  static Info get_info(CN<Str> path);
}; // Replay

struct Date {
  uint year {};
  uint month {};
  uint day {};
  uint hour {};
  uint minute {};
  uint second {};
};

struct Replay::Info {
  Date date {};
  utf32 player_name {};
  Str path {};
  Str date_str {};
  std::int64_t score {};
  Difficulty difficulty {};
  bool first_level_is_tutorial {};
};
