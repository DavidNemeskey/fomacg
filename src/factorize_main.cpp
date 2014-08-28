#include <iostream>
#include <sstream>

#include <fomalib.h>
#include <zlib.h>

#include "factorize.h"

struct fsm* load_fst(const std::string& fst_file)
    throw (std::invalid_argument) {
  struct fsm* fst = fsm_read_binary_file(fst_file.c_str());
  if (fst == NULL) {
    throw std::invalid_argument("Could not load FST from " + fst_file);
  }
  return fst;
}

/**
 * Transforms the sentence from a string in fomacg's inner representation to
 * the respective list of symbols.
 */
std::vector<short int> transform_sentence(
    const std::string& sentence, std::map<std::string, short int> rsigmas) {
  std::vector<short int> ret;
  size_t pos = 0;
  while (pos <= sentence.length()) {
    size_t new_pos = sentence.find(' ', pos);
    if (new_pos == std::string::npos) {
      break;
    }
    std::cout << "Sigma: '" << sentence.substr(pos, new_pos - pos + 1) << "'" << std::endl;
    auto it = rsigmas.find(sentence.substr(pos, new_pos - pos + 1));
    ret.push_back(it != rsigmas.end() ? it->second : 2);
    pos = new_pos + 1;
  }
  return ret;
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cout << "Usage: factorize <FST file>" << std::endl;
    exit(1);
  }

  struct fsm* fst = NULL;
  try {
    fst = load_fst(argv[1]);
  } catch (std::exception& e) {
    std::cout << e.what() << std::endl;
    exit(2);
  }

  /* The starting state should not be final. */
  struct fsm_state* curr_line;
  for (curr_line = fst->states; curr_line->state_no == 0; curr_line++) {
    curr_line->final_state = 0;
  }
  fst->finalcount--;

  struct fsm* A_1 = fsm_determinize(fsm_upper(fsm_copy(fst)));
  struct fsm* A_2 = fsm_determinize(fsm_reverse(fsm_upper(fsm_copy(fst))));
  snprintf(A_1->name, 40, "foma_A_1");
  snprintf(A_2->name, 40, "foma_A_2");

  struct fsm* upper = fsm_upper(fsm_copy(fst));
  sprintf(upper->name, "upper");
  struct fsm* reversed = fsm_reverse(fsm_upper(fsm_copy(fst)));
  sprintf(reversed->name, "reversed");

  std::stringstream ss;
  ss << "bi_" << argv[1];
  gzFile outfile = gzopen(ss.str().c_str(), "wb");
  foma_net_print(fst, (gzFile*)outfile);
  foma_net_print(upper, (gzFile*)outfile);
  foma_net_print(reversed, (gzFile*)outfile);
  foma_net_print(A_1, (gzFile*)outfile);
  foma_net_print(A_2, (gzFile*)outfile);
  fsm_destroy(upper);
  fsm_destroy(reversed);

  fsm_destroy(A_1);
  fsm_destroy(A_2);

  BiMachine bi(fst);
  foma_net_print(bi.A_1, (gzFile*)outfile);
  foma_net_print(bi.A_2, (gzFile*)outfile);

  LeftRightSequential lrs(fst, bi);
  foma_net_print(lrs.T_1, (gzFile*)outfile);
  foma_net_print(lrs.T_2, (gzFile*)outfile);
  gzclose(outfile);

  LeftRightSequential* lrs2 = fst_to_left_right(fsm_copy(fst));

//  std::cout << std::endl << std::endl << "fst" << std::endl << std::endl;
//  print_fst(fst);

  // TODO: this might not be the correct sigma
  std::map<short int, std::string> sigmas;
  std::map<std::string, short int> rsigmas;
  for (struct sigma* sigma = fst->sigma; sigma != NULL; sigma = sigma->next) {
    sigmas[sigma->number] = sigma->symbol;
    rsigmas[sigma->symbol] = sigma->number;
  }
  fsm_destroy(fst);

