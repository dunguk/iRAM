# (c) David T. Nguyen
# November 2015
# dunguk@gmail.com


# usage:
# ./convert_heap_format.sh $1
# converts heap size to numeric format in KB
# for instance from "128m" to "128000"
# $1 - input heap size in MB

# =======

INPUT=$1

OUTPUT=`echo $1 | cut -d'm' -f1`

OUTPUT2=`expr $OUTPUT \* 1000`

echo $OUTPUT2
