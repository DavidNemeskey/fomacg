#include "fomacg_rule_applier.h"

#include <cstdio>
#include <cstring>

#include <sstream>
#include <iostream>

#include "foma_extra.h"

const std::string RuleApplier::begin_cohort =
    std::string("$0$ \">>>\" #BOC# | #0# \">>>\" <>>>> | #EOC# ");

RuleApplier::RuleApplier() {}

RuleApplier RuleApplier::get(const std::string& fst_file)
    throw (std::invalid_argument, std::length_error) {
  RuleApplier ra;
  ra.load_file_tree(fst_file);
  ra.load_trie("trie.fst");
  return ra;
}

bool RuleApplier::is_delimiter(const std::string& cohort) const {
  return apply_detmin_fsa(delimiters.ah, cohort.c_str());
}

// TODO: move this to rule_condition_tree
FstPair* RuleApplier::find_rule(Node* rule,
                                const std::vector<Symbol>& split,
                                bool match) const {
//  fprintf(stderr, "Testing condition %s...\n", rule->fsa.fst->name);
  if (match || common_detmin_fsa(rule->fsa, split)) {
    if (rule->left == NULL) {  // Leaf node -- just return the rule
 //     fprintf(stderr, "Leaf rule: returning %s / %s...\n", rule->fsa.fst->name, rule->fst.fst->name);
      return &rule->fst;
    } else {                   // else we traverse down the tree
      FstPair* left = find_rule(rule->left, split);
      if (left != NULL) {
//        fprintf(stderr, "Found left: %s.\n", left->fst->name);
        return left;
      } else {
//        fprintf(stderr, "Not found, going right...\n");
        return find_rule(rule->right, split, true);
      }
    }
  } else {
//    fprintf(stderr, "Condition %s does not match.\n", rule->fsa.fst->name);
    return NULL;
  }
}

size_t RuleApplier::apply_rules(std::string& result,
                                const std::string& sentence) const {
  size_t applied = 0;
  /* Add the >>> and <<< tags. >>> comes in a separate cohort, while <<< is
   * appended to the tag list of the last cohort. It comes before the "| #EOC# "
   * at the end of the sentence (length = 8).
   */
  result = begin_cohort + sentence.substr(0, sentence.length() - 8) + "<<<<> " +
           sentence.substr(sentence.length() - 8);
//  fprintf(stderr, "Input: \n%s\n", sentence.c_str());

  /* The sentence split into symbols. */
  std::vector<Symbol> split = common_create_sigmatch(allsigma.ah, result);
  std::vector<Symbol> res_split;

  while (true) {
Continue:
    for (Node* rule = rules; rule != NULL; rule = rule->next) {
//      fprintf(stderr, "Trying rule %s...\n", rule->fsa.fst->name);
      FstPair* rule_pair = find_rule(rule, split);
      if (rule_pair != NULL) {
//        fprintf(stderr, "Rule found: %s\n", rule_pair->fst->name);
        if (common_apply_down(*rule_pair, split, res_split)) {
          split.swap(res_split);
        } else {
//          fprintf(stderr, "Rule %s failed.\n", rule_pair->fst->name);
        }
        res_split.clear();
//        fprintf(stderr, "Applied rule %s, result:\n%s\n",
//            rule_pair->fst->name, join_string(split).c_str());
        applied++;
        goto Continue;
      } else {
//          fprintf(stderr, "Couldn't do anything for >>>%s<<<\n", result.c_str());
      }
    }  // for rule
    break;
  }
  /* Return the resulting string without the >>> cohort and <<< tags. */
  std::ostringstream ss;
  for (size_t i = 0; i < split.size(); i++) {
    if (split[i].number == IDENTITY)
      ss << result.substr(split[i].pos, split[i].len);
    else
      ss << allsigma_sigma[split[i].number];
  }
  result = ss.str();
  result = result.erase(result.length() - 14, 6).substr(begin_cohort.length());
//  fprintf(stderr, "Output: %s\n", result.c_str());
  return applied;
}

void RuleApplier::load_trie(const std::string& fst_file) {
  trie = load_fst(fst_file);
}

void RuleApplier::load_file_tree(const std::string& fst_file) {
  RuleSetLoader loader(fst_file);  // throws exception

  loader.load_fst(delimiters);  // TODO: check return value
  loader.load_fst(allsigma);    // TODO: check return value

  /* Fill the universal sigma vector. */
  allsigma_sigma.resize(allsigma.ah->sigma_size);
  for (struct sigma* s = allsigma.fst->sigma; s != NULL; s = s->next) {
    allsigma_sigma[s->number] = s->symbol;
  }

  /* Now load all the rule conditions and FSTs ... */
  FstVector fsts;
  FstPair fst;
  while (loader.load_fst(fst)) {
    /* ... and the sigma mappings for the FST. */
    fst.fill_sigma(allsigma.ah->sigma_size);
    /* And now we can delete the sigma structures to save space. */
    fsm_sigma_destroy(fst.fst);
    apply_sigma_clear(fst.ah);
    fsts.push_back(fst);
  }

//  sleep(30);

//  std::cerr << "Num rules: " << fsts.size() << std::endl;
//  for (size_t i = 0; i < fsts.size(); i++) {
//    std::cerr << "Rule " << fsts[i].fst->name << " det: "
//              << fsts[i].fst->is_deterministic << " min: "
//              << fsts[i].fst->is_minimized << " eps: "
//              << fsts[i].fst->is_epsilon_free << " |sigma|: "
//              << fsts[i].sigma.size() << std::endl;
//  }

  SmallestFirstTreeMerger merger;
  rules = merger.deserialize(fsts);
}

RuleApplier::~RuleApplier() {
  delimiters.cleanup();
  allsigma.cleanup();
  free_nodes(rules);
}

//int main(int argc, char* argv[]) {
//  try {
//    RuleApplier::get(argv[1], ".");
//  } catch (std::exception& e) {
//    printf("Exception: %s\n", e.what());
//  }
//}
