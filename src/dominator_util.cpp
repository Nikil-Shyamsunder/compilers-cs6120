#include "common.hpp"
#include <iostream>

using namespace std;

// dom = {every block -> all blocks}
// dom[entry] = {entry}
// while dom is still changing:
//     for vertex in CFG except entry:
//         dom[vertex] = {vertex} ∪ ⋂(dom[p] for p in vertex.preds}
unordered_map<string, unordered_set<string>> dominators(string func_name, vector<vector<json>> blocks) {
    unordered_map<string, unordered_set<string>> dom;
    cfg_info cfg = build_cfg(func_name, blocks);

    // add dummy "entry"-labeled node to cfg, with back edge to first block in blocks
    string entry = "entry";
    string first_block = get_label(blocks, func_name, 0);
    cfg.predecessors[first_block].push_back(entry);
    cfg.successors[entry] = {first_block};
    cfg.predecessors[entry] = {};

    // get a list of all blocks to help with initialization
    unordered_set<string> all_blocks;
    for (size_t i = 0; i < blocks.size(); ++i) {
        string label = get_label(blocks, func_name, i);
        all_blocks.insert(label);
    }

    // initialize
    for (const auto& label : all_blocks) {
        dom[label] = all_blocks; // start with everything
    }

    string entry = get_label(blocks, func_name, 0);
    dom[entry] = {entry};

    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& [label, preds] : cfg.predecessors) {
            if (label == entry) continue; // for every vertex except entry

            unordered_set<string> new_dom;
            // calculate ⋂(dom[p] for p in vertex.preds}
            if (!preds.empty()) {
                new_dom = dom[*preds.begin()]; // copy first pred's dom set
                for (const auto& p : preds) {
                    if (p == *preds.begin()) continue; // since this is already in new_dom
                    unordered_set<string> temp; // get intersection with new_dom and put it in temp
                    for (const auto& n : new_dom) {
                        if (dom[p].count(n)) {
                            temp.insert(n);
                        }
                    }
                    new_dom = temp;
                }
            }
            new_dom.insert(label); // do the {vertex} ∪ ... part

            if (new_dom != dom[label]) {
                dom[label] = new_dom;
                changed = true;
            }
        }
    }

    return dom;
}

// dominator tree
// strategy: for each block B, iterate through all dominators and find the one that doesn't dominate any other dominator of B 
unordered_map<string, unordered_set<string>> dominator_tree(unordered_map<string, unordered_set<string>> dominators) {
    
}



int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    std::string input((std::istreambuf_iterator<char>(cin)), std::istreambuf_iterator<char>());
    json bril = json::parse(input);

    json out_funcs = json::array();
    for (const auto& func : bril.at("functions")) {
        auto blocks = gen_basic_blocks(func);

        // cout << "Function: " << func["name"] << "\n";
        auto reaching_defs = dominators(func["name"], blocks); 

        // print out reaching definitions
        for (const auto& [label, dominators] : reaching_defs) {
            cout << "Block " << label << ":\n";
            for (const auto& dom : dominators) {
                cout << "  " << dom << "\n";
            }
        }
    }
}
