umka_init shell
#set_mouse_pos_screen 40 30
ramdisk_init ../img/kolibri.raw
set_skin /sys/DEFAULT.SKN

load_cursor_from_file /sys/fill.cur
var $cur_fill
load_cursor_from_mem ../spray.cur
var $cur_spray

window_redraw 1
draw_window 10 300 5 200 0x000088 1 1 1 0 1 4 hello
window_redraw 2

set_cursor $cur_fill


scrot 051_#draw_#cursor_all.out.png


disk_del rd