//  std::cout << std::endl << std::endl << "Delta:" << std::endl;
//  for (std::map<BiTrans, short int>::const_iterator it = bi.delta.begin();
//       it != bi.delta.end(); ++it) {
//    std::cout << "(" << it->first.q_1 << ", " << it->first.a << "["
//              << sigmas[it->first.a] << "], " << it->first.q_2 << ") -> "
//              << it->second << "[" << sigmas[it->second] << "]" << std::endl;
//  }


// ^Hol/hol<adv><itg>$
// ^van/van<vbser><pres><sg><s3p><ind>$
// ^Jancsi/Jancsi<np><sg><nom>$
// ^?/?<sent>$
// ^Jancsi/Jancsi<np><sg><nom>$
// ^és/és<cnjcoo>$
// ^Mari/mari<adj><sg><nom>/mari<n><sg><nom>/Mari<np><sg><nom>$
// ^a/a<det><def>$
// ^kertben/kert<n><sg><ine>$
// ^vannak/van<vbser><pres><pl><s3p><ind>$
// ^./.<sent>$

  std::vector<short int> input = transform_sentence("$0$ \">>>\" #BOC# | #0# \">>>\" <>>>> | #EOC# $0$ \"<Jancsi>\" #BOC# | #0# \"Jancsi\" <np> <sg> <nom> | #EOC# $0$ \"<és>\" #BOC# | #0# \"és\" <cnjcoo> | #EOC# $0$ \"<Mari>\" #BOC# | #0# \"mari\" <adj> <sg> <nom> | #0# \"mari\" <n> <sg> <nom> | #0# \"Mari\" <np> <sg> <nom> | #EOC# $0$ \"<a>\" #BOC# | #0# \"a\" <det> <def> | #EOC# $0$ \"<kertben>\" #BOC# | #0# \"kert\" <n> <sg> <ine> | #EOC# $0$ \"<vannak>\" #BOC# | #0# \"van\" <vbser> <pres> <pl> <s3p> <ind> | #EOC# $0$ \"<.>\" #BOC# | #0# \".\" <sent> <<<<> | #EOC# ", rsigmas);

  std::cout << std::endl << "Input: ";
  for (size_t i = 0; i < input.size(); i++) {
    std::cout << sigmas[input[i]] << " ";
  }
  std::cout << std::endl;
  std::vector<short int> output = bi.bi_apply_down(input);
  std::cout << "Bi output: ";
  for (size_t i = 0; i < output.size(); i++) {
    if (output[i] != 0)  // epsilon
      std::cout << sigmas[output[i]] << " ";
  }
  std::cout << std::endl;
  output = lrs.ts_apply_down(input);
  std::cout << "Ts output: ";
  for (size_t i = 0; i < output.size(); i++) {
    if (output[i] != 0)  // epsilon
      std::cout << sigmas[output[i]] << " ";
  }
  std::cout << std::endl;
  output = lrs2->ts_apply_down(input);
  std::cout << "Ts output: ";
  for (size_t i = 0; i < output.size(); i++) {
    if (output[i] != 0)  // epsilon
      std::cout << sigmas[output[i]] << " ";
  }
  std::cout << std::endl;
  delete lrs2;

//  input.clear();
//  input.push_back(6);
//  input.push_back(3);
//  input.push_back(10);
//  input.push_back(9);
//  input.push_back(11);
//  std::cout << std::endl << "Input: ";
//  for (size_t i = 0; i < input.size(); i++) {
//    std::cout << sigmas[input[i]] << " ";
//  }
//  std::cout << std::endl;
//  output = bi.bi_apply_down(input);
//  std::cout << "Bi output: ";
//  for (size_t i = 0; i < output.size(); i++) {
//    std::cout << sigmas[output[i]] << " ";
//  }
//  std::cout << std::endl;
//  output = lrs.ts_apply_down(input);
//  std::cout << "Ts output: ";
//  for (size_t i = 0; i < output.size(); i++) {
//    std::cout << sigmas[output[i]] << " ";
//  }
//  std::cout << std::endl;
}

