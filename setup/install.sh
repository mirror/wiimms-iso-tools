#!/usr/bin/env bash

    #####################################################################
    ##                 __            __ _ ___________                  ##
    ##                 \ \          / /| |____   ____|                 ##
    ##                  \ \        / / | |    | |                      ##
    ##                   \ \  /\  / /  | |    | |                      ##
    ##                    \ \/  \/ /   | |    | |                      ##
    ##                     \  /\  /    | |    | |                      ##
    ##                      \/  \/     |_|    |_|                      ##
    ##                                                                 ##
    ##                        Wiimms ISO Tools                         ##
    ##                      http://wit.wiimm.de/                       ##
    ##                                                                 ##
    #####################################################################
    ##                                                                 ##
    ##   This file is part of the WIT project.                         ##
    ##   Visit http://wit.wiimm.de/ for project details and sources.   ##
    ##                                                                 ##
    ##   Copyright (c) 2009-2012 by Dirk Clemens <wiimm@wiimm.de>      ##
    ##                                                                 ##
    #####################################################################
    ##                                                                 ##
    ##   This file installs the distribution.                          ##
    ##                                                                 ##
    #####################################################################


#------------------------------------------------------------------------------

BASE_PATH="@@INSTALL-PATH@@"
BIN_PATH="$BASE_PATH/bin"
SHARE_PATH="@@SHARE-PATH@@"

BIN_FILES="@@BIN-FILES@@"
WDF_LINKS="@@WDF-LINKS@@"
SHARE_FILES="@@SHARE-FILES@@"

INST_FLAGS="-p"

#------------------------------------------------------------------------------

MACHINE="$(uname -m)"
MACHINE="${MACHINE//[!a-zA-Z0-9_+-]/}"

MBIN_PATH="$BIN_PATH-$MACHINE"
if [[ $M32 != 1 && -d $MBIN_PATH ]]
then
    HAVE_MBIN=1
    INPATH_MBIN=0
    echo $PATH | grep -Eq "(^|:)$MBIN_PATH(:|$)" && INPATH_MBIN=1
else
    PATH_MBIN=0
    HAVE_MBIN=0
fi

#------------------------------------------------------------------------------

make=0
if [[ $1 = --make ]]
then
    # it's called from make
    make=1
    shift
fi

#------------------------------------------------------------------------------

echo "*** install binaries to $BIN_PATH"

for f in $BIN_FILES
do
    [[ -f bin/$f ]] || continue

    inst_path="$BIN_PATH"
    if ((HAVE_MBIN))
    then
	((INPATH_MBIN)) || [[ -x $MBIN_PATH/$f ]] && inst_path="$MBIN_PATH"
    fi

    mkdir -p "$inst_path"
    install $INST_FLAGS bin/$f "$inst_path/$f"
done

#------------------------------------------------------------------------------

echo "*** create wdf links"

for f in $WDF_LINKS
do
    [[ -f "$MBIN_PATH/wdf" ]] && ln -f "$MBIN_PATH/wdf" "$MBIN_PATH/$f"
    [[ -f "$BIN_PATH/wdf"  ]] && ln -f "$BIN_PATH/wdf"  "$BIN_PATH/$f"
done

#------------------------------------------------------------------------------

echo "*** install share files to $SHARE_PATH"

for f in $SHARE_FILES
do
    mkdir -p "$SHARE_PATH"
    install $INST_FLAGS -m 644 share/$f "$SHARE_PATH/$f"
done

if [[ -f ./load-titles.sh ]]
then
    mkdir -p "$SHARE_PATH"
    install $INST_FLAGS -m 755 ./load-titles.sh "$SHARE_PATH/load-titles.sh"
fi

#------------------------------------------------------------------------------

((make)) || ./load-titles.sh
