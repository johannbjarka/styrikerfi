#!/bin/bash
problem=$(echo $0 | sed -e 's/^\.\///g' -e 's/[^0-9\.]*//g' -e 's/\.$//g')
description="Problem $problem"
printf "$description:\n"

cat << EOF
int matrix1[M][N];
int matrix2[N][M];

int copyOne(int i, int j)
{
    matrix1[i][j] = matrix2[j][i];
}
EOF

if [ ! -e ../answers/problem$problem.assembly.txt ]; then
    cp template.txt ../answers/problem$problem.assembly.txt
fi

EDITOR_FILE="../answers/editor.txt"
EDITOR=$(if [ -f "$EDITOR_FILE" ]; then cat $EDITOR_FILE; fi)
export EDITOR=${EDITOR:-"nano"}
$EDITOR ../answers/problem$problem.assembly.txt

cat ../answers/problem$problem.assembly.txt | sed '0,/---------- DO NOT CHANGE ANYTHING ABOVE THIS LINE ----------/d'

function problem()
{
    local letter=$1
    local answer 
    while :; do
	if [ -n $answer ]; then
	    printf "Enter $letter: "
	    read answer
	    if [[ $answer =~ ^[0-9]+$ ]]; then
		echo $answer > ../answers/problem$problem.$letter.txt
		printf "Your answer: $answer\n\n"
		break
	    fi
	    printf "Invalid option\n"
	fi
    done
}

problem "M"
problem "N"
