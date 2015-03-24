#!/bin/bash
problem=$(echo $0 | sed -e 's/^\.\///g' -e 's/[^0-9\.]*//g' -e 's/\.$//g')
subproblem=$(echo $problem | sed 's/^.*\.//g')
descriptionLine=$(sed -n "$subproblem"p problems.txt)
description="Problem $problem:"
while :; do 
    printf "Question: $descriptionLine"
    echo
    printf "\nValid? (Y/N): "
    read -N 1 answer
    answer=$(echo -n $answer | tr [A-Z] [a-z])
    if [[ $answer =~ (y|n) ]] ; then
	    echo $answer > ../answers/problem$problem.txt
	    printf "\nYour answer: $answer\n"
	    break
    fi
    printf "Invalid option\n"
done
