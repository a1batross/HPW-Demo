#include <cassert>
#include "game-config.hpp"
#include "game/game-common.hpp"
#include "game/game-core.hpp"
#include "game/game-canvas.hpp"
#include "game/game-window.hpp"
#include "game/game-graphic.hpp"
#include "game/game-sync.hpp"
#include "game/util/keybits.hpp"
#include "game/util/game-replay.hpp"
#include "game/util/game-palette.hpp"
#include "util/file/yaml.hpp"
#include "util/path.hpp"
#include "host/host-util.hpp"
#include "host/command.hpp"

int get_scancode(const hpw::keycode keycode) {
  if(hpw::keys_info.keys.empty())
    return -1;
  cauto key_info = hpw::keys_info.find(keycode);
  assert(key_info);
  return key_info->scancode;
}

void save_config() {
  auto& config = *hpw::config;
  config.set_bool("enable_replay", hpw::enable_replay);

  auto graphic_node = config.make_node("graphic");
  graphic_node.set_v_int("canvas_size",        {graphic::width, graphic::height} );
  graphic_node.set_int  ("light_quality",       scast<int>(graphic::light_quality) );
  graphic_node.set_bool ("fullscren",           graphic::fullscreen);
  graphic_node.set_bool ("draw_border",         graphic::draw_border);
  graphic_node.set_bool ("show_mouse_cursour",  graphic::show_mouse_cursour);
  graphic_node.set_int  ("resize_mode",         scast<int>(graphic::resize_mode) );
  graphic_node.set_bool ("enable_motion_blur",  graphic::enable_motion_blur);
  graphic_node.set_real ("blur_quality_mul",    graphic::blur_quality_mul);
  graphic_node.set_bool ("blink_particles",     graphic::blink_particles);
  graphic_node.set_bool ("blink_motion_blur",   graphic::blink_motion_blur);
  graphic_node.set_bool ("motion_blur_quality_reduct", graphic::motion_blur_quality_reduct);
  graphic_node.set_real ("max_motion_blur_quality_reduct", graphic::max_motion_blur_quality_reduct);
  graphic_node.set_bool ("start_focused",       graphic::start_focused);
  graphic_node.set_str  ("palette",             graphic::current_palette_file);
  graphic_node.set_int  ("frame_skip",          graphic::frame_skip);
  graphic_node.set_bool ("auto_frame_skip",     graphic::auto_frame_skip);

  auto sync_node = graphic_node.make_node("sync");
  sync_node.set_bool ("vsync",                 graphic::get_vsync());
  sync_node.set_bool ("wait_frame",            graphic::wait_frame);
  sync_node.set_int  ("target_fps",            graphic::get_target_fps());
  sync_node.set_bool ("cpu_safe",              graphic::cpu_safe);
  sync_node.set_real ("autoopt_timeout_max",   graphic::autoopt_timeout_max);
  sync_node.set_bool("disable_frame_limit",    graphic::get_disable_frame_limit());

  auto input_node = config.make_node("input");
  #define SAVE_KEY(name) input_node.set_int(#name, get_scancode(hpw::keycode::name));
  SAVE_KEY(enable)
  SAVE_KEY(escape)
  SAVE_KEY(bomb)
  SAVE_KEY(shoot)
  SAVE_KEY(focus)
  SAVE_KEY(mode)
  SAVE_KEY(up)
  SAVE_KEY(down)
  SAVE_KEY(left)
  SAVE_KEY(right)
  SAVE_KEY(screenshot)
  SAVE_KEY(fulscrn)
  #undef SAVE_KEY

  config.save( hpw::config->get_path() );
} // save_config

