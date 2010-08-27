#!/bin/bash
# (c) Wiimm, 2010-08-26

myname="${0##*/}"
base=wwt+wit
log=$base.log
err=$base.err

export LC_ALL=C

if [[ $# == 0 ]]
then
    cat <<- ---EOT---

	This script expect as parameters names of ISO files. ISO files are PLAIN,
	CISO, WBFS, WDF or WIA. Each source file is subject of this test suite.

	Tests:

	  * wwt ADD and EXTRACT
	    - WBFS-files with different sector sizes: 512, 1024, 2048, 4096
	    - ADD to and EXTRACT from PLAIN ISO, CISO, WDF, WIA, WBFS
	    - ADD from PIPE
	
	  * wit COPY
	    - convert to PLAIN ISO, CISO, WDF, WIA, WBFS

	Usage:  $myname [option]... iso_file...

	Options:
	  --fast        : Enter fast mode
	                => do only WDF tests and skip verify+fst+pipe tests
	  --verify      : enable  verify tests
	  --no-verify   : disable verify tests
	  --fst         : enable  "EXTRACT FST" tests
	  --no-fst      : disable "EXTRACT FST" tests
	  --pipe        : enable  pipe tests
	  --no-pipe     : disable pipe tests (default for cygwin)
	  --compress    : enable WIA compression tests
	  --no-compress : enable WIA compression tests

	---EOT---
    exit 1
fi

#
#------------------------------------------------------------------------------
# check exitence of needed tools

WWT=wwt
WIT=wit
WDFCAT=wdf-cat
[[ -x ./wwt ]] && WWT=./wwt
[[ -x ./wit ]] && WIT=./wit
[[ -x ./wdf-cat ]] && WDFCAT=./wdf-cat

errtool=
for tool in $WWT $WIT $WDFCAT cmp
do
    which $tool >/dev/null 2>&1 || errtool="$errtool $tool"
done

if [[ $errtool != "" ]]
then
    echo "$myname: missing tools in PATH:$errtool" >&2
    exit 2
fi

#
#------------------------------------------------------------------------------
# timer function

let BASE_SEC=$(date +%s)

function get_msec()
{
    echo $(($(date "+(%s-BASE_SEC)*1000+10#%N/1000000")))
}

#-----------------------------------

function print_timer()
{
    # $1 = start time
    # $2 = end time
    # $3 = name if set
    local tim ms s m h
    let tim=$2-$1
    let ms=tim%1000
    let tim=tim/1000
    let s=tim%60
    let tim=tim/60
    let m=tim%60
    let h=tim/60
    if ((h))
    then
	printf "%u:%02u:%02u.%03u hms  %s\n" $h $m $s $ms "$3"
    elif ((m))
    then
	printf "  %2u:%02u.%03u m:s  %s\n" $m $s $ms "$3"
    else
	printf "     %2u.%03u sec  %s\n" $s $ms "$3"
    fi
}

#-----------------------------------

function print_stat()
{
    printf "%-26s : " "$(printf "$@")"
}

#
#------------------------------------------------------------------------------
# setup

function f_abort()
{
    echo
    {
        msg="###  ${myname} CANCELED  ###"
        sep="############################"
        sep="$sep$sep$sep$sep"
        sep="${sep:1:${#msg}}"
        echo ""
        echo "$sep"
        echo "$msg"
        echo "$sep"
        echo ""
        echo "remove tempdir: $tempdir"
        rm -rf "$tempdir"
        sync
        echo ""
    } >&2

    sleep 1
    exit 2
}

tempdir=
trap f_abort INT TERM HUP
tempdir="$(mktemp -d ./.$base.tmp.XXXXXX)" || exit 1

WBFS_FILE=a.wbfs
WBFS="$tempdir/$WBFS_FILE"

MODELIST="iso wdf wia ciso wbfs"
BASEMODE="wdf"

FAST_MODELIST="wdf"
FAST_BASEMODE="wdf"

NOVERIFY=0
NOFST=0
NOPIPE=0
NOCOMPRESS=1
[[ $TERM = cygwin ]] && NOPIPE=1

FST_OPT="--psel data,update,channel,ptab0"

export WIT_OPT=
export WWT_OPT=
export WWT_WBFS=

# error codes

STAT_OK=0
STAT_NOISO=1
STAT_DIFF=2
ERROR=10

#
#------------------------------------------------------------------------------
# test function

function test_function()
{
    # $1 = short name
    # $2 = info
    # $3... = command

    ref="$1"
    print_stat " > %s" "$2"
    shift 2
    start=$(get_msec)
    #echo; echo "$@"
    "$@" || return $ERROR
    stop=$(get_msec)
    print_timer $start $stop
    return $STAT_OK
}

#
#------------------------------------------------------------------------------
# test suite

function test_suite()
{
    # $1 = valid source

    rm -rf "$tempdir"/*
    mkdir -p "$tempdir"

    local src dest count hss

    ref="-"
    ft="$($WWT FILETYPE -H -l "$1" | awk '{print $1}')" || return $ERROR
    id6="$($WWT FILETYPE -H -l "$1" | awk '{print $2}')" || return $ERROR

    printf "\n** %s %s %s\n" "$ft" "$id6" "$1"
    [[ $id6 = "-" ]] && return $STAT_NOISO

    #----- verify source

    if ((!NOVERIFY))
    then
	test_function "VERIFY" "wit VERIFY source" \
	    $WIT VERIFY -qq "$1" \
	    || return $ERROR
    fi

    #----- test WWT

    hss=512
    rm -f "$WBFS"
    test_function "INIT-$hss" "wwt INIT wbfs hss=$hss" \
	$WWT INIT -q --force --size=20g --hss=$hss "$WBFS" \
	|| return $ERROR

    test_function "ADD-src" "wwt ADD source" \
	$WWT -qp "$WBFS" ADD "$1" \
	|| return $ERROR

    for mode in $MODELIST
    do
	[[ $mode = wia ]] && continue

	dest="$tempdir/image.$mode"
	test_function "EXT-$mode" "wwt EXTRACT to $mode" \
	    $WWT -qp "$WBFS" EXTRACT "$id6=$dest" --$mode --no-compress \
	    || return $ERROR

	rm -f "$WBFS"
	test_function "INIT-$hss" "wwt INIT wbfs hss=$hss" \
	    $WWT INIT -q --force --size=20g --hss=$hss "$WBFS" \
	    || return $ERROR
	let hss*=2

	test_function "ADD-$mode" "wwt ADD from $mode" \
	    $WWT -qp "$WBFS" ADD "$dest" \
	    || return $ERROR

	[[ $mode = $BASEMODE ]] || rm -f "$dest"
    done

    test_function "CMP" "wit CMP wbfs source" \
	$WIT -ql CMP "$WBFS" "$1" \
	|| return $STAT_DIFF


    #----- test WIT copy

    count=0
    src="$tempdir/image.$BASEMODE"
    for mode in $MODELIST
    do
	dest="$tempdir/copy.$mode"

	test_function "COPY-$mode" "wit COPY to $mode" \
	    $WIT -q COPY "$src" "$dest" --$mode --no-compress \
	    || return $ERROR

	((count++)) && rm -f "$src"
	src="$dest"

	if ((!NOCOMPRESS)) && [[ $mode == wia ]]
	then
	    test_function "COPY-${mode}C" "wit COPY to $mode" \
		$WIT -q COPY "$src" "$dest" --$mode \
		|| return $ERROR

	    ((count++)) && rm -f "$src"
	    src="$dest"
	fi

    done

    test_function "CMP" "wit CMP copied source" \
	$WIT -ql CMP "$dest" "$1" \
	|| return $STAT_DIFF

    rm -f "$dest"


    #----- test WIT extract

    if ((!NOFST))
    then
	src="$tempdir/image.$BASEMODE"
	dest="$tempdir/fst"

	test_function "EXTR-FST0" "wit EXTRACT source" \
	    $WIT -q COPY "$1" "$dest/0" --fst -F :wit $FST_OPT \
	    || return $ERROR

	test_function "EXTR-FST1" "wit EXTRACT copy" \
	    $WIT -q COPY "$src" "$dest/1" --fst -F :wit $FST_OPT \
	    || return $ERROR

	test_function "DIF-FST01" "DIFF fst/0 fst/1" \
	    diff -rq "$dest/0" "$dest/1" \
	    || return $STAT_DIFF

	rm -rf "$dest/0"

	test_function "GEN-ISO" "wit COMPOSE" \
	    $WIT -q COPY "$dest/1" -D "$dest/a.wdf" --wdf --region=file \
	    || return $ERROR

	hss=512
	rm -f "$WBFS"
	test_function "INIT-$hss" "wwt INIT wbfs hss=$hss" \
	    $WWT INIT -q --force --size=20g --hss=$hss "$WBFS" \
	    || return $ERROR

	test_function "ADD-FST" "wwt ADD FST" \
	    $WWT -qp "$WBFS" ADD "$dest/1" --region=file \
	    || return $ERROR

	test_function "CMP" "wit CMP 2x composed" \
	    $WIT -ql CMP "$WBFS" "$dest/a.wdf" \
	    || return $STAT_DIFF

	test_function "EXTR-FST2" "wit EXTRACT" \
	    $WIT -q COPY "$dest/a.wdf" "$dest/2" --fst -F :wit $FST_OPT \
	    || return $ERROR

	#diff -rq "$dest/1" "$dest/2"
	find "$dest" -name tmd.bin -type f -exec rm {} \;
	find "$dest" -name ticket.bin -type f -exec rm {} \;

	test_function "DIF-FST12" "DIFF fst/1 fst/2" \
	    diff -rq "$dest/1" "$dest/2" \
	    || return $STAT_DIFF

	rm -rf "$dest"
    fi

    #----- test WWT pipe

    if ((!NOPIPE))
    then

	hss=1024
	rm -f "$WBFS"
	test_function "INIT-$hss" "wwt INIT wbfs hss=$hss" \
	    $WWT INIT -q --force --size=20g --hss=$hss "$WBFS" \
	    || return $ERROR

	ref="ADD pipe"
	if ! $WDFCAT "$tempdir/image.$BASEMODE" |
	    test_function "ADD-pipe" "wwt ADD $BASEMODE from pipe" \
		$WWT -qp "$WBFS" ADD - --psel=DATA
	then
	    ref="NO-PIPE"
	    return $STAT_OK
	fi

	test_function "CMP" "wit CMP wbfs source" \
	    $WIT -ql CMP "$WBFS" "$1" --psel=DATA \
	    || return $STAT_DIFF

    fi

    #----- all tests done

    ref=-
    return $STAT_OK
}

#
#------------------------------------------------------------------------------
# print header

{
    sep="-----------------------------------"
    printf "\n\f\n%s%s\n\n" "$sep" "$sep"
    date '+%F %T'
    echo
    $WWT --version
    $WIT --version
    $WDFCAT --version
    echo
} | tee -a $log $err

#
#------------------------------------------------------------------------------
# main loop

while (($#))
do
    src="$1"
    shift

    if [[ $src == --fast ]]
    then
	NOVERIFY=1
	NOFST=1
	NOPIPE=1
	MODELIST="$FAST_MODELIST"
	BASEMODE="$FAST_BASEMODE"
	printf "\n## --fast : check only %s, --no-fst --no-pipe\n" "$MODELIST"
	continue
    fi

    if [[ $src == --verify ]]
    then
	NOVERIFY=1
	printf "\n## --verify : verify tests enabled\n"
	continue
    fi

    if [[ $src == --no-verify || $src == --noverify ]]
    then
	NOVERIFY=1
	printf "\n## --no-verify : verify tests disabled\n"
	continue
    fi

    if [[ $src == --fst ]]
    then
	NOFST=0
	printf "\n## --fst : fst tests enabled\n"
	continue
    fi

    if [[ $src == --no-fst || $src == --nofst ]]
    then
	NOFST=1
	printf "\n## --no-fst : fst tests diabled\n"
	continue
    fi

    if [[ $src == --pipe ]]
    then
	NOPIPE=0
	printf "\n## --pipe : pipe tests enabled\n"
	continue
    fi

    if [[ $src == --no-pipe || $src == --nopipe ]]
    then
	NOPIPE=1
	printf "\n## --no-pipe : pipe tests disabled\n"
	continue
    fi

    if [[ $src == --compress ]]
    then
	NOCOMPRESS=0
	printf "\n## --compress : compress WIA tests enabled\n"
	continue
    fi

    if [[ $src == --no-compress || $src == --nocompress ]]
    then
	NOCOMPRESS=1
	printf "\n## --no-compress : WIA compress tests diabled\n"
	continue
    fi

    total_start=$(get_msec)
    test_suite "$src"
    stat=$?
    total_stop=$(get_msec)

    print_stat " * TOTAL TIME:"
    print_timer $total_start $total_stop

    case "$stat" in
	$STAT_OK)	stat="OK" ;;
	$STAT_NOISO)	stat="NO-ISO" ;;
	$STAT_DIFF)	stat="DIFFER" ;;
	*)		stat="ERROR" ;;
    esac

    printf "%s %-6s %-8s %-6s %s\n" $(date +%T) "$stat" "$ref" "$id6" "$src" | tee -a $log

done 2>&1 | tee -a $err

#
#------------------------------------------------------------------------------
# term

rm -rf "$tempdir/"
exit 0

