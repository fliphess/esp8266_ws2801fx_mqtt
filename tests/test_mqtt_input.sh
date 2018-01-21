#!/usr/bin/env bash

function runtests() {
    echo -n "Testing MQTT input to test the MQTT control api for errors"

    COLORS="ff0000' '00ff00' 'ffff00' '0000ff' 'ffffff' '80ff56' 'aabbcc'"

    for COLOR in ${COLORS}; do
        for MODE in $( seq 0 10 ) ; do

            DATA="{\"color\":\"$COLOR\",\"mode\":$MODE,\"speed\":200,\"brightness\":255}"
            mosquitto_pub -p 1884 -h server.home -u admin -P plop  \
                    -t "ledstrip1.home/in" \
                    -m "$DATA"

            if [ "$?" != 0 ] ; then
                echo -e "\n\nFailed: $OUTPUT\n\n"
                return 1
            fi
          done
	  sleep 2
    done
    echo -e "  [OK]\n"
    echo

}
