#include <utility>
#include <cassert>
#include <functional>
#include "plugin/graphic-effect/hpw-plugin-effect.h"
#include "plugin-graphic-effect.hpp"
#include "util/str-util.hpp"
#include "util/path.hpp"
#include "util/log.hpp"
#include "util/error.hpp"
#include "util/file/yaml.hpp"
#include "graphic/image/image.hpp"
#include "game/game-canvas.hpp"
#include "game/game-common.hpp"

// ----------- [!] ---------------
// вверх не перемещать
#include <DyLib/DyLib.hpp>
// ----------- [!] ---------------

Shared<DyLib> g_lib_loader {}; /// для кросплатформ загрузки либ
Vector<Shared<Param_pge>> g_pge_params {}; /// какие сейчас доступны настройки плагина
Str g_pge_path {}; /// текущий путь к плагину 
Str g_pge_name {}; /// текущее имя плагина
std::function<decltype(plugin_init)> g_plugin_init {};
std::function<decltype(plugin_apply)> g_plugin_apply {};
std::function<decltype(plugin_finalize)> g_plugin_finalize {};
void registrate_param_f32(cstr_t, cstr_t, real_t*, const real_t, const real_t, const real_t);
void registrate_param_i32(cstr_t, cstr_t, std::int32_t*, const std::int32_t, const std::int32_t, const std::int32_t);

void load_pge(Str libname) {
  conv_sep(libname);
  try {
    std::cout << "загрузка плагина: " << libname << std::endl;
    g_lib_loader = new_shared<DyLib>(libname);
    g_plugin_init = g_lib_loader->getFunction<decltype(plugin_init)>("plugin_init");
    g_plugin_apply = g_lib_loader->getFunction<decltype(plugin_apply)>("plugin_apply");
    g_plugin_finalize = g_lib_loader->getFunction<decltype(plugin_finalize)>("plugin_finalize");
    iferror( !g_plugin_init, "не удалось получить функцию plugin_init");
    iferror( !g_plugin_apply, "не удалось получить функцию plugin_apply");
    iferror( !g_plugin_finalize, "не удалось получить функцию plugin_finalize");

    auto context = new_shared<context_t>();
    context->dst = ptr2ptr<pal8_t*>(graphic::canvas->data());
    context->w = graphic::canvas->X;
    context->h = graphic::canvas->Y;
    context->registrate_param_f32 = &registrate_param_f32;
    context->registrate_param_i32 = &registrate_param_i32;

    auto result = new_shared<result_t>();
    g_plugin_init(context.get(), result.get());
    iferror( result->version != 1, "несовпадение версий плагина и API");
    iferror( !result->init_succsess, result->error);
    g_pge_path = libname;
    g_pge_name = get_filename(g_pge_path);
    std::cout << "плагин " << g_pge_name << " успешно загружен." << std::endl;
  } catch (CN<hpw::Error> err) {
    hpw_log("ошибка загрузки плагина: " << err.get_msg() << '\n');
    disable_pge();
  } catch (...) {
    hpw_log("неизвестная ошибка при загрузке плагина\n");
    disable_pge();
  }
} // load_pge

void apply_pge(const uint32_t state) {
  if (g_plugin_apply)
    g_plugin_apply(state);
}

void disable_pge() {
  detailed_log("отключение плагина " + get_cur_pge_name() + '\n');
  if (g_plugin_finalize)
    g_plugin_finalize();
  g_plugin_finalize = {};
  g_plugin_apply = {};
  g_plugin_init = {};
  g_lib_loader = {};
}

void load_pge_from_config() {
  // TODO
}

void save_pge_to_config() {
  cauto plugin_name = get_cur_pge_name();
  return_if(plugin_name.empty());

  assert(hpw::config);
  auto& config = *hpw::config;
  auto plugin_node = config.make_node_if_not_exist("plugin");
  auto graphic_node = plugin_node.make_node_if_not_exist("graphic");
  auto effect_node = graphic_node.make_node_if_not_exist(plugin_name);
  effect_node.set_str("path", get_cur_pge_path());
  // сейв текущих настроек плагина
  cnauto params = get_pge_params();
  for (uint id = 0; cnauto param: params) {
    auto param_node = effect_node.make_node_if_not_exist("param_" + n2s(id));
    ++id;
    assert(param);
    param->save(param_node);
  }
} // save_pge_to_config

template <class T, class Param_type>
void registrate_param(cstr_t title, cstr_t desc, T* val,
const T speedstep, const T min, const T max) {
  auto param = new_shared<Param_type>();
  param->max = max;
  param->min = min;
  param->description = desc;
  param->title = title;
  param->value = val;
  param->speed_step = speedstep;
  iferror(speedstep <= 0, "speedstep не должен быть меньше 1");
  iferror(min >= max, "min не должен быть больше max");
  iferror( !val, "неправильный адрес для value");
  iferror(Str(title).empty(), "параметру нужно задать имя");
  g_pge_params.push_back(std::move(param));
} // registrate_param

void registrate_param_f32(cstr_t title, cstr_t desc, real_t* val,
const real_t speedstep, const real_t min, const real_t max)
{ registrate_param<real_t, Param_pge_real>(title, desc, val, speedstep, min, max); }

void registrate_param_i32(cstr_t title, cstr_t desc, std::int32_t* val,
const std::int32_t speedstep, const std::int32_t min, const std::int32_t max)
{ registrate_param<std::int32_t, Param_pge_int>(title, desc, val, speedstep, min, max); }

CN< Vector<Shared<Param_pge>> > get_pge_params() { return g_pge_params; }
CN<Str> get_cur_pge_path() { return g_pge_path; }
CN<Str> get_cur_pge_name() { return g_pge_name; }

void Param_pge::save(Yaml& dst) const {
  dst.set_str("title", title);
  dst.set_str("description", description);
  dst.set_int("type", std::to_underlying(type));
}

void Param_pge_int::save(Yaml& dst) const {
  Param_pge::save(dst);
  assert(value);
  dst.set_int("value", *value);
  dst.set_int("min", min);
  dst.set_int("max", max);
  dst.set_int("speed_step", speed_step);
}

void Param_pge_real::save(Yaml& dst) const {
  Param_pge::save(dst);
  assert(value);
  dst.set_real("value", *value);
  dst.set_real("min", min);
  dst.set_real("max", max);
  dst.set_real("speed_step", speed_step);
}