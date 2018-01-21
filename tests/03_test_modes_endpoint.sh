#!/usr/bin/env bash

function runtests() {
    echo -n "Testing /modes endpoint $TEST_ROUNDS times to test the modes table page for errors"

    for i in $( seq ${TEST_ROUNDS} ) ; do
        COMMAND="curl -v --max-time 3 http://$IP_ADDRESS/modes 2>&1"
        OUTPUT="$( eval ${COMMAND} )"

        if [ "$?" != 0 ] ; then
            echo -e "\n\nFailed $COMMAND: \n $OUTPUT\n\n"
            return 1
        fi
    done

    echo -e "  [OK]\n"
    echo
}