void load_config() {
  hpw::config = new_shared<Yaml>(hpw::cur_dir + "config.yml", true);

  auto& config = *hpw::config;
  hpw::enable_replay = config.get_bool("enable_replay");

  auto path_node = config["path"];
  // добавить инфу о путях, если конфиг сделан в первый раз
  if ( !path_node.check()) {
    path_node = config.make_node("path");
    path_node.set_str("screenshots", "./screenshots/");
    path_node.set_str("data", "./data.zip");
    path_node.set_str("locale", "resource/locale/ru.yml");
  }
  // сделать папки, если их нет
  make_dir_if_not_exist(hpw::cur_dir + path_node.get_str("screenshots"));
  make_dir_if_not_exist(hpw::cur_dir + "replays/");

  cauto graphic_node = config["graphic"];
  cauto canvas_size = graphic_node.get_v_int("canvas_size", {graphic::width, graphic::height});
  graphic::width  = canvas_size.at(0);
  graphic::height = canvas_size.at(1);
  graphic::fullscreen = graphic_node.get_bool("fullscren", graphic::fullscreen);
  graphic::draw_border = graphic_node.get_bool("draw_border", graphic::draw_border);
  graphic::show_mouse_cursour = graphic_node.get_bool("show_mouse_cursour", graphic::show_mouse_cursour);
  graphic::resize_mode = scast<decltype(graphic::resize_mode)>(graphic_node.get_int("resize_mode", scast<int>(graphic::resize_mode)));
  graphic::light_quality = scast<decltype(graphic::light_quality)>(graphic_node.get_int("light_quality", scast<int>(graphic::light_quality)));
  graphic::enable_motion_blur = graphic_node.get_bool("enable_motion_blur", graphic::enable_motion_blur);
  graphic::blur_quality_mul = graphic_node.get_real("blur_quality_mul", graphic::blur_quality_mul);
  graphic::blink_particles = graphic_node.get_bool("blink_particles", graphic::blink_particles);
  graphic::blink_motion_blur = graphic_node.get_bool("blink_motion_blur", graphic::blink_motion_blur);
  graphic::motion_blur_quality_reduct = graphic_node.get_bool("motion_blur_quality_reduct", graphic::motion_blur_quality_reduct);
  graphic::max_motion_blur_quality_reduct = graphic_node.get_real("max_motion_blur_quality_reduct", graphic::max_motion_blur_quality_reduct);
  graphic::start_focused = graphic_node.get_bool("start_focused", graphic::start_focused);
  if (hpw::init_palette_from_archive)
    hpw::init_palette_from_archive( graphic_node.get_str("palette") );
  graphic::frame_skip = graphic_node.get_int("frame_skip", graphic::frame_skip);
  assert(graphic::frame_skip < 30);
  graphic::auto_frame_skip = graphic_node.get_bool("auto_frame_skip", graphic::auto_frame_skip);

  cauto sync_node = graphic_node["sync"];
  graphic::set_vsync( sync_node.get_bool("vsync", graphic::get_vsync()) );
  graphic::wait_frame = sync_node.get_bool("wait_frame", graphic::wait_frame);
  graphic::set_disable_frame_limit( sync_node.get_bool("disable_frame_limit", graphic::get_disable_frame_limit()) );
  graphic::set_target_fps( sync_node.get_int("target_fps", graphic::get_target_fps()) );
  graphic::cpu_safe = sync_node.get_bool("cpu_safe", graphic::cpu_safe);
  graphic::autoopt_timeout_max = sync_node.get_real("autoopt_timeout_max", graphic::autoopt_timeout_max);

  if (hpw::rebind_key_by_scancode) {
    cauto input_node = config["input"];
    #define LOAD_KEY(name) hpw::rebind_key_by_scancode(hpw::keycode::name, input_node.get_int(#name, get_scancode(hpw::keycode::name)) );
    LOAD_KEY(enable)
    LOAD_KEY(escape)
    LOAD_KEY(bomb)
    LOAD_KEY(shoot)
    LOAD_KEY(focus)
    LOAD_KEY(mode)
    LOAD_KEY(up)
    LOAD_KEY(down)
    LOAD_KEY(left)
    LOAD_KEY(right)
    LOAD_KEY(screenshot)
    LOAD_KEY(fulscrn)
    #undef LOAD_KEY
  }
} // load_config
