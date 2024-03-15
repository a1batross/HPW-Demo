#include <cassert>
#include "scene-replay-select.hpp"
#include "scene-manager.hpp"
#include "scene-loading.hpp"
#include "game/core/common.hpp"
#include "game/core/scenes.hpp"
#include "game/core/replays.hpp"
#include "game/util/replay.hpp"
#include "game/util/keybits.hpp"
#include "game/util/locale.hpp"
#include "game/util/game-util.hpp"
#include "game/menu/table-menu.hpp"
#include "game/menu/item/table-row-item.hpp"
#include "game/scene/scene-game.hpp"
#include "util/path.hpp"
#include "util/str-util.hpp"

struct Date {
  uint year {};
  uint month {};
  uint day {};
  uint hour {};
  uint minute {};
  uint second {};
};

struct Scene_replay_select::Impl {
  Unique<Menu> menu {};
  Vector<Replay::Info> m_replay_info_table {};

  inline Impl() {
    init_menu();
  }

  inline void update(double dt) {
    if (is_pressed_once(hpw::keycode::escape))
      hpw::scene_mgr->back();
    menu->update(dt);
  }

  inline void draw(Image& dst) const {
    menu->draw(dst);
  }

  inline void init_menu() {
    load_replays();

    menu = new_unique<Table_menu>(
      get_locale_str("scene.replay.name"),
      Table_menu::Rows {
        Table_menu::Row {.name = get_locale_str("scene.replay.table.player"), .sz = 140},
        Table_menu::Row {.name = get_locale_str("scene.replay.table.date"), .sz = 135},
        Table_menu::Row {.name = get_locale_str("scene.replay.table.difficulty"), .sz = 80},
        Table_menu::Row {.name = get_locale_str("scene.replay.table.levels"), .sz = 60},
        Table_menu::Row {.name = get_locale_str("scene.replay.table.score")},
      },
      25,
      generate_rows()
    );
  } // init_menu

  inline Menu_items generate_rows() {
    return_if(m_replay_info_table.empty(), {});
    // сортировать реплеи по дате создания
    std::sort(m_replay_info_table.begin(), m_replay_info_table.end(),
      &Impl::date_comparator);

    Menu_items ret;
    for (cnauto replay_info: m_replay_info_table) {
      ret.emplace_back( new_shared<Menu_item_table_row>(
        [replay_info] { // запуск файла реплея
          assert(!replay_info.path.empty());
          hpw::scene_mgr->add(new_shared<Scene_loading>( [replay_info]{
            hpw::replay_read_mode = true;
            hpw::cur_replay_file_name = replay_info.path;
            hpw::scene_mgr->add(new_shared<Scene_game>());
          } ));
        },
        Menu_item_table_row::Content_getters {
          [replay_info]->utf32 { return replay_info.player_name; },
          [replay_info]->utf32 { return sconv<utf32>(replay_info.date); },
          [replay_info]->utf32 { return difficulty_to_str(replay_info.difficulty); },
          [replay_info]->utf32 { return n2s<utf32>(replay_info.level); },
          [replay_info]->utf32 { return n2s<utf32>(replay_info.score); },
        }
      ) );
    }
    return ret;
  } // generate_rows

  inline void load_replays() {
    cauto replay_files = files_in_dir(hpw::cur_dir + "replays/");
    return_if(replay_files.empty());
    for (cnauto replay_file: replay_files) {
      try {
        cauto info = Replay::get_info(replay_file);
        m_replay_info_table.push_back(info);
      } catch (...) {
        hpw_log("не удалось загрузить один из реплеев\n");
      }
    }
  } // load_replays

  /// для сравнения времени создания реплея
  inline static bool date_comparator(const Replay::Info a, const Replay::Info b) {
    cauto a_date = Impl::to_date(a.date);
    cauto b_date = Impl::to_date(b.date);
    bool ret {true};
    if (a_date.year < b_date.year) {
      ret = false;
    } else if (a_date.month < b_date.month) {
      ret = false;
    } else if (a_date.day < b_date.day) {
      ret = false;
    } else if (a_date.hour < b_date.hour) {
      ret = false;
    } else if (a_date.minute < b_date.minute) {
      ret = false;
    } else if (a_date.second < b_date.second) {
      ret = false;
    }
    return !ret;
  } // date_comparator

  /// конвертирует дату и время из строки в удобный формат
  inline static Date to_date(CN<Str> date) {
    assert(!date.empty());
    // разделение на дату и время
    auto strs = split_str(date, ' ');
    cauto date_str = strs.at(0);
    cauto time_str = strs.at(1);
    Date ret;
    // разделить на DD.MM.YY
    strs = split_str(date_str, '.');
    ret.day = s2n<int>( strs.at(0) );
    ret.month = s2n<int>( strs.at(1) );
    ret.year = s2n<int>( strs.at(2) );
    // разделить на HH:MM:SS
    strs = split_str(time_str, ':');
    ret.hour = s2n<int>( strs.at(0) );
    ret.month = s2n<int>( strs.at(1) );
    ret.second = s2n<int>( strs.at(2) );
    return ret;
  } // to_date
}; // impl

Scene_replay_select::Scene_replay_select(): impl {new_unique<Impl>()} {}
Scene_replay_select::~Scene_replay_select() {}
void Scene_replay_select::update(double dt) { impl->update(dt); }
void Scene_replay_select::draw(Image& dst) const { impl->draw(dst); }
