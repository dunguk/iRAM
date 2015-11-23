# (c) David T. Nguyen
# July 2014
# dunguk@gmail.com


# usage:
# ./get_highest_heap_usage.sh
# returns the usage (in KB) of the process with the highest heap usage

# =======

dumpsys meminfo | awk '/Total PSS by process/{getline; print}' | xargs | cut -d' ' -f1
