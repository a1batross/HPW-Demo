#include "level-tutorial.hpp"
#include "util/math/vec.hpp"
#include "util/unicode.hpp"
#include "game/entity/player.hpp"
#include "game/entity/entity-manager.hpp"
#include "game/entity/collider/collider-qtree.hpp"
#include "game/core/entities.hpp"
#include "game/core/canvas.hpp"
#include "game/core/common.hpp"
#include "game/core/levels.hpp"
#include "game/core/fonts.hpp"
#include "game/level/util/level-tasks.hpp"
#include "game/level/util/task-timed.hpp"
#include "game/level/level-manager.hpp"
#include "game/util/sync.hpp"
#include "game/util/game-util.hpp"
#include "graphic/image/image.hpp"
#include "graphic/font/font.hpp"
#include "graphic/effect/bg-pattern.hpp"
#include "graphic/util/graphic-util.hpp"
#include "graphic/util/util-templ.hpp"

struct Level_tutorial::Impl {
  Level_tasks tasks {};
  utf32 bg_text {}; /// текст, который показывается на фоне

  inline explicit Impl() {
    hpw::shmup_mode = true;
    init_collider();
    make_player();
    init_tasks();
  }

  inline void update(const Vec vel, double dt) {
    execute_tasks(tasks, dt);
  }

  inline void draw(Image& dst) const {
    draw_bg(dst);
    draw_bg_text(dst);
  }

  inline void draw_upper_layer(Image& dst) const {}

  inline void make_player()
    { hpw::entity_mgr->make({}, "player.boo.dark", get_screen_center() + Vec(0, 100)); }

  inline void init_collider() {
    hpw::entity_mgr->set_collider(new_shared<Collider_qtree>(6, 1,
      graphic::width, graphic::height));
  }

  inline void init_tasks() {
    tasks = Level_tasks {
      Timed_task(30.0, [](double dt) {
        return false;
      }),
      // выйти с уровня
      [](double dt) {
        hpw::level_mgr->finalize_level();
        return true;
      },
    }; // Level_tasks c-tor
  } // init_tasks

  inline void draw_bg(Image& dst) const {
    bg_pattern_1(dst, graphic::frame_count >> 2);
    apply_brightness(dst, -140);
  }

  inline void draw_bg_text(Image& dst) const {
    return_if(bg_text.empty());
    // текст в центре
    cauto center = get_screen_center();
    cauto text_sz = graphic::font->text_size(bg_text);
    cauto pos = center - (text_sz / 2);
    // яркость текста сменяется
    cauto text_brightness = 150u + ((graphic::frame_count / 3u) % (255u - 150u));
    graphic::font->draw(dst, pos, bg_text, &blend_alpha, text_brightness);
  }
}; // Impl

Level_tutorial::Level_tutorial(): impl {new_unique<Impl>()} {}
Level_tutorial::~Level_tutorial() {}
void Level_tutorial::update(const Vec vel, double dt) {
  Level::update(vel, dt);
  impl->update(vel, dt);
}
void Level_tutorial::draw(Image& dst) const { impl->draw(dst); }
void Level_tutorial::draw_upper_layer(Image& dst) const { impl->draw_upper_layer(dst); }
