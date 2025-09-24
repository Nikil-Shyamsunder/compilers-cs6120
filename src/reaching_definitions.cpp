#include "common.hpp"
#include <iostream>

using namespace std;

// in[entry] = init
// out[*] = init

// worklist = all blocks
// while worklist is not empty:
//     b = pick any block from worklist
//     in[b] = merge(out[p] for every predecessor p of b)
//     out[b] = transfer(b, in[b])
//     if out[b] changed:
//         worklist += successors of b

struct definition {
    string var;
    json instr;

    bool operator==(const definition& other) const {
        return var == other.var;
    }
};

unordered_set<definition> reaching_transfer(vector<json> block, unordered_set<definition> in){ 
    unordered_set<definition> out = in;

    for (json instr : block) {
        if (has_dest(instr)) {
            string dest = instr["dest"].get<string>();
            definition def = {dest, instr};
            out.erase(def); // since equality is defined by var only, this should delete any instance of dest 
            out.emplace(def);
        }
    }

    return out;
};

map<std::string, unordered_set<definition>> reaching_definitions(string func_name, vector<vector<json>> blocks) {
    unordered_map<string, unordered_set<definition>> in;
    unordered_map<string, unordered_set<definition>> out;
    unordered_map<string, vector<json>> label_to_block;
    vector<string> worklist;
    cfg_info cfg = build_cfg(func_name, blocks);

    // in and out are empty as defaults
    for (int i = 0; i < blocks.size(); ++i) {
        string label = get_label(blocks, func_name, i);
        in.emplace(label, unordered_set<definition>());
        out.emplace(label, unordered_set<definition>());
        label_to_block.emplace(label, blocks[i]);
        worklist.push_back(label);
    }

    while (worklist.size() > 0) {
        string label = worklist[worklist.size() - 1]; // take a block from the worklist
        worklist.pop_back(); // remove the block from the worklist
        json block = label_to_block[label];

        // merge all the output of all predeccesors of block
        unordered_set<definition> merged_set;;
        for (string pred : cfg.predecessors[label]) {
            for (definition d : out[pred]){
                merged_set.insert(d);
            }
        }

        in[label] = merged_set;

        // apply transfer func 
        unordered_set<definition> new_out = reaching_transfer(block, in[label]);
        if (new_out != out[label]) {
            out[label] = new_out;
            for (string successor : cfg.successors[label]) {
                worklist.push_back(successor);
            }
        }
    }
}



int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    std::string input((std::istreambuf_iterator<char>(cin)), std::istreambuf_iterator<char>());
    json bril = json::parse(input);

    json out_funcs = json::array();
    for (const auto& func : bril.at("functions")) {
        auto blocks = gen_basic_blocks(func);

        auto reaching_defs = reaching_definitions(func["name"], blocks); 

        // print out reaching definitions
        for (const auto& [label, defs] : reaching_defs) {
            cerr << "Block " << label << ":\n";
            for (const auto& def : defs) {
                cerr << "  " << def.var << " defined at " << def.instr.dump() << "\n"; 
            }
        }
    }
}

