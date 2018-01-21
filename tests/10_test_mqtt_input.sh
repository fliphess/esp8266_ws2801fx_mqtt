#!/usr/bin/env bash


COLOR1='{"r":0,"g":255,"b":255}'
COLOR2='{"r":0,"g":0,"b":255}'
COLOR3='{"r":255,"g":0,"b":0}'
COLOR4='{"r":0,"g":0,"b":150}'
COLOR5='{"r":2,"g":20,"b":200}'
COLOR6='{"r":255,"g":255,"b":255}'

function runtests() {
    echo -n "Testing MQTT input to test the MQTT control api for errors"

    COLORS=(
      $COLOR1
      $COLOR2
      $COLOR3
      $COLOR4
      $COLOR5
      $COLOR6
    )

    for ROUND in $( seq ${TEST_ROUNDS} ) ; do
        for COLOR in ${COLORS[@]}; do
            for MODE in $( seq 0 10 ) ; do

            DATA="{\"color\":$COLOR,\"effect\":$MODE,\"speed\":200,\"brightness\":255}"
                mosquitto_pub -p 1884 -h server.home -u admin -P plop  \
                        -t "ledstrip1.home/in" \
                        -m "$DATA"

                if [ "$?" != 0 ] ; then
                    echo -e "\n\nFailed: $OUTPUT\n\n"
                    return 1
                fi
              done
        done
    done
    echo -e "  [OK]\n"
    echo

}
