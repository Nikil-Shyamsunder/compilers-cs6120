#include "common.hpp"
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <vector>
#include <string>
#include <algorithm>

using namespace std;

// helper func to map block label -> its index in the block vector
static unordered_map<string, size_t> block_index_map(const string &func_name, const vector<vector<json>> &blocks)
{
    unordered_map<string, size_t> idx;
    for (size_t i = 0; i < blocks.size(); ++i)
    {
        string label = get_label(blocks, func_name, i);
        idx[label] = i;
    }
    return idx;
}

// collect mapping from get destinations to their source variables (from sets)
static unordered_map<string, string> collect_set_mappings(
    const vector<vector<json>> &blocks,
    const string &func_name)
{
    unordered_map<string, string> mapping;
    for (const auto &block : blocks)
    {
        for (const auto &instr : block)
        {
            if (instr.contains("op") && instr["op"] == "set")
            {
                // set dest val
                auto args = instr["args"];
                if (args.size() == 2)
                {
                    string dest = args[0];
                    string src = args[1];
                    mapping[dest] = src;
                }
            }
        }
    }
    return mapping;
}

static vector<vector<json>> rewrite_blocks_from_ssa(
    const vector<vector<json>> &blocks,
    const string &func_name,
    const unordered_map<string, string> &mapping)
{
    vector<vector<json>> out = blocks;

    for (auto &block : out)
    {
        vector<json> new_instrs;

        for (auto &instr : block)
        {
            // skip SSA-only ops
            if (instr.contains("op"))
            {
                string op = instr["op"].get<string>();
                if (op == "get" || op == "set" || op == "undef")
                    continue;
            }

            // rewrite args
            if (instr.contains("args"))
            {
                for (auto &arg : instr["args"])
                {
                    if (arg.is_string())
                    {
                        string name = arg.get<string>();
                        if (mapping.find(name) != mapping.end())
                        {
                            arg = mapping.at(name);
                        }
                    }
                }
            }

            if (instr.contains("dest"))
            {
                string d = instr["dest"].get<string>();
                // if this dest was used as a get-target (and therefore mapped from another), drop suffix
                size_t dot = d.find('.');
                if (dot != string::npos)
                {
                    instr["dest"] = d.substr(0, dot);
                }
            }

            new_instrs.push_back(instr);
        }

        block = new_instrs;
    }

    return out;
}

// merge SSA versions by normalizing names
static void normalize_names(vector<vector<json>> &blocks)
{
    for (auto &block : blocks)
    {
        for (auto &instr : block)
        {
            // Dest
            if (instr.contains("dest"))
            {
                string d = instr["dest"].get<string>();
                size_t dot = d.find('.');
                if (dot != string::npos)
                {
                    instr["dest"] = d.substr(0, dot);
                }
            }
            // Args
            if (instr.contains("args"))
            {
                for (auto &arg : instr["args"])
                {
                    if (arg.is_string())
                    {
                        string s = arg.get<string>();
                        size_t dot = s.find('.');
                        if (dot != string::npos)
                        {
                            arg = s.substr(0, dot);
                        }
                    }
                }
            }
        }
    }
}

// rebuild flat instruction list
static vector<json> flatten_blocks(const vector<vector<json>> &blocks)
{
    vector<json> out;
    for (const auto &block : blocks)
    {
        for (const auto &instr : block)
        {
            out.push_back(instr);
        }
    }
    return out;
}

json func_from_ssa(json func)
{
    string func_name = func["name"].get<string>();
    vector<vector<json>> blocks = gen_basic_blocks(func);

    auto mapping = collect_set_mappings(blocks, func_name);

    auto rewritten = rewrite_blocks_from_ssa(blocks, func_name, mapping);

    normalize_names(rewritten);

    auto out_instrs = flatten_blocks(rewritten);
    func["instrs"] = out_instrs;
    return func;
}

json from_ssa(json bril)
{
    for (auto &func : bril["functions"])
    {
        func = func_from_ssa(func);
    }
    return bril;
}

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cerr << "Reading SSA program from stdin...\n";
    std::string input((std::istreambuf_iterator<char>(cin)), std::istreambuf_iterator<char>());
    json bril = json::parse(input);

    cerr << "Parsed!\n";

    bril = from_ssa(bril);
    cout << bril.dump(2) << "\n";

    return 0;
}
