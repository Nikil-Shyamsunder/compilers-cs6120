import json
import sys
from collections import defaultdict

def op_frequency(ir):
    freqs = defaultdict(int)
    for function in ir['functions']:
        for instr in function['instrs']:
            if 'op' in instr:
                freqs[instr['op']] += 1

    return dict(freqs)

if __name__ == "__main__":
    # load json from stdin
    bril = json.load(sys.stdin)
    op_freqs = op_frequency(bril)

    sorted_items = sorted(op_freqs.items())
    for k, v in sorted_items:
        print(f"{k}: {v}")

    # print(cfg)


