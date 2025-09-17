#include "common.hpp"
#include <iostream>

using namespace std;

static std::vector<json> local_tdce(const std::vector<json>& block) {
    enum { UNSEEN = 0, LATER_DEF_NO_USE = 1, USED_SINCE = 2 };
    std::unordered_map<std::string, int> state;

    std::vector<json> out_rev;
    out_rev.reserve(block.size());

    for (int i = (int)block.size() - 1; i >= 0; --i) {
        const json& instr = block[i];
        bool keep = true;

        if (has_dest(instr)) {
            const std::string x = instr["dest"].get<std::string>();
            auto it = state.find(x);

            // if we've seen a later def with no intervening use, this def is dead
            if (it != state.end() && it->second == LATER_DEF_NO_USE) keep = false;
            if (keep) state[x] = LATER_DEF_NO_USE;
        }

        if (keep) {
            for (const auto& a : get_args(instr)) state[a] = USED_SINCE;
            out_rev.push_back(instr);
        }
    }

    std::reverse(out_rev.begin(), out_rev.end());
    return out_rev;
}

static bool drop_globally_unused_once(json& func) {
    std::unordered_set<std::string> used;
    for (const auto& instr : func["instrs"]) {
        for (const auto& a : get_args(instr)) used.insert(a);
    }

    std::vector<json> filtered;
    filtered.reserve(func["instrs"].size());
    for (const auto& instr : func["instrs"]) {
        if (has_dest(instr)) {
            const std::string d = instr["dest"].get<std::string>();
            if (used.find(d) == used.end()) continue; // prune dead assign
        }
        filtered.push_back(instr);
    }

    bool changed = (filtered.size() != func["instrs"].size());
    func["instrs"] = std::move(filtered);
    return changed;
}

static void optimize_globally_unused_vars(json& func) {
    while (drop_globally_unused_once(func)) { /* iterate to fixpoint */ }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // Read stdin
    std::string input((std::istreambuf_iterator<char>(cin)), std::istreambuf_iterator<char>());
    json bril = json::parse(input);

    json out_funcs = json::array();
    for (const auto& func : bril.at("functions")) {
        json opt_func = func;

        // global pass
        optimize_globally_unused_vars(opt_func);

        // gen basic blocks + local pass
        auto blocks = gen_basic_blocks(opt_func);
        std::vector<std::vector<json>> tdce_blocks;
        tdce_blocks.reserve(blocks.size());
        for (const auto& b : blocks) tdce_blocks.push_back(local_tdce(b));

        json new_func = func;
        replace_func_instrs(new_func, tdce_blocks);
        out_funcs.push_back(std::move(new_func));
    }

    bril["functions"] = std::move(out_funcs);
    cout << bril.dump() << "\n";
    return 0;
}
