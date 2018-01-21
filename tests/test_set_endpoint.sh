

function test_get() {
    echo -n "Testing /set GET endpoint $TEST_ROUNDS times to test the set input page for errors"
    for i in $( seq ${TEST_ROUNDS} ) ; do
        COMMAND="curl --max-time 3 http://$IP_ADDRESS/set -v 2>&1"
        OUTPUT="$( eval $COMMAND )"

        if [ "$?" != 0 ] ; then
            echo -e "\n\nFailed $COMMAND: \n$OUTPUT\n\n"
            return 1
        fi

        if ! ( echo "$OUTPUT" | grep -q "HTTP/1.1 200 OK" ;) ; then
            echo -e "\n\nFailed $COMMAND: \n$OUTPUT\n\n"
            return 1
        fi
    done

    echo -e "  [OK]\n"
    echo

}

function test_post() {
    echo -n "Testing /json POST endpoint $TEST_ROUNDS times to test the json input page for errors"

    COLORS="ff000 00ff00 0000ff ffffff 80ff56"

    for i in $( seq ${TEST_ROUNDS} ) ; do

        for COLOR in ${COLORS}; do
            for MODE in $( seq 0 10 ) ; do
                COMMAND="curl --max-time 3 http://$IP_ADDRESS/set?m=\"$MODE\"&c=\"$COLOR\" -v 2>&1"
                OUTPUT="$( eval ${COMMAND} )"

                if [ "$?" != 0 ] ; then
                    echo -e "\n\nFailed $COMMAND: \n $OUTPUT\n\n"
                    return 1
                fi

                if ! ( echo "$OUTPUT" | grep -q "HTTP/1.1 200 OK" ;) ; then
                    echo -e "\n\nFailed $COMMAND: \n $OUTPUT\n\n"
                    return 1
                fi
              done
        done

    done

    echo -e "  [OK]\n"
    echo

}


function runtests() {
    test_get
    test_post
}
