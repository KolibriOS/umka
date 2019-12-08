disk_add ../img/xfs_v4_unicode.img hd0

stat80 /hd0/1/dir0
stat80 /hd0/1/dir0/
stat80 /hd0/1/дир❦
stat80 /hd0/1/дир❦/
stat80 /hd0/1/дир❦/дир11
stat80 /hd0/1/дир❦/дир11/
stat80 /hd0/1/❦❦❦
stat80 /hd0/1/❦❦❦/
stat80 /hd0/1/❦❦❦/д❦р22
stat80 /hd0/1/дир3

stat80 /hd0/1/dir0/file00
stat80 /hd0/1/dir0/file00/
stat80 /hd0/1/❦❦❦/д❦р22/❦❦
stat80 /hd0/1/❦❦❦/д❦р22/❦❦/
stat80 /hd0/1/❦❦❦/д❦р22/💗💗
stat80 /hd0/1/дир3/файл33
stat80 /hd0/1/дир3/файл33/

read80 /hd0/1/dir0/file00 0 100 -b
read80 /hd0/1/dir0/file00/ 0 100 -b
read80 /hd0/1/❦❦❦/д❦р22/❦❦ 0 100 -b
read80 /hd0/1/дир3/файл33 0 100 -b

disk_del hd0
