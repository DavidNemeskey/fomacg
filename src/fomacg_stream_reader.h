#ifndef FOMACG_STREAM_READER_H
#define FOMACG_STREAM_READER_H

/**
 * Parses the Apertium stream format and returns the cohorts. Also contains
 * methods that convert between the wstring representation used by Apertium and
 * the utf-8 format used by foma.
 *
 * @author Dávid Márk Nemeskey
 */

#include <string>
#include <sstream>

class StreamReader {
public:
  /**
   * Constructor.
   * @param ins[in] the input stream.
   */
  StreamReader(FILE* ins);

  /** Reads a cohort from @c ins. */
  std::wstring read_cohort();

private:
  /** The input stream. */
  FILE* ins;
  /** The buffer used by read_cohort(). */
  std::wstringstream word_ss;
};

#endif
