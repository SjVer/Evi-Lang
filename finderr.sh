#!/bin/bash
# finds location of internal error using error ID

# check for argument
if [ -z "$1" ]; then
	echo "No error ID given!";
	exit 1;
fi

# check if id is valid
if ! [[ "$1" =~ ^([A-Z][A-Z])([0-9]+)([A-Z])$ ]]; then
	echo "Invalid error ID given!";
	exit 1;
fi

id="$1";
echo "Error ID: $id";

name_start="${BASH_REMATCH[1]}";
echo " - File ID: $name_start";

line="${BASH_REMATCH[2]}";
echo " - Line: $line";

ext_start="${BASH_REMATCH[3]}";
echo " - File Type ID: $ext_start";

echo;

# find potentional files
IFS='
'
files=($(find ./src ./include -iname ${name_start,,}*.${ext_start,,}*))
IFS=' '

# if file doesnt have enough lines leave it out
to_discard=()
for (( i = 0; i < ${#files[@]}; i++ )); do
	f=${files[$i]};
	l=$(wc -l < "$f")
	if (( $l == 0 )) || (( $l < $line )); then
		echo "File $f discarded (too short)";
		to_discard+=( $i );
	fi
done
for i in "${to_discard[@]}"; do unset 'files[$i]'; done

if ! [ -z "${to_discard[@]}" ]; then echo; fi

# check if we found files
if [ -z "$files" ]; then
	echo "Could not find file!";
	exit 1;
fi

# if its just one file, choose that
if [ ${#files[@]} -eq "1" ]; then
	file=${files[0]};
else
	echo "Files found:"
	i=1
	for f in "${files[@]}"; do
		found=$(echo "$(sed -n $((line    ))p "$f")" | grep "INTERNAL_ERROR");
		foundmsg=$([ -z "$found" ] && echo "" || echo "(found \"INTERNAL_ERROR\")");
		printf " %d: %s %s\n" $i $f "$foundmsg";
		i=$((i + 1));
	done

	printf "Enter correct file number: "
    read index
	while ! ((index >= 1 && index <= ${#files[@]})) 2> /dev/null; do
	    echo "Integers from 1 to ${#files[@]} only!"
		printf "Enter correct file number: "
	    read index
	done

	file=${files[(($index - 1))]}
	echo;
fi


echo "File: $file";

max=$((line + 2));
l=${#max};

echo "Source:"
printf "    %${l}d│ %s\n" $((line - 2)) "$(sed -n $((line - 2))p "$file")";
printf "    %${l}d│ %s\n" $((line - 1)) "$(sed -n $((line - 1))p "$file")";
printf " -> %${l}d│ %s\n" $((line    )) "$(sed -n $((line    ))p "$file")";
printf "    %${l}d│ %s\n" $((line + 1)) "$(sed -n $((line + 1))p "$file")";
printf "    %${l}d│ %s\n" $((line + 2)) "$(sed -n $((line + 2))p "$file")";