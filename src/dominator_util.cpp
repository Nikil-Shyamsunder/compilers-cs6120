#include "common.hpp"
#include <iostream>

using namespace std;

// dom = {every block -> all blocks}
// dom[entry] = {entry}
// while dom is still changing:
//     for vertex in CFG except entry:
//         dom[vertex] = {vertex} ∪ ⋂(dom[p] for p in vertex.preds}
// TODO: No optimization for reverse post-order CFG traversal
unordered_map<string, unordered_set<string>> dominators(string func_name, vector<vector<json>> blocks) {
    unordered_map<string, unordered_set<string>> dom;
    cfg_info cfg = build_cfg(func_name, blocks);

    // add dummy "entry"-labeled node to cfg, with back edge to first block in blocks
    // this is now done in basic block construction
    // string entry = "entry";
    // string first_block = get_label(blocks, func_name, 0);
    // cfg.predecessors[first_block].push_back(entry);
    // cfg.successors[entry] = {first_block};
    // cfg.predecessors[entry] = {};

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

    auto entry = get_label(blocks, func_name, 0);
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
// TODO: is there a more efficient impl
unordered_map<string, unordered_set<string>> dominator_tree(unordered_map<string, unordered_set<string>> dominators) {
    unordered_map<string, unordered_set<string>> dom_tree;
    for (const auto& [block, doms] : dominators) {
        // if (doms.size() <= 1) continue; // skip entry block or any block with no dominators (tree can't have self loops)

        // find immediate dominator
        string idom;
        for (const auto& candidate : doms) {
            // skip self - no self loops. this should cover the case of entry block or block whose only dominator is itself
            if (candidate == block) continue; 
            bool is_idom = true;
            for (const auto& other : doms) {
                if (other == block || other == candidate) continue;
                if (dominators[other].count(candidate)) {
                    is_idom = false;
                    break;
                }
            }
            if (is_idom) {
                idom = candidate;
                break;
            }
        }
        if (!idom.empty()) {
            dom_tree[idom].insert(block);
        }
    }
    return dom_tree;
}


// from ECE 6775 slides
// q in dominance frontier of p if:
// (1) p does NOT strictly dominate q 
// (2) p dominates some predecessor(s) of q
// If above two conditions hold, qÎDF(p)

unordered_map<string, unordered_set<string>> dominance_frontier(unordered_map<string, unordered_set<string>> dominators, unordered_map<string, unordered_set<string>> dom_tree, cfg_info cfg) {
    // init
    unordered_map<string, unordered_set<string>> dom_frontier;
    for (const auto& [node, _] : dominators) {
        dom_frontier[node] = {};
    }

    // reverse the dom tree for child -> parent lookups
    unordered_map<string, string> idom;
    for (const auto& [parent, children] : dom_tree) {
        for (const auto& child : children) {
            idom[child] = parent;
        }
    }

    for (const auto& [b, preds] : cfg.predecessors) {
        if (preds.size() < 2) continue;
        for (const auto& p : preds) {
            string runner = p;
            while (!runner.empty() && idom.find(b) != idom.end() && runner != idom[b]) {
                dom_frontier[runner].insert(b);
                if (idom.find(runner) == idom.end()) break;
                runner = idom[runner];
            }
        }
    }

    return dom_frontier;
}

// enumerate all paths from entry to target
// returns a vector of paths, each path is a vector<string> of block labels
void dfs_paths(const string& node,
               const string& target,
               map<string, vector<string>>& succ,
               vector<string>& path,
               vector<vector<string>>& all_paths,
               unordered_set<string>& visited) {
    if (visited.count(node)) return;

    path.push_back(node);
    if (node == target) {
        all_paths.push_back(path);
        path.pop_back();
        return;
    }

    visited.insert(node);
    for (const auto& nxt : succ[node]) {
        dfs_paths(nxt, target, succ, path, all_paths, visited);
    }
    visited.erase(node);

    path.pop_back();
}

// Naive check: does A dominate B?
bool dominates_naive(const string& A, const string& B,
                     const string& entry,
                     map<string, vector<string>>& succ) {
    if (A == B) return true;
    vector<vector<string>> all_paths;
    vector<string> path;
    unordered_set<string> visited;
    dfs_paths(entry, B, succ, path, all_paths, visited);

    if (all_paths.empty()) return false; // unreachable

    for (const auto& p : all_paths) {
        if (find(p.begin(), p.end(), A) == p.end()) {
            return false; // found a path to B that avoids A
        }
    }
    return true;
}


int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    std::string input((std::istreambuf_iterator<char>(cin)), std::istreambuf_iterator<char>());
    json bril = json::parse(input);

    json out_funcs = json::array();
    for (const auto& func : bril.at("functions")) {
        auto blocks = gen_basic_blocks(func);
        blocks.insert(blocks.begin(), { { { "label", "entry" } } }); // add entry block

        // cout << "Function: " << func["name"] << "\n";
        auto reaching_defs = dominators(func["name"], blocks); 

        // print out reaching definitions
        for (const auto& [label, dominators] : reaching_defs) {
            cout << "Block " << label << ":\n";
            for (const auto& dom : dominators) {
                cout << "  " << dom << "\n";
            }
        }
        cout << endl;

        // print out dom tree
        auto dom_tree = dominator_tree(reaching_defs);
        cout << "Dominator Tree:\n";
        for (const auto& [parent, children] : dom_tree) {
            cout << "  " << parent << " -> ";
            for (const auto& child : children) {
                cout << child << " ";
            }
            cout << "\n";
        }
        cout << endl;
        
        // print out dom frontier
        auto cfg = build_cfg(func["name"], blocks);
        auto dom_frontier = dominance_frontier(reaching_defs, dom_tree, cfg);
        cout << "Dominance Frontier:\n";
        for (const auto& [block, frontier] : dom_frontier) {
            cout << "  " << block << " -> ";
            for (const auto& f : frontier) {
                cout << f << " ";
            }
            cout << endl;
        }        

        // check correctness by naive path enumeration
        cout << "Naive Dom Check:\n";
        string entry = get_label(blocks, func["name"], 0);
        for (const auto& [B, doms] : reaching_defs) {
            for (const auto& A : doms) {
                bool ok = dominates_naive(A, B, entry, cfg.successors);
                if (!ok) {
                    cout << "  ERROR: " << A << " should not dominate " << B << "\n";
                }
                else {
                    cout << ".";
                }
            }
        }
        cout << "Dom check complete\n\n";
    }
}
