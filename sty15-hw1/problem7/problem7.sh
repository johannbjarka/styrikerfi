#!/bin/bash
problem=$(echo $0 | sed -e 's/^\.\///g' -e 's/[^0-9\.]*//g' -e 's/\.$//g')
description="Problem $problem"
printf "$description:\n"

cat << EOF
typedef struct {
    short x[N][M]; /* Unknown constants N and M */
    int y;
} str1;

typedef struct {
    char array[M];
    int t;
    short s[M];
    int u;
} str2;

void updateStruct(str1 *a, str2 *b) {
    int v1 = b->t;
    int v2 = b->u;
    a->y = v1 + v2;
}
EOF

if [ ! -e ../answers/problem$problem.assembly.txt ]; then
cat << EOF > ../answers/problem$problem.assembly.txt
Problem 7.

Please comment every assembly line in the IA32 assembly given bellow.
---------- DO NOT CHANGE ANYTHING ABOVE THIS LINE ----------

xor %ebx,%ebx        // 
mov 0xc(%ebp),%edx   // 
mov 0x10(%edx),%eax  // 
add 0x30(%edx),%eax  // 
mov 0x8(%ebp),%edx   // 
mov %eax,0xec(%edx)  // 

EOF
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

problem "N"
problem "M"
