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
  /**
   * Sigma mapping for @c fst. Maps sigma ids to themselves, if they are part of
   * the alphabet of @c fst; to IDENTITY otherwise.
   *
   * Not computed automatically, as not always needed.
   *
   * @note Was a set before, but vector is much faster. Also, an unordered set
   *       at this small element number is SLOWER than the regular set.
   */
  std::vector<int> sigma;

  FstPair();
  FstPair(struct fsm* fst, struct apply_handle* ah);

  /** Fills the sigma vector. */
  void fill_sigma(size_t sigma_size);
  /**
   * Frees all resources associated with this FST.
   *
   * If @p only_handle is @c true, only the handle is deleted.
   */
  void cleanup(bool only_handle=false);
};

/** A vector of FSTs with their apply handle. */
// TODO: why not vector? Not that it matters
typedef std::deque<FstPair> FstVector;

/**
 * Loads an FST from a file.
 * @param fst_file the file from the FST is read.
 * @throws std::invalid_argument if @p fst_file doesn't exist.
 * @throws std::runtime_error if the apply handle could not be initialized.
 */
FstPair load_fst(const std::string& fst_file)
  throw (std::invalid_argument, std::runtime_error);
/**
 * Loads all FSTs from a file.
 * @param fst_file the file from the FSTs is read.
 * @throws std::invalid_argument if @p fst_file doesn't exist.
 */
FstVector load_fsts(const std::string& fst_file) throw (std::invalid_argument);

/** Loads a rule set. Basically a generator version of load_fsts(). */
struct RuleSetLoader {
  /** @throws std::invalid_argument if @p fst_file doesn't exist. */
  RuleSetLoader(const std::string& fst_file) throw (std::invalid_argument);

  /**
   * Loads the next FST to @p fst.
   * @return @c true on success and @c false on failure. In the latter case,
   *         there are no more FSTs in the file.
   * @throws std::runtime_error if the FST could not be initialized.
   */
  bool load_fst(FstPair& fst) throw (std::runtime_error);

private:
  /** The handle to the foma file reader. */
  fsm_read_binary_handle fsrh;
};

#endif
