/**
 * Applies the transducers created by fomacg to the text.
 *
 * @author Dávid Márk Nemeskey
 */
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include "fomacg_converter.h"
#include "fomacg_stream_reader.h"
#include "fomacg_rule_applier.h"

#include <time.h>

void test_converter(Converter* conv) {
  std::string fomacg = conv->apertium_to_fomacg(
//      L"^Volt/van<vbser><past>/volt<n><sg><nom>$^ebed/eb<n><sg><px2ss><nom>$^?/?<sent>$");
      L"^Volt/van<vbser><past>/volt<n><sg><nom>$^kutyad/kutya<n><sg><px2ss><nom>/kutya<adj><sg><px2ss><nom>$^?/?<sent>$");
  if (fomacg != Converter::FAILED) {
    printf("fomacg: %s\n", fomacg.c_str());
//    struct fsm* fsa = conv->fomacg_to_fsa(fomacg);
//    fsm_write_binary_file(fsa, "AHA.fst");
    std::wstring apertium = conv->fomacg_to_apertium(fomacg);
    std::wcout << L"apertium: " << apertium << std::endl;
  } else {
    printf("NULL!\n");
  }
}

void test_reader(StreamReader& reader) {
  while (true) {
    std::wstring str = reader.read_cohort();
    if (str == L"") break;
    std::wcout << str << std::endl;
  }
}

void do_it(StreamReader& reader, Converter& conv, RuleApplier& applier) {
  std::stringstream sentence;
  while (true) {
    std::wstring cohort = reader.read_cohort();
    if (cohort == L"") break;
    std::string cohort_fomacg = conv.apertium_to_fomacg(cohort);
    // TODO: stop if == FAILED
    sentence << cohort_fomacg;
    if (applier.is_delimiter(cohort_fomacg)) {
      std::string result;
      applier.apply_rules2(result, sentence.str());
      std::wcout << conv.fomacg_to_apertium(result) << std::endl;
      //size_t applied = applier.apply_rules(result, sentence.str());
      //std::wcout << L"Applied " << applied << " rules to get: "
      //           << conv.fomacg_to_apertium(result) << std::endl;
      sentence.str("");
    }
  }
}

int main(int argc, char* argv[]) {
  /* Locale. */
  std::locale::global(std::locale(""));

  // TODO proper CLI parsing
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " grammar_file " << std::endl;
    exit(1);
  }
  char* grammar_file = argv[1];
  Converter* conv = Converter::get("data/apertium_to_fomacg.fst");
  if (conv == NULL) {
    perror("FST file could not be loaded.");
    exit(EXIT_FAILURE);
  }
  //test_converter(conv);
  StreamReader reader(stdin);
  //test_reader(reader);
  RuleApplier applier = RuleApplier::get(*conv, grammar_file);

  struct timespec start, end;
  clock_gettime(CLOCK_REALTIME, &start);
  do_it(reader, *conv, applier);
  clock_gettime(CLOCK_REALTIME, &end);
  double elapsed = end.tv_sec - start.tv_sec +
                   (end.tv_nsec - start.tv_nsec) / 1000000000.0;
  fprintf(stderr, "Total time: %lf\n", elapsed);
  delete conv;
}