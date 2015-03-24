#!/bin/bash
./setup.sh
echo
for problem in problem*.sh; do
    ./$problem
    echo
done
