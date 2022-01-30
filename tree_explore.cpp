#include <algorithm>
#include <unordered_map>
#include "util.h"

typedef struct TreeNode {
  int guess_idx;
  int cost;
  unordered_map<mask_t, TreeNode*> children;
  TreeNode(): guess_idx(-1), cost(0), children() {}
  TreeNode(int _guess_idx, int _cost): guess_idx(_guess_idx), cost(_cost), children() {}
} TreeNode;

unordered_map<solutions_container, TreeNode*, container_hash<solutions_container>> cache;

TreeNode* foo(const solutions_container& remaining, int depth = 0) {
  TreeNode *ret = new TreeNode();
  if (remaining.size() == 1) {
    ret->cost = 1;
    ret->guess_idx = remaining[0];
    return ret;
  }

  if (depth == 5) return nullptr;

  TreeNode best = { -1, 9999 };
  for (int guess_idx = 0; guess_idx < possibles.size(); guess_idx++) {
    pair<mask_t, solutions_container> groups[243];
    TreeNode current = { guess_idx, 0 };
    for (int i = 0; i < 243; i++) { groups[i].first = i; }
    for (auto const& soln : remaining) {
      if (soln == guess_idx) continue;
      mask_t mask = mask_lookup[guess_idx * solutions.size() + soln];
      groups[mask].second.push_back(soln);
    }
    sort(groups, groups + 243, [](const pair<mask_t, solutions_container>& a, const pair<mask_t, solutions_container>& b) -> bool {
      return a.second.size() > b.second.size();
    });

    for (auto const& group : groups) {
      if (group.second.empty() || current.cost > best.cost) break;
      TreeNode *child = foo(group.second, depth + 1);
      if (!child) break;
      current.cost += child->cost;
      current.children[group.first] = child;
    }
    
    if (current.cost < best.cost) best = current;
  }

  return ret;
}

int main() {
  init_words();
}