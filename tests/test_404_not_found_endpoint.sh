

function runtests() {
    echo -n "Testing random endpoint $TEST_ROUNDS times to test the 404 page for errors"

    for i in $( seq ${TEST_ROUNDS} ) ; do
        COMMAND="curl -qs --max-time 3 http://$IP_ADDRESS/$RANDOM"
        OUTPUT="$( eval ${COMMAND} )"

        if [ "$?" != 0 ] || [ "${OUTPUT}" != "File Not Found" ] ; then
            echo "Failed $COMMAND: \n $OUTPUT"
            return 1
        fi
    done

    echo -e "  [OK]\n"
    echo
}
