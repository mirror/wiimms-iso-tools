#!/bin/bash

dir="$1"
shift

if [[ ! -d $dir ]]
then
    echo "Directory not found: $dir" >&2
    exit 1
fi

for src in "$@"
do
    src="${src##*/}"
    name="${src%.*}"
    name="text_${name//[-.]/_}"
    #echo "$name : $src -> $dest"

    {
	printf "\nconst char %s[] =\n{\n" "$name"
	if [[ $name =~ _cr$ ]]
	then
	    sed 's/\\/\\\\/g; s/"/\\"/g; s/^/  "/; s/$/\\r\\n"/' "$dir/$src"
	else
	    sed 's/\\/\\\\/g; s/"/\\"/g; s/^/  "/; s/$/\\n"/' "$dir/$src"
	fi
	printf "};\n\n"

    } > "$src"
done

