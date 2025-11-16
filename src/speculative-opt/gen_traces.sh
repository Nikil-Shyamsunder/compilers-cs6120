for f in ../../../bril/benchmarks/core/*.bril; do
    basename="${f##*/}"
    o="traces/${basename%.bril}.trc"
    ARGS=$(sed -n 's/^[[:space:]]*# ARGS: \(.*\)/\1/p' "$f")
    TFILE="/tmp/$$.trace"
    bril2json < "$f" | brili $ARGS | grep trace: > $TFILE
    cut -d':' -f2- $TFILE | head -n 10 > "$o"
    rm $TFILE
done