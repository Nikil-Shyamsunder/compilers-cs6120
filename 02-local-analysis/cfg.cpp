// cfg.cpp
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <utility>
#include <iterator>
#include <unordered_set>
#include "nlohmann/json.hpp"

using json = nlohmann::json;
using namespace std;

static vector<vector<json>> gen_basic_blocks(const json& function) {
    vector<vector<json>> basic_blocks;
    vector<json> curr_block;

    for (const auto& instr : function.at("instrs")) {
        bool has_label = instr.contains("label");
        bool has_op = instr.contains("op");
        bool is_term = has_op && (instr.at("op") == "br" || instr.at("op") == "jmp" || instr.at("op") == "ret");

        if (has_label) {
            if (!curr_block.empty()) {
                basic_blocks.push_back(curr_block);
            }
            curr_block.clear();
            curr_block.push_back(instr);
        } else if (is_term) {
            curr_block.push_back(instr);
            basic_blocks.push_back(curr_block);
            curr_block.clear();
        } else {
            curr_block.push_back(instr);
        }
    }
    if (!curr_block.empty()) {
        basic_blocks.push_back(curr_block);
        curr_block.clear();
    }
    return basic_blocks;
}

static string get_label(const vector<vector<json>>& basic_blocks, const string& func, size_t idx) {
    const auto& block = basic_blocks.at(idx);
    if (!block.empty() && block.front().contains("label")) {
        return block.front().at("label").get<string>();
    }
    return func + "-block" + to_string(idx);
}

static vector<pair<string, vector<string>>> build_cfg(const string& func_name,
                                                      const vector<vector<json>>& basic_blocks) {
    vector<pair<string, vector<string>>> items;

    for (size_t i = 0; i < basic_blocks.size(); ++i) {
        const auto& block = basic_blocks[i];
        string label = get_label(basic_blocks, func_name, i);
        vector<string> next;

        if (!block.empty() && block.back().contains("op")) {
            const auto& last = block.back();
            string op = last.at("op").get<string>();

            if (op == "br" || op == "jmp") {
                if (last.contains("labels")) {
                    for (const auto& l : last.at("labels")) {
                        next.push_back(l.get<string>());
                    }
                }
            } else if (op == "ret") {
                // next stays empty
            } else {
                if (i + 1 < basic_blocks.size()) {
                    next.push_back(get_label(basic_blocks, func_name, i + 1));
                }
            }
        } else {
            // label with nothing after; next stays empty
        }

        items.emplace_back(label, next);
    }
    return items;
}

static vector<vector<json>> elim_dead_blocks(vector<pair<string, vector<string>>> cfg, const string& func_name, const vector<vector<json>>& basic_blocks) {
    // create a set of all basic block's names to remove from
    unordered_set<string> unused_blocks;
    for (size_t i = 0; i < basic_blocks.size(); ++i) {
        unused_blocks.insert(get_label(basic_blocks, func_name, i));
    }

    // iterate through cfg values. if a block is in the list of values, eliminate it from the set
    for (const auto& [label, nxt] : cfg) {
        for (const auto& n : nxt) {
            unused_blocks.erase(n);
        }
    }

    // guarantee that the entry block is not eliminated
    if (!basic_blocks.empty()) {
        unused_blocks.erase(get_label(basic_blocks, func_name, 0)); 
    }

    // return a new clone set of basic blocks with any of the blocks with names in the set remove
    vector<vector<json>> new_blocks;
    for (size_t i = 0; i < basic_blocks.size(); ++i) {
        if (unused_blocks.find(get_label(basic_blocks, func_name, i)) == unused_blocks.end())
            new_blocks.push_back(basic_blocks[i]);
    }

    // print what blocks we're eliminating
    for (const auto& b : unused_blocks) {
        cerr << "Eliminating dead block: " << b << "\n";
    }
    return new_blocks;
}


// static json global_dce(const json& function) {
//     // TODO: this should be done on basic blocks and doesn't handle reassign
//     unordered_set<string> used_vars;

//     // First pass: collect used variables
//     for (const auto& instr : function.at("instrs")) {
//         if (instr.contains("args")){
//             for (const auto& arg : instr.at("args")) {
//                 used_vars.insert(arg.get<string>());
//             }
//         }
//     }

