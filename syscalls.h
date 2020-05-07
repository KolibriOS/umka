#ifndef SYSCALLS_H_INCLUDED
#define SYSCALLS_H_INCLUDED

#include <inttypes.h>
#include "kolibri.h"

static inline void umka_i40(pushad_t *regs) {

    __asm__ __inline__ __volatile__ (
        "push   ebp;"
        "mov    ebp, %[ebp];"
        "call   i40;"
        "pop    ebp"
        : "=a"(regs->eax),
          "=b"(regs->ebx)
        : "a"(regs->eax),
          "b"(regs->ebx),
          "c"(regs->ecx),
          "d"(regs->edx),
          "S"(regs->esi),
          "D"(regs->edi),
          [ebp] "Rm"(regs->ebp)
        : "memory");
}

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

static inline void umka_sys_process_info(int32_t pid, void *param) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(9),
          "b"(param),
          "c"(pid)
        : "memory");
}

static inline void umka_sys_window_redraw(int begin_end) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(12),
          "b"(begin_end)
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

static inline void umka_sys_get_screen_size(uint32_t *xsize, uint32_t *ysize) {
    uint32_t xysize;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(xysize)
        : "a"(14)
        : "memory");
    *xsize = (xysize >> 16) + 1;
    *ysize = (xysize & 0xffffu) + 1;
}

static inline void umka_sys_set_cwd(const char *dir) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(30),
          "b"(1),
          "c"(dir)
        : "memory");
}

static inline void umka_sys_get_cwd(const char *buf, size_t len) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(30),
          "b"(2),
          "c"(buf),
          "d"(len)
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

static inline void umka_sys_set_button_style(int style) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(48),
          "b"(1),
          "c"(style)
        : "memory");
}

static inline void umka_sys_set_window_colors(void *colors) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(48),
          "b"(2),
          "c"(colors),
          "d"(40)
        : "memory");
}

static inline void umka_sys_get_window_colors(void *colors) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(48),
          "b"(3),
          "c"(colors),
          "d"(40)
        : "memory");
}

static inline uint32_t umka_sys_get_skin_height() {
    uint32_t skin_height;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(skin_height)
        : "a"(48),
          "b"(4)
        : "memory");
    return skin_height;
}

static inline void umka_sys_get_screen_area(rect_t *wa) {
    uint32_t eax, ebx;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(eax),
          "=b"(ebx)
        : "a"(48),
          "b"(5)
        : "memory");
    wa->left   = eax >> 16;
    wa->right  = eax & 0xffffu;
    wa->top    = ebx >> 16;
    wa->bottom = ebx & 0xffffu;
}

static inline void umka_sys_set_screen_area(rect_t *wa) {
    uint32_t ecx, edx;
    ecx = (wa->left << 16) + wa->right;
    edx = (wa->top << 16) + wa->bottom;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(48),
          "b"(6),
          "c"(ecx),
          "d"(edx)
        : "memory");
}

static inline void umka_sys_get_skin_margins(rect_t *wa) {
    uint32_t eax, ebx;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(eax),
          "=b"(ebx)
        : "a"(48),
          "b"(7)
        : "memory");
    wa->left   = eax >> 16;
    wa->right  = eax & 0xffffu;
    wa->top    = ebx >> 16;
    wa->bottom = ebx & 0xffffu;
}

static inline int32_t umka_sys_set_skin(const char *path) {
    int32_t status;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(status)
        : "a"(48),
          "b"(8),
          "c"(path)
        : "memory");
    return status;
}

static inline int umka_sys_get_font_smoothing() {
    int type;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(type)
        : "a"(48),
          "b"(9)
        : "memory");
    return type;
}

static inline void umka_sys_set_font_smoothing(int type) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(48),
          "b"(10),
          "c"(type)
        : "memory");
}

static inline int umka_sys_get_font_size() {
    uint32_t size;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(size)
        : "a"(48),
          "b"(11)
        : "memory");
    return size;
}

static inline void umka_sys_set_font_size(uint32_t size) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(48),
          "b"(12),
          "c"(size)
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
          [row_offset] "Rm"(row_offset)
        : "memory");
}

