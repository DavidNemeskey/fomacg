#include "foma_extra.h"

#include <sstream>

/*
 * Good to know:
 * h->instring: the input string
 * h->outstring: the output string
 * h->current_instring_length: what its name implies
 * h->ipos: current position in the input word
 * h->opos: current position in the output word
 * h->sigmatch_array: the list of symbols in the input; index by h->ipos
 * h->ptr: the current state (stable)
 * h->curr_ptr: the current state (dynamic, for loops, etc.)
 * h->gstates: fsm->states == transitions; index by h->ptr
 */

bool apply_detmin_fsa(struct apply_handle *h, const char *word) {
  static int count = 0;
  h->instring = const_cast<char*>(word);
  apply_create_sigmatch(h);

  struct fsm* fsa = h->last_net;
  //    fprintf(stderr, "Sorted: %d, arity: %d\n", fsa->arcs_sorted_in, fsa->arity);
  if (fsa == NULL || fsa->finalcount == 0) {
    return false;
  }
//  struct fsm_state* transitions = fsa->states;

  h->ptr = 0; h->ipos = 0;
  for (; h->ipos < h->sigmatch_array_size;) {
Detmin_Fsa_Outer:
    /* Is this word[i] == 0. */
    if ((h->sigmatch_array + h->ipos)->signumber == 0 || h->ipos >= h->current_instring_length) break;
    //	fprintf(stderr, "Reading %d (%c) ...\n",
    //		(h->sigmatch_array+i)->signumber, word[i]);
    /* Linear search for the moment. */
    //	fprintf(stderr, "numlines for state %d: %d\n", curr_state, *(h->numlines+curr_state));
    if (*(h->numlines + h->ptr) == 0) return false;  /* I think... */
    int j = 0;
    //	fprintf(stderr, "Checking transitions %d(%d) through %d(%d)...\n",
    //		*(h->statemap + curr_state), j,
    //		*(h->statemap + curr_state) + *(h->numlines + curr_state),
    //		*(h->numlines + curr_state));
    /* Assumption: FSAs don't support UNKNOWN; detmin FSAs are epsilon-free. */
    if ((h->sigmatch_array + h->ipos)->signumber == IDENTITY) {
      //	    fprintf(stderr, "%d <= IDENTITY\n", (h->sigmatch_array+i)->signumber);
      struct fsm_state* tr = h->gstates + *(h->statemap + h->ptr);
      //	    fprintf(stderr, "%dth transition: in %d, target: %d\n", j, tr->in, tr->target);
      h->ptr = tr->target;
      h->ipos += (h->sigmatch_array + h->ipos)->consumes;
      //	    fprintf(stderr, " found!\n");
      goto Detmin_Fsa_Outer;
    } else {
      struct fsm_state* tr = find_transition(h);
      if (tr != NULL) {
        h->ptr = tr->target;
        h->ipos += (h->sigmatch_array + h->ipos)->consumes;
        goto Detmin_Fsa_Outer;
      }
      //	    for (j = 0; j < *(h->numlines + curr_state); j++) {
      //		count++;
      //		struct fsm_state* tr = transitions + *(h->statemap + curr_state) + j;
      //    //	    fprintf(stderr, "%dth transition: in %d, target: %d\n", j, tr->in, tr->target);
      //		if (tr->in == (h->sigmatch_array+i)->signumber) {
      //		    curr_state = tr->target;
      //		    i += (h->sigmatch_array+i)->consumes;
      //    //		fprintf(stderr, " found!\n");
      //		    goto Detmin_Outer;
      //		} else {
      //    //		fprintf(stderr, " not found!\n");
      //		}
      //	    }
    }
    //	fprintf(stderr, "Count: %d\n", count);
    return false;  /* I think... */
  }
  /* TODO: final state? */
  if ((h->gstates + *(h->statemap + h->ptr))->final_state) {
    //	fprintf(stderr, "Count: %d\n", count);
    return true;
  } else {
    //	fprintf(stderr, "Not in final state!\n");
    //	fprintf(stderr, "Count: %d\n", count);
    return false;
  }
  //    fprintf(stderr, "That's all folks!\n");
}

