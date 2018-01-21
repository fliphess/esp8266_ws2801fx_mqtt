#!/bin/bash
if [ "$#" != 1 ] || [ "x$1" == "x" ]; then
    echo "Usage $0 <ip of esp>"
    exit 0
fi

export IP_ADDRESS="$1"
export TEST_ROUNDS=15
export TEST_DIRECTORY="tests"


echo -e "*********************************************"
echo -e "*** Running stresstest for $IP_ADDRESS"
echo -e "*********************************************\n"

for TESTFILE in ${TEST_DIRECTORY}/*.sh ; do
    echo "*** Running $TESTFILE"
    source ${TESTFILE}
    runtests
done
