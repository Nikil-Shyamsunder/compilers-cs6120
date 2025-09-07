import json
import sys
from collections import defaultdict

def op_frequency(ir):
    freqs = defaultdict(int)
    for instr in ir['functions'][0]['instrs']:
        if 'op' in instr:
            freqs[instr['op']] += 1

    return dict(freqs)

if __name__ == "__main__":
    # load json from stdin
    bril = json.load(sys.stdin)
    print(op_frequency(bril))
    # print(cfg)


