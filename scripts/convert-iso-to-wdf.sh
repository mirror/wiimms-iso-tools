#!/bin/bash

#-----------------------------------------------------------------------------
# setup

script="${0##*/}"
log="${script%.sh}.log"

#-----------------------------------------------------------------------------
# print help

function help_exit()
{
    cat <<- ---EOT---

	$script [option...] source-file... source-dir... 

	This script converts all ISO-Images into WDF files. After generating it
	compares the output against the source. The output file will be removed if
	this check failes. For each file a status line is written to stdout and
	appended to the file '$log' in the current directory.

	  Options:

	   -h  --help         : Print this help to stdout and 'exit 1'

	   -r  --relative dir : Define destination dir relative to each source file
	   -d  --dest     dir : Define destination dirtogether for all files.
	                         -> default: directory of source file

	   -s  --scrub        : Scrub the file (add to and extract from WBFS).
	                        Without this option the ISO image will be converted
	                        with 'iso2wdf' without scrubbing.

	                        For discs <2GB scrubbing is faster.

	       --remove       : Remove if the final check (comparing output and source)
	                        was successful.

	   -q  --quiet        : Print only error and fail messages to stdout.
	   -v  --verbose      : Print a message for each action
	                        Default: Prnt a stausline fpr each converted file.

	   -t  --test         : Tes mode: do nothing, just tell what is planned.

         Sources:
         
	   Each file is subject for a conversion. Directories will be searches
	   recursive for files that might be ISO images.

	---EOT---

    exit 1
}

#-----------------------------------------------------------------------------
# analyze args

args=`getopt -n "$script" \
	-l help,relative:,dest:,scrub,remove,quiet,verbose,test,debug \
	-o hr:d:sqvtD \
	-- "$@"`
if (( $? )); then
    echo "$script: try option -h or --help for more help" 1>&2
    exit 1
fi

eval set -- $args

#------------------------------------------------------------------------------
# check tool existence

errtool=
for tool in wwt iso2wdf wdf-cat cmp
do
    which $tool >/dev/null 2>&1 || errtool="$errtool $tool"
done

if [[ $errtool != "" ]]
then
    echo "$myname: missing tools in PATH:$errtool" >&2
    exit 2
fi

#-----------------------------------------------------------------------------
# scan options

abs_path=""
rel_path=""
scrub=0
remove=0
quiet=0
verbose=0
testmode=0
debug=0

scan_opt=1

while (( $# && scan_opt ))
do

    case "$1" in
	--)		scan_opt=0 ;;
	-h|--help)	help_exit ;;
	-r|--relative)	shift; abs_path=""; rel_path="$1" ;;
	-d|--dest)	shift; abs_path="$1"; rel_path="" ;;
	-s|--scrub)	scrub=1 ;;
	   --remove)	remove=1 ;;
	-q|--quiet)	quiet=1; verbose=0 ;;
	-v|--verbose)	quiet=0; verbose=1 ;;
	-t|--test)	testmode=1 ;;
	-D|--debug)	debug=1 ;;

	*) echo "$script: INTERNAL ERROR 'option': $1" 1>&2; exit 2
    esac
    shift
done


#-----------------------------------------------------------------------------
# analyze options

if [[ ${rel_path:0:1} = "/" ]]
then
    abs_path="$rel_path"
    rel_path=""
elif [[ $rel_path != "" ]]
then
    rel_path="/$rel_path"
fi

if ((debug))
then
    printf "## ABS-PATH: %s\n" "$abs_path"
    printf "## REL-PATH: %s\n" "$rel_path"
fi

FILTER=cat
((quiet)) && FILTER="grep -vE '^([^ ]|OK|REMOVED)'"

#-----------------------------------------------------------------------------
# functions

function STATUS()
{
    local stat="$1"
    shift
    printf "%-7s %s\n" "$stat" "$*"
}

function TRACE()
{
    ((verbose)) && printf "        * %s\n" "$*"
}

function JOB()
{
    if ((testmode))
    then
	printf "          - %s\n" "$*"
    else
	"$@"
    fi
}

#-----------------------------------------------------------------------------
# scan parameters

(($#)) || help_exit

for source in "$@"
do
    if [[ -d $source ]]
    then
	dirmode=1
	((verbose)) && printf "\n\f\n--- %s : scan directory: %s\n\n" "$(date '+%F %T')" "$source"
    elif [[ -f $source ]]
    then
	dirmode=0
    else
	STATUS NO-FILE "Not a file: $source"
	continue
    fi
    
    find "$source" -type f -print0 | while read -d "" src
    do
	ft=$(wwt ft "$src" | awk '{print $1}')
	if [[ $ft != ISO ]]
	then
	    ((dirmode)) || STATUS NO-ISO "Not a ISO image: $src"
	    continue
	fi

	id6=$(wwt ft --long "$src" | awk '{print $2}')
	srcname="${src##*/}"
	srcdir="${src%/*}"
	[[ $srcname = "$src" ]] && srcdir=.
	src="$srcdir/$srcname"
	destname="${srcname%.iso}"
	destname="${destname%.ISO}.wdf"
	if [[ $abs_path != "" ]]
	then
	    destdir="$abs_path"
	else
	    destdir="$srcdir$rel_path"
	fi
	dest="$destdir/$destname"

	if ((debug))
	then
	    printf "## SOURCE: %s\n" "$src"
	    printf "## DEST:   %s\n" "$dest"
	fi

	if [[ -a "$dest" ]]
	then
	    STATUS EXISTS "File already exists: $dest"
	    continue
	fi

	if ((scrub))
	then
	    wbfs="$destdir/a.wbfs.tmp"

	    TRACE "create WBFS:  $wbfs"
	    JOB mkdir -p "$destdir"
	    JOB wwt -q init "$wbfs" --force --size=12G

	    TRACE "add [$id6]: $src"
	    JOB wwt -qp "$wbfs" add "$src"

	    TRACE "ext [$id6]: $dest"
	    JOB wwt -qp "$wbfs" extract --wdf "$id6=$dest"

	    TRACE "remove WBFS:  $wbfs"
	    JOB rm -f "$wbfs"

	else
	    TRACE "convert $srcname -> $dest"
	    JOB mkdir -p "$destdir"
	    if ((testmode))
	    then
		JOB iso2wdf -q - "<$src" ">$dest"
	    else
		JOB iso2wdf -q - < "$src" > "$dest"
	    fi
	fi

	if ((testmode))
	then
	    STATUS TEST "$dest"
	    if ((remove))
	    then
		TRACE "may remove $src"
		JOB rm -f "$src"
	    fi
	else
	    TRACE "compare src + $dest "
	    if wdf-cat "$dest" | cmp --quiet "$src"
	    then
		STATUS OK "$dest"

		if ((remove))
		then
		    TRACE "remove $src"
		    JOB rm -f "$src"
		    STATUS REMOVED "$src"
		fi
	    else
		STATUS FAILED "$dest"

		TRACE "remove $dest"
		JOB rm -f "$dest"
		STATUS REMOVED "$dest"
	    fi
	fi
    done
done | tee -a "$log" | $FILTER

#-----------------------------------------------------------------------------
# end

