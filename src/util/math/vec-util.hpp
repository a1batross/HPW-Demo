#pragma once
#include "util/macro.hpp"
#include "util/math/num-types.hpp"

#ifdef DEBUG
#include "util/str.hpp"
#endif

struct Vec;

Vec rotate_rad(const Vec center, const Vec src, real radian);
Vec rotate_deg(const Vec center, const Vec src, real degree);
real length(const Vec src);
Vec normalize_stable(const Vec src);
Vec normalize_graphic(const Vec src);
[[nodiscard]] Vec floor(const Vec src);
[[nodiscard]] Vec ceil(const Vec src);
[[nodiscard]] Vec abs(const Vec src);
/// угол между вектором в радианах
real rad_between_vecs(const Vec a, const Vec b);
/// угол между вектором
real get_between_vecs(const Vec a, const Vec b);
/// получить случайную точку вокруг pos в радиусе r
Vec make_rand_by_radius(const Vec pos, real r);

#ifdef DEBUG
Str to_str(const Vec src);
#endif

/// единичный вектор направления по углу
Vec deg_to_vec(real deg);

/// угол по единичному вектору
real vec_to_deg(Vec vec);

/// случайный угол для без защиты рандома
real rand_degree_graphic();
/// случайный угол для c защитой рандома
real rand_degree_stable();

/// случайный единичный вектор без защиты рандома
Vec rand_normalized_graphic();
/// случайный единичный вектор c защитой рандома
Vec rand_normalized_stable();

real distance(const Vec a, const Vec b);
real fast_distance(const Vec a, const Vec b);
/// угол между двумя векторами направления
[[nodiscard]] real deg_between_vecs(const Vec a, const Vec b);