//     // Second pass: remove assignments to unused variables
//     json new_function = function;
//     new_function["instrs"] = json::array();
//     for (const auto& instr : function.at("instrs")) {
//         if (instr.contains("op") && instr.at("op").get<string>() == "const" && (used_vars.find(instr.at("dest").get<string>()) == used_vars.end())){
//             // skip assignments to unused variables
//         } else {
//             new_function["instrs"].push_back(instr);
//         }
//     }

//     return new_function;
// }

static vector<json> local_dce(const vector<json>& block) {
    int numDeleted = 1;
    vector<json> new_block = block;
    vector<json> curr_block;

    while (numDeleted != 0) {
        curr_block = new_block;
        new_block.clear();
        numDeleted = 0;
        unordered_set<string> curr_used;

        // iterate in reverse through block 
        for (int i = curr_block.size() - 1; i >= 0; --i) {
            json stmt = curr_block[i];
            if (stmt.contains("args")){
                for (const auto& arg : stmt.at("args")) {
                    // insert is idempotent
                    curr_used.insert(arg.get<string>());
                }
            }
            
            // if we have an assignment stmt
            if (stmt.contains("op") && stmt.at("op").get<string>() == "const") {
                if (curr_used.find(stmt.at("dest").get<string>()) != curr_used.end()) {
                    // if this is the newest assignment of the var
                    curr_used.erase(stmt.at("dest").get<string>());
                } else { // redundant assignment or completely unused var
                    numDeleted += 1;
                    continue; // don't add to the updated block
                }  
            } 
            
            new_block.insert(new_block.begin(), stmt);
        } 
    }
    // cout << new_block << "\n";
    // convergence achieved, return new_block
    return new_block;

}


int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // Read all of stdin into a string
    std::string input((std::istreambuf_iterator<char>(cin)), std::istreambuf_iterator<char>());

    json bril = json::parse(input);

    // cout << bril.dump() << "\n";

    // Collect cfg items across all functions
    json dce_functions = json::array();
    for (const auto& func : bril.at("functions")) {
        // gen basic blocks 
        vector<vector<json>> blocks = gen_basic_blocks(func);

        // eliminate dead blocks
        vector<vector<json>> new_blocks = elim_dead_blocks(build_cfg(func.at("name").get<string>(), blocks), func.at("name").get<string>(), blocks);

        // local dce
        vector<vector<json>> dce_blocks;
        for (const vector<json> new_block : new_blocks) {
            vector<json> dce_block = local_dce(new_block);
            dce_blocks.push_back(dce_block);
        }

        // flatten dce_blocks vectors into instruction list
        // replace function instrs with dce instrs
        json new_func = func;
        new_func["instrs"] = json::array();
        for (const auto& block : dce_blocks) {
            for (const auto& instr : block) {
                new_func["instrs"].push_back(instr);
            }
        }

        dce_functions.push_back(new_func);
    }

    bril["functions"] = dce_functions;
    cout << bril.dump() << "\n";

    // Perform basic DCE 

    // // Collect cfg items across all functions
    // vector<pair<string, vector<string>>> cfg;
    // for (const auto& func : bril.at("functions")) {
    //     auto blocks = gen_basic_blocks(func);
    //     auto func_cfg = build_cfg(func.at("name").get<string>(), blocks);
    //     cfg.insert(cfg.end(), func_cfg.begin(), func_cfg.end());
    // }

    // // Print as JSON array of [label, next_list]
    // json out = json::array();
    // for (const auto& [label, nxt] : cfg) {
    //     json pair = json::array();
    //     pair.push_back(label);
    //     pair.push_back(nxt);
    //     out.push_back(pair);
    // }
    // cout << out.dump() << "\n";

    // // Eliminate dead blocks
    // vector<vector<json>> all_blocks;
    // for (const auto& func : bril.at("functions")) {
    //     auto blocks = gen_basic_blocks(func);
    //     auto func_cfg = build_cfg(func.at("name").get<string>(), blocks);
    //     auto new_blocks = elim_dead_blocks(func_cfg, func.at("name").get<string>(), blocks);

    //     // print the new blocks to stdout 
    //     for (const auto& block : new_blocks) {
    //         for (const auto& instr : block) {
    //             cout << instr.dump() << "\n";
    //         }
    //     }
    // }
    return 0;
}
