#!/bin/bash
if [ "$#" != 1 ] || [ "x$1" == "x" ]; then
    echo "Usage $0 <ip of esp>"
    exit 0
fi

export IP_ADDRESS="$1"
export TEST_ROUNDS=15
export TEST_DIRECTORY="tests"


set -e

echo -e "*********************************************"
echo -e "*** Running tests in directory $DIRECTORY"
echo -e "*********************************************\n"

for TESTFILE in ${TEST_DIRECTORY}/*.sh ; do
    echo "*** Running $TESTFILE"
    source ${TESTFILE}
    runtests
done
