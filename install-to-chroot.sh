#!/bin/bash
#
# Ideas and some parts from the original dgl-create-chroot (by joshk@triplehelix.org, modifications by jilles@stack.nl)
# More by <paxed@alt.org>
# More by Michael Andrew Streib <dtype@dtype.org>
# Licensed under the MIT License
# https://opensource.org/licenses/MIT

# autonamed chroot directory. Can rename.
DATESTAMP=`date +%Y%m%d-%H%M%S`
NAO_CHROOT="/opt/nethack/chroot"
# already compiled versions of dgl and nethack
NETHACK_GIT="/home/build/dNetHack"
# the user & group from dgamelaunch config file.
USRGRP="games:games"
# fixed data to copy (leave blank to skip)
NH_GIT="/home/build/dNetHack"
# HACKDIR from include/config.h; aka nethack subdir inside chroot
# Make a new one each time save compat is broken
NHSUBDIR="dnethack-3.21.3"
#for combining xlogfile, etc on minor version bump
#NH_LOG_SYMLINK_TARGET="/dnethack-3.21.3"
# END OF CONFIG
##############################################################################

errorexit()
{
    echo "Error: $@" >&2
    exit 1
}

findlibs()
{
  for i in "$@"; do
      if [ -z "`ldd "$i" | grep 'not a dynamic executable'`" ]; then
         echo $(ldd "$i" | awk '{ print $3 }' | egrep -v ^'\(' | grep lib)
         echo $(ldd "$i" | grep 'ld-linux' | awk '{ print $1 }')
      fi
  done
}

set -e

umask 022

echo "Creating inprogress and userdata directories"
mkdir -p "$NAO_CHROOT/dgldir/inprogress-dnh3213"
chown "$USRGRP" "$NAO_CHROOT/dgldir/inprogress-dnh3213"
mkdir -p "$NAO_CHROOT/dgldir/extrainfo-dnh"
chown "$USRGRP" "$NAO_CHROOT/dgldir/extrainfo-dnh"

echo "Making $NAO_CHROOT/$NHSUBDIR"
mkdir -p "$NAO_CHROOT/$NHSUBDIR"
chown "$USRGRP" "$NAO_CHROOT/$NHSUBDIR"

NETHACKBIN="$NETHACK_GIT/src/dnethack"
if [ -n "$NETHACKBIN" -a ! -e "$NETHACKBIN" ]; then
  errorexit "Cannot find NetHack binary $NETHACKBIN"
fi

if [ -n "$NETHACKBIN" -a -e "$NETHACKBIN" ]; then
  echo "Copying $NETHACKBIN"
  cd "$NAO_CHROOT/$NHSUBDIR"
  NHBINFILE="`basename $NETHACKBIN`-$DATESTAMP"
  cp "$NETHACKBIN" "$NHBINFILE"
  ln -fs "$NHBINFILE" dnethack
  LIBS="$LIBS `findlibs $NETHACKBIN`"
  cd "$NAO_CHROOT"
fi

echo "Copying NetHack playground stuff"
cp "$NETHACK_GIT/dat/nhdat" "$NAO_CHROOT/$NHSUBDIR"
chmod 644 "$NAO_CHROOT/$NHSUBDIR/nhdat"

echo "Creating NetHack variable dir stuff."
# mkdir -p "$NAO_CHROOT/$NHSUBDIR/var"
# chown -R "$USRGRP" "$NAO_CHROOT/$NHSUBDIR/var"
mkdir -p "$NAO_CHROOT/$NHSUBDIR/save"
chown -R "$USRGRP" "$NAO_CHROOT/$NHSUBDIR/save"
mkdir -p "$NAO_CHROOT/$NHSUBDIR/whereis"
chown -R "$USRGRP" "$NAO_CHROOT/$NHSUBDIR/whereis"

#symlink the logs to the symlink target
if [ -z "$NH_LOG_SYMLINK_TARGET" -o ! -e "$NAO_CHROOT$NH_LOG_SYMLINK_TARGET" -o "$NH_LOG_SYMLINK_TARGET" = "/$NHSUBDIR" ]; then
  # don't symlink file to itself
  touch "$NAO_CHROOT/$NHSUBDIR/logfile"
  chown -R "$USRGRP" "$NAO_CHROOT/$NHSUBDIR/logfile"
  touch "$NAO_CHROOT/$NHSUBDIR/record"
  chown -R "$USRGRP" "$NAO_CHROOT/$NHSUBDIR/record"
  touch "$NAO_CHROOT/$NHSUBDIR/xlogfile"
  chown -R "$USRGRP" "$NAO_CHROOT/$NHSUBDIR/xlogfile"
  touch "$NAO_CHROOT/$NHSUBDIR/livelog"
  chown -R "$USRGRP" "$NAO_CHROOT/$NHSUBDIR/livelog"
  touch "$NAO_CHROOT/$NHSUBDIR/perm"
  chown -R "$USRGRP" "$NAO_CHROOT/$NHSUBDIR/perm"
else
  if [ -f $NAO_CHROOT/$NHSUBDIR/xlogfile ]; then
    errorexit "$NAO_CHROOT/$NHSUBDIR/xlogfile exists as a regular file. Proceeding will casuse data loss."
  fi
  ln -fs $NH_LOG_SYMLINK_TARGET/xlogfile $NAO_CHROOT/$NHSUBDIR
  ln -fs $NH_LOG_SYMLINK_TARGET/livelog $NAO_CHROOT/$NHSUBDIR
  ln -fs $NH_LOG_SYMLINK_TARGET/record $NAO_CHROOT/$NHSUBDIR
  ln -fs $NH_LOG_SYMLINK_TARGET/logfile $NAO_CHROOT/$NHSUBDIR
  ln -fs $NH_LOG_SYMLINK_TARGET/perm $NAO_CHROOT/$NHSUBDIR
fi

RECOVER="$NETHACK_GIT/util/recover"

if [ -n "$RECOVER" -a -e "$RECOVER" ]; then
  echo "Copying $RECOVER"
  cp "$RECOVER" "$NAO_CHROOT/$NHSUBDIR"
  LIBS="$LIBS `findlibs $RECOVER`"
  cd "$NAO_CHROOT"
fi


LIBS=`for lib in $LIBS; do echo $lib; done | sort | uniq`
echo "Copying libraries:" $LIBS
for lib in $LIBS; do
        mkdir -p "$NAO_CHROOT`dirname $lib`"
        if [ -f "$NAO_CHROOT$lib" ]
	then
		echo "$NAO_CHROOT$lib already exists - skipping."
	else
		cp $lib "$NAO_CHROOT$lib"
	fi
done

echo "Finished."
