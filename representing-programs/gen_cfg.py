import json
import sys
from collections import defaultdict

def gen_basic_blocks(ir):
    basic_blocks = []
    curr_block = []

    for function in ir['functions']:
        for instr in function['instrs']:
            if 'label' in instr:
                if curr_block:
                    basic_blocks.append(curr_block)
                curr_block = [instr]
            elif 'op' in instr and instr['op'] in ['br', 'jmp', 'ret']:
                curr_block.append(instr)
                basic_blocks.append(curr_block)
                curr_block = []
            else:
                curr_block.append(instr)
        if curr_block:
            basic_blocks.append(curr_block)
            curr_block = []

    return basic_blocks


def get_label(basic_blocks, idx):
    block = basic_blocks[idx]
    if 'label' in block[0]:
        label = block[0]['label'] 
    else:
        label = f"block{idx}"
    
    return label


def build_cfg(basic_blocks):
    cfg = {}
    
    for i, basic_block in enumerate(basic_blocks):
        label = get_label(basic_blocks, i)

        next = []
        if 'op' not in basic_block[-1]: # we have a label with nothing after
            pass
        else:
            op = basic_block[-1]['op']
            if op == 'br' or op == 'jmp':
                next += basic_block[-1]['labels']
            elif op == 'ret':
                pass # next should stay empty
            elif i + 1 < len(basic_blocks): # there is a next block
                next = [get_label(basic_blocks, i + 1)]
            else:
                pass # next should stay 
            
        cfg[label] = next
    
    return cfg


if __name__ == "__main__":
    # load json from stdin
    bril = json.load(sys.stdin)
    basic_blocks = gen_basic_blocks(bril)

    # for basic_block in basic_blocks:
    #     print(basic_block)
    #     print()
    # print(basic_blocks)
    cfg = build_cfg(basic_blocks)
    print(cfg)
    # print(cfg)

