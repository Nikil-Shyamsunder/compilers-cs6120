#include "common.hpp"
// #include "dominator_util.h"
#include <iostream>
#include <queue>
#include <functional>
#include <tuple>
using namespace std;

unordered_map<string, unordered_set<string>> dominators(string func_name, vector<vector<json>> blocks)
{
    unordered_map<string, unordered_set<string>> dom;
    cfg_info cfg = build_cfg(func_name, blocks);

    // get a list of all blocks to help with initialization
    unordered_set<string> all_blocks;
    for (size_t i = 0; i < blocks.size(); ++i)
    {
        string label = get_label(blocks, func_name, i);
        all_blocks.insert(label);
    }

    // initialize
    for (const auto &label : all_blocks)
    {
        dom[label] = all_blocks; // start with everything
    }

    auto entry = get_label(blocks, func_name, 0);
    dom[entry] = {entry};

    bool changed = true;
    while (changed)
    {
        changed = false;
        for (const auto &[label, preds] : cfg.predecessors)
        {
            if (label == entry)
                continue; // for every vertex except entry

            unordered_set<string> new_dom;
            // calculate ⋂(dom[p] for p in vertex.preds}
            if (!preds.empty())
            {
                new_dom = dom[*preds.begin()]; // copy first pred's dom set
                for (const auto &p : preds)
                {
                    if (p == *preds.begin())
                        continue;               // since this is already in new_dom
                    unordered_set<string> temp; // get intersection with new_dom and put it in temp
                    for (const auto &n : new_dom)
                    {
                        if (dom[p].count(n))
                        {
                            temp.insert(n);
                        }
                    }
                    new_dom = temp;
                }
            }
            new_dom.insert(label); // do the {vertex} ∪ ... part

            if (new_dom != dom[label])
            {
                dom[label] = new_dom;
                changed = true;
            }
        }
    }

    return dom;
}

unordered_map<string, unordered_set<string>> dominator_tree(unordered_map<string, unordered_set<string>> dominators)
{
    // invert the dominators map: for each block, which blocks does it dominate?
    // TODO: factor this out into a helper
    unordered_map<string, unordered_set<string>> dom_tree;
    unordered_map<string, unordered_set<string>> inv_dom;
    for (const auto &[k, vs] : dominators)
    {
        inv_dom[k] = {};
        dom_tree[k] = {};
    }
    for (const auto &[k, vs] : dominators)
    {
        for (const auto &v : vs)
        {
            inv_dom[v].insert(k);
        }
    }

    // Compute strict dominators: dom_inv_strict[a] = {b in inv_dom[a] | b != a}
    unordered_map<string, unordered_set<string>> dom_inv_strict;
    for (const auto &[a, bs] : inv_dom)
    {
        for (const auto &b : bs)
        {
            if (b != a)
            {
                dom_inv_strict[a].insert(b);
            }
        }
    }

    // dom_inv_strict_2x[a] = union of dom_inv_strict[b] for all b in dom_inv_strict[a]
    unordered_map<string, unordered_set<string>> dom_inv_strict_2x;
    for (const auto &[a, bs] : dom_inv_strict)
    {
        unordered_set<string> union_set;
        for (const auto &b : bs)
        {
            if (dom_inv_strict.find(b) != dom_inv_strict.end())
            {
                union_set.insert(dom_inv_strict[b].begin(), dom_inv_strict[b].end());
            }
        }
        dom_inv_strict_2x[a] = union_set;
    }

    // Result: a dominates b iff b in dom_inv_strict[a] and b not in dom_inv_strict_2x[a]
    for (const auto &[a, bs] : dom_inv_strict)
    {
        for (const auto &b : bs)
        {
            if (dom_inv_strict_2x[a].find(b) == dom_inv_strict_2x[a].end())
            {
                dom_tree[a].insert(b);
            }
        }
    }
    return dom_tree;
}

unordered_map<string, unordered_set<string>> dominance_frontier(unordered_map<string, unordered_set<string>> dominators, cfg_info cfg)
{
    map<string, vector<string>> succ = cfg.successors;

    // invert the dominators map: for each block, which blocks does it dominate?
    unordered_map<string, unordered_set<string>> df;
    unordered_map<string, unordered_set<string>> inv_dom;
    for (const auto &[k, vs] : dominators)
    {
        inv_dom[k] = {};
        df[k] = {};
    }
    for (const auto &[k, vs] : dominators)
    {
        for (const auto &v : vs)
        {
            inv_dom[v].insert(k);
        }
    }

    for (const auto &[k, vs] : inv_dom)
    {
        unordered_set<string> dom_succs;

        for (auto &v : vs)
        {
            for (auto &d : succ[v])
            {
                dom_succs.insert(d);
            }
        }

        for (const auto &b : dom_succs)
        {
            if (vs.find(b) == vs.end() || b == k)
            {
                df[k].insert(b);
            }
        }
        // # Find all successors of dominated blocks.
        // dominated_succs = set()
        // for dominated in dom_inv[block]:
        //     dominated_succs.update(succ[dominated])

        // # You're in the frontier if you're not strictly dominated by the
        // # current block.
        // frontiers[block] = [
        //     b for b in dominated_succs if b not in dom_inv[block] or b == block
        // ]
    }

    return df;
}

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <algorithm>
#include <functional>
#include <iterator>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

