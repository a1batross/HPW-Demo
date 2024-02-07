#pragma once
#include "level.hpp"
#include "util/mem-types.hpp"

/// Уровень для копипаста
class Level_tutorial final: public Level {
  nocopy(Level_tutorial);

  struct Impl;
  Unique<Impl> impl {};

public:
  Level_tutorial();
  ~Level_tutorial();
  void update(const Vec vel, double dt) override;
  void draw(Image& dst) const override;
  void draw_upper_layer(Image& dst) const override;
};
