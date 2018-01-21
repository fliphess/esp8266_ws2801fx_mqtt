

function test_get() {
    for i in $(seq 3 ); do
        COMMAND="curl --max-time 3 http://$IP_ADDRESS/json -v 2>&1"
        OUTPUT="$( eval ${COMMAND} )"

        if [ "$?" != 0 ] ; then
            echo -e "\n\nFailed: $OUTPUT\n\n"
            return 1
        fi

        if ! ( echo "$OUTPUT" | grep -q "HTTP/1.1 500 Internal Server Error" ;) ; then
            echo -e "\n\nFailed: $OUTPUT\n\n"
            return 1
        fi
    done
}

function test_post() {
    COLORS="ff000 00ff00 0000ff ffffff 80ff56"

    for COLOR in ${COLORS}; do
        for MODE in $( seq 0 10 ) ; do
            DATA="{\"color\":\"$COLOR\",\"mode\":$MODE,\"speed\":200,\"brightness\":255}"
            COMMAND="curl --max-time 3 -d \"${DATA}\" http://${IP_ADDRESS}/json -v 2>&1"
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
          sleep 1
    done
}


function runtests() {
    echo -n "Testing /json endpoint $TEST_ROUNDS times to test the json input page for errors"

    for i in $( seq ${TEST_ROUNDS} ) ; do
        test_get
        test_post
    done
}
