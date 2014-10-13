#include "foma_extra.h"

#include <set>
#include <map>
#include <iostream>

#include <fomalibconf.h>

/* From apply.c :/. */
#define UP 8
#define DOWN 16

/*
 * Good to know:
 * h->instring: the input string
 * h->outstring: the output string
 * h->current_instring_length: what its name implies
 * h->ipos: current position in the input word
 * h->opos: current position in the output word
 * h->sigmatch_array: the list of symbols in the input; index by h->ipos
 * h->sigs: the list of symbols (string, length).
 * h->sigma_trie: a trie for symbols
 * h->sigma_trie_arrays: keeps track of the trie arrays. For memory cleanup.
 * h->ptr: the current state (stable)
 * h->curr_ptr: the current state (dynamic, for loops, etc.)
 * h->gstates: fsm->states == transitions; index by h->ptr
 * h->statemap: the offset of the state's transition list from h->gstates.
 * h->numlines: the number of transitions from a state.
 * h->last_net: the (last?) FST
 */

/** Object that emulates a stack for common_apply_down(). */
struct StackItem {
  /** The state of the FST. */
  int state;
  /** The position in the input. */
  int ipos;
  /** The then-current size of the output. */
  size_t osize;
  /** The symbol at ipos. */
  int signum;
  /** The epsilon move from the state (if any). */
  struct fsm_state* epsilon_move;
  /** The unknown move from the state (if any). */
  struct fsm_state* unknown_move;
  /** The first regular move where input is @c signum. */
  struct fsm_state* tr;
  /** How many arcs are there with the same input? */
  size_t fork;
  /** The branch of the fork we took. */
  size_t branch;
  size_t ipos2;

  StackItem(int state_, int ipos_, size_t osize_, int signum_,
            struct fsm_state* epsilon_move_, struct fsm_state* unknown_move_,
            struct fsm_state* tr_, size_t fork_, size_t branch_, size_t ipos2_)
    : state(state_), ipos(ipos_), osize(osize_), signum(signum_),
      epsilon_move(epsilon_move_), unknown_move(unknown_move_), tr(tr_),
      fork(fork_), branch(branch_), ipos2(ipos2_) {}
};

//inline static char* apply_detmin_fst(struct apply_handle *h, const char *word);
inline static void add_output(struct apply_handle* h, int out_symbol);
inline static void add_output(struct apply_handle* h, char* out_str, int out_len);
/**
 * Replaces the sigma of @p fsm according to the mapping @p sigmas. Helper
 * method for merge_sigma().
 */
static void replace_sigma(struct fsm* fsm, std::map<std::string, int> sigmas);
/**
 * Finds and returns the basic transitions (EPSILON, UNKNOWN) for a state.
 * @param[in] h the apply_handle.
 * @param[out] epsilon_move the pointer to the EPSILON arc.
 * @param[out] unknown_move the pointer to the UNKNOWN arc.
 */
inline static void get_basic_transitions(struct apply_handle* h,
                                         struct fsm_state** epsilon_move,
                                         struct fsm_state** unknown_move);

bool apply_detmin_fsa(struct apply_handle *h, const char *word) {
  h->instring = const_cast<char*>(word);
  apply_create_sigmatch(h);

//  bool debug = !strcmp(h->last_net->name, "C_1_530") || !strcmp(h->last_net->name, "S_1_60");
//  if (debug) {
//    fprintf(stderr, "Name: %s\n", h->last_net->name);
//    apply_print_sigma(h->last_net);
//    fprintf(stderr, "Sigmatch array:\n");
//    for (int i = 0; i < h->current_instring_length;) {
//      if ((h->sigmatch_array+i)->signumber == 0) break;
//      fprintf(stderr, "Item %d: signum: %d (%.*s), consumes: %d\n", i,
//          (h->sigmatch_array+i)->signumber,
//          (h->sigs+(h->sigmatch_array+i)->signumber)->length,
////          (h->sigs+(h->sigmatch_array+i)->signumber)->symbol,
//          h->instring+i,
//          (h->sigmatch_array+i)->consumes);
//      i += (h->sigmatch_array+i)->consumes;
//    }
//  }

  h->ptr = 0; h->ipos = 0;
  while (h->ipos < h->current_instring_length) {
//    if (debug) fprintf(stderr, "State %d, ipos: %d, symbol: %d(%.*s)\n",
//        h->ptr, h->ipos, (h->sigmatch_array + h->ipos)->signumber,
//        (h->sigs+(h->sigmatch_array + h->ipos)->signumber)->length,
//        h->instring + h->ipos);
    /* Trap state. */
    if (*(h->numlines + h->ptr) == 0) return false;

    /* Assumption: FSAs don't support UNKNOWN; detmin FSAs are epsilon-free. */
    if ((h->sigmatch_array + h->ipos)->signumber == IDENTITY) {
      struct fsm_state* tr = h->gstates + *(h->statemap + h->ptr);
      h->ptr = tr->target;
      h->ipos += (h->sigmatch_array + h->ipos)->consumes;
    } else {
      struct fsm_state* tr = find_transition(
          h, (h->sigmatch_array + h->ipos)->signumber);
      if (tr == NULL) break;

      h->ptr = tr->target;
      h->ipos += (h->sigmatch_array + h->ipos)->consumes;
    }
  }

  if ((h->gstates + *(h->statemap + h->ptr))->final_state) {
//    if (debug) fprintf(stderr, "Returning true\n");
    return true;
  } else {
//    if (debug) fprintf(stderr, "Returning false\n");
    return false;
  }
}

