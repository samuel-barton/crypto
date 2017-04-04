
i=1

while [ $i -lt 100001 ]
do
    j=$(($i*$1))

    echo "N = " $j
    ./random $j freq
    i=$(($i + 1))
done
