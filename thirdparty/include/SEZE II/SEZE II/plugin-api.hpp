#pragma once
/** @brief SEZE II plugin API (for HPW)
  * @author HPW-dev (hpwdev0@gmail.com)
  * @version 2.0
  * @date 2023-05-25
  * @copyright Copyright (c) 2021-2023, HPW-dev */
#include "hpw/types.hpp"
#include "hpw/macro.hpp"

//! avaliable color formats
enum color_t {
  seze_none,
  seze_RAW,
  seze_RGB8,
  seze_RGB24, ///< RGB 8:8:8
  seze_BGR24, ///< BGR 8:8:8
  seze_RGB555, ///< RGB 5:5:5
  seze_RGB565, ///< RGB 5:6:5
  seze_YUV24, ///< YUV 4:4:4 (24bit)
  seze_YUVf,
  seze_YUVld,
  seze_RGBi,
  seze_RGBf,
  seze_RGBld,
  seze_f_gray,
  seze_ld_gray,
  seze_gray, ///< 1 byte grayscale
}; // color_t

//! enable multithread processing
constexpr uint PLGNINF_MULTITHREAD = 1 << 0;

struct PluginInfo {
  CP(char) title {}; ///< plugin name
  CP(char) info {}; ///< instruction of using
  uint version {}; ///< plugin ver
  int in_x {}, in_y {}; ///< internal framebuffer resolution
  enum color_t color_type {}; ///< color type for plugin
  uint flags {};
};

inline static void PluginInfo_init(PluginInfo* info) {
  info->title = 0;
  info->info = 0;
  info->version = 2;
  info->in_x = 0;
  info->in_y = 0;
  info->color_type = seze_none;
  info->flags = PLGNINF_MULTITHREAD;
}

inline static void bit_enable(uint* flag, uint bit)
{ *flag |= bit; }

inline static void bit_disable(uint* flag, uint bit)
{ *flag &= ~bit; }

inline static void bit_set_if(uint* flag, uint bit, int cond) {
  if (cond)
    bit_enable(flag, bit);
  else
    bit_disable(flag, bit);
}

extern "C" {
struct PluginInfo init(CP(char) options);
void core(byte* dst, int mx, int my, int stride, enum color_t color_type);
void finalize();
}