map<string, unordered_set<string>> compute_get_targets(
    const map<string, vector<json>> &blocks,
    const unordered_map<string, unordered_set<string>> &frontiers,
    const unordered_map<string, unordered_set<string>> &definitions)
{
    map<string, unordered_set<string>> need_get;
    for (const auto &[name, _] : blocks)
        need_get[name] = {};

    for (const auto &[var, def_blocks] : definitions)
    {
        std::vector<string> worklist(def_blocks.begin(), def_blocks.end());
        unordered_set<string> defsites(def_blocks.begin(), def_blocks.end());

        while (!worklist.empty())
        {
            string b = worklist.back();
            worklist.pop_back();

            auto it = frontiers.find(b);
            if (it == frontiers.end())
                continue;

            for (const auto &d : it->second)
            {
                if (!need_get[d].count(var))
                {
                    need_get[d].insert(var);

                    if (!defsites.count(d))
                    {
                        defsites.insert(d);
                        worklist.push_back(d);
                    }
                }
            }
        }
    }

    return need_get;
}

struct RenameInfo
{
    map<string, vector<tuple<string, string, string>>> sets; // block -> [(succ, old, val)]
    map<string, map<string, string>> get_targets;            // block -> {old : new}
    map<string, string> initial_values;                      // old -> init name
};

RenameInfo perform_ssa_renaming(
    map<string, vector<json>> &blocks,
    const map<string, unordered_set<string>> &need_get,
    const map<string, vector<string>> &succ,
    const unordered_map<string, unordered_set<string>> &dom_tree,
    const unordered_set<string> &args,
    const unordered_map<string, unordered_set<string>> &doms,
    const string &entry)
{
    unordered_map<string, vector<string>> name_stack;
    for (const auto &a : args)
        name_stack[a].push_back(a);

    map<string, map<string, string>> get_target_map;
    for (const auto &kv : blocks)
    {
        const string &b = kv.first;
        map<string, string> &dest_map = get_target_map[b];

        // OLD: if (auto it = need_get.find(b); it != need_get.end())
        auto it = need_get.find(b);
        if (it != need_get.end())
        {
            for (const auto &v : it->second)
                dest_map[v] = "";
        }
    }

    map<string, vector<tuple<string, string, string>>> outgoing_sets;
    for (const auto &kv : blocks)
        outgoing_sets[kv.first] = {};

    map<string, string> inits;
    unordered_map<string, int> version_ctr;

    auto new_name = [&](const string &v)
    {
        string n = v + "." + to_string(version_ctr[v]++);
        name_stack[v].insert(name_stack[v].begin(), n);
        return n;
    };

    auto current_name = [&](const string &v) -> string
    {
        if (!name_stack[v].empty())
            return name_stack[v][0];
        string init = v + ".init";
        inits[v] = init;
        return init;
    };

    function<void(const string &)> rename_block = [&](const string &bname)
    {
        cerr << "Renaming block " << bname << "\n";
        auto saved = name_stack;

        // OLD: if (auto it = need_get.find(bname); it != need_get.end())
        auto it_need = need_get.find(bname);
        if (it_need != need_get.end())
        {
            for (const auto &v : it_need->second)
                get_target_map[bname][v] = new_name(v);
        }

        // Rename args and dests
        for (auto &inst : blocks[bname])
        {
            if (inst.contains("args"))
            {
                for (auto &a : inst["args"])
                    if (a.is_string())
                        a = current_name(a.get<string>());
            }
            if (inst.contains("dest"))
                inst["dest"] = new_name(inst["dest"].get<string>());
        }

        // Add sets to successors
        // OLD: if (auto it = succ.find(bname); it != succ.end())
        auto it_succ = succ.find(bname);
        if (it_succ != succ.end())
        {
            for (const auto &succ_block : it_succ->second)
            {
                // OLD: if (auto jt = need_get.find(succ_block); jt != need_get.end())
                auto jt = need_get.find(succ_block);
                if (jt != need_get.end())
                {
                    for (const auto &v : jt->second)
                        outgoing_sets[bname].push_back(
                            make_tuple(succ_block, v, current_name(v)));
                }
            }
        }

        // Recurse over dominator tree
        // OLD: if (auto it = dom_tree.find(bname); it != dom_tree.end())
        auto it_dom = dom_tree.find(bname);
        if (it_dom != dom_tree.end())
        {
            vector<string> kids(it_dom->second.begin(), it_dom->second.end());
            sort(kids.begin(), kids.end());
            for (const auto &c : kids)
                rename_block(c);
        }

        name_stack = saved;
    };

    rename_block(entry);
    return {outgoing_sets, get_target_map, inits};
}

