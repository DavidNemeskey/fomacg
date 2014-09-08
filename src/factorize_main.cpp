#include <iostream>
#include <sstream>
#include <fstream>

#include <fomalib.h>
#include <zlib.h>

#include "factorize.h"
#include "fomacg_common.h"
#include "foma_extra.h"

const std::string begin_cohort =
    std::string("$0$ \">>>\" #BOC# | #0# \">>>\" <>>>> | #EOC# ");

FstPair load_rule_fst(const std::string& fst_file) throw (std::invalid_argument) {
  fsm_read_binary_handle fsrh;
  if ((fsrh = fsm_read_binary_file_multiple_init(fst_file.c_str())) == NULL) {
    throw std::invalid_argument("Could not open FST file " + fst_file);
  }

  /* We only need the last net. */
  struct fsm* prev = NULL;
  struct fsm* net;
  while ((net = fsm_read_binary_file_multiple(fsrh)) != NULL) {
    if (prev != NULL) {
      fsm_destroy(prev);
    }
    prev = net;
    /* Should be sorted already by fomacg. */
    if (!net->arcs_sorted_in) {
      fsm_sort_arcs(net, 1);
    }
  }

  struct apply_handle* ah = apply_init(prev);
  if (ah == NULL) {
    throw std::runtime_error("Failed to initialize apply handle for FST " +
                             std::string(prev->name));
    fsm_destroy(prev);
  }
  return FstPair(prev, ah);
}

