#!/bin/bash
printf "Which is your prefered editor?\n"
select answer in nano vim emacs other; do
    case $answer in
	nano)
	    echo $answer > answers/editor.txt
	    break
	    ;;
	vim)
	    echo $answer > answers/editor.txt
	    break
	    ;;
	emacs)
	    echo $answer > answers/editor.txt
	    break
	    ;;
	other)
	    printf "Enter name of editor: "
	    read answer
	    echo $answer > answers/editor.txt
	    break
	    ;;
	*)
	    printf "Invalid option\n"
    esac
done
