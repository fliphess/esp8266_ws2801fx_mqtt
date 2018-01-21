
function runtests() {
    echo -n "Testing /modes endpoint $TEST_ROUNDS times to test the modes table page for errors"

    for i in $( seq ${TEST_ROUNDS} ) ; do
        COMMAND="curl -qs --max-time 3 http://$IP_ADDRESS/modes"
        OUTPUT="$( eval ${COMMAND} )"

        if [ "$?" != 0 ] ; then
            echo -e "\n\nFailed $COMMAND: \n $OUTPUT\n\n"
            return 1
        fi
    done

    echo -e "  [OK]\n"
    echo
}
