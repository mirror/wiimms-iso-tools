#!/bin/bash

myname="${0##*/}"
log="${myname%.sh}.log"
err="${myname%.sh}.err"

obj=./.${myname}.obj.tmp

if [[ $# == 0 ]]
then
    cat <<- ---EOT---

	$myname : Test the tools 'iso2wdf', 'wdf2iso' and 'wdf-cat'

	This script expect as parameters names of files or directories. All files
	within his directory (recurse enabled) are subject for an WDF test.

	The WDF test does the following:
	  1.) 'iso2wdf' converts the source file to a WDF file.
	  2.) 'wdf2iso' converts the WDF file back to a normal file.
	  3.) Compare the created normal file against the source file.
	  4.) Compare the output of 'wdf-cat' against the source file.
	The 4 steps will be done in file mode using open() and again in stream
	mode using fopen().

	While doing the tests 3 temporary files will be created and removed in
	the current working directory:
	    $obj $obj.wdf $obj.iso

	For each file one status line is *appended* at the end of the
	file '$log' in the following format:
	    STATUS '<TAB>' filename
	STATUS is one of:
	    'OK'      : source and both destination files are identical.
	    'NOFILE'  : source file not found
	    'CHANGED' : source file changed during test.
	    'ERROR'   : source and at least one destination file are different.
	Not 'OK' files are also reported to stdout. Error messages are *appended*
	to the file '$err'.
	
	---EOT---

    echo "usage: $myname [-s|--stream] dir_or_file..." >&2
    echo
    exit 1
fi

#------------------------------------------------------------------------------

ISO2WDF=iso2wdf
WDF2ISO=wdf2iso
WDFCAT=wdf-cat
[[ -x ./iso2wdf ]] && ISO2WDF=./iso2wdf
[[ -x ./wdf2iso ]] && WDF2ISO=./wdf2iso
[[ -x ./wdf-cat ]] && WDFCAT=./wdf-cat

err=
for tool in $ISO2WDF $WDF2ISO $WDFCAT cmp find
do
    which $tool >/dev/null 2>&1 || err="$err $tool"
done

if [[ $err != "" ]]
then
    echo "$myname: missing tools in PATH:$err" >&2
    exit 2
fi

#------------------------------------------------------------------------------

psel=raw

for src in "$@"
do
    printf "\n\f\n--- %s : %s\n\n" "$(date '+%F %T')" "$src"
    ((stream)) && echo " - Stream mode enabled"

    find "$src" -type f -print0 | while read -d "" file
    do
	rm -f "$obj".[FS] "$obj".[FS].iso "$obj".[FS].wdf
	ln -s "$file" "$obj.F"
	ln -s "$file" "$obj.S"
	if [[ -f "$obj.F" && -f "$obj.S" ]]
	then
	    filestat="$(stat -L -c '%y/%z/%s' "$file")"
	    $ISO2WDF -q --io=0 "$obj.F" --psel=$psel
	    $ISO2WDF -q --io=3 "$obj.S" --psel=$psel
	    $WDF2ISO -q --io=0 "$obj.F.wdf" --psel=$psel
	    $WDF2ISO -q --io=3 "$obj.S.wdf" --psel=$psel
	    if     cmp -s "$obj.F" "$obj.F.iso" \
		&& cmp -s "$obj.S.wdf" "$obj.F.wdf" \
		&& wdf-cat --io=0 $opt_io "$obj.F.wdf" | cmp -s "$obj.F.iso" \
		&& wdf-cat --io=3 $opt_io "$obj.S.wdf" | cmp -s "$obj.S.iso"
	    then
		stat=OK
	    elif [[ $filestat != "$(stat -L -c '%y/%z/%s' "$file")" ]]
	    then
		stat=CHANGED
	    else
		stat=ERROR
	    fi
	else
	    stat=NOFILE
	fi
	printf "%s\t%s\n" "$stat" "$file"
    done
done | tee -a $log 2>&1 | awk '/^OK/ {next} {print;fflush()}' | tee -a $err

rm -f "$obj".[FS] "$obj".[FS].iso "$obj".[FS].wdf

