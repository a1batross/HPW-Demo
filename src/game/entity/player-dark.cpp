#include <cmath>
#include <algorithm>
#include <cassert>
#include "player-dark.hpp"
#include "entity-manager.hpp"
#include "util/entity-util.hpp"
#include "util/phys.hpp"
#include "util/anim-ctx.hpp"
#include "game/game-common.hpp"
#include "game/game-core.hpp"
#include "game/game-canvas.hpp"
#include "game/util/keybits.hpp"
#include "game/util/game-util.hpp"
#include "game/entity/util/scatter.hpp"
#include "game/entity/util/info/anim-info.hpp"
#include "game/entity/util/info/collidable-info.hpp"
#include "graphic/animation/anim.hpp"
#include "graphic/animation/frame.hpp"
#include "graphic/sprite/sprite.hpp"
#include "graphic/image/image.hpp"
#include "graphic/effect/light.hpp"
#include "util/hpw-util.hpp"
#include "util/math/vec-util.hpp"
#include "util/math/random.hpp"
#include "util/file/yaml.hpp"

Player_dark::Player_dark()
: Player()
, shoot_timer(0.0666666) // TODO конфиг
, m_shoot_price(70) // TODO конфиг
, m_energy_regen(37) // TODO конфиг
{
  energy_max = 25'000; // TODO конфиг
  m_energy_for_power_shoot = energy_max * 0.95; // TODO конфиг
  m_power_shoot_price = energy_max * 0.75; // TODO конфиг
}

/// спавнит мелкие пульки в след за мощным выстрелом
void spawn_small_bullets(Entity& master, double dt) {
  // TODO all vals by config

  // TODO by dt timer
  if ((hpw::game_updates_safe % 5) == 0) {
    auto it = hpw::entity_mgr->make(&master, "bullet.player.small",
      master.phys.get_pos());
    // замедлить эти пули
    it->phys.set_speed(it->phys.get_speed() * rndr(0.5, 1));
    // чтобы пули разлетались во все стороны
    it->phys.set_deg(it->phys.get_deg() + rndr(-45, 45));
    it->phys.set_force( 7.5_pps );
    it->move_update_callback( Kill_by_timeout(rndr(0.1, 0.7)) );
    it->status.layer_up = false;
  } // if timer
}

void Player_dark::shoot(double dt) {
  // стрелять менее часто, при нехватке энергии
  if (energy <= 100) // TODO конфиг
    dt *= 0.3; // TODO конфиг

  // усиленый выстрел
  if (energy >= m_energy_for_power_shoot && ability.power_shoot) {
    sub_en(m_power_shoot_price);

    cfor (_, 30) { // TODO конфиг
      cauto spawn_pos = phys.get_pos() + Vec(rndr(-7, 7), 0); // TODO конфиг
      auto bullet = hpw::entity_mgr->make(this, "bullet.player.mid", spawn_pos);
      // пуля смотрит вверх в шмап моде
      bullet->phys.set_deg(270);
      // передача импульса для шмап мода
      bullet->phys.set_speed( pps(rndr(13, 17)) ); // TODO конфиг
      bullet->phys.set_vel(bullet->phys.get_vel() + phys.get_vel());
      // разброс
      bullet->phys.set_deg(bullet->phys.get_deg() + rndr(-75, 75)); // TODO конфиг
      bullet->move_update_callback(&spawn_small_bullets);
      bullet->status.layer_up = true;
    }

    // отдача
    hpw::entity_mgr->add_scatter(Scatter {
      .pos{ phys.get_pos() + Vec(0, -10) }, // создаёт источнив взрыва перед ноосом
      .range{ 50 }, // TODO конфиг
      .power{ pps(60) }, // TODO конфиг
    });
  } else { // обычные выстрелы
    sub_en(m_shoot_price);

    cfor (_, shoot_timer.update(dt)) {
      cfor (__, 2) { /// по три выстрела за раз
        cauto spawn_pos = phys.get_pos() + Vec(rndr(-7, 7), 5); // TODO конфиг
        auto bullet = hpw::entity_mgr->make(this, "bullet.player.small", spawn_pos);
        // пуля смотрит вверх в шмап моде
        bullet->phys.set_deg(270);
        // передача импульса для шмап мода
        bullet->phys.set_speed(20_pps); // TODO конфиг
        bullet->phys.set_vel(bullet->phys.get_vel() + phys.get_vel());
        // разброс
        bullet->phys.set_deg(bullet->phys.get_deg() + rndr(-7, 7)); // TODO конфиг
      }
    }
  }
} // shoot

void Player_dark::sub_en(hp_t val) { energy = std::max<hp_t>(0, energy - val); }
void Player_dark::energy_regen() { energy = std::min<hp_t>(energy_max, energy + m_energy_regen); }

void Player_dark::draw(Image& dst, const Vec offset) const {
  Collidable::draw(dst, offset);
  blink_contour();
  draw_stars(dst);
}

void Player_dark::update(double dt) {
  assert(hpw::shmup_mode); // вне шмап-мода этот класс не юзать

  energy_regen();
  set_dmg(get_hp()); /// урон при столкновении с игроком равен текущему хп игрока
  Collidable::update(dt);
  check_input(dt);
}

