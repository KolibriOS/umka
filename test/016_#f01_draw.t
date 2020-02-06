draw_window
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
scrot
