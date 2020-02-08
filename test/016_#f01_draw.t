disk_add ../img/kolibri.img rd
set_skin /sys/DEFAULT.SKN
window_redraw 1
draw_window 0 300 0 200 0x000088 1 1 1 0 1 4 hello
set_pixel 0 0 0x0000ff
set_pixel 1 1 0xff0000
set_pixel 2 2 0x00ff00
draw_line 10 510 10 510 0xff0000
draw_rect 60 20 30 20 0x00ff00
put_image chess_image.rgb 8 8 5 15
put_image_palette chess_image.rgb 12 12 5 30 9 0
write_text 10 70 0xffff00 hello 0 0 0 0 0 5 0
button 55 40 5 20 0xc0ffee 0xffffff 1 0
display_number 0 10 4 0 0 1234 5 45 0xffff00 1 1 0 0 0x0000ff
blit_bitmap chess_image.rgba 20 35 8 8  0 0 8 8  0 0 0 1  32
window_redraw 2

set_window_caption hi_there 0
move_window 220 35 150 200
get_font_smoothing
set_font_smoothing 0
get_font_smoothing

window_redraw 1
draw_window 0 300 0 200 0x000088 1 1 1 0 1 4 hello
set_pixel 0 0 0x0000ff
set_pixel 1 1 0xff0000
set_pixel 2 2 0x00ff00
window_redraw 2

set_window_caption hi_2there 0

dump_win_stack 5
dump_win_pos 5

process_info -1

scrot umka.rgba

disk_del rd
