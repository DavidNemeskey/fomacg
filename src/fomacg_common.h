#ifndef FOMACG_COMMON_H
#define FOMACG_COMMON_H

/**
 * Defines data types and methods used in more than one source files in
 * fomacg_proc.
 */

#include <string>
#include <deque>
#include <set>
#include <vector>
#include <stdexcept>

#include "fomalib.h"

/**
 * All data we need for an FST: its struct and apply handle. The main goal of
 * this object is to keep data in one place for convenience. As such, it does
 * not manage the resources associated with the fst automatically; the user must
 * explicitly call cleanup() to free up the resources used.
 */
struct FstPair {
  struct fsm* fst;
  struct apply_handle* ah;
  /** The sigma of @c fst. Not computed automatically, as not always needed. */
  std::vector<int> sigma2;
  /* Unordered set at this small element size is SLOWER than the regular set. */
  std::set<int> sigma;

  FstPair();
  FstPair(struct fsm* fst, struct apply_handle* ah);

  /** Fills the sigma set. */
  void fill_sigma();
  void fill_sigma2(size_t sigma_size);
  /** Frees all resources associated with this FST. */
  void cleanup();
};

/** A vector of FSTs with their apply handle. */
typedef std::deque<FstPair> FstVector;

/**
 * Loads an FST from a file.
 * @throws std::invalid_argument if @p fst_file doesn't exist.
 * @throws std::runtime_error if the apply handle could not be initialized.
 */
FstPair load_fst(const std::string& fst_file)
  throw (std::invalid_argument, std::runtime_error);
/**
 * Loads all FSTs from a file.
 * @throws std::invalid_argument if @p fst_file doesn't exist.
 */
FstVector load_fsts(const std::string& fst_file) throw (std::invalid_argument);

#endif
