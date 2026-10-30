#!/bin/bash

var=\$USER
echo $var

var=\\$USER
echo $var

var=\"$USER\"
echo $var

var="$USER \\$USER"
echo $var

var='$USER \\$USER'
echo $var

var=`date +%F`
echo $var

