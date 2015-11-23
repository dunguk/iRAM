# (c) David T. Nguyen
# July 2014
# dunguk@gmail.com


# usage:
# ./convert_heap_format_to_short.sh $1
# for instance from "128000" to "128m"
# $1 - input heap size in KB

# =======

INPUT=$1

OUTPUT=`expr $INPUT \/ 1000`

OUTPUT2=`echo ${OUTPUT}m`

echo $OUTPUT2
