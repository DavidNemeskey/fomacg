#include "foma_extra.h"

bool apply_detmin_fsa(struct apply_handle *h, const char *word) {
  static int count = 0;
  h->instring = const_cast<char*>(word);
  apply_create_sigmatch(h);

  struct fsm* fsa = h->last_net;
  //    fprintf(stderr, "Sorted: %d, arity: %d\n", fsa->arcs_sorted_in, fsa->arity);
  if (fsa == NULL || fsa->finalcount == 0) {
    return false;
  }
  struct fsm_state* transitions = fsa->states;
  int curr_state = 0;

  int i;
  for (i = 0; i < h->sigmatch_array_size;) {
Detmin_Outer:
    /* Is this word[i] == 0. */
    if ((h->sigmatch_array+i)->signumber == 0 || word[i] == 0) break;
    //	fprintf(stderr, "Reading %d (%c) ...\n",
    //		(h->sigmatch_array+i)->signumber, word[i]);
    /* Linear search for the moment. */
    //	fprintf(stderr, "numlines for state %d: %d\n", curr_state, *(h->numlines+curr_state));
    if (*(h->numlines+curr_state) == 0) return false;  /* I think... */
    int j = 0;
    //	fprintf(stderr, "Checking transitions %d(%d) through %d(%d)...\n",
    //		*(h->statemap + curr_state), j,
    //		*(h->statemap + curr_state) + *(h->numlines + curr_state),
    //		*(h->numlines + curr_state));
    /* Assumption: FSAs don't support UNKNOWN; detmin FSAs are epsilon-free. */
    if ((h->sigmatch_array+i)->signumber == IDENTITY) {
      //	    fprintf(stderr, "%d <= IDENTITY\n", (h->sigmatch_array+i)->signumber);
      struct fsm_state* tr = transitions + *(h->statemap + curr_state);
      //	    fprintf(stderr, "%dth transition: in %d, target: %d\n", j, tr->in, tr->target);
      curr_state = tr->target;
      i += (h->sigmatch_array+i)->consumes;
      //	    fprintf(stderr, " found!\n");
      goto Detmin_Outer;
    } else {
      int begin = *(h->statemap + curr_state);
      int end   = begin + *(h->numlines + curr_state);
      while (begin < end) {
        count++;
        int middle = (begin + end) / 2;
        struct fsm_state* tr = transitions + middle;
        //	    fprintf(stderr, "begin: %d, end: %d, middle: %d, symbol at middle: %d, symbol: %d(%c)\n",
        //		    begin, end, middle, tr->in, (h->sigmatch_array+i)->signumber, word[i]);
        if (tr->in == (h->sigmatch_array+i)->signumber) {
          curr_state = tr->target;
          i += (h->sigmatch_array+i)->consumes;
          goto Detmin_Outer;
        } else if ((h->sigmatch_array+i)->signumber < tr->in) {
          end = middle;
        } else {
          begin = middle + 1;
        }
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
  if ((transitions + *(h->statemap + curr_state))->final_state) {
    //	fprintf(stderr, "Count: %d\n", count);
    return true;
  } else {
    //	fprintf(stderr, "Not in final state!\n");
    //	fprintf(stderr, "Count: %d\n", count);
    return false;
  }
  //    fprintf(stderr, "That's all folks!\n");
}

