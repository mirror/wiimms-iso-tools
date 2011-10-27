#!/usr/bin/env bash

#set -v

rm -f templates/*.bak templates/*.tmp
mkdir -p doc

list="$*"
[[ $list = "" ]] && list="$(/bin/ls templates/)"

awkprog='
 /@@MODULE\(.*\)@@/ {
	gsub(/@@MODULE\(/,"templates/module/");
	gsub(/\)@@/,"");
	system("cat " $0);
	next
 }

 /@@EXEC\(.*\)@@/ {
	gsub(/@@EXEC\(/,"");
	gsub(/\)@@/,"");
	system($0);
	next
 }

 { print $0 }
'

for fname in $list
do
    src="setup/$fname"
    [[ -f $src ]] || src="templates/$fname"
    if [[ -f "$src" ]]
    then
	dest="doc/$fname"
	ext="${dest##*.}"
	#[[ $ext == edit-list ]] && continue
	[[ $ext = "sh" || $ext = "bat" || $ext = "h" ]] && dest="$fname"
	#echo "|$src|$dest|"
	if [[ $ext = "bat" ]]
	then
	    awk "$awkprog" $src | sed -f templates.sed | sed 's/$/\r/' >$dest
	else
	    awk "$awkprog" $src | sed -f templates.sed  >$dest
	fi
	[[ $ext = "sh" ]] && chmod a+x "$fname"
    fi
done

exit 0
