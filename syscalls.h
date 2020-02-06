#ifndef SYSCALLS_H_INCLUDED
#define SYSCALLS_H_INCLUDED

#include <inttypes.h>
#include "kolibri.h"

static inline void umka_sys_draw_window(size_t x, size_t xsize,
                                        size_t y, size_t ysize,
                                        uint32_t color, int has_caption,
                                        int client_relative,
                                        int fill_workarea, int gradient_fill,
                                        int movable, uint32_t style,
                                        const char *caption) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(0),
          "b"((x << 16) + xsize),
          "c"((y << 16) + ysize),
          "d"((gradient_fill << 31) + (!fill_workarea << 30)
              + (client_relative << 29) + (has_caption << 28) + (style << 24)
              + color),
          "S"(!movable << 24),
          "D"(caption)
        : "memory");
}

static inline void umka_sys_set_pixel(size_t x, size_t y, uint32_t color,
                                      int invert) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(1),
          "b"(x),
          "c"(y),
          "d"((invert << 24) + color)
        : "memory");
}

static inline void umka_sys_write_text(size_t x, size_t y,
                                       uint32_t color,
                                       int asciiz, int fill_background,
                                       int font_and_encoding,
                                       int draw_to_buffer,
                                       int scale_factor,
                                       const char *string, size_t length,
                                       uintptr_t background_color_or_buffer) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(4),
          "b"((x << 16) + y),
          "c"((asciiz << 31) + (fill_background << 30)
              + (font_and_encoding << 28) + (draw_to_buffer << 27)
              + (scale_factor << 24) + color),
          "d"(string),
          "S"(length),
          "D"(background_color_or_buffer)
        : "memory");
}

static inline void umka_sys_put_image(void *image, size_t xsize, size_t ysize,
                                      size_t x, size_t y) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(7),
          "b"(image),
          "c"((xsize << 16) + ysize),
          "d"((x << 16) + y)
        : "memory");
}

static inline void umka_sys_button(size_t x, size_t xsize,
                                   size_t y, size_t ysize,
                                   size_t button_id,
                                   int draw_button, int draw_frame,
                                   uint32_t color) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(8),
          "b"((x << 16) + xsize),
          "c"((y << 16) + ysize),
          "d"((!draw_button << 30) + (!draw_frame << 29) + button_id),
          "S"(color)
        : "memory");
}

static inline void umka_sys_draw_rect(size_t x, size_t xsize,
                                      size_t y, size_t ysize,
                                      uint32_t color, int gradient) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(13),
          "b"((x << 16) + xsize),
          "c"((y << 16) + ysize),
          "d"((gradient << 31) + color)
        : "memory");
}

static inline void umka_sys_draw_line(size_t x, size_t xend,
                                      size_t y, size_t yend,
                                      uint32_t color, int invert) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(38),
          "b"((x << 16) + xend),
          "c"((y << 16) + yend),
          "d"((invert << 24) + color)
        : "memory");
}

static inline void umka_sys_display_number(int is_pointer, int base,
                                           int digits_to_display, int is_qword,
                                           int show_leading_zeros,
                                           int number_or_pointer,
                                           size_t x, size_t y,
                                           uint32_t color, int fill_background,
                                           int font, int draw_to_buffer,
                                           int scale_factor,
                                           uintptr_t background_color_or_buffer) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(47),
          "b"(is_pointer + (base << 8) + (digits_to_display << 16)
              + (is_qword << 30) + (show_leading_zeros << 31)),
          "c"(number_or_pointer),
          "d"((x << 16) + y),
          "S"(color + (fill_background << 30) + (font << 28)
              + (draw_to_buffer << 27) + (scale_factor << 24)),
          "D"(background_color_or_buffer)
        : "memory");
}

static inline void umka_sys_put_image_palette(void *image,
                                              size_t xsize, size_t ysize,
                                              size_t x, size_t y,
                                              size_t bpp, void *palette,
                                              size_t row_offset) {
    __asm__ __inline__ __volatile__ (
        "push   ebp;"
        "mov    ebp, %[row_offset];"
        "call   i40;"
        "pop    ebp"
        :
        : "a"(65),
          "b"(image),
          "c"((xsize << 16) + ysize),
          "d"((x << 16) + y),
          "S"(bpp),
          "D"(palette),
          [row_offset] "rm"(row_offset)
        : "memory");
}

static inline void umka_sys_lfn(void *f7080sXarg, f7080ret_t *r,
                                f70or80_t f70or80) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a" (r->status),
          "=b" (r->count)
        : "a"(f70or80),
          "b"(f7080sXarg)
        : "memory");
}

#endif
