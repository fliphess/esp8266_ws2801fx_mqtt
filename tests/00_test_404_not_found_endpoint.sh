#!/usr/bin/env bash


function runtests() {
    echo -n "Testing random endpoint $TEST_ROUNDS times to test the 404 page for errors"

    for i in $( seq ${TEST_ROUNDS} ) ; do
        COMMAND="curl -v --max-time 3 http://$IP_ADDRESS/$RANDOM 2>&1"
        OUTPUT="$( eval ${COMMAND} )"

        if [ "$?" != 0 ] ; then
            echo "Failed $COMMAND: \n $OUTPUT"
            return 1
        fi

        if ! ( echo "${OUTPUT}" | grep -q "HTTP/1.1 404 Not Found" ) ; then
            echo "Failed $COMMAND: \n $OUTPUT"
            return 1
        fi
    done

    echo -e "  [OK]\n"
    echo
}
