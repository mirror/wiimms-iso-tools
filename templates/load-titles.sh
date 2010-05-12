#!/bin/bash

NEEDED="wwt wget comm"

BASE_PATH="@@INSTALL-PATH@@"
LIB_PATH="$BASE_PATH/share/wwt"
URI_TITLES=@@URI-TITLES@@
LANGUAGES="@@LANGUAGES@@"

#------------------------------------------------------------------------------

function load_and_store()
{
    local URI="$1"
    local DEST="$2"
    local ADD="$3"

    echo "***    load $DEST from $URI"

    if wget -q -O- "$URI" | wwt titles / - >"$DEST.tmp" && test -s "$DEST.tmp"
    then
	if [[ $ADD != "" ]]
	then
	    wwt titles / "$ADD" "$DEST.tmp" >"$DEST.tmp.2"
	    mv "$DEST.tmp.2" "$DEST.tmp"
	fi
	grep -v ^TITLES "$DEST"     >"$DEST.tmp.1"
	grep -v ^TITLES "$DEST.tmp" >"$DEST.tmp.2"
	if ! diff -q "$DEST.tmp.1" "$DEST.tmp.2" >/dev/null
	then
	    echo "            => content changed!"
	    mv "$DEST.tmp" "$DEST"
	fi
    fi
    rm -f "$DEST.tmp" "$DEST.tmp.1" "$DEST.tmp.2"
}

#------------------------------------------------------------------------------

make=0
if [[ $1 = --make ]]
then
    # it's called from make
    make=1
    shift
fi

#------------------------------------------------------------------------------

errtool=
for tool in $NEEDED
do
    which $tool >/dev/null 2>&1 || errtool="$errtool $tool"
done

if [[ $errtool != "" ]]
then
    echo "missing tools in PATH:$errtool" >&2
    exit 2
fi

#------------------------------------------------------------------------------

mkdir -p "$LIB_PATH" lib

load_and_store "$URI_TITLES" "lib/titles.txt"

# load language specifig title files

for lang in $LANGUAGES
do
    LANG="$( echo $lang | awk '{print toupper($0)}' )"
    load_and_store $URI_TITLES?LANG=$LANG "lib/titles-$lang.txt" lib/titles.txt
done

if ((!make))
then
    echo "*** install titles to $LIB_PATH"
    mkdir -p "$LIB_PATH"
    cp -p lib/titles*.txt "$LIB_PATH"
fi

