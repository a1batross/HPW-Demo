#include <ctime>
#include "scene-main-menu.hpp"
#include "scene-manager.hpp"
#include "scene-difficulty.hpp"
#include "scene-replay-select.hpp"
#include "scene-options.hpp"
#include "scene-info.hpp"
#include "scene-loading.hpp"
#include "scene-game.hpp"
#include "game/game-common.hpp"
#include "game/game-font.hpp"
#include "game/game-core.hpp"
#include "game/game-canvas.hpp"
#include "game/util/game-archive.hpp"
#include "game/util/keybits.hpp"
#include "game/util/game-util.hpp"
#include "game/util/locale.hpp"
#include "game/util/replay.hpp"
#include "game/util/version.hpp"
#include "game/util/game-replay.hpp"
#include "game/menu/text-menu.hpp"
#include "game/menu/item/text-item.hpp"
#include "graphic/image/image.hpp"
#include "graphic/image/image-io.hpp"
#include "graphic/util/util-templ.hpp"
#include "graphic/util/graphic-util.hpp"
#include "graphic/util/blur.hpp"
#include "graphic/util/resize.hpp"
#include "graphic/font/font.hpp"
#include "graphic/effect/bg-pattern.hpp"
#include "util/math/random.hpp"
#include "util/str-util.hpp"
#include "util/hpw-util.hpp"
#include "util/file/archive.hpp"
#include "util/file/yaml.hpp"

Scene_main_menu::Scene_main_menu() {
  init_menu();
  init_logo();
  init_bg();
}

void Scene_main_menu::update(double dt) {
#ifdef DEBUG
  if (is_pressed_once(hpw::keycode::escape))
    hpw::scene_mgr->back();
#endif

  bg_state = std::fmod(bg_state + dt, 12.0);
  menu->update(dt);

  if (
    // поменять фон возвращаясь из сцены
    hpw::scene_mgr->status.came_back ||
    // поменять фон через кнопку
    is_pressed_once(hpw::keycode::fast_forward) ||
    // поменять фон по таймеру
    change_bg_timer.update(dt)
  ) {
    init_logo();
    init_bg();
    change_bg_timer.reset();
  }
}

void Scene_main_menu::draw_bg(Image& dst) const {
  bg_pattern_pf(dst, std::floor(pps(bg_state)));

  // вырезаем часть фона и блюрим
  Rect rect(120, 50, 270, 280);
  auto for_blur = fast_cut(dst, rect.pos.x, rect.pos.y, rect.size.x, rect.size.y);

  #ifdef DEBUG
    blur_fast(for_blur, 5);
  #elifdef ECOMEM
    blur_fast(for_blur, 5);
  #else
    adaptive_blur(for_blur, 5);
  #endif

  //apply_invert(for_blur);
  // мягкий контраст
  apply_contrast(for_blur, 0.5);
  // затенение
  sub_brightness(for_blur, Pal8::from_real(0.33333));

  insert(dst, for_blur, rect.pos);
  // контур
  draw_rect<&blend_diff>(dst, rect, Pal8::white);
} // draw_bg

void Scene_main_menu::init_logo() {
  std::call_once(m_logo_load_once, [this]{ cache_logo_names(); });
  assert(!m_logo_names.empty());
  // случайный выбор логотипа игры из списка
  Str logo_name = m_logo_names.at(rndu_fast(m_logo_names.size()));

  auto finded_sprite = hpw::store_sprite->find(logo_name);
  assert(finded_sprite && scast<bool>(*finded_sprite));
  logo = new_shared<Sprite>(*finded_sprite);

  // маленькие логотипы заресайзить
  if (logo->X() < 32 && logo->Y() < 16)
    zoom_x8(*logo);
  else if (logo->X() < 64 && logo->Y() < 32)
    zoom_x4(*logo);
  else if (logo->X() < 128 && logo->Y() < 64)
    zoom_x2(*logo);

  const Vec logo_sz(logo->X(), logo->Y());
  const Vec menu_sz(graphic::canvas->X, 160);
  logo_pos = center_point(menu_sz, logo_sz);
  logo_pos.y += 50;
} // init_logo

