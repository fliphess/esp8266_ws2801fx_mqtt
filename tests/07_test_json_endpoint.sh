#!/usr/bin/env bash


function test_get() {
    for i in $(seq 3 ); do
        COMMAND="curl -qs --max-time 3 http://$IP_ADDRESS/json 2>&1"
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

COLOR1='{\"r\":0,\"g\":255,\"b\":255}'
COLOR2='{\"r\":0,\"g\":0,\"b\":255}'
COLOR3='{\"r\":255,\"g\":0,\"b\":0}'
COLOR4='{\"r\":0,\"g\":0,\"b\":150}'
COLOR5='{\"r\":2,\"g\":20,\"b\":200}'
COLOR6='{\"r\":255,\"g\":255,\"b\":255}'

function test_post() {
    COLORS=(
      $COLOR1
      $COLOR2
      $COLOR3
      $COLOR4
      $COLOR5
      $COLOR6
    )

    for COLOR in ${COLORS[@]}; do
        for MODE in $( seq 0 10 ) ; do
            DATA="{\"color\":$COLOR,\"effect\":$MODE,\"speed\":200,\"brightness\":255}"
            COMMAND="curl -v --max-time 3 -d \"${DATA}\" http://${IP_ADDRESS}/json 2>&1"
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
}


function runtests() {
    echo -n "Testing /json endpoint $TEST_ROUNDS * 50 times to test the json input page for errors"

    for i in $( seq ${TEST_ROUNDS} ) ; do
        test_get
        test_post
    done
}
