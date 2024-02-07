#include <utility>
#include "sprite-io.hpp"
#include "graphic/image/image.hpp"
#include "sprite.hpp"
//#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
//#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#include "util/str-util.hpp"
#include "util/file/file.hpp"
#include "util/log.hpp"
#include "util/error.hpp"
#include "graphic/util/convert.hpp"

/** делает контур картинки жирнее
Нужен для картинки в rotsprite, чтобы при повороте не было артефактов */
inline void expand_contour(Image& image, CN<Image> mask) {
  cfor (y, image.Y)
  cfor (x, image.X) {
    if (mask(x, y) == Pal8::mask_visible) {
      if (mask.get(x-1, y) == Pal8::mask_invisible)
        image.set(x-1, y, image(x, y));
      if (mask.get(x+1, y) == Pal8::mask_invisible)
        image.set(x+1, y, image(x, y));
      if (mask.get(x, y-1) == Pal8::mask_invisible)
        image.set(x, y-1, image(x, y));
      if (mask.get(x, y+1) == Pal8::mask_invisible)
        image.set(x, y+1, image(x, y));
    }
  } // for y, x
} // expand_contour

inline void _data_to_sprite(Sprite &dst, CN<File> file) {
  iferror(file.data.empty(), "_data_to_sprite: file is empty");
  // декодирование:
  int x, y;
  int comp; // сколько цветовых каналов
  stbi_uc *decoded = stbi_load_from_memory( scast<CP<stbi_uc>>(file.data.data()),
    file.data.size(), &x, &y, &comp, STBI_rgb_alpha );
  iferror( !decoded, "_data_to_image: image data is not decoded. File: \"" << file.get_path() << "\"");
// переносим данные в спрайт:
  Image image(x, y);
  Image mask(x, y, Pal8::mask_invisible); // полностью прозрачная маска
  // привязать пути
  auto file_path = file.get_path();
  image.set_path(file_path);
  mask.set_path(file_path);
  // сконвертировать цвета
  int rgba_index = 0;
  for (int i = 0; i < x * y; ++i) {
    auto r = decoded[rgba_index + 0];
    auto g = decoded[rgba_index + 1];
    auto b = decoded[rgba_index + 2];
    auto a = decoded[rgba_index + 3];
    rgba_index += 4;
    Pal8 col = desaturate_average(r, g, b);
    image.fast_set(i, col, {});
    if (a > 127)
      mask.fast_set(i, Pal8::mask_visible, {}); // не прозрачный пиксель
  } // for size
  stbi_image_free(decoded);
  expand_contour(image, mask);
  dst.move_image(std::move(image));
  dst.move_mask(std::move(mask));
} // _data_to_sprite

void load(Sprite &dst, CN<File> file) {
  detailed_log("Sprite.load_file \"" << file.get_path() << "\"\n");
  _data_to_sprite(dst, file);
} // load

void load(Sprite &dst, CN<Str> name) {
  detailed_log("Sprite.load_file(F) \"" << name << "\"\n");
  File file {mem_from_file(name), name};
  _data_to_sprite(dst, file);
} // load

void load(CN<File> file, Sprite &dst) {
  iferror( file.data.empty(), "load sprite(M): file is empty");
  _data_to_sprite(dst, file);
}
