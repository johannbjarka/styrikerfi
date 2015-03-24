#!/bin/bash
problem=$(echo $0 | sed -e 's/^\.\///g' -e 's/[^0-9\.]*//g' -e 's/\.$//g')
subproblem=$(echo $problem | sed 's/^.*\.//g')
descriptionLine=$(sed -n "$subproblem"p problems.txt)
description="Problem $problem:"

echo "The program looks as follows."
WORD="`sed -n \"$(whoami |sum |cut -d' ' -f1)p\" /labs/sty15/data/sowpods.txt`"
echo '
void incompetence(char *x) {
  int buf[1];
  strcpy((char *)buf, x);
}

void callfunction() {
  foo("ABCDEFGHI");
}
' |sed "s/ABCDEFGHI/$WORD/g"


while :; do 
    echo -e "$description\n"
    echo -n "$descriptionLine"
    read  answer
    answer=$(echo $answer | tr '[A-Z]' '[a-z]')
    if [[ $answer =~ ^([0-9]|[a-f])+$ ]] ; then
	    echo $answer > ../answers/problem$problem.txt
	    printf "\nYour answer: $answer\n"
	    break
    fi
    printf "Invalid option\n\n"
done
