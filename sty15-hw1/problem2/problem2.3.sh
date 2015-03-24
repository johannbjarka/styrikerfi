#!/bin/bash
problem=$(echo $0 | sed -e 's/^\.\///g' -e 's/[^0-9\.]*//g' -e 's/\.$//g')
subproblem=$(echo $problem | sed 's/^.*\.//g')
descriptionLine=$(sed -n "$subproblem"p problems.txt)
description="Problem $problem:"
while :; do 
    printf "$description\n\n"
    cat options.txt
    printf "\nQuestion: $descriptionLine"
    printf "\nChoice: "
    read -N 1 answer
    answer=$(echo $answer | tr '[A-Z]' '[a-z]')
    case $answer in
	[a-z])
	    echo $answer > ../answers/problem$problem.txt
	    printf "\nYour answer: $answer\n"
	    break
	    ;;
	*)
	    printf "\nInvalid option\n"
    esac
done
