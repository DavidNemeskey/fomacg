#include "rule_condition_tree.h"

#include <stdlib.h>
#include <string.h>

#include <algorithm>

SmallestFirstTreeMerger::SmallestFirstTreeMerger(int max_states) :
    max_states(max_states) {}

FixLevelTreeMerger::FixLevelTreeMerger(int levels) : levels(levels) {}

/********************************* Comparison *********************************/

bool ConditionMerger::rule_compare(const struct cg_rules* rule1,
                                   const struct cg_rules* rule2) {
  if (rule1->section_no < rule2->section_no) {
    return true;
  } else if (rule1->section_no > rule2->section_no) {
    return false;
  } else {
    if (rule1->conditions->statecount < rule2->conditions->statecount) {
      return true;
    } else{
      return false;
    }
  }
}

bool TreeConditionMerger::tree_compare(const struct Node* tree1,
                                       const struct Node* tree2) {
  return tree1->fsa.fst->statecount < tree2->fsa.fst->statecount;
}

/********************************** Merging ***********************************/

struct Node* SmallestFirstTreeMerger::build_section_tree(
    std::vector<struct cg_rules*>& rules, size_t begin, size_t length) {
  fprintf(stderr, "build_section_tree(%zu, %zu)\n", begin, length);
  std::deque<struct Node*> trees(length);
  /* Let's fill the array. */
  size_t i;
  for (i = 0; i < length; i++) {
    int rule = begin + i;
    trees[i] = (struct Node*)calloc(1, sizeof(struct Node));
    trees[i]->fsa.fst = rules[rule]->conditions;
    trees[i]->fst.fst = rules[rule]->rule;
    trees[i]->section = rules[rule]->section_no;
    trees[i]->no_rules = 1;
    fprintf(stderr, "section %s : %d\n", rules[rule]->conditions->name, rules[rule]->section_no);
  }

  /** And now: smallest-first union. */
  while (trees.size() > 1 && trees[0]->fsa.fst->statecount < MAX_STATES
                          && trees[1]->fsa.fst->statecount < MAX_STATES) {
    struct Node* new_tree = (struct Node*)calloc(1, sizeof(struct Node));
    new_tree->fsa.fst = fsm_topsort(
                          fsm_minimize(
                            fsm_determinize(
                              fsm_union(
                                fsm_copy(trees[0]->fsa.fst),
                                fsm_copy(trees[1]->fsa.fst)))));
    fsm_sort_arcs(new_tree->fsa.fst, 1);
    sprintf(new_tree->fsa.fst->name, "union");
    new_tree->left  = trees[0];
    new_tree->right = trees[1];
    new_tree->section = trees[0]->section;
    new_tree->no_rules = trees[0]->no_rules + trees[1]->no_rules;
    trees[1] = new_tree;
    trees.pop_front();
    std::sort(trees.begin(), trees.end(), tree_compare);
  }

  /* Finishing touches... */
  struct Node* ret = trees[0];
  for (i = 0; i < trees.size(); i++) {
    if (i < trees.size() - 1) {
      trees[i]->next = trees[i + 1];
    }
    if (strncmp(trees[i]->fsa.fst->name, "C_", 2)) {
      snprintf(trees[i]->fsa.fst->name, 40, "S_%d_%zu", trees[i]->section, i);
    }
  }

  struct Node* p = trees[0];
  while (p != NULL) {
    fprintf(stderr, "Tree %s : %d (%zu)\n", p->fsa.fst->name, p->fsa.fst->statecount, p->no_rules);
    p = p->next;
  }

  return ret;
}

struct Node* FixLevelTreeMerger::build_section_tree(
    std::vector<struct cg_rules*>& rules, size_t begin, size_t length) {
  fprintf(stderr, "build_section_tree(%zu, %zu)\n", begin, length);
  std::deque<struct Node*> trees(length);
  /* Let's fill the array. */
  for (size_t i = 0; i < length; i++) {
    int rule = begin + i;
    trees[i] = (struct Node*)calloc(1, sizeof(struct Node));
    trees[i]->fsa.fst = rules[rule]->conditions;
    trees[i]->fst.fst = rules[rule]->rule;
    trees[i]->section = rules[rule]->section_no;
    trees[i]->no_rules = 1;
    fprintf(stderr, "section %s : %d\n", rules[rule]->conditions->name, rules[rule]->section_no);
  }

  for (int curr_level = 1; trees.size() > 2 && curr_level < levels;
         curr_level++) {
//    fprintf(stderr, "curr level: %d, levels: %d\n", curr_level, levels);
    /* Union trees in the first half of the array with those in the second. */
    for (size_t i = 0; i < trees.size() - 1; i++) {
//      fprintf(stderr, "Merging %s(%zu) - %d & %s(%zu) - %d ...\n",
//          trees[i]->fsa.fst->name, i, trees[i]->fsa.fst->statecount,
//          trees[trees.size() - 1]->fsa.fst->name, trees.size() - 1,
//          trees[trees.size() - 1]->fsa.fst->statecount);
      struct Node* new_tree = (struct Node*)calloc(1, sizeof(struct Node));
      new_tree->fsa.fst = fsm_topsort(
                            fsm_minimize(
                              fsm_determinize(
                                fsm_union(
                                  fsm_copy(trees[i]->fsa.fst),
                                  fsm_copy(trees.back()->fsa.fst)))));
      fsm_sort_arcs(new_tree->fsa.fst, 1);
      sprintf(new_tree->fsa.fst->name, "union");
      new_tree->left  = trees[i];
      new_tree->right = trees[trees.size() - 1];
      new_tree->section = trees[i]->section;
      new_tree->no_rules = trees[i]->no_rules + trees.back()->no_rules;
      trees[i] = new_tree;
      trees.pop_back();
    }
    std::sort(trees.begin(), trees.end(), tree_compare);
  }

  /* Finishing touches... */
  struct Node* ret = trees[0];
  for (size_t i = 0; i < trees.size(); i++) {
    if (i < trees.size() - 1) {
      trees[i]->next = trees[i + 1];
    }
    if (strncmp(trees[i]->fsa.fst->name, "C_", 2)) {
      snprintf(trees[i]->fsa.fst->name, 40, "S_%d_%zu", trees[i]->section, i);
    }
  }

  struct Node* p = trees[0];
  while (p != NULL) {
    fprintf(stderr, "Tree %s : %d (%zu)\n", p->fsa.fst->name, p->fsa.fst->statecount, p->no_rules);
    p = p->next;
  }

  return ret;
}