static inline void umka_sys_move_window(size_t x, size_t y,
                                        ssize_t xsize, ssize_t ysize) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(67),
          "b"(x),
          "c"(y),
          "d"(xsize),
          "S"(ysize)
        : "memory");
}

static inline void umka_sys_lfn(void *f7080sXarg, f7080ret_t *r,
                                f70or80_t f70or80) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r->status),
          "=b" (r->count)
        : "a"(f70or80),
          "b"(f7080sXarg)
        : "memory");
}

static inline void umka_sys_set_window_caption(const char *caption,
                                               int encoding) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(71),
          "b"(encoding ? 2 : 1),
          "c"(caption),
          "d"(encoding)
        : "memory");
}

static inline void umka_sys_blit_bitmap(int operation, int background,
                                        int transparent, int client_relative,
                                        void *params) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(73),
          "b"((client_relative << 29) + (transparent << 5) + (background << 4)
              + operation),
          "c"(params)
        : "memory");
}

static inline uint32_t umka_sys_net_get_dev_count() {
    uint32_t count;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(count)
        : "a"(74),
          "b"(255)
        : "memory");
    return count;
}

static inline int32_t umka_sys_net_get_dev_type(uint8_t dev_num) {
    int32_t type;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(type)
        : "a"(74),
          "b"((dev_num << 8) + 0)
        : "memory");
    return type;
}

static inline int32_t umka_sys_net_get_dev_name(uint8_t dev_num, char *name) {
    int32_t status;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(status)
        : "a"(74),
          "b"((dev_num << 8) + 1),
          "c"(name)
        : "memory");
    return status;
}

static inline int32_t umka_sys_net_dev_reset(uint8_t dev_num) {
    int32_t status;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(status)
        : "a"(74),
          "b"((dev_num << 8) + 2)
        : "memory");
    return status;
}

static inline int32_t umka_sys_net_dev_stop(uint8_t dev_num) {
    int32_t status;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(status)
        : "a"(74),
          "b"((dev_num << 8) + 3)
        : "memory");
    return status;
}

static inline intptr_t umka_sys_net_get_dev(uint8_t dev_num) {
    intptr_t dev;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(dev)
        : "a"(74),
          "b"((dev_num << 8) + 4)
        : "memory");
    return dev;
}

static inline uint32_t umka_sys_net_get_packet_tx_count(uint8_t dev_num) {
    uint32_t count;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(count)
        : "a"(74),
          "b"((dev_num << 8) + 6)
        : "memory");
    return count;
}

static inline uint32_t umka_sys_net_get_packet_rx_count(uint8_t dev_num) {
    uint32_t count;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(count)
        : "a"(74),
          "b"((dev_num << 8) + 7)
        : "memory");
    return count;
}

static inline uint32_t umka_sys_net_get_byte_tx_count(uint8_t dev_num) {
    uint32_t count;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(count)
        : "a"(74),
          "b"((dev_num << 8) + 8)
        : "memory");
    return count;
}

static inline uint32_t umka_sys_net_get_byte_rx_count(uint8_t dev_num) {
    uint32_t count;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(count)
        : "a"(74),
          "b"((dev_num << 8) + 9)
        : "memory");
    return count;
}

static inline uint32_t umka_sys_net_get_link_status(uint8_t dev_num) {
    uint32_t status;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(status)
        : "a"(74),
          "b"((dev_num << 8) + 10)
        : "memory");
    return status;
}

static inline f75ret_t umka_sys_net_open_socket(uint32_t domain, uint32_t type,
                                                uint32_t protocol) {
    f75ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.value),
          "=b"(r.errorcode)
        : "a"(75),
          "b"(0),
          "c"(domain),
          "d"(type),
          "S"(protocol)
        : "memory");
    return r;
}

static inline f75ret_t umka_sys_net_close_socket(uint32_t fd) {
    f75ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.value),
          "=b"(r.errorcode)
        : "a"(75),
          "b"(1),
          "c"(fd)
        : "memory");
    return r;
}

