#!/bin/bash

#------------------------------------------------------------------------------

BASE_PATH="@@INSTALL-PATH@@"
BIN_PATH="$BASE_PATH/bin"
SHARE_PATH="$BASE_PATH/share/wit"

BIN_FILES="@@BIN-FILES@@"
WDF_LINKS="@@WDF-LINKS@@"
SHARE_FILES="@@SHARE-FILES@@"

INST_FLAGS="-p"

#------------------------------------------------------------------------------

make=0
if [[ $1 = --make ]]
then
    # it's called from make
    make=1
    shift
fi

#------------------------------------------------------------------------------

mkdir -p "$BIN_PATH" "$SHARE_PATH"

echo "*** install binaries to $BIN_PATH"

for f in $BIN_FILES
do
    install $INST_FLAGS bin/$f "$BIN_PATH/$f"
done

for f in $WDF_LINKS
do
    ln -f "$BIN_PATH/wdf" "$BIN_PATH/$f"
done

echo "*** install share files to $SHARE_PATH"

for f in $SHARE_FILES
do
    install $INST_FLAGS share/$f "$SHARE_PATH/$f"
done

#------------------------------------------------------------------------------

((make)) || ./load-titles.sh
