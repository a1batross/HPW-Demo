#include "level-tutorial.hpp"
#include "util/math/vec.hpp"
#include "util/math/random.hpp"
#include "util/unicode.hpp"
#include "util/hpw-util.hpp"
#include "game/entity/player.hpp"
#include "game/entity/entity-manager.hpp"
#include "game/entity/collider/collider-qtree.hpp"
#include "game/entity/util/entity-util.hpp"
#include "game/core/entities.hpp"
#include "game/core/canvas.hpp"
#include "game/core/common.hpp"
#include "game/core/levels.hpp"
#include "game/core/fonts.hpp"
#include "game/core/scenes.hpp"
#include "game/scene/scene-manager.hpp"
#include "game/level/util/level-tasks.hpp"
#include "game/level/util/task-timed.hpp"
#include "game/level/level-manager.hpp"
#include "game/util/sync.hpp"
#include "game/util/game-util.hpp"
#include "game/util/keybits.hpp"
#include "graphic/image/image.hpp"
#include "graphic/font/font.hpp"
#include "graphic/effect/bg-pattern.hpp"
#include "graphic/util/graphic-util.hpp"
#include "graphic/util/util-templ.hpp"

struct Level_tutorial::Impl {
  Level_tutorial* master {};
  Level_tasks tasks {};
  utf32 bg_text {}; /// текст, который показывается на фоне

  inline explicit Impl(Level_tutorial* _master): master{_master}
    { restart(); }

