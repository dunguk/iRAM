# (c) David T. Nguyen
# July 2014
# dunguk@gmail.com


# usage:
# ./double_heap.sh 
# doubles the current heap size



# =======

# source configure.sh 

INPUT=`getprop dalvik.vm.heapsize`

OUTPUT=`echo $INPUT | cut -d'm' -f1`

OUTPUT2=`expr $OUTPUT \* 2`

OUTPUT3=`echo ${OUTPUT2}m`

setprop dalvik.vm.heapsize "$OUTPUT3"


