#!/bin/bash

CYGWIN=0
[[ $TERM = cygwin ]] && CYGWIN=1

for src in "$@"
do
    file="$(readlink -m "$src")"
    dir="${file%/*}"
    if ((CYGWIN))
    then
	dev="$( df "$dir" | awk '$1 ~ "^[A-Z]:" { print $1; exit }' )"
    else
	dev="$( df "$dir" | awk '$1 ~ "^/dev/" { print $1; exit }' )"
    fi
    inode="<$(stat -c%i "$file")>"
    usage=$(($(stat -c'%B/512*%b' "$file")/2048))
    size=$(($(stat -c%s "$file")/1024/1024))

    echo "----- $src -> $dev $inode, $usage/$size MiB -----"
    DEBUGFS_PAGER=cat /sbin/debugfs "$dev" -R "stat $inode" 2>&1 \
	| sed -r '0,/^(EXTENTS|BLOCKS):/ d; s/, */\n/g'
    echo
done
