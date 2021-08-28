GAMEDIR = /opt/nethack/chroot/dnethack-3.21.1
CFLAGS = -g3 -O0 -Wno-format-overflow
CPPFLAGS = -DWIZARD=\"build\" -DCOMPRESS=\"/bin/gzip\" -DCOMPRESS_EXTENSION=\".gz\"
CPPFLAGS += -DHACKDIR=\"/dnethack-3.21.1\" -DDUMPMSGS=200
CPPFLAGS += -DDUMP_FN=\"/dgldir/userdata/%N/%n/dnethack/dumplog/%t.dnh.txt\"
CPPFLAGS += -DHUPLIST_FN=\"/dgldir/userdata/%N/%n/dnethack/hanguplist.txt\"
CPPFLAGS += -DEXTRAINFO_FN=\"/dgldir/extrainfo-dnh/%n.extrainfo\"

