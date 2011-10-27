#!/usr/bin/env bash

revision=0

if [[ -d .svn ]] && which svn >/dev/null 2>&1
then
    revision="$( svn info . | awk '$1=="Revision:" {print $2}' )"
    if which svnversion >/dev/null 2>&1
    then
	rev="$(svnversion|sed 's/.*://')"
	(( ${revision//[!0-9]/} < ${rev//[!0-9]/} )) && revision=$rev
    fi
fi

revision_num="${revision//[!0-9]/}"
revision_next=$revision_num
[[ $revision = $revision_num ]] || let revision_next++

tim=($(date '+%s %Y-%m-%d %T'))

have_fuse=0
[[ $NO_FUSE != 1 && -r /usr/include/fuse.h || -r /usr/local/include/fuse.h ]] \
	&& have_fuse=1

if [[ $M32 = 1 ]]
then
    force_m32=1
    have_fuse=0
    xflags="-m32"
    defines=-DFORCE_M32=1
else
    force_m32=0
    xflags=
    defines=
fi

[[ -r /usr/include/bits/fcntl.h ]] \
	&& grep -qw fallocate /usr/include/bits/fcntl.h \
	&& defines="$defines -DHAVE_FALLOCATE=1"

[[ -r /usr/include/fcntl.h ]] \
	&& grep -qw posix_fallocate /usr/include/fcntl.h \
	&& defines="$defines -DHAVE_POSIX_FALLOCATE=1"

[[ $STATIC = 1 ]] || STATIC=0

cat <<- ---EOT--- >Makefile.setup
	REVISION	:= $revision
	REVISION_NUM	:= $revision_num
	REVISION_NEXT	:= $revision_next
	BINTIME		:= ${tim[0]}
	DATE		:= ${tim[1]}
	TIME		:= ${tim[2]}

	FORCE_M32	:= $force_m32
	HAVE_FUSE	:= $have_fuse
	STATIC		:= $STATIC
	XFLAGS		+= $xflags
	DEFINES1	:= $defines

	---EOT---

gcc $xflags system.c -o system.tmp && ./system.tmp >>Makefile.setup
rm -f system.tmp

