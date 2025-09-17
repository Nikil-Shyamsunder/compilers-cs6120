#include "common.hpp"
#include <iostream>

using namespace std;


static vector<json> lvn(vector<json> block, const vector<string>& params) {
    vector<json> new_block;
    map<value, pair<int, string>> table; // value -> (value number, canonical var name)
    map<string, int> var2num;

    int next_vn = 1;

    for (const auto& p : params) {
        var2num[p] = next_vn++;

        // add to table as id of themselves, so future uses can be canon'd
        table.insert({value{"id", {p}}, std::make_pair(var2num[p], p)});
    }

    for (int i = 0; i < (int)block.size(); ++i) {
        json instr = block[i];

        // ignore things that aren't candidates for replacement
        if (!instr.contains("dest")) {
            new_block.push_back(instr);
            continue;
        }

        const bool is_call = instr.contains("op") && instr["op"].is_string() && instr["op"] == "call";
        value v = value::from_json(instr);

        // case for calls
        if (is_call) {
            int num = next_vn++;
            string dest = instr["dest"];

            bool will_be_overwritten = false;
            for (int j = i + 1; j < (int)block.size(); ++j) {
                if (block[j].contains("dest") && block[j]["dest"] == dest) {
                    will_be_overwritten = true;
                    break;
                }
            }

            if (will_be_overwritten) {
                string new_dest = dest + to_string(num);
                for (int j = i + 1; j < (int)block.size(); ++j) {
                    if (block[j].contains("args")) {
                        json new_args = json::array();
                        for (auto& arg : block[j]["args"]) {
                            new_args.push_back(arg.get<string>() == dest ? json(new_dest) : arg);
                        }
                        block[j]["args"] = new_args;
                    } 
                    if (block[j].contains("dest") && block[j]["dest"] == dest) {
                        break;
                    }
                }
                dest = new_dest;
                instr["dest"] = dest;
            }

            // do NOT add calls to table
            var2num[instr["dest"]] = num;
            new_block.push_back(instr);
            continue;
        }

        // normal lvn (no side effects)
        auto it = table.find(v);
        if (it != table.end()) {
            // value already computed, reuse via id
            const string canonical_var = it->second.second;
            const string dest_var = instr["dest"];
            json new_instr = {
                {"args", json::array({canonical_var})},
                {"dest", dest_var},
                {"op", "id"}
            };
            if (instr.contains("type")) new_instr["type"] = instr["type"];
            new_block.push_back(new_instr);
            var2num[dest_var] = it->second.first;
        } else {
            int num = next_vn++;
            string dest = instr["dest"];

            // if value will be overwritten later, give a fresh name and rewrite future uses
            bool will_be_overwritten = false;
            for (int j = i + 1; j < (int)block.size(); ++j) {
                if (block[j].contains("dest") && block[j]["dest"] == dest) { will_be_overwritten = true; break; }
            }

            if (will_be_overwritten) {
                string new_dest = dest + to_string(num);
                for (int j = i + 1; j < (int)block.size(); ++j) {
                    if (block[j].contains("args")) {
                        json new_args = json::array();
                        for (auto& arg : block[j]["args"]) {
                            new_args.push_back(arg.get<string>() == dest ? json(new_dest) : arg);
                        }
                        block[j]["args"] = new_args;
                    } 
                    if (block[j].contains("dest") && block[j]["dest"] == dest) break;
                }
                dest = new_dest;
                instr["dest"] = dest;
            }

            table.insert({v, std::make_pair(num, dest)});

            if (instr.contains("args")) {
                vector<string> new_args;
                for (const string& arg : instr["args"]) {
                    auto vn_it = var2num.find(arg);
                    if (vn_it != var2num.end()) {
                        int vn = vn_it->second;
                        bool found = false;
                        for (const auto& [val, entry] : table) {
                            if (entry.first == vn) { new_args.push_back(entry.second); found = true; break; }
                        }
                        if (!found) new_args.push_back(arg);
                    } else {
                        new_args.push_back(arg);
                    }
                }
                instr["args"] = new_args;
            }

            new_block.push_back(instr);
            var2num[instr["dest"]] = num;
        }
    }

    return new_block;
}


int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    std::string input((std::istreambuf_iterator<char>(cin)), std::istreambuf_iterator<char>());
    json bril = json::parse(input);

    json out_funcs = json::array();
    for (const auto& func : bril.at("functions")) {
        vector<string> params;
        if (func.contains("args") && func["args"].is_array()) {
            for (const auto& a : func["args"]) {
                if (a.contains("name") && a["name"].is_string()) {
                    params.push_back(a["name"].get<string>());
                }
            }
        }

        auto blocks = gen_basic_blocks(func);

        std::vector<std::vector<json>> lvn_blocks;
        lvn_blocks.reserve(blocks.size());
        for (auto b : blocks) {
            lvn_blocks.push_back(lvn(std::move(b), params));
        }

        json new_func = func;
        replace_func_instrs(new_func, lvn_blocks);
        out_funcs.push_back(std::move(new_func));
    }

    bril["functions"] = std::move(out_funcs);
    cout << bril.dump(2) << "\n";
    return 0;
}

