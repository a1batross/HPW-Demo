#include <cassert>
#include "game-app.hpp"
#include "host/command.hpp"

#ifdef DEBUG
  #include "game/scene/scene-main-menu.hpp"
#else
  #include "game/scene/scene-validation.hpp"
#endif

#include "game/scene/scene-manager.hpp"
#include "game/game-common.hpp"
#include "game/game-core.hpp"
#include "game/game-font.hpp"
#include "game/game-canvas.hpp"
#include "game/game-sync.hpp"
#include "game/game-graphic.hpp"
#include "game/game-debug.hpp"
#include "game/util/game-config.hpp"
#include "game/util/game-util.hpp"
#include "game/util/locale.hpp"
#include "game/util/game-archive.hpp"
#include "graphic/image/image.hpp"
#include "graphic/font/unifont.hpp"
#include "graphic/util/util-templ.hpp"
#include "graphic/image/color-table.hpp"
#include "util/file/archive.hpp"
#include "util/file/yaml.hpp"
#include "util/math/random.hpp"
#include "util/hpw-util.hpp"

Game_app::Game_app(int argc, char *argv[])
: Host_glfw(argc, argv)
{
  check_color_tables();
  load_resources();
  load_locale();
  load_font();
  
  init_scene_mgr();
  #ifdef DEBUG
  hpw::scene_mgr->add( new_shared<Scene_main_menu>() );
  #else 
  hpw::scene_mgr->add( new_shared<Scene_validation>() );
  #endif

  /* к этому моменту кеймапер будет инициализирован и
  управление можно будет переназначить с конфига */
  load_config();
} // c-tor

void Game_app::update(double dt) {
  ALLOW_STABLE_RAND
  assert(dt == hpw::target_update_time);
  update_graphic_autoopt(dt);
  
  Host_glfw::update(dt);

  auto st = get_time();
  if ( !hpw::scene_mgr->update(dt) ) {
    detailed_log("scenes are over, call soft_exit\n");
    hpw::soft_exit();
  }
  hpw::update_time_unsafe = get_time() - st;
} // update

void Game_app::draw() {
  auto st = get_time();

  hpw::scene_mgr->draw(*graphic::canvas);
  if (graphic::draw_border) // рамка по краям
    draw_border(*graphic::canvas);

  graphic::soft_draw_time = get_time() - st;
  graphic::check_autoopt();

  Host_glfw::draw();
}

void Game_app::draw_border(Image& dst) const
  { draw_rect(dst, Rect{0,0, dst.X, dst.Y}, Pal8::white); }

void Game_app::load_locale() {
  hpw::locale = new_shared<Locale>();
  auto locale_path = (*hpw::config)["path"].get_str("locale", "resource/locale/en.yml");
  auto locale_yml_mem = hpw::archive->get_file(locale_path);
  auto locale_yml = Yaml(locale_yml_mem);
  load_locales_to_store(locale_yml);
}

void Game_app::load_font() {
  auto font_mem {hpw::archive->get_file("resource/font/unifont-13.0.06.ttf")};
  graphic::font = new_shared<Unifont>(font_mem, 16, true);
}

void Game_app::update_graphic_autoopt(double dt) {
  // если рендер не будет лагать, то после таймера - тригер автооптимизации сбросится
  graphic::autoopt_timeout = std::max(graphic::autoopt_timeout - dt, 0.0);
  if (graphic::autoopt_timeout == 0)
    graphic::render_lag = false;
}
