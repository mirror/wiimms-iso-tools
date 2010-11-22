#!/bin/bash

#------------------------------------------------------------------------------

BASE_PATH="@@INSTALL-PATH@@"
BIN_PATH="$BASE_PATH/bin"
LIB_PATH="$BASE_PATH/share/wit"
OLD_LIB_PATH="$BASE_PATH/share/wwt"

BIN_FILES="@@BIN-FILES@@"
WDF_LINKS="@@WDF-LINKS@@"
LIB_FILES="@@LIB-FILES@@"

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

[[ -d $OLD_LIB_PATH/ && ! -a $LIB_PATH ]] && mv "$OLD_LIB_PATH" "$LIB_PATH"
mkdir -p "$BIN_PATH" "$LIB_PATH"

echo "*** install binaries to $BIN_PATH"

for f in $BIN_FILES
do
    install $INST_FLAGS bin/$f "$BIN_PATH/$f"
done

for f in $WDF_LINKS
do
    ln -f "$BIN_PATH/wdf" "$BIN_PATH/$f"
done

echo "*** install lib files to $LIB_PATH"

for f in $LIB_FILES
do
    install $INST_FLAGS lib/$f "$LIB_PATH/$f"
done

#------------------------------------------------------------------------------

((make)) || ./load-titles.sh
