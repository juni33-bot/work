#!/bin/bash

name="linux"
course="expert"

# "linux expert"
var="$name $course"
echo $var

# "$linux \expert"
var="\$$name \\$course"
echo $var

# "$$linux \\expert"
var="\$\$$name \\\\$course"
echo $var

# "experted linux"
var="${course}d $name"
echo $var

# "linux_expert_170101"
var="${name}_${course}_170101"
echo $var




name="linux"
course="expert"

var="$name $course"
echo $var

echo \"$name \\$course\"
echo \"\$\$$name \\\\$course\"
echo \"$course"d" $name\"
echo \"$name"_"$course"_170101"\"
