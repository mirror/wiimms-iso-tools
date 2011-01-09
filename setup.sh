#!/bin/bash

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

if [[ $M32 = 1 ]]
then
    force_m32=1
    cflags="-m32"
    defines=
    #defines="-DHAVE_FALLOCATE=1"
else
    force_m32=0
    cflags=
    defines=
fi

cat <<- ---EOT--- >Makefile.setup
	REVISION	:= $revision
	REVISION_NUM	:= $revision_num
	REVISION_NEXT	:= $revision_next
	BINTIME		:= ${tim[0]}
	DATE		:= ${tim[1]}
	TIME		:= ${tim[2]}

	FORCE_M32	:= $force_m32
	CFLAGS		:= $cflags
	DEFINES1	:= $defines

	---EOT---

gcc $cflags system.c -o system.tmp && ./system.tmp >>Makefile.setup
rm -f system.tmp

