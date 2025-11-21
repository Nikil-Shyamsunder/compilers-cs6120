import json
import os
import re
from pathlib import Path

def process_trace(lines):
    """Process a single trace file."""
    result = []
    i = 0
    last_line_num = None

    while i < len(lines):
        line = lines[i].strip()

        if not line:
            i += 1
            continue

        json_str = line
        current_line_num = None
        if line.startswith("trace (line "):
            match = re.match(r'trace \(line (\d+)\): (.+)', line)
            if match:
                current_line_num = int(match.group(1))
                json_str = match.group(2)
            else:
                result.append(line)
                i += 1
                continue
        elif not line.startswith('{'):
            result.append(line)
            i += 1
            continue

        try:
            instr = json.loads(json_str)
        except json.JSONDecodeError:
            result.append(line)
            i += 1
            continue

        op = instr.get('op', '')

        # If call/ret/print instruction, stop processing
        if op == 'call' or op == 'ret' or op == 'print':
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

            # next line should be the taken label - parse it from new format
            taken_label = None
            if i + 1 < len(lines):
                next_line = lines[i + 1].strip()
                # Check if it's a "trace: label" line
                if next_line.startswith("trace: "):
                    taken_label = next_line.split("trace: ", 1)[1]
                elif next_line and not next_line.startswith('{') and not next_line.startswith('trace (line'):
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
                    # shouldn't happen (i.e. rust unreachable! lol )
                    guard_arg = bool_arg

                # create guard instruction
                guard_instr = {
                    'op': 'guard',
                    'args': [guard_arg],
                    'labels': ['end_speculation']
                }
                result.append(json.dumps(guard_instr))

                if current_line_num is not None:
                    last_line_num = current_line_num

                i += 2
                continue

            i += 1

        else:
            result.append(json_str)
            if current_line_num is not None:
                last_line_num = current_line_num
            i += 1

    result.insert(0, json.dumps({"op": "speculate"}))

    result.append(json.dumps({"op": "commit"}))
    result.append(json.dumps({"labels": ["success"], "op": "jmp"}))

    if last_line_num is not None:
        result.append(str(last_line_num))

    return result


def main():
    raw_traces_dir = Path('raw_traces')
    processed_traces_dir = Path('processed_traces')

    if not raw_traces_dir.exists():
        print(f"Error: {raw_traces_dir} directory not found")
        return

    processed_traces_dir.mkdir(exist_ok=True)

    for trace_file in raw_traces_dir.glob('*.trc'):
        print(f"Processing {trace_file.name}...")
        with open(trace_file, 'r') as f:
            lines = f.readlines()

        processed = process_trace(lines)

        output_file = processed_traces_dir / trace_file.name
        with open(output_file, 'w') as f:
            for line in processed:
                f.write(line + '\n')

    print("Done!")


if __name__ == '__main__':
    main()
