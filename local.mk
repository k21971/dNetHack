GAMEDIR = /opt/nethack/chroot/dnethack-3.24.0
#GAMEDIR = /usr/games/lib/dnethackdir
CFLAGS = -g3 -O0 -Wno-format-overflow
CPPFLAGS = -DWIZARD=\"build\" -DCOMPRESS=\"/bin/gzip\" -DCOMPRESS_EXTENSION=\".gz\"
CPPFLAGS += -DHACKDIR=\"/dnethack-3.24.0\" -DDUMPMSGS=100
#CPPFLAGS += -DHACKDIR=\"/usr/games/lib/dnethackdir\" -DDUMPMSGS=100
CPPFLAGS += -DDUMP_FN=\"/dgldir/userdata/%N/%n/dnethack/dumplog/%t.dnh.txt\"
CPPFLAGS += -DHUPLIST_FN=\"/dgldir/userdata/%N/%n/dnethack/hanguplist.txt\"
CPPFLAGS += -DEXTRAINFO_FN=\"/dgldir/extrainfo-dnh/%n.extrainfo\"
