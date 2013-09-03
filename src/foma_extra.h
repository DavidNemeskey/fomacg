#ifndef FOMA_EXTRA_H
#define FOMA_EXTRA_H

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
char* apply_detmin_fst(struct apply_handle* h, const char* word);

/** Finds the transition for the current state and input via binary search. Returns @c NULL if it could not be found. */
inline struct fsm_state* find_transition(struct apply_handle* h);
// TODO: up & down

#endif
