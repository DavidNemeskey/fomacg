#include "fomacg_stream_reader.h"

#include <cstdlib>
#include <iostream>

StreamReader::StreamReader(FILE* ins) : ins(ins), arr_size(1000) {
  arr = new wchar_t[arr_size];
}
StreamReader::~StreamReader() {
  delete[] arr;
}

// TODO: return bool and use a param[out] for return value
std::wstring StreamReader::read_cohort() {
  wchar_t wc;
  bool in_cohort = false;
  bool escape = false;
  size_t i = 0;

  //while ((wc = fgetwc(ins)) != WEOF) {
  while ((wc = fgetwc_unlocked(ins)) != WEOF) {
    if (i == arr_size) {
      arr = static_cast<wchar_t*>(realloc(arr, 2 * arr_size * sizeof(wchar_t)));
      arr_size *= 2;
    }
    if (!in_cohort) {             // between cohorts: skip everything
      if (wc == L'^') {
        arr[i++] = wc;
        in_cohort = true;
      }
    } else {                      // in a cohort
      if (escape) {
        arr[i++] = wc;
        in_cohort = true;
        escape = false;
      } else if (wc == L'\\') {
        escape = true;
      } else if (wc == L'$') {
        arr[i++] = wc;
        in_cohort = true;
        return std::wstring(arr, i);
      } else {
        if (!iswspace(wc)) {  // Might not be needed, or not everywhere
          arr[i++] = wc;
        }
      }
    }
  }  // while
  /* WEOF -- what to do, what to do? */
  return L"";
}