void Scene_main_menu::draw_logo(Image& dst) const {
  insert<&blend_diff>(dst, *logo, logo_pos);
}

void Scene_main_menu::draw(Image& dst) const {
  draw_bg(dst);
  draw_logo(dst);
  menu->draw(dst);

  graphic::font->draw(dst, {140, 300}, U"Версия игры: "
    + sconv<utf32>(cstr_to_cxxstr(get_game_version())) );
}

void Scene_main_menu::init_menu() {
  menu = new_shared<Text_menu>(
    Menu_items {
      new_shared<Menu_text_item>(get_locale_str("scene.main_menu.start"), []{
        hpw::scene_mgr->add(new_shared<Scene_difficulty>());
      }),
      new_shared<Menu_text_item>(get_locale_str("scene.replay.name"), []{
        hpw::scene_mgr->add(new_shared<Scene_replay_select>());
      }),
      /*new_shared<Menu_text_item>(get_locale_str("scene.main_menu.info"), []{
        hpw::scene_mgr->add(new_shared<Scene_info>());
      }), TODO*/
      new_shared<Menu_text_item>(get_locale_str("scene.options.name"), []{
        hpw::scene_mgr->add(new_shared<Scene_options>());
      }),
      new_shared<Menu_text_item>(get_locale_str("common.exit"), []{
        hpw::scene_mgr->back();
      }),
    },
    Vec{140, 200}
  );
} // init_menu

void Scene_main_menu::cache_logo_names() {
  cauto config_file = hpw::archive->get_file("resource/image/logo/list.yml");
  cauto config_yml = Yaml(config_file);
  m_logo_names = config_yml.get_v_str("logos");
}

void bg_copy_1(Image& dst, const int state) {
  const Vec pos {
    rnd_fast(-1, 1),
    rnd_fast(-1, 1)
  };
  return_if( !pos);
  static Image buffer(dst.X, dst.Y);
  assert(buffer.size == dst.size);
  insert_fast(buffer, dst);
  insert<&blend_diff>(dst, buffer, pos);
}

void bg_copy_2(Image& dst, const int state) {
  const Vec pos {
    rnd_fast(-1, 1),
    rnd_fast(-1, 1)
  };
  return_if( !pos);
  static Image buffer(dst.X, dst.Y);
  assert(buffer.size == dst.size);
  insert_fast(buffer, dst);
  insert<&blend_xor>(dst, buffer, pos);
}

void bg_copy_3(Image& dst, const int state) {
  Vec pos {
    rnd_fast(-3, 3),
    rnd_fast(-3, 3)
  };
  pos += Vec(
    std::cos(scast<double>(state) * 0.01) * 3.0,
    std::sin(scast<double>(state) * 0.01) * 3.0
  );
  return_if( !pos);
  static Image buffer(dst.X, dst.Y);
  assert(buffer.size == dst.size);
  insert_fast(buffer, dst);
  insert(dst, buffer, pos);
}

void bg_copy_4(Image& dst, const int state) {
  cauto speed = rndr_fast(0, 4);
  const Vec pos(
    std::cos(scast<double>(state) * 0.01) * speed,
    std::sin(scast<double>(state) * 0.01) * speed
  );
  return_if( !pos);
  static Image buffer(dst.X, dst.Y);
  assert(buffer.size == dst.size);
  insert_fast(buffer, dst);
  insert(dst, buffer, pos);
}

void Scene_main_menu::init_bg() {
  sconst Vector<decltype(bg_pattern_pf)> bg_patterns {
    &bg_pattern_1,
    &bg_pattern_2,
    &bg_pattern_4,
    #ifndef ECOMEM
    &bg_copy_1,
    &bg_copy_2,
    &bg_copy_3,
    &bg_copy_4,
    &bg_pattern_5,
    &bg_pattern_6,
    #endif
  };
  bg_pattern_pf = bg_patterns.at( rndu_fast(bg_patterns.size()) );
}