// efficient to just inline this
static inline bool is_terminator(const json &i)
{
    if (!i.contains("op"))
        return false;
    string op = i["op"];
    return op == "br" || op == "jmp" || op == "ret";
}

void add_sets_and_gets(
    map<string, vector<json>> &blocks,
    const map<string, vector<tuple<string, string, string>>> &sets,
    const map<string, map<string, string>> &get_targets,
    const unordered_map<string, string> &types)
{
    for (auto &[b, instrs] : blocks)
    {
        // Insert SETs before terminators
        auto svec = sets.at(b);
        sort(svec.begin(), svec.end());
        size_t insert_pos = instrs.empty() ? 0 : instrs.size();
        if (!instrs.empty() && is_terminator(instrs.back()))
            insert_pos = instrs.size() - 1;

        for (const auto &[succ, var, val] : svec)
        {
            auto s_it = get_targets.find(succ);
            if (s_it == get_targets.end())
                continue;
            const auto &mapping = s_it->second;
            auto m_it = mapping.find(var);
            if (m_it == mapping.end() || m_it->second.empty())
                continue;

            json inst;
            inst["op"] = "set";
            inst["args"] = {m_it->second, val};
            instrs.insert(instrs.begin() + insert_pos, inst);
            ++insert_pos;
        }

        // Insert GETs at the top
        size_t top = (!instrs.empty() && instrs.front().contains("label")) ? 1 : 0;
        vector<pair<string, string>> gvec(get_targets.at(b).begin(), get_targets.at(b).end());
        sort(gvec.begin(), gvec.end());
        for (const auto &[oldv, newv] : gvec)
        {
            if (newv.empty())
                continue;
            json g;
            g["op"] = "get";
            g["dest"] = newv;
            g["type"] = types.at(oldv);
            instrs.insert(instrs.begin() + top, g);
            ++top;
        }
    }
}

void add_undef_inits(
    vector<json> &entry_block,
    const map<string, string> &inits,
    const unordered_map<string, string> &types)
{
    vector<pair<string, string>> sorted(inits.begin(), inits.end());
    sort(sorted.begin(), sorted.end());
    for (const auto &[orig, name] : sorted)
    {
        json u;
        u["op"] = "undef";
        u["type"] = types.at(orig);
        u["dest"] = name;
        entry_block.insert(entry_block.begin(), u);
    }
}

unordered_map<string, string> collect_types(const json &func)
{
    unordered_map<string, string> t;
    if (func.contains("args"))
        for (const auto &a : func["args"])
            t[a["name"].get<string>()] = a["type"].get<string>();

    if (func.contains("instrs"))
        for (const auto &i : func["instrs"])
            if (i.contains("dest"))
                t[i["dest"].get<string>()] = i["type"].get<string>();
    return t;
}

json convert_func_to_ssa(json func)
{
    string fname = func["name"];
    auto blocks_vec = add_entry(gen_basic_blocks(func), fname);

    map<string, vector<json>> blocks;
    vector<string> order;
    for (size_t i = 0; i < blocks_vec.size(); ++i)
    {
        string lbl = get_label(blocks_vec, fname, i);
        blocks[lbl] = blocks_vec[i];
        order.push_back(lbl);
    }

    auto cfg = build_cfg(fname, blocks_vec);
    auto doms = dominators(fname, blocks_vec);
    auto dom_tree = dominator_tree(doms);
    auto frontiers = dominance_frontier(doms, cfg);
    auto defs = def_blocks(blocks);
    auto types = collect_types(func);

    unordered_set<string> arg_names;
    if (func.contains("args"))
        for (const auto &a : func["args"])
            arg_names.insert(a["name"].get<string>());

    auto need_get = compute_get_targets(blocks, frontiers, defs);
    auto [sets, gets, inits] =
        perform_ssa_renaming(blocks, need_get, cfg.successors, dom_tree, arg_names, doms, order[0]);

    add_sets_and_gets(blocks, sets, gets, types);
    add_undef_inits(blocks[order[0]], inits, types);

    vector<json> merged;
    for (auto &lbl : order)
        for (auto &i : blocks[lbl])
            merged.push_back(i);

    func["instrs"] = merged;
    return func;
}

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cerr << "Reading program...\n";
    string input((istreambuf_iterator<char>(cin)), {});
    json bril = json::parse(input);

    cerr << "Transforming to SSA...\n";
    json new_funcs = json::array();
    for (auto &f : bril["functions"])
    {
        f = convert_func_to_ssa(f);
        new_funcs.push_back(f);
    }
    bril["functions"] = new_funcs;

    cout << bril.dump(2) << "\n";
    return 0;
}
