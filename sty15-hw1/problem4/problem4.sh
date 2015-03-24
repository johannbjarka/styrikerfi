#!/bin/bash
problem=$(echo $0 | sed -e 's/^\.\///g' -e 's/[^0-9\.]*//g' -e 's/\.$//g')
EDITOR_FILE="../answers/editor.txt"
EDITOR=$(if [ -f "$EDITOR_FILE" ]; then cat $EDITOR_FILE; fi)
export EDITOR=${EDITOR:-"nano"}

if [ ! -e "../answers/problem$problem.txt" ]; then
    cp template.txt ../answers/problem$problem.txt

fi
$EDITOR ../answers/problem$problem.txt
