for f in ../../../bril/benchmarks/core/*.bril; do
    basename="${f##*/}"
    o="raw_traces/${basename%.bril}.trc"
    ARGS=$(sed -n 's/^[[:space:]]*# ARGS: \(.*\)/\1/p' "$f")
    bril2json < "$f" | brili --trace $ARGS 2>&1 | grep "trace" > "$o"
done