void TreeConditionMerger::add_trees_to_ret(struct Node* trees,
                                           struct Node** ret,
                                           struct Node** last) {
  if (*ret == NULL) {
    *ret = *last = trees;
    for (; (*last)->next != NULL; *last = (*last)->next);
  } else {
    (*last)->next = trees;
    for (; (*last)->next != NULL; *last = (*last)->next);
  }
}

struct Node* TreeConditionMerger::merge(struct cg_rules* cg_rules,
                                        size_t no_rules) {
  std::vector<struct cg_rules*> rules(no_rules);
  struct Node* ret = NULL;
  struct Node* last = NULL;
  size_t i;

  for (i = 0; i < no_rules; i++) {
    rules[i] = cg_rules;
    cg_rules = cg_rules->next;
  }

  /* First, we sort the rules by section and size. */
  std::sort(rules.begin(), rules.end(), rule_compare);
  /* XXX */
  for (i = 0; i < no_rules; i++) {
    fprintf(stderr, "Rule %s : %d\n", rules[i]->conditions->name, rules[i]->conditions->statecount);
  }
  fprintf(stderr, "Total number of rules: %zu\n", no_rules);

  /* Then, we build the tree groups section-by-section. */
  int curr_section = rules[0]->section_no;
  int section_first = 0;

  for (i = 0; i < no_rules; i++) {
    if (rules[i]->section_no != curr_section) {
      struct Node* trees = build_section_tree(rules, section_first,
                                              i - section_first);
      add_trees_to_ret(trees, &ret, &last);
      curr_section = rules[i]->section_no;
      section_first = i;
    }
  }
  struct Node* trees = build_section_tree(rules, section_first,
                                          i - section_first);
  add_trees_to_ret(trees, &ret, &last);

  return ret;
}

/******************************* Serialization ********************************/

size_t TreeConditionMerger::count_fsms(struct Node* tree, size_t count) {
  if (tree->fsa.fst != NULL) count++;
  if (tree->fst.fst != NULL) count++;
  if (tree->left != NULL) count = count_fsms(tree->left, count);
  if (tree->right != NULL) count = count_fsms(tree->right, count);
  if (tree->next != NULL) count = count_fsms(tree->next, count);
  return count;
}

size_t TreeConditionMerger::fill_fsms(struct Node* tree, struct fsm** rules,
                                      size_t i) {
  if (tree->fsa.fst != NULL) rules[i++] = tree->fsa.fst;
  if (tree->fst.fst != NULL) rules[i++] = tree->fst.fst;
  if (tree->left != NULL) i = fill_fsms(tree->left, rules, i);
  if (tree->right != NULL) i = fill_fsms(tree->right, rules, i);
  if (tree->next != NULL) i = fill_fsms(tree->next, rules, i);
  return i;
}

struct fsm** TreeConditionMerger::serialize(struct Node* tree, size_t* num_rules) {
  *num_rules = count_fsms(tree);
  struct fsm** rules = (struct fsm**)calloc(*num_rules, sizeof(struct fsm*));
  fill_fsms(tree, rules);
  return rules;
}

/****************************** De-serialization ******************************/

struct Node* TreeConditionMerger::array_to_tree(const FstVector& rules,
                                                size_t* index) {
  struct Node* new_tree = (struct Node*)calloc(1, sizeof(struct Node));
  /* Leaf node. */
  if (!strncmp(rules[*index].fst->name, "C", 1)) {
    new_tree->no_rules = 1;
    new_tree->fsa = rules[(*index)++];
    new_tree->fst = rules[(*index)++];
  } else {
    new_tree->fsa = rules[(*index)++];
    struct Node* left_tree = array_to_tree(rules, index);
    struct Node* right_tree = array_to_tree(rules, index);
    new_tree->no_rules = left_tree->no_rules + right_tree->no_rules;
    new_tree->left  = left_tree;
    new_tree->right = right_tree;
  }
  return new_tree;
}

struct Node* TreeConditionMerger::deserialize(const FstVector& rules) {
  struct Node* ret = NULL;   // the returned tree
  struct Node* curr = NULL;  // the last tree in the chain
  size_t index = 0;
  while (index < rules.size()) {
    struct Node* new_tree = array_to_tree(rules, &index);
    if (ret == NULL) {
      ret = curr = new_tree;
    } else {
      curr->next = new_tree;
      curr = new_tree;
    }
  }
  return ret;
}

