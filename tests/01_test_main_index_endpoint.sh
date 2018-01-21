#!/usr/bin/env bash

function runtests() {
    echo -n "Testing / endpoint $TEST_ROUNDS times to test the main index page for errors"

    for i in $( seq ${TEST_ROUNDS} ) ; do
        COMMAND="curl -v --max-time 3 http://$IP_ADDRESS/ 2>&1"
        OUTPUT="$( eval ${COMMAND} )"

        if [ "$?" != 0 ] ; then
            echo -e "\n\nFailed $COMMAND: \n $OUTPUT\n\n"
            return 1
        fi
    done

    echo -e "  [OK]\n"
    echo
}
