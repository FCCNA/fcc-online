#!/bin/bash

alarm_message=$1
generate_post_data(){
    
cat <<EOF
{
"text": "@all Midas alarm triggered with message = $alarm_message :aaaa:",
"icon_emoji":"rotating_light:",
"username":"pcfcc"
}
EOF
}
curl -i -X POST -H 'Content-Type: application/json' --data "$(generate_post_data)" https://mattermost.web.cern.ch/hooks/8t4madmiapgyzcumyi67detpih
#echo "$(generate_post_data)" >/home/fcc/test.txt
