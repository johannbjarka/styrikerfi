#!/bin/bash
problem=$(echo $0 | sed -e 's/^\.\///g' -e 's/[^0-9\.]*//g' -e 's/\.$//g')
subproblem=$(echo $problem | sed 's/^.*\.//g')
descriptionLine=$(sed -n "$subproblem"p problems.txt)
description="Problem $problem:"
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