  inline void restart() {
    hpw::entity_mgr->clear();
    hpw::shmup_mode = true;
    bg_text = {};
    init_collider();
    make_player();
    init_tasks();

    // рестарт уровня при смерти
    master->on_player_death_action = [this] { restart(); };
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
      // в начале ничего не происходит
      Timed_task(3.3),

      Timed_task(9.0, Task_draw_motion_keys(this)),
      Spawner_border_bullet(this, 40, 0.6),
      Timed_task(4.5, [this](double dt) { bg_text = get_locale_str("scene.tutorial.text.move_up"); return false; }),
      Up_speed_test(this),
      Timed_task(6, [this](double dt) { draw_shoot_key(); return false; }),
      Spawner_enemy_noshoot(this, 4.0),
      Timed_task(2.5, [this](double dt) { bg_text = {}; return false; }),
      Spawner_enemy_shoot(this, 8.0),
      Timed_task(5.0, [this](double dt) { bg_text = get_locale_str("scene.tutorial.text.energy_info"); return false; }),
      Energy_test(this),
      Timed_task(5.0, [this](double dt) { return draw_focus_key(); }),
      Focus_test(this),
      Timed_task(1.5),

      // проверить что игрок дожил до конца
      [](double dt) {
        cauto player = hpw::entity_mgr->get_player();
        if (player)
          return player->status.live;
        return false;
      },
      Timed_task(7.0, [this](double dt) { bg_text = get_locale_str("scene.tutorial.text.end"); return false; }),
      &exit_from_level,
    }; // Level_tasks c-tor
  } // init_tasks

  inline void draw_bg(Image& dst) const {
    bgp_bit_1(dst, graphic::frame_count >> 2);
    apply_brightness(dst, -140);
  }

  inline void draw_bg_text(Image& dst) const {
    return_if(bg_text.empty());
    // текст в центре
    cauto center = get_screen_center();
    cauto text_sz = graphic::font->text_size(bg_text);
    cauto pos = center - (text_sz / 2);
    // затемнение фона текста
    cauto border = Vec(11, 11);
    cauto bg_shadow = Rect(pos - border, text_sz + border * 2);
    draw_rect_filled<&blend_158>(dst, bg_shadow, Pal8::black);
    // яркость текста сменяется
    cauto text_brightness = 150u + ((graphic::frame_count / 3u) % (255u - 150u));
    graphic::font->draw(dst, pos, bg_text, &blend_alpha, text_brightness);
  }

  /// Кнопки движения. Пропуск, когда всё нажато
  struct Task_draw_motion_keys {
    Impl* master {};
    bool key_l {};
    bool key_r {};
    bool key_u {};
    bool key_d {};

    inline explicit Task_draw_motion_keys (Impl* _master)
    : master {_master} {}

    inline bool operator()(double dt) {
      assert(master);
      master->draw_motion_keys();
      key_u |= is_pressed(hpw::keycode::up);
      key_d |= is_pressed(hpw::keycode::down);
      key_l |= is_pressed(hpw::keycode::left);
      key_r |= is_pressed(hpw::keycode::right);
      // выйти со сцены, если всё нажато
      return key_u && key_d && key_l && key_r;
    }
  }; // Task_draw_motion_keys

  /// показать кнопки движения
  inline void draw_motion_keys() {
    bg_text = get_locale_str("scene.tutorial.text.move_keys");
    #define KEY_TEXT(key_name) { \
      const bool pressed = is_pressed(hpw::keycode::key_name); \
      cauto scope_l = pressed ? U'{' : U' '; \
      cauto scope_r = pressed ? U'}' : U' '; \
      bg_text += U'\n' + get_locale_str("scene.input."#key_name) + U" - "; \
      bg_text += scope_l + hpw::keys_info.find(hpw::keycode::key_name)->name + scope_r; \
    }
    KEY_TEXT(left)
    KEY_TEXT(right)
    KEY_TEXT(up)
    KEY_TEXT(down)
    #undef KEY_TEXT
  } // draw_motion_keys

  /// показать кнопку стрельбы
  inline void draw_shoot_key() {
    bg_text = get_locale_str("scene.tutorial.text.shoot_key");
    const bool pressed = is_pressed(hpw::keycode::shoot);
    cauto scope_l = pressed ? U'<' : U' ';
    cauto scope_r = pressed ? U'>' : U' ';
    bg_text += utf32(U": ") +
      scope_l + hpw::keys_info.find(hpw::keycode::shoot)->name + scope_r;
  }

  /// выйти с уровня
  inline static bool exit_from_level(double dt) {
    hpw::level_mgr->finalize_level();
    hpw::need_tutorial = false;
    return true;
  }

  inline Entity* make_wave(const Vec pos)
    { return hpw::entity_mgr->make({}, "bullet.sphere.wave.tutorial", pos); }

  /// спавнит пули по краям экрана, чтобы заставить игрока подвигаться
  struct Spawner_border_bullet {
    Impl* master {};
    uint count {}; /// когда станет 0, завершить
    Timer spwan_timer {}; /// скорость спавна пуль
    real wave_speed = 1.4_pps; /// скорость пули

    inline bool operator()(double dt) {
      master->draw_motion_keys();
      // надпись об уклонении
      master->bg_text += U"\n\n" + get_locale_str("scene.tutorial.text.wave_warning");

      cfor (_, spwan_timer.update(dt)) {
        --count;

        cauto w = graphic::width;
        cauto h = graphic::height;
        cauto player = hpw::entity_mgr->get_player();
        cont_if(!player);
        cauto px = player->phys.get_pos().x;
        cauto py = player->phys.get_pos().y;

        struct Wave_dir {
          Vec rnd_st {};
          Vec rnd_ed {};
          Vec vel {};
        };
        Vector<Wave_dir> wave_dir_table {
          Wave_dir {.rnd_st={10, -20},  .rnd_ed={w-10, -20},  .vel={0, wave_speed}},
          Wave_dir {.rnd_st={10, h-10}, .rnd_ed={w-10, h-10}, .vel={0, -wave_speed}},
          Wave_dir {.rnd_st={-20, 10},  .rnd_ed={-20, h-10},  .vel={wave_speed, 0}},
          Wave_dir {.rnd_st={w-10, 10}, .rnd_ed={w-10, h-10}, .vel={-wave_speed, 0}},
          // нацелены на игрока:
          Wave_dir {.rnd_st={px, -20},  .rnd_ed={px, -20},  .vel={0, wave_speed}},
          Wave_dir {.rnd_st={px, h-10}, .rnd_ed={px, h-10}, .vel={0, -wave_speed}},
          Wave_dir {.rnd_st={-20, py},  .rnd_ed={-20, py},  .vel={wave_speed, 0}},
          Wave_dir {.rnd_st={w-10, py}, .rnd_ed={w-10, py}, .vel={-wave_speed, 0}},
        };
        cnauto wave_dir = wave_dir_table.at( rndu(wave_dir_table.size()-1) );

        cauto pos = get_rand_pos_safe(wave_dir.rnd_st.x, wave_dir.rnd_st.y,
          wave_dir.rnd_ed.x, wave_dir.rnd_ed.y);
        auto wave = master->make_wave(pos);
        wave->phys.set_vel(wave_dir.vel);
        wave->anim_ctx.set_speed_scale(0.8);
        wave_speed += 0.03_pps; // ускорять пулю
      }
      return count == 0 || count >= 9999;
    } // op ()

    inline explicit Spawner_border_bullet(Impl* _master, const uint _count,
    const real _spwan_timer)
    : master{_master}
    , count{_count}
    , spwan_timer{_spwan_timer}
    {
      assert(_master);
      assert(count > 0 && count < 9999);
      assert(_spwan_timer > 0 && _spwan_timer < 100);
    }
  }; // Spawner_border_bullet

  /// заставить игрока подвигаться вверх и вниз чтобы увидеть разницу в скорости
  struct Up_speed_test {
    constx real wave_speed_down {2.25_pps}; /// скорость пуль при движении волны вниз
    constx real wave_speed_up {4.5_pps}; /// скорость пуль при движении волны вверх
    constx real delay {2.2}; /// сколько подождать перед спавном следующей волны
    constx real step {25}; /// шаг между пулями
    constx real spawn_delay {0.1}; /// сколько ждать перед спавном следующей пули

    Impl* master {};
    uint repeats {3}; /// сколько раз спавнить волны
    real spawn_height {}; /// высота спавна волны
    bool reverse {true}; /// спавнить волны снизу вверх
    real wave_speed {}; /// скорость пуль
    Timer spwan_timer {spawn_delay};
    Timer delay_timer {};

    inline explicit Up_speed_test(Impl* _master): master{ _master} {
      spawn_height = graphic::height + 20;
    }

    inline bool operator ()(double dt) {
      // при перерыве между волнами ничего не делать
      if (delay_timer.update(dt)) {
        delay_timer = {};
      } else {
        return false;
      }

      cfor (_, spwan_timer.update(dt)) {
        spawn_height += reverse ? -step : +step;
        wave_speed = reverse ? wave_speed_up : wave_speed_down;
        // полна дошла до верха
        if (reverse && spawn_height <= 35) {
          reverse = false;
          spawn_height = -30;
          delay_timer = Timer(delay);
          --repeats;
        }
        // полна дошла до низа
        if (!reverse && spawn_height >= graphic::height - 50) {
          reverse = true;
          spawn_height = graphic::height + 30;
          delay_timer = Timer(delay);
        }

        cauto anim_speed_scale = reverse ? 2.0 : 0.9;
        // сделать пули по бокам
        auto
        wave = master->make_wave({-20, spawn_height});
        assert(wave->status.collidable);
        wave->phys.set_vel({+wave_speed, 0});
        wave->anim_ctx.set_speed_scale(anim_speed_scale);
        ptr2ptr<Collidable*>(wave)->set_dmg(24'000/6);
        wave = master->make_wave({graphic::width + 20, spawn_height});
        wave->phys.set_vel({-wave_speed, 0});
        wave->anim_ctx.set_speed_scale(anim_speed_scale);
        ptr2ptr<Collidable*>(wave)->set_dmg(24'000/6);
      }
      return repeats == 0 || repeats >= 10;
    }
  }; // Up_speed_test

  /// создаёт противников, которые не стреляют в игрока
  struct Spawner_enemy_noshoot {
    Impl* master {};
    Timer spawn_timer {0.5}; /// через сколько спавнить противника
    Timer lifetime {}; /// сколько времени будет работать спавнер
    bool check_death {}; /// начать проверять, что все соспавненные объекты умерли
    uint live_count {}; /// сколько объектов сейчас живы

    explicit Spawner_enemy_noshoot(Impl* _master, const real _total_time=1.0)
    : master {_master}
    , lifetime {_total_time}
    {
      assert(_total_time > 0 && _total_time < 60);
    }

    inline bool operator()(const double dt) {
      master->draw_shoot_key();

      if (!check_death) {
        cfor (_, spawn_timer.update(dt)) {
          spawn({graphic::width/2, -20});
        }
      }

      if (lifetime.update(dt))
        check_death = true;
      if (check_death)
        return live_count == 0 || live_count >= 100'000u;
      return false;
    } // op ()

    /// движение зигзагом для противника
    struct Zigzag_motion {
      real state {};

      inline void operator()(Entity& self, double dt) {
        state += dt;
        constexpr real SPEED = 0.666_pps;
        Vec vel(std::cos(state * 2.0) * SPEED * 2.5, SPEED);
        self.phys.set_vel(vel);
      }
    };

    Collidable* spawn(const Vec pos) {
      auto enemy = hpw::entity_mgr->make({}, "enemy.tutorial", pos);
      enemy->move_update_callback(Zigzag_motion());
      enemy->move_kill_callback([this](Entity&){ --live_count; });
      ++live_count;
      assert(enemy->status.collidable);
      return ptr2ptr<Collidable*>(enemy);
    }
  }; // Spawner_enemy_noshoot

  /// создаёт стреляющий противников по бокам
  struct Spawner_enemy_shoot {
    Impl* master {};
    Timer spawn_timer {0.8}; /// через сколько спавнить противника
    Timer lifetime {}; /// сколько времени будет работать спавнер
    bool check_death {}; /// начать проверять, что все соспавненные объекты умерли
    uint live_count {}; /// сколько объектов сейчас живы

    explicit Spawner_enemy_shoot(Impl* _master, const real _total_time=1.0)
    : master {_master}
    , lifetime {_total_time}
    {
      assert(_total_time > 0 && _total_time < 60);
    }

    inline bool operator()(const double dt) {
      if (!check_death) {
        cfor (_, spawn_timer.update(dt)) {
          spawn({+20, -20});
          spawn({graphic::width-20, -20});
        }
      }

      if (lifetime.update(dt))
        check_death = true;
      if (check_death)
        return live_count == 0 || live_count >= 100'000u;
      return false;
    } // op ()

    Collidable* spawn(const Vec pos) {
      auto enemy = hpw::entity_mgr->make({}, "enemy.tutorial", pos);
      enemy->phys.set_vel({0, 1_pps});
      /// стрельба в игрока
      enemy->move_update_callback([](Entity& self, double dt)->void {
        if (rndu(1'500) == 0) {
          auto bullet = hpw::entity_mgr->make(&self, "enemy.tutorial.bullet",
            self.phys.get_pos());
          bullet->phys.set_speed(2_pps);
          bullet->phys.set_deg( deg_to_target(*bullet, hpw::entity_mgr->target_for_enemy()) );
        }
      });
      enemy->move_kill_callback([this](Entity&){ --live_count; });
      ++live_count;
      assert(enemy->status.collidable);
      return ptr2ptr<Collidable*>(enemy);
    }
  }; // Spawner_enemy_shoot

  /// спавн волн противников, чтобы показать игроку как копить ману
  struct Energy_test {
    Impl* master {};
    Timer spawn_delay {0.3666}; /// сколько ждать между спавном
    constx uint WAVES {8}; /// сколько волн врагов в пачке
    constx real ENEMY_SPEED {2.5_pps};
    uint wave {WAVES};
    bool delay_state {false}; /// не спавнить врагов, когда true
    Timer wave_delay {3.0}; /// задержка перед волнами
    uint repeats {4}; /// сколько повторять волны

    inline explicit Energy_test(Impl* _master): master {_master} {}

    inline bool operator()(const double dt) {
      if (delay_state)
        accept_wave_delay(dt);
      else
        wave_spawn(dt);
      // выключить табличку после нескольких волн
      if (repeats <= 2)
        master->bg_text = {};
      return repeats == 0 || repeats >= 100;
    } // op ()

    // пауза между волнами
    inline void accept_wave_delay(const double dt) {
      cfor (_, wave_delay.update(dt)) {
        delay_state = false;
        wave = WAVES;
        --repeats;
      }
    }

    // спавнить волны врагов
    inline void wave_spawn(const double dt) {
      cfor (_, spawn_delay.update(dt)) {
        spawn_line();
        --wave;
      }
      if (wave == 0 || wave >= 100)
        delay_state = true;
    }

    /// создаёт линию противников
    inline void spawn_line() {
      constexpr real STEP = 50;
      for (real x = 0; x < graphic::width; x += STEP) {
        const Vec pos(x, -20);
        auto enemy = hpw::entity_mgr->make({}, "enemy.tutorial", pos);
        enemy->phys.set_vel({0, ENEMY_SPEED});
      }
    }
  }; // Energy_test

  /// показать кнопку фокусировки
  inline bool draw_focus_key() {
    bg_text = get_locale_str("scene.tutorial.text.focus_key");
    cauto key = hpw::keycode::focus;
    cauto pressed = is_pressed(key);
    cauto scope_l = pressed ? U'[' : U' ';
    cauto scope_r = pressed ? U']' : U' ';
    bg_text += utf32(U": ") + scope_l + hpw::keys_info.find(key)->name + scope_r;
    return pressed;
  }

  /// спавнит сетку сужающихся пуль чтобы научить игрока медленно двигаться
  struct Focus_test {
    Impl* master {};
    Timer lifetime_state_1 {18.5}; /// сколько продлится спавн пуль сверху
    Timer lifetime_state_2 {18.5}; /// сколько продлится спавн пуль сбоку
    Timer bullet_delay {0.23}; /// задержка перед спавном пули
    constx real BULLET_SPEED {1.5_pps};
    constx real MAX_STEP {110}; /// максимальное расстояние между пулями на старте
    constx real MIN_STEP {64}; /// конечное расстояние между пулями
    constx real STEP_DECREASE {0.01717}; /// скорость изменения расстояния между пулями
    real step_x {MAX_STEP};
    real step_y {MAX_STEP};
    bool complete_state_1 {};
    bool complete_state_2 {};

    inline explicit Focus_test(Impl* _master): master {_master} {}

    inline bool operator()(const double dt) {
      // убрать табличку через время
      if (step_x != MIN_STEP)
        master->draw_focus_key();
      else
        master->bg_text = {};
      // сделать сетку
      cfor (_, bullet_delay.update(dt))
        spwan_hatch(dt);
      // сужение пуль
      step_x = std::clamp<real>(step_x - STEP_DECREASE, MIN_STEP, MAX_STEP);
      if (complete_state_1)
        step_y = std::clamp<real>(step_y - STEP_DECREASE, MIN_STEP, MAX_STEP);
      // задержка между фазами
      cfor (_, lifetime_state_1.update(dt))
        complete_state_1 = true;
      if (complete_state_1)
        cfor (_, lifetime_state_2.update(dt))
          complete_state_2 = true;
      return complete_state_1 && complete_state_2;
    } // op ()

    // создаёт сетку пуль
    inline void spwan_hatch(const double dt) {
      // пули сверху
      for (real x = 25; x < graphic::width; x += step_x) {
        const Vec pos(x, -20);
        auto bullet = hpw::entity_mgr->make({}, "enemy.tutorial.bullet", pos);
        bullet->phys.set_vel({0, BULLET_SPEED});
      }
      // пули сбоку
      if (complete_state_1) {
        for (real y = 25; y < graphic::height; y += step_y) {
          const Vec pos(-20, y);
          auto bullet = hpw::entity_mgr->make({}, "enemy.tutorial.bullet", pos);
          bullet->phys.set_vel({BULLET_SPEED, 0});
        }
      }
    } // spwan_hatch
  }; // Focus_test
}; // Impl

Level_tutorial::Level_tutorial(): impl {new_unique<Impl>(this)} {}
Level_tutorial::~Level_tutorial() {}
void Level_tutorial::update(const Vec vel, double dt) {
  Level::update(vel, dt);
  impl->update(vel, dt);
}
void Level_tutorial::draw(Image& dst) const { impl->draw(dst); }
void Level_tutorial::draw_upper_layer(Image& dst) const { impl->draw_upper_layer(dst); }
Str Level_tutorial::level_name() const { return Str{Level_tutorial::NAME}; }