/*
// '$0$ "<Hej>" #BOC# | #0# "hej" <ij> | #EOC# $0$ "<anyjuk>" #BOC# | #0# "anya" <n> <sg> <px3ps> <nom> | #0# "anyjuk" <n> <sg> <nom> | #EOC# $0$ "<,>" #BOC# | #0# "," <cm> | #EOC# $0$ "<az>" #BOC# | #0# "az" <det> <def> | #EOC# $0$ "<anyjuk>" #BOC# | #0# "anya" <n> <sg> <px3ps> <nom> | #0# "anyjuk" <n> <sg> <nom> | #EOC# $0$ "<jön>" #BOC# | #0# "jön" <vblex> <pres> <sg> <s3p> | #EOC# '
  input.push_back(10);
  input.push_back(2);
  input.push_back(7);
  input.push_back(29);
  input.push_back(3);
  input.push_back(2);
  input.push_back(91);
  input.push_back(29);
  input.push_back(8);
  input.push_back(10);
  input.push_back(2);
  input.push_back(7);
  input.push_back(29);
  input.push_back(3);
  input.push_back(2);
  input.push_back(2);
  input.push_back(2);
  input.push_back(2);
  input.push_back(2);
  input.push_back(29);
  input.push_back(3);
  input.push_back(90);
  input.push_back(2);
  input.push_back(2);
  input.push_back(2);
  input.push_back(29);
  input.push_back(8);
  input.push_back(10);
  input.push_back(2);
  input.push_back(7);
  input.push_back(29);
  input.push_back(3);
  input.push_back(2);
  input.push_back(44);
  input.push_back(29);
  input.push_back(8);
  input.push_back(10);
  input.push_back(2);
  input.push_back(7);
  input.push_back(29);
  input.push_back(3);
  input.push_back(2);
  input.push_back(2);
  input.push_back(2);
  input.push_back(29);
  input.push_back(8);
  input.push_back(10);
  input.push_back(2);
  input.push_back(7);
  input.push_back(29);
  input.push_back(3);
  input.push_back(2);
  input.push_back(2);
  input.push_back(2);
  input.push_back(2);
  input.push_back(2);
  input.push_back(29);
  input.push_back(3);
  input.push_back(90);
  input.push_back(2);
  input.push_back(2);
  input.push_back(2);
  input.push_back(29);
  input.push_back(8);
  input.push_back(10);
  input.push_back(2);
  input.push_back(7);
  input.push_back(29);
  input.push_back(3);
  input.push_back(2);
  input.push_back(2);
  input.push_back(2);
  input.push_back(2);
  input.push_back(2);
  input.push_back(29);
  input.push_back(8);
////  '$0$ "<Hello>" #BOC# | #0# "hello" <ij> | #EOC# $0$ "<kutya>" #BOC# | #0# "kutya" <adj> <nom> | #0# "kutya" <n> <nom> | #EOC# $0$ "<!>" #BOC# | #0# "!" <punct> | #EOC# '
//  input.push_back(10);
//  input.push_back(2);
//  input.push_back(7);
//  input.push_back(29);
//  input.push_back(3);
//  input.push_back(2);
//  input.push_back(2);
//  input.push_back(29);
//  input.push_back(8);
//  input.push_back(10);
//  input.push_back(2);
//  input.push_back(7);
//  input.push_back(29);
//  input.push_back(3);
//  input.push_back(83);
//  input.push_back(2);
//  input.push_back(2);
//  input.push_back(29);
//  input.push_back(3);
//  input.push_back(83);
//  input.push_back(15);
//  input.push_back(2);
//  input.push_back(29);
//  input.push_back(8);
//  input.push_back(10);
//  input.push_back(2);
//  input.push_back(7);
//  input.push_back(29);
//  input.push_back(3);
//  input.push_back(15);
//  input.push_back(2);
//  input.push_back(29);
//  input.push_back(8);
//  */
