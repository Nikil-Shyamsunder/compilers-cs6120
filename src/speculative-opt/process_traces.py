import json
import os
from pathlib import Path

# Process trace files to:
# - Remove everything from call instruction onwards
# - Remove jump instructions
# - Convert branch instructions to guard instructions

def process_trace(lines):
    """Process a single trace file."""
    result = []
    i = 0

    while i < len(lines):
        line = lines[i].strip()

        if not line:
            i += 1
            continue

        if not line.startswith('{'):
            result.append(line)
            i += 1
            continue

        try:
            instr = json.loads(line)
        except json.JSONDecodeError:
            result.append(line)
            i += 1
            continue

        op = instr.get('op', '')

        # If call instruction, stop processing 
        if op == 'call':
            break

        # If jump instruction, skip this line
        elif op == 'jmp':
            i += 1
            continue

        # if branch instruction, convert to guard
        elif op == 'br':
            # get the branch labels and boolean argument
            labels = instr.get('labels', [])
            bool_arg = instr.get('args', [])[0] if instr.get('args') else None

            # next line should be the taken label
            taken_label = None
            if i + 1 < len(lines):
                next_line = lines[i + 1].strip()
                if next_line and not next_line.startswith('{'):
                    taken_label = next_line

            if taken_label and bool_arg and len(labels) >= 2:
                # check if the taken label is the left (true) or right (false) branch
                if taken_label == labels[0]:
                    # left label taken - boolean was true, use as-is
                    guard_arg = bool_arg
                elif taken_label == labels[1]:
                    # right label taken - boolean was false, need to negate
                    # Create a negation instruction
                    not_dest = f"not_{bool_arg}"
                    not_instr = {
                        'dest': not_dest,
                        'op': 'not',
                        'type': 'bool',
                        'args': [bool_arg]
                    }
                    result.append(json.dumps(not_instr))
                    guard_arg = not_dest
                else:
                    # Taken label doesn't match either branch label - shouldn't happen
                    guard_arg = bool_arg

                # create guard instruction
                guard_instr = {
                    'op': 'guard',
                    'args': [guard_arg],
                    'labels': ['end_speculation']
                }
                result.append(json.dumps(guard_instr))

                # Skip the label line - don't add it to the result
                i += 2
                continue

            i += 1

        else:
            result.append(line)
            i += 1

    result.insert(0, json.dumps({"op": "speculate"}))
    result.append(json.dumps({"op": "commit"}))
    result.append(json.dumps({"label": "end_speculation"}))

    return result


def main():
    traces_dir = Path('traces')
    if not traces_dir.exists():
        print(f"Error: {traces_dir} directory not found")
        return
    
    for trace_file in traces_dir.glob('*.trc'):
        print(f"Processing {trace_file.name}...")
        with open(trace_file, 'r') as f:
            lines = f.readlines()

        processed = process_trace(lines)

        with open(trace_file, 'w') as f:
            for line in processed:
                f.write(line + '\n')

    print("Done!")


if __name__ == '__main__':
    main()
