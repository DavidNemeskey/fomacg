#include "foma_extra.h"

#include <set>
#include <iostream>

/*
 * Good to know:
 * h->instring: the input string
 * h->outstring: the output string
 * h->current_instring_length: what its name implies
 * h->ipos: current position in the input word
 * h->opos: current position in the output word
 * h->sigmatch_array: the list of symbols in the input; index by h->ipos
 * h->sigs: the list of symbols (string, length).
 * h->ptr: the current state (stable)
 * h->curr_ptr: the current state (dynamic, for loops, etc.)
 * h->gstates: fsm->states == transitions; index by h->ptr
 * h->statemap: the offset of the state's transition list from h->gstates.
 * h->numlines: the number of transitions from a state.
 */

inline static void add_output(struct apply_handle* h, int out_symbol);
inline static void add_output(struct apply_handle* h, char* out_str, int out_len);

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
    //int j = 0;
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

char* apply_detmin_fst(struct apply_handle *h, const char *word) {
//  for (int i = 0; i <= 19; i++) {
//    for (int j = 0; j < *(h->numlines + i); j++) {
//      struct fsm_state* st = h->gstates + *(h->statemap + i) + j;
//      std::cout << "State " << i << ", tr " << j << ", state_no " << st->state_no
//                << ", in " << ((h->sigs) + st->in)->symbol
//                << ", out " << ((h->sigs) + st->out)->symbol
//                << ", target " << st->target << ", final_state "
//                << int(st->final_state) << ", start_state "
//                << int(st->start_state) << std::endl;
//    }
//  }
  static int count = 0;
  h->instring = const_cast<char*>(word);
  /* Also sets h->current_instring_length. */
  apply_create_sigmatch(h);

  struct fsm* fst = h->last_net;
//  fprintf(stdout, "Sorted: %d, arity: %d\n", fst->arcs_sorted_in, fst->arity);
  if (fst == NULL || fst->finalcount == 0) {
    return NULL;
  }

  // TODO: use h->outstring
  h->ptr = 0; h->ipos = 0; h->opos = 0;

//  std::cout << "length: " << h->current_instring_length << std::endl;
  while (true) {
//    std::cout << "h->ipos = " << h->ipos << ", state: " << h->ptr << std::endl;
    /*
     * If we hit the end of the input: exit the loop, and follow only epsilon
     * moves from now on.
     */
    if (h->ipos == h->current_instring_length) {
//      std::cout << "Input consumed in state " << h->ptr << std::endl;
      break;
    }

    /* We have not yet consumed the input, yet we are trapped in a state: failure. */
    if (*(h->numlines + h->ptr) == 0) return NULL;

    /* Is there an epsilon transition in the current state? */
    struct fsm_state* epsilon_move = h->gstates + *(h->statemap + h->ptr);
    if (epsilon_move->in != EPSILON) {
      epsilon_move = NULL;
    }
//    std::cout << "is there an epsilon move? " << (epsilon_move == NULL ? "no" : "yes") << std::endl;

    /* Is there a regular transition with the next symbol of the input? */
    struct fsm_state* tr = find_transition(h);
    if (tr != NULL) {
//      std::cout << "Regular transition for " << (h->sigs + (h->sigmatch_array + h->ipos)->signumber)->symbol << std::endl;
      if (tr->out != IDENTITY) {
//        std::cout << "Adding " << ((h->sigs) + tr->out)->symbol << " (" << tr->out << ")" << std::endl;
        add_output(h, tr->out);
      } else {
//        std::cout << "Adding " << std::string(h->instring + h->ipos, (h->sigmatch_array + h->ipos)->consumes) << std::endl;
        add_output(h, h->instring + h->ipos, (h->sigmatch_array + h->ipos)->consumes);
      }
      h->ptr = tr->target;
      h->ipos += (h->sigmatch_array + h->ipos)->consumes;
    } else if (epsilon_move != NULL) {
//      std::cout << "No transition for " << (h->sigmatch_array + h->ipos)->signumber << std::endl;
      /* No regular transitions: follow the epsilon transition. */
      h->ptr = epsilon_move->target;
      add_output(h, epsilon_move->out);
//      std::cout << "Adding " << ((h->sigs) + epsilon_move->out)->symbol << std::endl;
    } else {
      /* No transitions: the FST failed to recognize the input. */
      return NULL;
    }
  }

  /*
   * Handle the possible epsilon moves after the input has been consumend. If we
   * hit an accepting state, we return successfully; otherwise, the transduction
   * fails.
   */
  /* Shortcut for the state we end up with the input. */
  if ((h->gstates+h->ptr)->final_state == 1) {
//    return sstream_to_chars(h, ret);
    return h->outstring;
  }

  std::set<int> visited;
  while (true) {
    if (visited.find(h->ptr) != visited.end()) {
//      std::cout << "Visited " << h->ptr << " (" << (h->gstates+h->ptr)->state_no << ")" << std::endl;
      return NULL;
    } else if ((h->gstates + *(h->statemap + h->ptr))->final_state == 1) {
//      std::cout << "Final " << h->ptr << std::endl;
//      return sstream_to_chars(h, ret);
      return h->outstring;
    } else {
      struct fsm_state* epsilon_move = h->gstates + *(h->statemap + h->ptr);
      if (epsilon_move->in != EPSILON) {
//        std::cout << "No epsilon move " << h->ptr << std::endl;
        return NULL;
      } else {
//        std::cout << "Epsilon move " << h->ptr << std::endl;
        visited.insert((h->gstates+h->ptr)->state_no);
        h->ptr = epsilon_move->target;
        add_output(h, epsilon_move->out);
      }
    }
  }
}

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

void add_output(struct apply_handle* h, int out_symbol) {
  char* out_str = ((h->sigs) + out_symbol)->symbol;
  int   out_len = ((h->sigs) + out_symbol)->length;
  add_output(h, out_str, out_len);
//  while (h->outstringtop <= h->opos + out_len + 3) {
//    h->outstring = static_cast<char*>(xxrealloc(h->outstring, sizeof(char) * h->outstringtop * 2));
//    h->outstringtop = h->outstringtop * 2;
//  }
//  strcpy(h->outstring + h->opos, out_str);
//  h->opos += out_len;
}

void add_output(struct apply_handle* h, char* out_str, int out_len) {
  while (h->outstringtop <= h->opos + out_len + 3) {
    h->outstring = static_cast<char*>(xxrealloc(h->outstring, sizeof(char) * h->outstringtop * 2));
    h->outstringtop = h->outstringtop * 2;
  }
  strncpy(h->outstring + h->opos, out_str, out_len);
  h->opos += out_len;
  h->outstring[h->opos] = 0;
}

