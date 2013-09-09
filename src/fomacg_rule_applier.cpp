#include "fomacg_rule_applier.h"

#include <cstdio>
#include <cstring>

#include <sstream>
#include <iostream>

#include "fomacg_converter.h"
#include "foma_extra.h"

namespace {
  std::vector<std::string> split_string(std::string s, const char& delim) {
    std::vector<std::string> ret;
    size_t pos = 0;
    while (true) {
      size_t found = s.find(delim, pos);
      if (found != std::string::npos) {
        ret.push_back(s.substr(pos, found - pos + 1));
        pos = found + 1;
      } else {
        return ret;
      }
    }
  }
};

const std::string RuleApplier::begin_cohort =
    std::string("$0$ \">>>\" #BOC# | #0# \">>>\" <>>>> | #EOC# ");

RuleApplier::RuleApplier(Converter& converter, const std::string& fst_file)
    : converter(converter), fst_file(fst_file) {}

RuleApplier RuleApplier::get(Converter& converter, const std::string& fst_file)
    throw (std::invalid_argument, std::length_error) {
  RuleApplier ra(converter, fst_file);
  ra.load_file_tree();
  //ra.load_file();
  return ra;
}

bool RuleApplier::is_delimiter(const std::string& cohort) const {
  return apply_detmin_fsa(delimiters.ah, cohort.c_str());
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
  size_t old_length = result.length();

  while (true) {
Continue:
    for (size_t section = 0; section < sections.size(); section++) {
      for (size_t rule = 0; rule < sections[section].size(); rule++) {
//        fprintf(stderr, "Trying rule %s...\n", sections[section][rule].fst->name);
        char* fomacg_result = apply_down(sections[section][rule].ah,
                                         result.c_str());
        if (fomacg_result != NULL) {
          size_t new_length = strlen(fomacg_result);
          if (old_length != new_length) {
//          fprintf(stderr, "Applied rule %s, result:\n%s\n",
//              sections[section][rule].fst->name, fomacg_result);
            result = fomacg_result;
            old_length = new_length;
            applied++;
            goto Continue;
          } else {
//            fprintf(stderr, "Same: >>>%s<<<\n", sentence.c_str());
          }
        } else {
//          fprintf(stderr, "Couldn't do anything for >>>%s<<<\n", sentence.c_str());
        }
      }  // for rule
    }  // for section
    break;
  }
  /* Return the resulting string without the >>> cohort and <<< tags. */
  result = result.erase(result.length() - 14, 6).substr(begin_cohort.length());
//  fprintf(stderr, "Output: %s\n", result.c_str());
  return applied;
}

// TODO: move this to rule_condition_tree
FstPair* RuleApplier::find_rule(Node* rule, const std::string& sentence,
                                const std::vector<std::string>& split,
                                bool match) const {
//  fprintf(stderr, "Testing condition %s...\n", rule->fsa.fst->name);
  if (match || custom_detmin_fsa(rule->fsa.ah, split)) {
    if (rule->left == NULL) {  // Leaf node -- just return the rule
 //     fprintf(stderr, "Leaf rule: returning %s / %s...\n", rule->fsa.fst->name, rule->fst.fst->name);
      return &rule->fst;
    } else {                   // else we traverse down the tree
      FstPair* left = find_rule(rule->left, sentence, split);
      if (left != NULL) {
//        fprintf(stderr, "Found left: %s.\n", left->fst->name);
        return left;
      } else {
//        fprintf(stderr, "Not found, going right...\n");
        return find_rule(rule->right, sentence, split, true);
      }
    }
  } else {
//    fprintf(stderr, "Condition %s does not match.\n", rule->fsa.fst->name);
    return NULL;
  }
}

size_t RuleApplier::apply_rules2(std::string& result,
                                 const std::string& sentence) const {
  size_t applied = 0;
  /* Add the >>> and <<< tags. >>> comes in a separate cohort, while <<< is
   * appended to the tag list of the last cohort. It comes before the "| #EOC# "
   * at the end of the sentence (length = 8).
   */
  result = begin_cohort + sentence.substr(0, sentence.length() - 8) + "<<<<> " +
           sentence.substr(sentence.length() - 8);
//  fprintf(stderr, "Input: \n%s\n", sentence.c_str());

  while (true) {
Continue:
    /* The sentence split into symbols. */
    std::vector<std::string> split = split_string(result, ' ');

    for (Node* rule = rules; rule != NULL; rule = rule->next) {
//      fprintf(stderr, "Trying rule %s...\n", rule->fsa.fst->name);
//        char* fomacg_result = apply_down(sections[section][rule + 1].ah,
//                                         result.c_str());
//        if (fomacg_result != NULL) {
      FstPair* rule_pair = find_rule(rule, result, split);
      if (rule_pair != NULL) {
//        fprintf(stderr, "Rule found: %s\n", rule_pair->fst->name);
        char* fomacg_result = apply_down(rule_pair->ah, result.c_str());
//          fprintf(stderr, "Applied rule %s, result:\n%s\n",
//              rule_pair->fst->name, fomacg_result);
        result = fomacg_result;
        applied++;
        goto Continue;
      } else {
//          fprintf(stderr, "Couldn't do anything for >>>%s<<<\n", result.c_str());
      }
    }  // for rule
    break;
  }
  /* Return the resulting string without the >>> cohort and <<< tags. */
  result = result.erase(result.length() - 14, 6).substr(begin_cohort.length());
//  fprintf(stderr, "Output: %s\n", result.c_str());
  return applied;
}