std::string load_sentence(const std::string& text_file) throw (std::invalid_argument) {
  std::string line;
  std::ifstream myfile(text_file);
  std::stringstream ss;
  if (myfile.is_open()) {
    while (getline(myfile,line)) {
      ss << line;
    }
    return ss.str();
  } else {
    throw std::invalid_argument("Could not open sentence file " + text_file);
  }
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

void print_fst(struct fsm* fst) {
  std::cout << "Name: " << fst->name << std::endl;
  std::cout << "arity: " << fst->arity << std::endl;
  std::cout << "arccount: " << fst->arccount << std::endl;
  std::cout << "statecount: " << fst->statecount << std::endl;
  std::cout << "linecount: " << fst->linecount << std::endl;
  std::cout << "finalcount: " << fst->finalcount << std::endl;
  std::cout << "pathcount: " << fst->pathcount << std::endl;
  std::cout << "is_deterministic: " << fst->is_deterministic << std::endl;
  std::cout << "is_pruned: " << fst->is_pruned << std::endl;
  std::cout << "is_minimized: " << fst->is_minimized << std::endl;
  std::cout << "is_epsilon_free: " << fst->is_epsilon_free << std::endl;
  std::cout << "is_loop_free: " << fst->is_loop_free << std::endl;
  std::cout << "is_completed: " << fst->is_completed << std::endl;
  std::cout << "arcs_sorted_in: " << fst->arcs_sorted_in << std::endl;
  std::cout << "arcs_sorted_out: " << fst->arcs_sorted_out << std::endl;
  std::map<short int, std::string> sigmas;
  for (struct sigma* sigma = fst->sigma; sigma != NULL; sigma = sigma->next) {
    sigmas[sigma->number] = sigma->symbol;
  }
  struct fsm_state* elem = fst->states;
  for (int i = 0; ; i++) {
    std::cout << "Elem:" << std::endl;
    std::cout << "  state_no: " << (elem + i)->state_no << std::endl;
    std::cout << "  in: " << (elem + i)->in << " -- "
              << sigmas[(elem + i)->in] << std::endl;
    std::cout << "  out: " << (elem + i)->out << " -- "
              << sigmas[(elem + i)->out] << std::endl;
    std::cout << "  target: " << (elem + i)->target << std::endl;
    std::cout << "  final_state: " << (char)((elem + i)->final_state + 48) << std::endl;
    std::cout << "  start_state: " << (char)((elem + i)->start_state + 48) << std::endl;
    if ((elem + i)->state_no == -1) break;
  }
  for (struct sigma* sigma = fst->sigma; sigma != NULL; sigma = sigma->next) {
    std::cout << "Sigma " << sigma->number << ": " << sigma->symbol << std::endl;
  }
}

/**
 * Prints the @p sentence according, transforming them to sigma symbols
 * according to @p sigmas.
 */
void print_sentence(const std::string& header,
                    std::vector<short int> sentence,
                    std::map<short int, std::string> sigmas) {
  std::cout << std::endl << std::endl << header << ": " << std::endl;
  for (size_t i = 0; i < sentence.size(); i++) {
    if (sentence[i] != 0)  // epsilon
      std::cout << sigmas[sentence[i]] << " ";
  }
  std::cout << std::endl;
}

/**
 * Reads a fomacg rule file (with a single rule) and a sentence file (single
 * sentence, in the Apertium stream format). Applies both the original FST
 * rule, as well as the bimachine and the left-right transducer rules on the
 * sentence.
 */
int main(int argc, char* argv[]) {
  if (argc < 4) {
    std::cout << "Usage: factorize <fomacg file> <converter fst> <sentence file>"
              << std::endl;
    exit(1);
  }

  FstPair fst;
  FstPair transformer;
  std::string sentence;
  try {
    fst = load_rule_fst(argv[1]);
    transformer = load_fst(argv[2]);
    sentence = load_sentence(argv[3]);
  } catch (std::exception& e) {
    std::cout << e.what() << std::endl;
    exit(2);
  }

  std::string fomacg_sent = apply_detmin_fst_down(transformer.ah, sentence.c_str());
  fomacg_sent = begin_cohort + fomacg_sent.substr(0, fomacg_sent.length() - 8) +
                "<<<<> " + fomacg_sent.substr(fomacg_sent.length() - 8);
  transformer.cleanup();
  std::cout << std::endl << std::endl << "Rule FST:" << std::endl;
  print_fst(fst.fst);

  /* The starting state should not be final. */
  struct fsm_state* curr_line;
  for (curr_line = fst.fst->states; curr_line->state_no == 0; curr_line++) {
    if (curr_line->final_state) {
      if (curr_line == fst.fst->states) fst.fst->finalcount--;
      curr_line->final_state = 0;
    }
  }

  struct fsm* A_1 = fsm_determinize(fsm_upper(fsm_copy(fst.fst)));
  struct fsm* A_2 = fsm_determinize(fsm_reverse(fsm_upper(fsm_copy(fst.fst))));
  snprintf(A_1->name, 40, "foma_A_1");
  snprintf(A_2->name, 40, "foma_A_2");

  struct fsm* upper = fsm_upper(fsm_copy(fst.fst));
  sprintf(upper->name, "upper");
  struct fsm* reversed = fsm_reverse(fsm_upper(fsm_copy(fst.fst)));
  sprintf(reversed->name, "reversed");

  std::stringstream ss;
  ss << "bi_" << argv[1];
  gzFile outfile = gzopen(ss.str().c_str(), "wb");
  foma_net_print(fst.fst, (gzFile*)outfile);
  foma_net_print(upper, (gzFile*)outfile);
  foma_net_print(reversed, (gzFile*)outfile);
  foma_net_print(A_1, (gzFile*)outfile);
  foma_net_print(A_2, (gzFile*)outfile);
  fsm_destroy(upper);
  fsm_destroy(reversed);

  fsm_destroy(A_1);
  fsm_destroy(A_2);

  std::cout << std::endl << std::endl << "Creating BI:" << std::endl;

  BiMachine bi(fst.fst);
  foma_net_print(bi.A_1, (gzFile*)outfile);
  foma_net_print(bi.A_2, (gzFile*)outfile);

  std::cout << std::endl << std::endl << "Creating LRS:" << std::endl;

  LeftRightSequential lrs(fst.fst, bi);
  foma_net_print(lrs.T_1, (gzFile*)outfile);
  foma_net_print(lrs.T_2, (gzFile*)outfile);
  gzclose(outfile);

  std::cout << std::endl << std::endl << "Creating LRS2:" << std::endl;

  LeftRightSequential* lrs2 = fst_to_left_right(fsm_copy(fst.fst));

//  std::cout << std::endl << std::endl << "fst" << std::endl << std::endl;
//  print_fst(fst);

  // TODO: this might not be the correct sigma
  std::map<short int, std::string> sigmas;
  std::map<std::string, short int> rsigmas;
  for (struct sigma* sigma = fst.fst->sigma; sigma != NULL; sigma = sigma->next) {
    sigmas[sigma->number] = sigma->symbol;
    rsigmas[sigma->symbol] = sigma->number;
  }

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

  std::cout << std::endl << std::endl << "Before" << std::endl;
  std::vector<short int> input = transform_sentence(fomacg_sent, rsigmas);
  std::cout << std::endl << "Fomacg input:" << std::endl << fomacg_sent << std::endl;
  print_sentence("Input", input, sigmas);

  std::vector<short int> output = bi.bi_apply_down(input);
  print_sentence("BI output", output, sigmas);

  output = lrs.ts_apply_down(input);
  print_sentence("LRS output", output, sigmas);

  output = lrs2->ts_apply_down(input);
  print_sentence("LRS2 output", output, sigmas);
  delete lrs2;

  std::cout << std::endl << std::endl << "Rule output: " << std::endl;
  std::cout << apply_down(fst.ah, fomacg_sent.c_str()) << std::endl;
  fst.cleanup();
}

/*
// '$0$ "<Hej>" #BOC# | #0# "hej" <ij> | #EOC# $0$ "<anyjuk>" #BOC# | #0# "anya" <n> <sg> <px3ps> <nom> | #0# "anyjuk" <n> <sg> <nom> | #EOC# $0$ "<,>" #BOC# | #0# "," <cm> | #EOC# $0$ "<az>" #BOC# | #0# "az" <det> <def> | #EOC# $0$ "<anyjuk>" #BOC# | #0# "anya" <n> <sg> <px3ps> <nom> | #0# "anyjuk" <n> <sg> <nom> | #EOC# $0$ "<jön>" #BOC# | #0# "jön" <vblex> <pres> <sg> <s3p> | #EOC# '
////  '$0$ "<Hello>" #BOC# | #0# "hello" <ij> | #EOC# $0$ "<kutya>" #BOC# | #0# "kutya" <adj> <nom> | #0# "kutya" <n> <nom> | #EOC# $0$ "<!>" #BOC# | #0# "!" <punct> | #EOC# '
//  */
