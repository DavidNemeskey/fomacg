#include "rule_condition_tree.h"

#include <stdlib.h>
#include <string.h>

// TODO: to parameter
#define MAX_STATES 1000

/**
 * Comparison function for qsort() that sorts the rules first by section, then
 * by size.
 */
static int rule_compare(const void* rule1, const void* rule2) {
  struct cg_rules* r1 = *((struct cg_rules **)rule1);
  struct cg_rules* r2 = *((struct cg_rules **)rule2);
  if (r1->section_no < r2->section_no) {
    return -1;
  } else if (r1->section_no > r2->section_no) {
    return 1;
  } else {
    if (r1->conditions->statecount < r2->conditions->statecount) {
      return -1;
    } else if (r1->conditions->statecount > r2->conditions->statecount) {
      return 1;
    } else {
      return 0;
    }
  }
}

/** Comparison function for qsort() that sorts the tree(-node)s by size. */
static int tree_compare(const void* tree1, const void* tree2) {
  struct Node* t1 = *((struct Node**) tree1);
  struct Node* t2 = *((struct Node**) tree2);
  /* NULL goes to the end. */
  if (t1 == NULL) {
    return t2 != NULL ? 1 : 0;
  } else {
    if (t2 == NULL) {
      return -1;
    } else {
      if (t1->fsa.fst->statecount < t2->fsa.fst->statecount) {
        return -1;
      } else if (t1->fsa.fst->statecount > t2->fsa.fst->statecount) {
        return 1;
      } else {
        return 0;
      }
    }
  }
}

/**
 * Builds the binary tree for the current section.
 * @param rules the rules array.
 * @paran begin the index of the first rule in the section.
 * @param length the length of the section.
 * @return the tree.
 */
static struct Node* build_section_tree(struct cg_rules* rules[],
                                       size_t begin, size_t length) {
  fprintf(stderr, "build_section_tree(%zu, %zu)\n", begin, length);
  struct Node** trees = (struct Node**)calloc(length, sizeof(struct Node*));
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

  int trees_still = length;
  /** And now: smallest-first union. */
  while (trees_still > 1 && trees[0]->fsa.fst->statecount < MAX_STATES
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
    trees[0] = NULL;
    trees[1] = new_tree;
    qsort(trees, length, sizeof(struct Node*), tree_compare);
    trees_still--;
  }

  /* Finishing touches... */
  struct Node* ret = trees[0];
  for (i = 0; i < length && trees[i] != NULL; i++) {
    trees[i]->next = trees[i + 1];
    if (strncmp(trees[i]->fsa.fst->name, "C_", 2)) {
      snprintf(trees[i]->fsa.fst->name, 40, "S_%d_%zu", trees[i]->section, i);
    }
  }

  struct Node* p = trees[0];
  while (p != NULL) {
    fprintf(stderr, "Tree %s : %d (%zu)\n", p->fsa.fst->name, p->fsa.fst->statecount, p->no_rules);
    p = p->next;
  }

  free(trees);
  return ret;
}

/**
 * Called by create_binary_tree() to add the newly created tree group @p trees
 * to the returned set (which starts at @p ret and ends in @p last). Updates the
 * latter two.
 */
static void add_trees_to_ret(struct Node* trees,
                             struct Node** ret, struct Node** last) {
  if (*ret == NULL) {
    *ret = *last = trees;
    for (; (*last)->next != NULL; *last = (*last)->next);
  } else {
    (*last)->next = trees;
    for (; (*last)->next != NULL; *last = (*last)->next);
  }
}

struct Node* create_binary_tree(struct cg_rules* cg_rules, size_t no_rules) {
  struct cg_rules** rules = (struct cg_rules**)calloc(no_rules, sizeof(struct cg_rules*));
  struct Node* ret = NULL;
  struct Node* last = NULL;
  size_t i;

  for (i = 0; i < no_rules; i++) {
    rules[i] = cg_rules;
    cg_rules = cg_rules->next;
  }

  /* First, we sort the rules by section and size. */
  qsort(rules, no_rules, sizeof(struct cg_rules*), rule_compare);
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

  free(rules);
  return ret;
}

/******************************* Serialization ********************************/

/**
 * Counts the non-NULL struct fsm*'s in @p tree. When you call this func tion,
 * @p count should be @c 0.
 */
static size_t count_fsms(struct Node* tree, size_t count) {
  if (tree->fsa.fst != NULL) count++;
  if (tree->fst.fst != NULL) count++;
  if (tree->left != NULL) count = count_fsms(tree->left, count);
  if (tree->right != NULL) count = count_fsms(tree->right, count);
  if (tree->next != NULL) count = count_fsms(tree->next, count);
  return count;
}

/** Fills @p rules with the rules and conditions in @p tree. */
static size_t fill_fsms(struct Node* tree, struct fsm** rules, size_t i) {
  if (tree->fsa.fst != NULL) rules[i++] = tree->fsa.fst;
  if (tree->fst.fst != NULL) rules[i++] = tree->fst.fst;
  if (tree->left != NULL) i = fill_fsms(tree->left, rules, i);
  if (tree->right != NULL) i = fill_fsms(tree->right, rules, i);
  if (tree->next != NULL) i = fill_fsms(tree->next, rules, i);
  return i;
}

struct fsm** serialize_tree(struct Node* tree, size_t* num_rules) {
  *num_rules = count_fsms(tree, 0);
  struct fsm** rules = (struct fsm**)calloc(*num_rules, sizeof(struct fsm*));
  fill_fsms(tree, rules, 0);
  return rules;
}

/****************************** De-serialization ******************************/

/**
 * The recursive function that walks through @p rules and builds trees from it.
 *
 * @param rules the rule fsm array.
 * @param num_rules the total number of rules.
 * @param index the current index in @p rules.
 */
static struct Node* array_to_tree(const FstVector& rules, size_t* index) {
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

struct Node* deserialize_tree(const FstVector& rules) {
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

