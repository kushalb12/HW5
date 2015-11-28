#! /bin/sh

N=$1

shift

b=$1
shift
c=$1
shift
F=$1
shift
B1=$1
shift
P=$1
shift
S=$1
shift
T=$1

while [[  $COUNTER -lt $N ]]; do
    ./p4 $N $b $c $F $B1 $P $S $T &
    let COUNTER=COUNTER+1
    sleep 3 
done

trap "kill -TERM -$$" SIGINT
wait
