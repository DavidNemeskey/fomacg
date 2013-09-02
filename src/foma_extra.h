#ifndef FOMA_EXTRA_H
#define FOMA_EXTRA_H

#include <fomalib.h>

/** Custom versions of the few relevant foma functions. */

/**
 * This function runs a deterministic, minimal, epsilon-free FSA much faster
 * than the regular apply_down() function. The arcs must be sorted.
 */
bool apply_detmin_fsa(struct apply_handle *h, const char *word);

#endif
