#!/bin/bash
# (c) Wiimm, 2010-09-21

myname="${0##*/}"
base=image-size
log=$base.log
err=$base.err

WIT=wit
[[ -x ./wit ]] && WIT=./wit

WDF=wdf
[[ -x ./wdf ]] && WDF=./wdf

export LC_ALL=C
METHODS="$(echo $($WIT compr))"
PSEL=
IO=
LEVEL=.0

if [[ $# == 0 ]]
then
    cat <<- ---EOT---

	This script expect as parameters names of ISO files. ISO files are PLAIN,
	WDF, CISO, WBFS or FST. Each source file is subject of this test suite.

	Each source is converted to several formats to find out the size of the
	output. Output formats are: WDF, PLAIN ISO, CISO, WBFS, WIA.
	Some output images are additonally compressed with bzip2, rar and 7z.

	Usage:  $myname [option]... iso_file...

	Options:
	  --fast       : Enter fast mode => do only WDF and WIA
	  
	  --bzip2      : enable bzip2 tests (default if tool 'bzip2' found)
	  --no-bzip2   : disable bzip2 tests
	  --rar        : enable rar tests (default if tool 'rar' found)
	  --no-rar     : disable rar tests
	  --7zip       : enable 7zip tests (default if tool '7z' found)
	  --no-7zip    : disable 7zip tests
	  --all        : shortcut for --bzip2 --rar --7z

	  --data       : DATA partition only, scrub all others
	  --compr list : Define a list of WIA compressing methods.
	                 Default: $METHODS
	  --level list : Test compression levels and chunk size factors.
	                 Syntax: [.level][@factors] ...

	  --decrypt    : Enable WDF/DECRYPT tests.
	  --diff       : Enable "wit DIFF" to verify WIA archives.
	  --dump       : 'wdf +dump a.wia >$dumpdir'

	---EOT---
    exit 1
fi

#
#------------------------------------------------------------------------------
# check existence of needed tools

errtool=
for tool in $WIT
do
    which $tool >/dev/null 2>&1 || errtool="$errtool $tool"
done

if [[ $errtool != "" ]]
then
    echo "$myname: missing tools in PATH:$errtool" >&2
    exit 2
fi

let HAVE_BZIP2=!$(which bzip2 >/dev/null 2>&1; echo $?)
let HAVE_RAR=!$(which rar >/dev/null 2>&1; echo $?)
let HAVE_7Z=!$(which 7z >/dev/null 2>&1; echo $?)

OPT_FAST=0
OPT_BZIP2=0
OPT_RAR=0
OPT_7ZIP=0
OPT_DECRYPT=0
OPT_DIFF=0
OPT_DUMP=0

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
    last_time=$tim
    let ms=tim%1000
    let tim=tim/1000
    let s=tim%60
    let tim=tim/60
    let m=tim%60
    let h=tim/60
    if ((h))
    then
	last_ftime=$(printf "%u:%02u:%02u.%03u hms" $h $m $s $ms)
    elif ((m))
    then
	last_ftime=$(printf "  %2u:%02u.%03u m:s" $m $s $ms)
    else
	last_ftime=$(printf "     %2u.%03u sec" $s $ms)
    fi
    printf "%s  %s\n" "$last_ftime" "$3"
}

#-----------------------------------

function print_stat()
{
    printf "%-26s : " "$(printf "$@")"
}

#
#------------------------------------------------------------------------------
# calc real methods

function calc_real_methods()
{
    REALMETHODS=
    for method in $METHODS
    do	
	for level in $LEVEL
	do
	    local mode="$($WIT compr $method$level)"
	    if [[ $mode == - ]]
	    then
		echo "Illegal compression mode: $method$level" >&2
		exit 1
	    fi
	    REALMETHODS="$REALMETHODS $mode"
	done
    done
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

dumpdir=./dumpdir.tmp
mkdir -p $dumpdir

export WIT_OPT=
export WWT_OPT=

#
#------------------------------------------------------------------------------
# test function

function test_function()
{
    # $1 = base time
    # $2 = dest file
    # $3 = info
    # $4... = command

    local basetime dest start stop size perc

    basetime="$1"
    dest="$2"
    info="$3"
    shift 3
    print_stat " > %s" "$info"
    #-----
    sync
    sleep 1
    start=$(get_msec)
    #echo; echo "$@"
    "$@" || return 1
    sync
    let stop=$(get_msec)+basetime
    #-----
    size="$(stat -c%s "$tempdir/$dest")"
    ((basesize)) || basesize=$size
    let perc=size*10000/basesize
    if ((perc>99999))
    then
	perc="$(printf "%6u%%" $((perc/100)))"
    else
	perc="$(printf "%3u.%02u%%" $((perc/100)) $((perc%100)))"
    fi
    size="$(printf "%10d %s" $size "$perc" )"
    print_timer $start $stop "$size"
    printf "  %s %s  %s\n" "$size" "$last_ftime" "$info" >>"$tempdir/time.log"
    return 0
}

#
#------------------------------------------------------------------------------
# test suite

function test_suite()
{
    # $1 = valid source

    rm -rf "$tempdir"/*
    mkdir -p "$tempdir"

    local ft id6 name

    ft="$($WIT FILETYPE -H -l "$1" | awk '{print $1}')" || return 1
    id6="$($WIT FILETYPE -H -l "$1" | awk '{print $2}')" || return 1

    printf "\n** %s %s %s\n" "$ft" "$id6" "$1"
    [[ $id6 = "-" ]] && return $STAT_NOISO

    name="$($WIT ID6 --long --source "$1")"
    name="$( echo "$name" | sed 's/  /, /')"


    #----- START, read source once

    basesize=0
    test_function 0 b.wdf "WDF/start" \
        $WIT_CP "$1" --wdf "$tempdir/b.wdf" || return 1
    rm -f "$tempdir/time.log"


    #----- wdf

    basesize=0

    test_function 0 a.wdf "WDF" \
	$WIT_CP "$1" --wdf "$tempdir/a.wdf" || return 1
    local wdf_time=$last_time

    if ((OPT_BZIP2))
    then
	test_function $wdf_time a.wdf.bz2 "WDF + BZIP2" \
	    bzip2 --keep "$tempdir/a.wdf" || return 1
    fi

    if ((OPT_RAR))
    then
	test_function $wdf_time a.wdf.rar "WDF + RAR" \
	    rar a -inul "$tempdir/a.wdf.rar" "$tempdir/a.wdf" || return 1
    fi

    if ((OPT_7ZIP))
    then
	test_function $wdf_time a.wdf.7z "WDF + 7Z" \
	    7z a -bd "$tempdir/a.wdf.7z" "$tempdir/a.wdf" || return 1
    fi

    rm -f "$tempdir/a.wdf"*


    #----- wdf/decrypt
 
    if ((OPT_DECRYPT))
    then   
	test_function 0 a.wdf "WDF/DECRYPT" \
	    $WIT_CP "$1" --wdf --enc decrypt "$tempdir/a.wdf" || return 1
	local wdf_time=$last_time

	if ((OPT_BZIP2))
	then
	    test_function $wdf_time a.wdf.bz2 "WDF/DECRYPT + BZIP2" \
		bzip2 --keep "$tempdir/a.wdf" || return 1
	fi

	if ((OPT_RAR))
	then
	    test_function $wdf_time a.wdf.rar "WDF/DECRYPT + RAR" \
		rar a -inul "$tempdir/a.wdf.rar" "$tempdir/a.wdf" || return 1
	fi

	if ((OPT_7ZIP))
	then
	    test_function $wdf_time a.wdf.7z "WDF/DECRYPT + 7Z" \
		7z a -bd "$tempdir/a.wdf.7z" "$tempdir/a.wdf" || return 1
	fi

	rm -f "$tempdir/a.wdf"*
    fi

    #----- wia

    for method in $REALMETHODS
    do
	test_function 0 a.wia "WIA/$method" \
	    $WIT_CP "$1" --wia --compr $method "$tempdir/a.wia" || return 1
	local wdf_time=$last_time

	((OPT_DUMP)) && $WDF +dump "$tempdir/a.wia" >"$dumpdir/$id6-$method.dump"

	if [[ ${method:0:4}  == NONE ]]
	then
	    if ((OPT_BZIP2))
	    then
		test_function $wdf_time a.wia.bz2 "WIA/$method + BZIP2" \
		    bzip2 --keep "$tempdir/a.wia" || return 1
	    fi

	    if ((OPT_RAR))
	    then
		test_function $wdf_time a.wia.rar "WIA/$method + RAR" \
		    rar a -inul "$tempdir/a.wia.rar" "$tempdir/a.wia" || return 1
	    fi

	    if ((OPT_7ZIP))
	    then
		test_function $wdf_time a.wia.7z "WIA/$method + 7Z" \
		    7z a -bd "$tempdir/a.wia.7z" "$tempdir/a.wia" || return 1
	    fi
	fi

	if ((OPT_DIFF))
	then
	    echo " - wit DIFF orig-source a.wia"
	    wit diff $PSEL "$1" "$tempdir/a.wia" \
		|| echo "!!! $id6: wit DIFF orig-source a.wia/$method FAILED!" | tee -a "$log"
	fi

	rm -f "$tempdir/a.wia"*
    done


    #----- plain iso

    if ((!OPT_FAST))
    then
	test_function 0 a.iso "PLAIN ISO" \
	    $WIT_CP "$1" --iso "$tempdir/a.iso" || return 1
	local iso_time=$last_time

	if ((OPT_BZIP2))
	then
	    test_function iso_time a.iso.bz2 "PLAIN ISO + BZIP2" \
		bzip2 --keep "$tempdir/a.iso" || return 1
	fi

	if ((OPT_RAR))
	then
	    test_function iso_time a.iso.rar "PLAIN ISO + RAR" \
		rar a -inul "$tempdir/a.iso.rar" "$tempdir/a.iso" || return 1
	fi

	if ((OPT_7ZIP))
	then
	    test_function iso_time a.iso.7z "PLAIN ISO + 7Z" \
		7z a -bd "$tempdir/a.iso.7z" "$tempdir/a.iso" || return 1
	fi

	rm -f "$tempdir/a.iso"*
    fi


    #----- ciso

    if ((!OPT_FAST))
    then
	test_function 0 a.ciso "CISO" \
	    $WIT_CP "$1" --ciso "$tempdir/a.ciso" || return 1

	rm -f "$tempdir/a.ciso"*
    fi


    #----- wbfs

    if ((!OPT_FAST))
    then
	test_function 0 a.wbfs "WBFS" \
	    $WIT_CP "$1" --wbfs "$tempdir/a.wbfs" || return 1

	rm -f "$tempdir/a.wbfs"*
    fi


    #----- summary

    {
	printf "\nSummary of %s:\n\n" "$name"
	sort "$tempdir/time.log"
	printf "\n"
    
    } | tee -a "$log"
    
    return 0
}

#
#------------------------------------------------------------------------------
# print header

{
    sep="-----------------------------------"
    printf "\n\f\n%s%s\n\n" "$sep" "$sep"
    date '+%F %T'
    echo
    $WIT --version
    echo
    echo "PARAM: $*"
    echo
} | tee -a $log $err

#
#------------------------------------------------------------------------------
# main loop

opts=1
calc_real_methods

while (($#))
do
    src="$1"
    shift

    if [[ $src == --fast ]]
    then
	OPT_FAST=1
	((opts++)) || printf "\n"
	printf "## --fast : Do only WDF and WIA tests\n"
	continue
    fi

    if [[ $src == --bzip2 || $src == --bz2 ]]
    then
	OPT_BZIP2=$HAVE_BZIP2
	((opts++)) || printf "\n"
	printf "## --bzip2 : bzip2 tests enabled\n"
	continue
    fi

    if [[ $src == --no-bzip2 || $src == --nobzip2 || $src == --no-bz2 || $src == --nobz2 ]]
    then
	OPT_BZIP2=0
	((opts++)) || printf "\n"
	printf "## --no-bzip2 : bzip2 tests disabled\n"
	continue
    fi

    if [[ $src == --rar ]]
    then
	OPT_RAR=$HAVE_RAR
	((opts++)) || printf "\n"
	printf "## --rar : rar tests enabled\n"
	continue
    fi

    if [[ $src == --no-rar || $src == --norar ]]
    then
	OPT_RAR=0
	((opts++)) || printf "\n"
	printf "## --no-rar : rar tests disabled\n"
	continue
    fi

    if [[ $src == --7zip || $src == --7z ]]
    then
	OPT_7ZIP=$HAVE_7Z
	((opts++)) || printf "\n"
	printf "## --7zip : 7zip tests enabled\n"
	continue
    fi

    if [[ $src == --no-7zip || $src == --no7zip || $src == --no-7z || $src == --no7z ]]
    then
	OPT_7ZIP=0
	((opts++)) || printf "\n"
	printf "## --no-7zip : 7zip tests disabled\n"
	continue
    fi

    if [[ $src == --all ]]
    then
	OPT_BZIP2=$HAVE_BZIP2
	OPT_RAR=$HAVE_RAR
	OPT_7ZIP=$HAVE_7Z
	((opts++)) || printf "\n"
	printf "## --all : shortcut for --bzip2 --rar --7z\n"
	continue
    fi

    if [[ $src == --decrypt ]]
    then
	OPT_DECRYPT=1
	((opts++)) || printf "\n"
	printf "## --decrypt : WDF/DECRYPT tests enabled\n"
	continue
    fi

    if [[ $src == --diff ]]
    then
	OPT_DIFF=1
	((opts++)) || printf "\n"
	printf "## --diff : 'wit DIFF' tests enabled\n"
	continue
    fi

    if [[ $src == --dump ]]
    then
	OPT_DUMP=1
	((opts++)) || printf "\n"
	printf "## --dump : 'wdf +dump a.wia >$dumpdir'\n"
	continue
    fi

    if [[ $src == --data ]]
    then
	PSEL="--psel data"
	((opts++)) || printf "\n"
	printf "## --data : DATA partition only, scrub all others\n"
	continue
    fi

    if [[ $src == --compr ]]
    then
	METHODS="$( echo "$1" | tr 'a-z,' 'A-Z ')"
	shift
	((opts++)) || printf "\n"
	printf "## --compr : WIA compression methods set to: $METHODS\n"
	calc_real_methods
	printf "## compr+level expanded to: $REALMETHODS\n"
	continue
    fi

    if [[ $src == --level ]]
    then
	LEVEL="$( echo "$1" | tr ',' ' ' | tr -cd '0-9@. ' )"
	shift
	((opts++)) || printf "\n"
	printf "## --level : WIA compression level set to: $LEVEL\n"
	calc_real_methods
	printf "## compr+level expanded to: $REALMETHODS\n"
	continue
    fi

    #---- hidden options

    if [[ $src == --io ]]
    then
	IO="--io $( echo "$1" | tr -cd '0-9' )"
	shift
	((opts++)) || printf "\n"
	printf "## Define option : $IO\n"
	continue
    fi

    opts=0
    WIT_CP="$WIT COPY $PSEL $IO -q"

    total_start=$(get_msec)
    test_suite "$src"
    stat=$?
    total_stop=$(get_msec)

    print_stat " * TOTAL TIME:"
    print_timer $total_start $total_stop

done 2>&1 | tee -a $err

#
#------------------------------------------------------------------------------
# term

rm -rf "$tempdir/"
exit 0

