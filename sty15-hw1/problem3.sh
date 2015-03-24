#!/bin/bash
cd problem$(echo "$0" | sed 's/[^0-9]//g')
for problem in *.sh; do
    ./$problem
    echo
done
