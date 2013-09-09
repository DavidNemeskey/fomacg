#ifndef FOMA_EXTRA_H
#define FOMA_EXTRA_H

#include <vector>
#include <string>

#include <fomalib.h>

/** Custom versions of the few relevant foma functions. */

/**
 * This function runs a deterministic, minimal, epsilon-free FSA much faster
 * than the regular apply_down() function. The arcs must be sorted.
 */
bool apply_detmin_fsa(struct apply_handle *h, const char *word);

/**
 * This function runs a deterministic, minimal, FST much faster than the regular
 * apply_down() function. By "deterministic" we mean that if from a state, there
 * is a transition where the input is the epsilon symbol, but there are other
 * transitions as well, the latter will get higher priority. The arcs must be
 * sorted.
 */
char* apply_detmin_fst_down(struct apply_handle* h, const char* word);
// /** Same as apply_detmin_fst_down(), but up. */
// char* apply_detmin_fst_up(struct apply_handle* h, const char* word);

/** Finds the transition for the current state and input via binary search. Returns @c NULL if it could not be found. */
inline struct fsm_state* find_transition(struct apply_handle* h);

/**
 * Custom version of the already custom apply_detmin_fsa() function. Since our
 * input always consists of symbols separated by space, we can as well pass the
 * already segmented sentence to the method.
 *
 * @param[in,out] h the handle.
 * @param[in] word the input as a character string. 
 * @param[in] sentence the input, segmented.
 * @param[in] length the length of the input.
 */
bool custom_detmin_fsa(struct apply_handle* h, const std::string& word,
                       const std::vector<std::string>& sentence);

/**
 * Merges the sigma of all fsms in @p fsms. Creates an fsm whose sigma is the
 * union of those in @p fsms. Rewrites the sigma table of every transducer in
 * @p fsms.
 *
 * @param[in, out] fsms the fsms whose sigma is to be merged.
 * @return an fsm whose sigma is the union of the others'.
 */
struct fsm* merge_sigma(std::vector<struct fsm*> fsms);

#endif