void Player_dark::check_input(double dt) {
  real spd;

  if (is_pressed(hpw::keycode::focus)) {
    spd = focus_speed;
    phys.set_force(focus_force);
  } else {
    spd = max_speed;
    phys.set_force(default_force);
  }

  Vec dir;
  if (is_pressed(hpw::keycode::up))    dir += Vec(0, -1);
  if (is_pressed(hpw::keycode::down))  dir += Vec(0, +1);
  if (is_pressed(hpw::keycode::left))  dir += Vec(-1, 0);
  if (is_pressed(hpw::keycode::right)) dir += Vec(+1, 0);

  // если игрок двигается
  if (dir) {
    auto motion = normalize_stable(dir) * spd;
    auto cur_vel = phys.get_vel();
    auto cur_speed = phys.get_speed();
    phys.set_vel(cur_vel + motion);

    // добавить скорость, но не больше чем есть сейчас или spd
    auto avaliable_speed = std::max(cur_speed, spd);
    phys.set_speed(std::min(avaliable_speed, phys.get_speed()) );
  }

  // стрельба
  if (is_pressed(hpw::keycode::shoot))
    shoot(dt);
} // check_input

void Player_dark::blink_contour() const {
  assert(energy_max > 0);
  // конгда энергии мало, контур тусклый
  if (energy < energy_max * 0.9) { // TODO conf
    cauto ratio = energy / scast<real>(energy_max);
    anim_ctx.contour_bf = &blend_158;
    // чем меньше энергии, тем реже мерцать
    status.disable_contour = rndr_fast() >= ratio;
  } else { // конгда энергии много, контур белый и мерцает рандомно
    anim_ctx.contour_bf = &blend_past;
    status.disable_contour = rndb_fast() & 1;
  }
}

void Player_dark::draw_stars(Image& dst) const {
  assert(energy_max > 0);
  // расположение всех окошек бумера
  sconst Vector<Vec> window_pos_table {

                              {0, 0},
                        {-3, 2},     {3, 2},
                 {-6, 5},                  {6, 5},
          {-9, 8},                                {9, 8},
    {-12, 11},                                         {12, 11},

  }; // window_pos_table

  cauto ratio = energy / scast<real>(energy_max);
  for (cnauto window_pos: window_pos_table) {
    // вспышка не всегда появляется
    cont_if (rndb_fast() > 16);

    // рисует звезду
    Light star;
    star.set_duration(1);
    star.radius = ratio * rndr_fast(0, 15); // TODO get star len from config
    star.flags.no_sphere = true;
    star.draw(dst, window_pos + anim_ctx.get_draw_pos());
  }
} // draw_stars

/// ограничитель позиции игрока в пределах экрана
struct Bound_off_screen {
  Vec screen_lu {}; /// ограничение слева сверху
  Vec screen_rd {}; /// ограничение справа снизу

  inline explicit Bound_off_screen(CN<Entity> src) {
    // получить размеры игрока
    auto anim = src.get_anim();
    assert(anim);
    auto frame = anim->get_frame(0);
    assert(frame);
    auto direct = frame->get_direct(0);
    assert(direct);
    auto sprite = direct->sprite.lock();
    assert(sprite);
    auto image = *sprite->get_image();
    Vec player_sz(image.X, image.Y);
    screen_lu = Vec(
      -1 * direct->offset.x,
      -1 * direct->offset.y
    );
    screen_rd = Vec(
      graphic::width  - player_sz.x - direct->offset.x,
      graphic::height - player_sz.y - direct->offset.y
    );
  } // c-tor

  inline void operator()(Entity& dst, double dt) {
    auto pos = dst.phys.get_pos();
    bool decrease_speed {false};
    if (pos.x < screen_lu.x)
      { pos.x = screen_lu.x; decrease_speed = true; }
    if (pos.x >= screen_rd.x)
      { pos.x = screen_rd.x-1; decrease_speed = true; }
    if (pos.y < screen_lu.y)
      { pos.y = screen_lu.y; decrease_speed = true; }
    if (pos.y >= screen_rd.y)
      { pos.y = screen_rd.y-1; decrease_speed = true; }
    // это фиксит быстрое движение при отталкивании
    if (decrease_speed)
      dst.phys.set_speed( dst.phys.get_speed() * 0.25 );
    dst.phys.set_pos(pos);
  } // op ()
}; // Bound_off_screen

struct Player_dark::Loader::Impl {
  Anim_info m_anim_info {};
  Collidable_info m_collidable_info {};
  real m_force {};
  real m_focus_force {};
  real m_max_speed {};
  real m_focus_speed {};

  inline explicit Impl(CN<Yaml> config) {
    m_collidable_info.load(config);
    m_anim_info.load(config["animation"]);
    m_force       = config.get_real("force");
    m_focus_force = config.get_real("focus_force");
    m_max_speed   = config.get_real("max_speed");
    m_focus_speed = config.get_real("focus_speed");
  } // c-tor

  inline Entity* operator()(Entity* master, const Vec pos, Entity* parent) {
    auto entity = hpw::entity_mgr->allocate<Player_dark>();
    Entity_loader::prepare(*entity, master, pos);
    m_collidable_info.accept(*entity);
    m_anim_info.accept(*entity);
    
    nauto it = *scast<Player_dark*>(entity);
    it.move_update_callback( Bound_off_screen(it) );
    it.phys.set_force( pps(m_force) );
    it.default_force = pps(m_force);
    it.focus_force = pps(m_focus_force);
    it.focus_speed = pps(m_focus_speed);
    it.max_speed = pps(m_max_speed);
    // TODO en
    // TODO fuel
    it.hp_max = it.get_hp();

    return entity;
  } // op ()
}; // Impl

Player_dark::Loader::Loader(CN<Yaml> config): impl{new_unique<Impl>(config)} {}
Player_dark::Loader::~Loader() {}
Entity* Player_dark::Loader::operator()(Entity* master, const Vec pos, Entity* parent) { return impl->operator()(master, pos, parent); }
