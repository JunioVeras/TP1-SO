#!/bin/bash
set -eu

# Executes an automate test for all the tests in tests folder.
# Pass the argument TEST=<path to the test file name without extension, e.g.: TEST=./tests/test100>

make TEST=./tests/test100

total=0
ecnt=0

for FILE in tests/*.c; do
    total=$(( $total + 1 ));
    FILE_NAME=$(basename $FILE .c)
    if ! tests/"$FILE_NAME.sh" ; then ecnt=$(( $ecnt + 1 )) ; fi
    # echo $FILE_NAME;
done

echo "your code passes $(( $total - $ecnt )) of $total tests"
rm -f dlist.o dccthread.o
exit 0