//char* apply_detmin_fst_down(struct apply_handle *h, const char *word) {
//  h->mode = DOWN;
//  h->binsearch = (h->last_net->arcs_sorted_in == 1) ? 1 : 0;
//  return apply_detmin_fst(h, word);
//}
//char* apply_detmin_fst_up(struct apply_handle *h, const char *word) {
//  h->mode = UP;
//  h->binsearch = (h->last_net->arcs_sorted_out == 1) ? 1 : 0;
//  return apply_detmin_fst(h, word);
//}

char* apply_detmin_fst_down(struct apply_handle *h, const char *word) {
//  if (debug) {
//    fprintf(stderr, "Name: %s\n", h->last_net->name);
//    apply_print_sigma(h->last_net);
//    fprintf(stderr, "Sigmatch array:\n");
//    for (int i = 0; i < h->current_instring_length;) {
//      if ((h->sigmatch_array+i)->signumber == 0) break;
//      fprintf(stderr, "Item %d: signum: %d (%.*s), consumes: %d\n", i,
//          (h->sigmatch_array+i)->signumber,
//          (h->sigs+(h->sigmatch_array+i)->signumber)->length,
////          (h->sigs+(h->sigmatch_array+i)->signumber)->symbol,
//          h->instring+i,
//          (h->sigmatch_array+i)->consumes);
//      i += (h->sigmatch_array+i)->consumes;
//    }
//  }

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
  h->instring = const_cast<char*>(word);
  /* Also sets h->current_instring_length. */
  apply_create_sigmatch(h);
//  std::cout << "instring: >" << h->instring << "<" << std::endl;

  h->ptr = 0; h->ipos = 0; h->opos = 0;

  /*
   * If we hit the end of the input: exit the loop, and follow only epsilon
   * moves from now on.
   */
//  std::cout << "length: " << h->current_instring_length << std::endl;
  while (h->ipos < h->current_instring_length) {
//    std::cout << "h->ipos = " << h->ipos << ", state: " << h->ptr << std::endl;

    /* We have not yet consumed the input, yet we are trapped in a state: failure. */
    if (*(h->numlines + h->ptr) == 0) return NULL;

    /* Is there an epsilon transition in the current state? */
    struct fsm_state* epsilon_move = h->gstates + *(h->statemap + h->ptr);
    if (epsilon_move->in != EPSILON) {
      epsilon_move = NULL;
    }
//    std::cout << "is there an epsilon move? " << (epsilon_move == NULL ? "no" : "yes") << std::endl;

    /* Is there a regular transition with the next symbol of the input? */
    struct fsm_state* tr = find_transition(
        h, (h->sigmatch_array + h->ipos)->signumber);
    if (tr != NULL) {
//      std::cout << "Regular transition for " << (h->sigs + (h->sigmatch_array + h->ipos)->signumber)->symbol << std::endl;
      if (tr->out == EPSILON) {
//        std::cout << "Epsilon!" << std::endl;
      } else if (tr->out == IDENTITY) {
//        std::cout << "Adding " << std::string(h->instring + h->ipos, (h->sigmatch_array + h->ipos)->consumes) << std::endl;
        add_output(h, h->instring + h->ipos, (h->sigmatch_array + h->ipos)->consumes);
      } else {  // if (tr->out != IDENTITY) {
//        std::cout << "Adding " << ((h->sigs) + tr->out)->symbol << " (" << tr->out << ")" << std::endl;
        add_output(h, tr->out);
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
  if ((h->gstates + *(h->statemap + h->ptr))->final_state == 1) {
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
        visited.insert(h->ptr);
        h->ptr = epsilon_move->target;
        add_output(h, epsilon_move->out);
      }
    }
  }
}

struct fsm_state* find_transition(struct apply_handle *h, int signum) {
  int begin = *(h->statemap + h->ptr);
  int end   = begin + *(h->numlines + h->ptr);
  while (begin < end) {
    int middle = (begin + end) / 2;
    struct fsm_state* tr = h->gstates + middle;
    //	    fprintf(stderr, "begin: %d, end: %d, middle: %d, symbol at middle: %d, symbol: %d(%c)\n",
    //		    begin, end, middle, tr->in, (h->sigmatch_array+i)->signumber, word[i]);
    if (tr->in == signum) {
      return tr;
    } else if (signum < tr->in) {
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

bool custom_detmin_fsa(struct apply_handle* h,
                       const std::deque<std::string>& sentence) {
///  h->instring = const_cast<char*>(word.c_str());
///  /* Also sets h->current_instring_length. */
///  custom_create_sigmatch(h, sentence, static_cast<int>(word.length()));
  custom_create_sigmatch(h, sentence);

  h->ptr = 0; h->ipos = 0;
///  for (; h->ipos < h->current_instring_length;) {
  while (static_cast<size_t>(h->ipos) < sentence.size()) {
    /* Trap state. */
    if (*(h->numlines + h->ptr) == 0) return false;

    /* Assumption: FSAs don't support UNKNOWN; detmin FSAs are epsilon-free. */
    if ((h->sigmatch_array + h->ipos)->signumber == IDENTITY) {
      struct fsm_state* tr = h->gstates + *(h->statemap + h->ptr);
      h->ptr = tr->target;
///      h->ipos += (h->sigmatch_array + h->ipos)->consumes;
    } else {
      struct fsm_state* tr = find_transition(
          h, (h->sigmatch_array + h->ipos)->signumber);
      if (tr == NULL) break;
      
      h->ptr = tr->target;
///      h->ipos += (h->sigmatch_array + h->ipos)->consumes;
    }
    h->ipos++;
  }

  if ((h->gstates + *(h->statemap + h->ptr))->final_state) {
    return true;
  } else {
    return false;
  }
}

bool common_detmin_fsa(FstPair& fst, const std::vector<Symbol>& sentence) {
  struct apply_handle* h = fst.ah;
  h->ptr = 0; h->ipos = 0;
  while (static_cast<size_t>(h->ipos) < sentence.size()) {
    /* Trap state. */
    if (*(h->numlines + h->ptr) == 0) return false;

    /*
     * Symbols in the "universal" alphabet, but not in this machine's are
     * replaced by IDENTITY.
     */
    int signum = sentence[h->ipos].number;
    signum = fst.sigma[signum];
    /* Assumption: FSAs don't support UNKNOWN; detmin FSAs are epsilon-free. */
    if (signum == IDENTITY) {
      struct fsm_state* tr = h->gstates + *(h->statemap + h->ptr);
      h->ptr = tr->target;
    } else {
      struct fsm_state* tr = find_transition(h, signum);
      if (tr == NULL) break;
      
      h->ptr = tr->target;
    }
    h->ipos++;
  }

  if ((h->gstates + *(h->statemap + h->ptr))->final_state) {
    return true;
  } else {
    return false;
  }
}

bool common_apply_down(FstPair& fst,
                       const std::vector<Symbol>& sentence,
                       std::vector<Symbol>& result) {
  /* h->ipos and result size (~h->opos) stack for ND paths. */
  std::vector<StackItem> stack;

  struct apply_handle* h = fst.ah;
  /* Are we returning the state from a failed branch? */
  bool failed = false;
  int signum;
  struct fsm_state* epsilon_move;
  struct fsm_state* unknown_move;
  struct fsm_state* tr;
  /* How many arcs with the same input? */
  size_t fork;
  /* Which branch to choose */
  size_t branch;
  h->ptr = 0; h->ipos = 0;
  size_t ipos2 = 0;
  while (static_cast<size_t>(h->ipos) < sentence.size()) {
Continue:
    /* If we are not returning from a failed branch. */
    if (!failed) {
      /* Trap state. */
      if (*(h->numlines + h->ptr) == 0) {
//        std::cerr << "trap" << std::endl;
        if (stack.size() > 0) {
          failed = true;
          continue;
        } else {
          return false;
        }
      }

      /*
       * Symbols in the "universal" alphabet, but not in this machine's are
       * replaced by IDENTITY.
       */
      signum = sentence[h->ipos].number;
      signum = fst.sigma[signum];

      /* Is there an epsilon transition in the current state? */
      get_basic_transitions(h, &epsilon_move, &unknown_move);

      /* Is there a regular transition with the next symbol of the input? */
      tr = find_transition(h, signum);
      if (tr != NULL) {
//        std::cerr << "XXX signum " << signum << " state " << h->ptr << std::endl;
        fork = 1;
        while ((tr - 1)->in == signum) {
//          std::cerr << "tr - 1 signum " << (tr - 1)->in << " state "
//                    << (tr - 1)->state_no << std::endl;
          tr--;  // The first in:* arc
        }
        for (size_t i = 1; (tr + i)->in == signum; i++) {
//          std::cerr << "tr + i signum " << (tr + i)->in << " state "
//                    << (tr + i)->state_no << std::endl;
          fork++;  // How many arcs?
        }
      }
      branch = 0;
//      std::cerr << "new step: state " << h->ptr << " ipos " << h->ipos
//                << " (" << ipos2 << ") " << " osize "
//                << result.size() << " signum " << signum
//                << " fork " << fork << " branch " << branch
//                << " stack size: " << stack.size() << std::endl;
//      std::cerr << "new step: state " << h->ptr << " ipos " << h->ipos << " osize "
//                << result.size() << " signum " << signum << "(" << sentence[h->ipos]
//                << ") epsilon_move " << (epsilon_move != NULL)
//                << " unknown_move " << (unknown_move != NULL) << " tr "
//                << (tr != NULL) << " fork " << fork << " branch " << branch
//                << std::endl;
    } else {
      /* When we return from a failed branch, we will have already computed
       * all this. */
      h->ptr       = stack.back().state;
      h->ipos      = stack.back().ipos;
      signum       = stack.back().signum;
      epsilon_move = stack.back().epsilon_move;
      unknown_move = stack.back().unknown_move;
      tr           = stack.back().tr;
      fork         = stack.back().fork;
      branch       = stack.back().branch + 1;
      ipos2        = stack.back().ipos2;
      result.resize(stack.back().osize);
      stack.pop_back();
      failed = false;
//      std::cerr << "returning: state " << h->ptr << " ipos " << h->ipos
//                << " (" << ipos2 << ") "
//                << " osize "
//                << result.size() << " signum " << signum
//                << " fork " << fork << " branch " << branch << std::endl;
    }

    if (tr != NULL) {
//      std::cerr << "Regular move" << std::endl;
      /* For when there are more than one arcs with the same upper symbol. */
      if (fork > branch + 1) {
//        std::cerr << "forking" << std::endl;
        stack.push_back(StackItem(h->ptr, h->ipos, result.size(), signum,
                                  epsilon_move, unknown_move, tr, fork, branch, ipos2));
      }

      tr += branch;
      if (tr->out == EPSILON) {
//        std::cerr << "NOT adding EPSILON" << std::endl;
      } else if (tr->out == IDENTITY) {
//        std::cerr << "Adding IDENTITY " << sentence[h->ipos].number << std::endl;
        result.push_back(sentence[h->ipos]);
      } else {
//        std::cerr << "Adding symbol " << tr->out << std::endl;
        result.push_back(Symbol(tr->out));
      }
      h->ptr = tr->target;
//      ipos2 += sentence[h->ipos].symbol.length();
      h->ipos++;
    } else if (signum == IDENTITY && unknown_move != NULL) {
//      std::cerr << "Unknown move" << std::endl;
      /* Let's try the joker transition. */
      h->ptr = unknown_move->target;
//      ipos2 += sentence[h->ipos].symbol.length();
      h->ipos++;
//      std::cerr << "Adding symbol " << unknown_move->out << std::endl;
      if (unknown_move->out != EPSILON)
        result.push_back(Symbol(unknown_move->out));
    } else if (epsilon_move != NULL) {
//      std::cerr << "Epsilon move" << std::endl;
      /* No regular transitions: follow the epsilon transition. */
      h->ptr = epsilon_move->target;
//      std::cerr << "Adding symbol " << epsilon_move->out << std::endl;
      if (epsilon_move->out != EPSILON)
        result.push_back(Symbol(epsilon_move->out));
    } else {
      /* No transitions: the FST failed to recognize the input. */
//      std::cerr << "no transitions" << std::endl;
      if (stack.size() > 0) {
        failed = true;
        continue;
      } else {
        return false;
      }
    }
  }

//  std::cerr << "Read input, in state " << h->ptr << " ipos " << h->ipos
//              << " (" << ipos2 << ") " << " osize "
//              << result.size() << " signum " << signum
//              << " fork " << fork << " branch " << branch
//              << " stack size: " << stack.size() << std::endl;

  /*
   * Handle the possible epsilon moves after the input has been consumend. If we
   * hit an accepting state, we return successfully; otherwise, the transduction
   * fails. We do not allow non-determinism here.
     */
  /* Shortcut for the state we end up with the input. */
  if ((h->gstates + *(h->statemap + h->ptr))->final_state == 1) {
//    std::cerr << "State " << h->ptr << " is a final state. Returning true..."
//              << std::endl;
    return true;
  }

  std::set<int> visited;
  while (true) {
    if (visited.count(h->ptr) != 0) {
//      std::cerr << "Already visited state " << h->ptr << "." << std::endl;
      if (stack.size() > 0) {
        failed = true;
        goto Continue;
      } else {
        return false;
      }
    } else if ((h->gstates + *(h->statemap + h->ptr))->final_state == 1) {
//      std::cerr << "State " << h->ptr << " is a final state. Returning true..."
//                << std::endl;
      return true;
    } else {
      struct fsm_state* epsilon_move = h->gstates + *(h->statemap + h->ptr);
      if (epsilon_move->in != EPSILON) {
        if (stack.size() > 0) {
          failed = true;
          goto Continue;
        } else {
          return false;
        }
      } else {
        visited.insert(h->ptr);
        h->ptr = epsilon_move->target;
//        std::cerr << "Adding symbole " << epsilon_move->out << std::endl;
        if (epsilon_move->out != EPSILON)
          result.push_back(Symbol(epsilon_move->out));
      }
    }
  }
}

inline static void get_basic_transitions(struct apply_handle* h,
                                         struct fsm_state** epsilon_move,
                                         struct fsm_state** unknown_move) {
  *epsilon_move = *unknown_move = NULL;
  for (struct fsm_state* p = h->gstates + *(h->statemap + h->ptr);
       p->in < IDENTITY; p++) {
    if (p->in == EPSILON) *epsilon_move = p;
    if (p->in == UNKNOWN) *unknown_move = p;
  }
}

std::vector<Symbol> common_create_sigmatch(
    struct apply_handle* h, const std::string& sentence) {
  std::vector<Symbol> ret;
  size_t pos = 0;
  while (true) {
    size_t found = sentence.find(' ', pos);
    if (found != std::string::npos) {
      struct apply_handle::sigma_trie *st = h->sigma_trie;
      int signum = IDENTITY;

      for (size_t i = pos; i <= found; i++) {
        st += (unsigned char)sentence[i];
        if (i == found) {
          if (st->signum != 0) {
            signum = st->signum;
          }
        } else if (st->next != NULL) {
          st = st->next;
        } else {
          break;
        }
      }  // for i

      if (signum != IDENTITY) {
        ret.push_back(Symbol(signum));
      } else {
        ret.push_back(Symbol(IDENTITY, pos, found - pos + 1));
      }

      pos = found + 1;
    } else {
      return ret;
    }
  }
}

/**
 * Variant of apply_create_sigmatch(), tailored to the needs of
 * custom_detmin_fsa(). Each word in sentence is handled as a single IDENTITY
 * symbol, if it is not in sigma; as opposed to the regular
 * one-character-per-IDENTITY practice.
 *
 * @param inlen the length of the sentence in bytes.
 */
void custom_create_sigmatch(struct apply_handle *h,
                            const std::deque<std::string>& sentence) {
///                            const std::vector<std::string>& sentence,
///                            int inlen) {
//  /* We create a sigmatch array only in case we match against a real string */
//  if (((h->mode) & ENUMERATE) == ENUMERATE) { return; }

///  h->current_instring_length = inlen;
///  if (inlen >= h->sigmatch_array_size) {
  if (sentence.size() >= static_cast<size_t>(h->sigmatch_array_size)) {
    xxfree(h->sigmatch_array);
    h->sigmatch_array = static_cast<struct apply_handle::sigmatch_array*>(
///      xxmalloc(sizeof(struct apply_handle::sigmatch_array)*(inlen)));
      xxmalloc(sizeof(struct apply_handle::sigmatch_array)*(sentence.size())));
///    h->sigmatch_array_size = inlen;
    h->sigmatch_array_size = sentence.size();
  }

///  for (size_t word = 0, at = 0; word != sentence.size(); ++word) {
  for (size_t word = 0; word != sentence.size(); ++word) {
    const std::string& symbol = sentence[word];
    const size_t& wlen = symbol.length();
    struct apply_handle::sigma_trie *st = h->sigma_trie;
    int signum = 0;

    for (size_t i = 0; i < wlen; i++) {
      st += (unsigned char)symbol[i];
      if (i == wlen - 1) {
        if (st->signum != 0) {
          signum = st->signum;
        }
      } else if (st->next != NULL) {
        st = st->next;
      } else {
        break;
      }
    }  // for i

    if (signum != 0) {
///      (h->sigmatch_array + at)->signumber = signum;
///      (h->sigmatch_array + at)->consumes = (h->sigs + signum)->length;  // wlen
      (h->sigmatch_array + word)->signumber = signum;
      (h->sigmatch_array + word)->consumes = (h->sigs + signum)->length;  // wlen
    } else {
      /* Not found */
///      (h->sigmatch_array + at)->signumber = IDENTITY;
///      (h->sigmatch_array + at)->consumes = wlen;
      (h->sigmatch_array + word)->signumber = IDENTITY;
      (h->sigmatch_array + word)->consumes = wlen;
    }

///    at += wlen;
  }  // for sentence
}

struct fsm* merge_sigma(struct fsm** fsms, size_t num_fsms) {
  std::vector<struct fsm*> fsmv;
  fsmv.reserve(num_fsms);
  for (size_t i = 0; i < num_fsms; i++) {
    fsmv.push_back(fsms[i]);
  }
  return merge_sigma(fsmv);
}

struct fsm* merge_sigma(std::vector<struct fsm*> fsms) {
  std::map<std::string, int> sigmas;
  struct fsm* ret = fsm_empty_string();

  /* First, we build the unified sigma table. */
  int last_id = IDENTITY;
  for (std::vector<struct fsm*>::const_iterator it = fsms.begin();
       it != fsms.end(); ++it) {
///    std::cout << "FST" << std::endl;
    struct fsm* fsm = *it;
    for (struct sigma* s = fsm->sigma; s != NULL; s = s->next) {
///      std::cout << "SIGMA " << s->symbol << ": " << s->number << std::endl;
      if (s->number > IDENTITY && sigmas.find(s->symbol) == sigmas.end()) {
        sigmas[s->symbol] = ++last_id;
        ret = fsm_concat(ret, fsm_symbol(s->symbol));
      }
    }
  }

///  for (std::map<std::string, int>::const_iterator it = sigmas.begin();
///       it != sigmas.end(); ++it) {
///    std::cout << it->first << ": " << it->second << std::endl;
///  }

  /* Then, we replace the sigmas. */
  for (std::vector<struct fsm*>::const_iterator it = fsms.begin();
       it != fsms.end(); ++it) {
    replace_sigma(*it, sigmas);
  }
  replace_sigma(ret, sigmas);
  return fsm_minimize(ret);
}

static void replace_sigma(struct fsm* fsm, std::map<std::string, int> sigmas) {
  std::map<int, int> sigma_mapping;
  for (struct sigma* s = fsm->sigma; s != NULL; s = s->next) {
    if (s->number > IDENTITY) {
      int new_number = sigmas[s->symbol];
      sigma_mapping[s->number] = new_number;
      s->number = new_number;
    }
  }
  for (int i = 0; (fsm->states + i)->state_no != -1; i++) {
    if ((fsm->states + i)->in > IDENTITY)
      (fsm->states + i)->in = sigma_mapping[(fsm->states + i)->in];
    if ((fsm->states + i)->out > IDENTITY)
      (fsm->states + i)->out = sigma_mapping[(fsm->states + i)->out];
  }

  /* Sort the input arcs -- necessary after we messed with the sigmas. */
  fsm_sort_arcs(fsm, 1);
}

