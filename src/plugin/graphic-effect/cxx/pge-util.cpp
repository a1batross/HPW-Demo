#include "plugin/graphic-effect/hpw-plugin-effect.h"
#include "pge-util.hpp"
#include "graphic/image/color.hpp"

NOT_EXPORTED
bool check_params(const context_t* context, result_t* result) {
  result->error = "";
  result->version = DEFAULT_EFFECT_API_VERSION;
  result->init_succsess = true;

  #define iferror(cond, msg) if (cond) { \
    result->error = msg; \
    result->init_succsess = false; \
    return false; \
  }
  iferror( !context->dst, "context.dst is null");
  iferror(context->h == 0, "context.h is 0");
  iferror(context->w == 0, "context.w is 0");
  iferror( !context->registrate_param_f32, "registrate_param_f32 is null");
  iferror( !context->registrate_param_i32, "registrate_param_i32 is null");
  iferror( !context->registrate_param_bool, "registrate_param_bool is null");
  #undef iferror
  return true;
} // check_params

NOT_EXPORTED
Pal8& get_pixel_fast(Pal8 image[], const int x, const int y,
const int width)
  { return image[x + width * y]; }

NOT_EXPORTED
void set_pixel_fast(Pal8 image[], const int x, const int y,
const int width, const Pal8 val)
  { image[x + width * y] = val; }

NOT_EXPORTED
Pal8 get_pixel_safe(Pal8 image[], const int x, const int y,
const int width, const int height) {
  if (x >= 0 && x < width && y >= 0 && y < height)
    return image[x + width * y];
  return Pal8::black;
}

NOT_EXPORTED
void set_pixel_safe(Pal8 image[], const int x, const int y,
const int width, const int height, const Pal8 val) {
  if (x >= 0 && x < width && y >= 0 && y < height)
    image[x + width * y] = val;
}