size_t RuleApplier::apply_rules3(std::string& result,
                                 const std::string& sentence) const {
  size_t applied = 0;
  /* Add the >>> and <<< tags. >>> comes in a separate cohort, while <<< is
   * appended to the tag list of the last cohort. It comes before the "| #EOC# "
   * at the end of the sentence (length = 8).
   */
  result = begin_cohort + sentence.substr(0, sentence.length() - 8) + "<<<<> " +
           sentence.substr(sentence.length() - 8);
//  fprintf(stderr, "Input: \n%s\n", sentence.c_str());

  while (true) {
Continue:
    for (size_t section = 0; section < sections.size(); section++) {
      for (size_t rule = 0; rule < sections[section].size(); rule +=2) {
//        fprintf(stderr, "Trying rule %s...\n", sections[section][rule].fst->name);
//        char* fomacg_result = apply_down(sections[section][rule + 1].ah,
//                                         result.c_str());
//        if (fomacg_result != NULL) {
        if (apply_detmin_fsa(sections[section][rule + 1].ah, result.c_str())) {
          char* fomacg_result = apply_down(sections[section][rule].ah,
                                           result.c_str());
//          fprintf(stderr, "Applied rule %s, result:\n%s\n",
//              sections[section][rule].fst->name, fomacg_result);
          result = fomacg_result;
          applied++;
          goto Continue;
        } else {
//          fprintf(stderr, "Couldn't do anything for >>>%s<<<\n", sentence.c_str());
        }
      }  // for rule
    }  // for section
    break;
  }
  /* Return the resulting string without the >>> cohort and <<< tags. */
  result = result.erase(result.length() - 14, 6).substr(begin_cohort.length());
//  fprintf(stderr, "Output: %s\n", result.c_str());
  return applied;
}

// TODO: logging?
void RuleApplier::load_file() {
  size_t num_sections = 0;

  FstVector fsts = load_fsts(fst_file);
  delimiters = fsts[0];
  FstVector this_section;
  for (size_t i = 2; i < fsts.size(); i++) {
    if (!strcmp(fsts[i].fst->name, "---")) {
      if (this_section.size() > 0) {
        sections.push_back(this_section);
        num_sections++;
      }
      this_section = FstVector();
    } else {
      this_section.push_back(fsts[i]);
    }
  }
  if (this_section.size() > 0) {
    sections.push_back(this_section);
    num_sections++;
  }

  /* One last check: are the section numbers contiguous? */
  if (num_sections != sections.size()) {
    throw std::length_error("Section numbers are not contiguous. ");
  }
}

// TODO: logging?
void RuleApplier::load_file_tree() {
  FstVector fsts = load_fsts(fst_file);
  delimiters = fsts[0];
  fsts.pop_front();  // delimiters
  //fsts.pop_front();  // TODO: allsigma
  std::cerr << "Num rules: " << fsts.size() << std::endl;
  for (size_t i = 0; i < fsts.size(); i++) {
    std::cerr << "Rule " << fsts[i].fst->name << " det: "
              << fsts[i].fst->is_deterministic << " min: "
              << fsts[i].fst->is_minimized << " eps: "
              << fsts[i].fst->is_epsilon_free << std::endl;
  }

  SmallestFirstTreeMerger merger;
  rules = merger.deserialize(fsts);
}

RuleApplier::~RuleApplier() {
  delimiters.cleanup();
  for (size_t i = 0; i < sections.size(); i++) {
    for (size_t j = 0; j < sections[i].size(); j++) {
      sections[i][j].cleanup();
    }
  }
}

//int main(int argc, char* argv[]) {
//  try {
//    RuleApplier::get(argv[1], ".");
//  } catch (std::exception& e) {
//    printf("Exception: %s\n", e.what());
//  }
//}