//std::string apply_detmin_fst(struct apply_handle *h, const char *word) {
//  std::istringstream ret;
//  static int count = 0;
//  h->instring = const_cast<char*>(word);
//  /* Also sets h->current_instring_length. */
//  apply_create_sigmatch(h);
//
//  struct fsm* fst = h->last_net;
//  //    fprintf(stderr, "Sorted: %d, arity: %d\n", fst->arcs_sorted_in, fst->arity);
//  if (fst == NULL || fst->finalcount == 0) {
//    return NULL;
//  }
//  struct fsm_state* transitions = fst->states;
//  int curr_state = 0;
//
//  // TODO: use h->outstring
//  h->iptr = NULL; h->ptr = 0; h->ipos = 0; h->opos = 0;
//
//  while (true) {
//    /* If we hit an accepting state after consuming the input: return! */
//    if (h->ipos == h->current_instring_length &&
//        (h->gstates+h->ptr)->finals_state == 1) {
//      // TODO to char* 
//      return ret.str();
//    }
//Detmin_Fst_Outer:
//    /* Is this word[i] == 0. */
//    if ((h->sigmatch_array+i)->signumber == 0 || word[i] == 0) break;
//    //	fprintf(stderr, "Reading %d (%c) ...\n",
//    //		(h->sigmatch_array+i)->signumber, word[i]);
//    /* Linear search for the moment. */
//    //	fprintf(stderr, "numlines for state %d: %d\n", curr_state, *(h->numlines+curr_state));
//    if (*(h->numlines+curr_state) == 0) return false;  /* I think... */
//    int j = 0;
//    //	fprintf(stderr, "Checking transitions %d(%d) through %d(%d)...\n",
//    //		*(h->statemap + curr_state), j,
//    //		*(h->statemap + curr_state) + *(h->numlines + curr_state),
//    //		*(h->numlines + curr_state));
//    /* Assumption: FSAs don't support UNKNOWN; detmin FSAs are epsilon-free. */
//    if ((h->sigmatch_array+i)->signumber == IDENTITY) {
//      //	    fprintf(stderr, "%d <= IDENTITY\n", (h->sigmatch_array+i)->signumber);
//      struct fsm_state* tr = transitions + *(h->statemap + curr_state);
//      //	    fprintf(stderr, "%dth transition: in %d, target: %d\n", j, tr->in, tr->target);
//      curr_state = tr->target;
//      i += (h->sigmatch_array+i)->consumes;
//      //	    fprintf(stderr, " found!\n");
//      goto Detmin_Fst_Outer;
//    } else {
//      struct fsm_state* tr = find_transition(h, curr_state, i);
//      if (tr != NULL) {
//        curr_state = tr->target;
//        ipos += (h->sigmatch_array + ipos)->consumes;
//        goto Detmin_Fst_Outer;
//      }
//      //	    for (j = 0; j < *(h->numlines + curr_state); j++) {
//      //		count++;
//      //		struct fsm_state* tr = transitions + *(h->statemap + curr_state) + j;
//      //    //	    fprintf(stderr, "%dth transition: in %d, target: %d\n", j, tr->in, tr->target);
//      //		if (tr->in == (h->sigmatch_array+i)->signumber) {
//      //		    curr_state = tr->target;
//      //		    i += (h->sigmatch_array+i)->consumes;
//      //    //		fprintf(stderr, " found!\n");
//      //		    goto Detmin_Outer;
//      //		} else {
//      //    //		fprintf(stderr, " not found!\n");
//      //		}
//      //	    }
//    }
//    //	fprintf(stderr, "Count: %d\n", count);
//    return false;  /* I think... */
//  }
//  /* TODO: final state? */
//  if ((transitions + *(h->statemap + curr_state))->final_state) {
//    //	fprintf(stderr, "Count: %d\n", count);
//    return true;
//  } else {
//    //	fprintf(stderr, "Not in final state!\n");
//    //	fprintf(stderr, "Count: %d\n", count);
//    return false;
//  }
//  //    fprintf(stderr, "That's all folks!\n");
//}

// TODO curr_state, ipos to h
struct fsm_state* find_transition(struct apply_handle *h) {
  int begin = *(h->statemap + h->ptr);
  int end   = begin + *(h->numlines + h->ptr);
  while (begin < end) {
    int middle = (begin + end) / 2;
    struct fsm_state* tr = h->gstates + middle;
    //	    fprintf(stderr, "begin: %d, end: %d, middle: %d, symbol at middle: %d, symbol: %d(%c)\n",
    //		    begin, end, middle, tr->in, (h->sigmatch_array+i)->signumber, word[i]);
    if (tr->in == (h->sigmatch_array + h->ipos)->signumber) {
      return tr;
    } else if ((h->sigmatch_array + h->ipos)->signumber < tr->in) {
      end = middle;
    } else {
      begin = middle + 1;
    }
  }
  return NULL;
}