static inline f75ret_t umka_sys_net_bind(uint32_t fd, void *sockaddr,
                                         size_t sockaddr_len) {
    f75ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.value),
          "=b"(r.errorcode)
        : "a"(75),
          "b"(2),
          "c"(fd),
          "d"(sockaddr),
          "S"(sockaddr_len)
        : "memory");
    return r;
}

static inline f75ret_t umka_sys_net_listen(uint32_t fd, uint32_t backlog) {
    f75ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.value),
          "=b"(r.errorcode)
        : "a"(75),
          "b"(3),
          "c"(fd),
          "d"(backlog)
        : "memory");
    return r;
}

static inline f75ret_t umka_sys_net_connect(uint32_t fd, void *sockaddr,
                                            size_t sockaddr_len) {
    f75ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.value),
          "=b"(r.errorcode)
        : "a"(75),
          "b"(4),
          "c"(fd),
          "d"(sockaddr),
          "S"(sockaddr_len)
        : "memory");
    return r;
}

static inline f75ret_t umka_sys_net_accept(uint32_t fd, void *sockaddr,
                                           size_t sockaddr_len) {
    f75ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.value),
          "=b"(r.errorcode)
        : "a"(75),
          "b"(5),
          "c"(fd),
          "d"(sockaddr),
          "S"(sockaddr_len)
        : "memory");
    return r;
}

static inline f76ret_t umka_sys_net_eth_read_mac(uint32_t dev_num) {
    f76ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.eax),
          "=b"(r.ebx)
        : "a"(76),
          "b"((0 << 16) + (dev_num << 8) + 0)
        : "memory");
    return r;
}

// Function 76, Protocol 1 - IPv4, Subfunction 0, Read # Packets sent =
// Function 76, Protocol 1 - IPv4, Subfunction 1, Read # Packets rcvd =

static inline f76ret_t umka_sys_net_ipv4_get_addr(uint32_t dev_num) {
    f76ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.eax),
          "=b"(r.ebx)
        : "a"(76),
          "b"((1 << 16) + (dev_num << 8) + 2)
        : "memory");
    return r;
}

static inline f76ret_t umka_sys_net_ipv4_set_addr(uint32_t dev_num,
                                                  uint32_t addr) {
    f76ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.eax),
          "=b"(r.ebx)
        : "a"(76),
          "b"((1 << 16) + (dev_num << 8) + 3),
          "c"(addr)
        : "memory");
    return r;
}

// Function 76, Protocol 1 - IPv4, Subfunction 4, Read DNS address ===
// Function 76, Protocol 1 - IPv4, Subfunction 5, Set DNS address ===
// Function 76, Protocol 1 - IPv4, Subfunction 6, Read subnet mask ===
// Function 76, Protocol 1 - IPv4, Subfunction 7, Set subnet mask ===
// Function 76, Protocol 1 - IPv4, Subfunction 8, Read gateway ====
// Function 76, Protocol 1 - IPv4, Subfunction 9, Set gateway =====
// Function 76, Protocol 2 - ICMP, Subfunction 0, Read # Packets sent =
// Function 76, Protocol 2 - ICMP, Subfunction 1, Read # Packets rcvd =
// Function 76, Protocol 3 - UDP, Subfunction 0, Read # Packets sent ==
// Function 76, Protocol 3 - UDP, Subfunction 1, Read # Packets rcvd ==
// Function 76, Protocol 4 - TCP, Subfunction 0, Read # Packets sent ==
// Function 76, Protocol 4 - TCP, Subfunction 1, Read # Packets rcvd ==
// Function 76, Protocol 5 - ARP, Subfunction 0, Read # Packets sent ==
// Function 76, Protocol 5 - ARP, Subfunction 1, Read # Packets rcvd ==
// Function 76, Protocol 5 - ARP, Subfunction 2, Read # ARP entries ==
// Function 76, Protocol 5 - ARP, Subfunction 3, Read ARP entry ====
// Function 76, Protocol 5 - ARP, Subfunction 4, Add ARP entry ====
// Function 76, Protocol 5 - ARP, Subfunction 5, Remove ARP entry ====
// Function 76, Protocol 5 - ARP, Subfunction 6, Send ARP announce ==
// Function 76, Protocol 5 - ARP, Subfunction 7, Read # conflicts ===

#endif
