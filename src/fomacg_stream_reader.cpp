#include "fomacg_stream_reader.h"

#include <iostream>

StreamReader::StreamReader(FILE* ins) : ins(ins) {}

// TODO: return bool and use a param[out] for return value
// TODO: fgetwc_unlocked
std::wstring StreamReader::read_cohort() {
  wchar_t wc;
  bool in_cohort = false;
  bool escape = false;

  word_ss.str(L"");
  while ((wc = fgetwc(ins)) != WEOF) {
    if (!in_cohort) {             // between cohorts: skip everything
      if (wc == L'^') {
        word_ss << wc;
        in_cohort = true;
      }
    } else {                      // in a cohort
      if (escape) {
        word_ss << wc;
        escape = false;
      } else if (wc == L'\\') {
        escape = true;
      } else if (wc == L'$') {
        word_ss << wc;
        return word_ss.str();
      } else {
        if (!iswspace(wc)) {  // Might not be needed, or not everywhere
          word_ss << wc;
        }
      }
    }
  }  // while
  /* WEOF -- what to do, what to do? */
  return L"";
}

