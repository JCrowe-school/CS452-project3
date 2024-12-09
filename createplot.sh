#!/usr/bin/env bash
function usage() {
    echo "$0 usage:" && grep " .)\ #" $0
    exit 0
}
[ $# -eq 0 ] && usage
while getopts "hs:f:" arg; do
    case $arg in
    s) # The size of the array to sort.
        size=${OPTARG}
        ;;
    f) # The plot file name
        name=${OPTARG}
        ;;
    h | *) # Display help.
        usage
        exit 0
        ;;
    esac
done
if [ "$name" == "" ] || [ "$size" == "" ]
then
        usage
        exit 0
fi
if [ -e ./myprogram ]; then
    if [ -e "data.dat" ]; then
        rm -f data.dat
    fi
    echo "Running myprogram to generate data"
    echo "#Time Threads" >> data.dat
    for n in {1..32}; do
        echo -ne "running $n thread \r"
        ./myprogram "$size" "$n" >> data.dat
    done
    gnuplot -e "filename='$name.png'" graph.plt
    echo "Created plot $name.png from data.dat file"
else
    echo "myprogram is not present in the build directory. Did you compile your code?"
fi