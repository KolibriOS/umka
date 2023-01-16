umka_init shell

ramdisk_init ../img/kolibri.raw
set_skin /sys/DEFAULT.SKN

window_redraw 1
draw_window 10 300 5 200 0x000088 1 1 1 0 1 3 hello
window_redraw 2

set_mouse_pos_screen 307 202
osloop

mouse_move -x +1 -y +1
osloop

scrot 054_#draw_#cursor_#osloop_resize.out.png

disk_del rd
