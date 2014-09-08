#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <fomalib.h>

#include <string>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <vector>
#include <set>
#include <map>
#include <algorithm>

#include "factorize.h"
#include "stl_extra.h"

/** Nameless namespace to hide garbage. */
namespace {

/** From constructions.c. I HATE how foma is organized. */
// int add_fsm_arc(struct fsm_state *fsm, int offset, int state_no, int in,
//                int out, int target, int final_state, int start_state);

/**
 * Compares @p trans to a virtual transition with source state @p state_no and
 * input label @p in. Returns @c 0 if the transitions match, @c -1 if @p trans
 * should come before the virtual transition, and @c 1 if it should come after.
 *
 * @note @p trans should be a valid transition, i.e. its state_no must not be -1.
 */
int inline trans_cmp(struct fsm_state* trans, State state_no, SigmaSymbol in) {
  if (trans->state_no == state_no) {
    if (trans->in == in) {
      return 0;
    } else {
      return (trans->in < in) ? -1 : 1;
    }
  } else {
    return (trans->state_no < state_no) ? -1 : 1;
  }
}

/**
 * Finds the integer offset of a transition from state @p state_no with input
 * label @p in, if any. The offset is counted from <tt>fst->states</tt>.
 */
int find_transition(struct fsm *fst, State state_no, SigmaSymbol in) {
  int begin = 0;
  int end   = fst->linecount - 1;  // Last line is the -1, -1, -1, -1 line
  struct fsm_state* start = fst->states;
  while (begin < end) {
    int middle = (begin + end) / 2;
    struct fsm_state* trans = start + middle;
    int cmp = trans_cmp(trans, state_no, in);
    if (cmp == 0) {
      return middle;
    } else if (cmp < 0) {
      begin = middle + 1;
    } else {
      end = middle;
    }
  }
  return -1;
}

/**
 * Copies the sigma structure orig, but discards all entries not in @p filter
 * (if @p retain is @c true) or IN @p filter (if @p retain is @c false).
 */
struct sigma* copy_filter_sigma(struct sigma* orig, std::set<SigmaSymbol>& filter,
                                bool retain=true) {
  struct sigma* ret = NULL;
  struct sigma* last_sigma = NULL;
  for (; orig != NULL; orig = orig->next) {
    bool in_filter = filter.count(orig->number) == 1;
    if ((in_filter && retain) || (!in_filter && !retain)) {
      struct sigma* new_sigma = static_cast<struct sigma*>(
          calloc(1, sizeof(struct sigma)));
      new_sigma->number = orig->number;
      if (orig->symbol != NULL)
        new_sigma->symbol = strdup(orig->symbol);
      if (ret == NULL) {
        ret = last_sigma = new_sigma;
      } else {
        last_sigma->next = new_sigma;
        last_sigma = new_sigma;
      }
    }
  }
  return ret;
}

/** Simply copies the sigma alphabet. */
struct sigma* copy_sigma(struct sigma* orig) {
  std::set<SigmaSymbol> filter;
  return copy_filter_sigma(orig, filter, false);
}

/** Sequential FST apply down -- without the handle; who needs that? :) */
std::vector<SigmaSymbol> fst_apply_down(struct fsm* fst,
                                        const std::vector<SigmaSymbol>& input) {
  std::vector<SigmaSymbol> output;
  State q = 0;
  struct fsm_state* start = fst->states;
  for (std::vector<SigmaSymbol>::const_iterator symbol = input.begin();
       symbol != input.end(); ++symbol) {
    int trans_offset = find_transition(fst, q, *symbol);
    std::cout << "trans_offset " << trans_offset << std::endl;
    q = (fst->states + trans_offset)->target;
    output.push_back((fst->states + trans_offset)->out);
  }
  return output;
}

};

/** Returns a set that contains the start states of @p fst. */
std::set<State> get_start_states(const struct fsm* fst) {
  std::set<State> ret;
  struct fsm_state* elem = fst->states;
  for (int i = 0; (elem + i)->state_no != -1; i++) {
    if ((elem + i)->start_state == YES) ret.insert((elem + i)->state_no);
  }
  return ret;
}

/** Returns the set of final states in @p fst. */
std::set<State> get_final_states(const struct fsm* fst) {
  std::set<State> ret;
  struct fsm_state* elem = fst->states;
  for (int i = 0; (elem + i)->state_no != -1; i++) {
    if ((elem + i)->final_state == YES) ret.insert((elem + i)->state_no);
  }
  return ret;
}

std::vector<std::set<State> > reverse_set_mapping(
    const std::vector<std::set<State> >& C) {
  std::vector<std::set<State> > C_reverse;
  State C_len = static_cast<State>(C.size());
  for (State i = 0; i < C_len; i++) {
    for (std::set<State>::const_iterator it = C[i].begin(); it != C[i].end();
         ++it) {
      if (*it >= static_cast<State>(C_reverse.size())) {
        C_reverse.resize(static_cast<size_t>(*it) + 1);
      }
      C_reverse[*it].insert(i);
    }
  }
  return C_reverse;
}

/********************************* BiMachine **********************************/

BiMachine::BiMachine(struct fsm* fst) : A_1(NULL), A_2(NULL) {
  if (!fst->arcs_sorted_in) {
    fsm_sort_arcs(fst, 1);
  }
  struct fsm* upper = fsm_upper(fsm_copy(fst));
  std::vector<std::set<State> > C_1 = compute_A_1(upper);
  std::vector<std::set<State> > C_2 = compute_A_2(upper);
  fsm_destroy(upper);
  compute_delta(fst, C_1, C_2);
}

BiMachine::~BiMachine() {
  if (A_1 != NULL) {
    fsm_destroy(A_1);
    A_1 = NULL;
  }
  if (A_2 != NULL) {
    fsm_destroy(A_2);
    A_2 = NULL;
  }
}

std::vector<SigmaSymbol> BiMachine::bi_apply_down(std::vector<SigmaSymbol> input) {
  std::vector<SigmaSymbol> ret;
  std::vector<State> A_1_states;

  /* First, A_1 processes the string from left to right. */
  A_1_states.push_back(0);
  for (size_t a = 0; a < input.size(); a++) {
    //std::cout << "Read input[" << a << "] = " << input[a] << std::endl;
    struct fsm_state* elem = A_1->states;
    for (int i = 0; (elem + i)->state_no != -1; i++) {
      if ((elem + i)->state_no == A_1_states.back() && (elem + i)->in == input[a]) {
        //std::cout << (elem + i)->state_no << " -- " << (elem + i)->in
        //          << " --> " << (elem + i)->target << std::endl;
        A_1_states.push_back((elem + i)->target);
        break;
      }
    }
  }
  //A_1_states.pop_back();  // We are not interested in the final state
  std::cout << "A_1_states: " << join(A_1_states) << std::endl;

  State q2 = 0;
  for (size_t a = input.size(); a > 0; a--) {
    BiTrans bit(A_1_states[a - 1], input[a - 1], q2);
//    std::cout << "BiTrans(" << A_1_states[a-1] << ", " << input[a-1]
//              << ", " << q2 << ")" << std::endl;
    std::map<BiTrans, SigmaSymbol>::const_iterator it = delta.end();
    if (input[a - 1] != 2) {
      it = delta.find(bit);
    } else {
      it = find_delta_cell(bit, BiTrans(A_1_states[a - 1], 1, q2));
    }
    if (it != delta.end()) {
//      std::cout << "Found bitrans: " << it->second << std::endl;
      ret.push_back(it->second);
      struct fsm_state* elem = A_2->states;
      for (int i = 0; (elem + i)->state_no != -1; i++) {
        if ((elem + i)->state_no == q2 && (elem + i)->in == input[a - 1]) {
          q2 = (elem + i)->target;
        }
      }
    } else {
      std::cout << "Could not find bitrans" << std::endl;
      return std::vector<SigmaSymbol>();
    }
  }
  std::reverse(ret.begin(), ret.end());
  return ret;
}

std::map<BiTrans, SigmaSymbol>::const_iterator BiMachine::find_delta_cell(
    const BiTrans& bit1, const BiTrans& bit2) {
  std::map<BiTrans, SigmaSymbol>::const_iterator it = delta.find(bit1);
  if (it == delta.end())
    it = delta.find(bit2);
  return it;
}

std::vector<std::set<State> > BiMachine::compute_A_1(const struct fsm* fst) {
  std::map<Trans, State> d_A_1;
  std::set<SigmaSymbol> A_1_sigma;
  std::vector<std::set<State> > C_1;
  C_1.push_back(get_start_states(fst));
  std::set<State> orig_finals = get_final_states(fst);

  for (State q = 0; q < static_cast<State>(C_1.size()); q++) {
    //printf("q: %d, C_1: %zu, states: %d\n", q, C_1.size(), fst->statecount);
    std::set<State> S = C_1[q];
    /*
     * The algorithm loops by sigma, not source state, but that's not
     * convenient with struct fsm.
     */
    std::map<SigmaSymbol, std::set<State> > _S;  // S' = map<sigma, targets>
    struct fsm_state* elem = fst->states;
    for (int i = 0; (elem + i)->state_no != -1; i++) {
      /* Second condition: it's not an end-of-path state. */
      if (S.count((elem + i)->state_no) && (elem + i)->in >= 0) {
        _S[(elem + i)->in].insert((elem + i)->target);
        A_1_sigma.insert((elem + i)->in);
      }
    }
    for (std::map<SigmaSymbol, std::set<State> >::const_iterator it = _S.begin();
         it != _S.end(); ++it) {
      State e = add_set(C_1, it->second);
      //printf("Added set %s as state %d\n", join(it->second).c_str(), e);
      d_A_1[Trans(q, it->first)] = e;
    }
  }

  /* Final states and end-of-path states. */
  std::set<State> new_finals;
  std::set<State> eof_states;
  for (State q = 0; q < static_cast<State>(C_1.size()); q++) {
    if (set_in(C_1[q], orig_finals)) new_finals.insert(q);
    /* First we add all states as end-of-path, and filter in the next step. */
    eof_states.insert(q);
  }
  for (std::map<Trans, State>::const_iterator it = d_A_1.begin();
       it != d_A_1.end(); ++it) {
    eof_states.erase(it->first.source);
  }
  printf("New finals: %s, EOPs: %s\n", join(new_finals).c_str(), join(eof_states).c_str());

  A_1 = create_fsa("A_1", d_A_1, C_1, A_1_sigma, new_finals, eof_states, fst);
  return C_1;
}

std::vector<std::set<State> > BiMachine::compute_A_2(const struct fsm* fst) {
  std::map<Trans, State> d_A_2;
  std::set<SigmaSymbol> A_2_sigma;
  std::vector<std::set<State> > C_2;
  C_2.push_back(get_final_states(fst));
  std::set<int> orig_finals = get_start_states(fst);

  for (State q = 0; q < static_cast<State>(C_2.size()); q++) {
    std::set<State> S = C_2[q];
    printf("q: %d, S: (%s)\n", q, join(S).c_str());
    /*
     * The algorithm loops by sigma, not source state, but that's not
     * convenient with struct fsm.
     */
    std::map<SigmaSymbol, std::set<State> > _S;  // S' = map<sigma, sources>
    struct fsm_state* elem = fst->states;
    for (int i = 0; (elem + i)->state_no != -1; i++) {
      /* Second condition: it's not an end-of-path state. */
      if (S.count((elem + i)->target) && (elem + i)->in >= 0) {
        _S[(elem + i)->in].insert((elem + i)->state_no);
        A_2_sigma.insert((elem + i)->in);
      }
    }
    for (std::map<SigmaSymbol, std::set<State> >::const_iterator it = _S.begin();
         it != _S.end(); ++it) {
      State e = add_set(C_2, it->second);
      d_A_2[Trans(q, it->first)] = e;
      printf("q: %d -- %d --> %d = set: (%s)\n", q, it->first,
             e, join(it->second).c_str());
    }
  }

  /* Final states and end-of-path states. */
  std::set<State> new_finals;
  std::set<State> eof_states;
  for (State q = 0; q < static_cast<State>(C_2.size()); q++) {
    if (set_in(C_2[q], orig_finals)) new_finals.insert(q);
    /* First we add all states as end-of-path, and filter in the next step. */
    eof_states.insert(q);
  }
  for (std::map<Trans, int>::const_iterator it = d_A_2.begin();
       it != d_A_2.end(); ++it) {
    eof_states.erase(it->first.source);
  }

  A_2 = create_fsa("A_2", d_A_2, C_2, A_2_sigma, new_finals, eof_states, fst);
  return C_2;
}

int BiMachine::add_set(std::vector<std::set<State> >& C, const std::set<State>& x) {
  for (State e = 0; e < static_cast<State>(C.size()); e++) {
    if (set_equal(x, C[e])) return e;
  }
  C.push_back(x);
  return static_cast<State>(C.size() - 1);
}

struct fsm* BiMachine::create_fsa(const std::string& name,
                                  std::map<Trans, State> d_A,
                                  std::vector<std::set<State> > C,
                                  std::set<SigmaSymbol> A_sigma,
                                  std::set<State> new_finals,
                                  std::set<State> eof_states,
                                  const struct fsm* fst) {
  /** First the transitions... */
  struct fsm* A = static_cast<struct fsm*>(calloc(1, sizeof(struct fsm)));
  sprintf(A->name, "%s", name.c_str());
  A->states = static_cast<struct fsm_state*>(
      calloc(d_A.size() + eof_states.size() + 1, sizeof(struct fsm_state)));
  int offset = 0;
  /*
   * End-of-path states need entries, too. C_covered maintains which states are
   * "covered"; i.e. which states are sources of transitions, and hence not EOP.
   */
  std::vector<bool> C_covered(C.size(), false);
  /* First we add the reguler transitions... */
  for (std::map<Trans, int>::const_iterator it = d_A.begin();
       it != d_A.end(); ++it) {
    C_covered[it->first.source] = true;
    offset = add_fsm_arc(A->states, offset, it->first.source,
                         it->first.in, it->first.in, it->second,
                         new_finals.count(it->first.source) /* final? */,
                         C[0].count(it->first.source) /* start? */);
  }
  /* ... and then the EOP states. */
  for (State q = 0; q < static_cast<int>(C_covered.size()); q++) {
    if (!C_covered[q]) {
      offset = add_fsm_arc(A->states, offset, q, -1, -1, -1,
                           new_finals.count(q) /* final? */,
                           C[0].count(q) /* start? */);
    }
  }
  /* End-of-transitions mark. */
  add_fsm_arc(A->states, offset, -1, -1, -1, -1, -1, -1);

  /* ... then the sigma ... */
  A->sigma = copy_filter_sigma(fst->sigma, A_sigma);

  /* ... and finally the miscellaneous stuff. */
  A->arity = 1;
  A->arccount = static_cast<int>(d_A.size());
  A->statecount = static_cast<int>(C.size());
  A->linecount = static_cast<int>(d_A.size() + eof_states.size() + 1);
  A->finalcount = static_cast<int>(new_finals.size());
  A->pathcount = PATHCOUNT_UNKNOWN;  // PATHCOUNT_UNKNOWN
  A->is_deterministic = A->is_minimized = A->is_epsilon_free = 1;
  A->is_pruned       = UNK;
  A->is_minimized    = UNK;
  A->is_epsilon_free = (A->sigma->number == EPSILON) ? NO : YES;
  A->is_loop_free    = UNK;
  A->is_completed    = UNK;
  fsm_sort_arcs(A, 1);

  return A;
}

void BiMachine::compute_delta(const struct fsm* fst,
                              std::vector<std::set<State> > C_1,
                              std::vector<std::set<State> > C_2) {
  /* Reverse mappings: { old state: new states }.*/
  std::vector<std::set<State> > C_1_reverse = reverse_set_mapping(C_1);
  std::vector<std::set<State> > C_2_reverse = reverse_set_mapping(C_2);

  struct fsm_state* elem = fst->states;
  for (int i = 0; (elem + i)->state_no != -1; i++) {
    /* It's not an end-of-path state. */
    if ((elem + i)->in >= 0) {
      State q1 = (elem + i)->state_no;
      State q2 = (elem + i)->target;
      SigmaSymbol a = (elem + i)->in;
      SigmaSymbol w = (elem + i)->out;
      for (std::set<State>::const_iterator new_q1 = C_1_reverse[q1].begin();
           new_q1 != C_1_reverse[q1].end(); ++new_q1) {
        for (std::set<State>::const_iterator new_q2 = C_2_reverse[q2].begin();
             new_q2 != C_2_reverse[q2].end(); ++new_q2) {
          delta[BiTrans(*new_q1, a, *new_q2)] = w;
        }  // new_q2
      }  // new_q1
    }  // if
  }  // for

  std::map<short int, std::string> sigmas;
  for (struct sigma* sigma = fst->sigma; sigma != NULL; sigma = sigma->next) {
    sigmas[sigma->number] = sigma->symbol;
  }

  std::cout << std::endl << std::endl << "Delta:" << std::endl;
  for (std::map<BiTrans, short int>::const_iterator it = delta.begin();
       it != delta.end(); ++it) {
    std::cout << "(" << it->first.q_1 << "(" << join(C_1[it->first.q_1])
              << "), " << it->first.a << "[" << sigmas[it->first.a] << "], "
              << it->first.q_2 << "(" << join(C_2[it->first.q_2]) << ")) -> "
              << it->second << "[" << sigmas[it->second] << "]" << std::endl;
  }
}

/**************************** LeftRightSequential *****************************/

LeftRightSequential::LeftRightSequential(const struct fsm* fst,
                                         const BiMachine& bimachine) {
  compute_ts(fst, bimachine);
}

LeftRightSequential::LeftRightSequential(
    struct fsm* T_1_, struct fsm* T_2_) : T_1(T_1_), T_2(T_2_) {}

LeftRightSequential::~LeftRightSequential() {
  if (T_1 != NULL) {
    fsm_destroy(T_1);
    T_1 = NULL;
  }
  if (T_2 != NULL) {
    fsm_destroy(T_2);
    T_2 = NULL;
  }
}

void LeftRightSequential::compute_ts(const struct fsm* fst,
                                     const BiMachine& bimachine) {
  T_1 = fsm_copy(bimachine.A_1);
  T_2 = fsm_copy(bimachine.A_2);
  fsm_sort_arcs(T_1, 1);
  fsm_sort_arcs(T_2, 1);
  std::map<Trans, SigmaSymbol> alphabet;  // (source, in) -> T_1_out_symbol mapping
  SigmaSymbol next_symbol = IDENTITY + 1;

  /*
   * First modify T_1's output: a unique symbol for each source state - input
   * symbol pair.
   */
  struct fsm_state* T_1_transitions = T_1->states;
  for (int i = 0; (T_1_transitions + i)->state_no != -1; i++) {
    if ((T_1_transitions + i)->in == -1) continue;  // End-of-path state

    Trans t((T_1_transitions + i)->state_no, (T_1_transitions + i)->in);
    std::map<Trans, SigmaSymbol>::iterator it = alphabet.find(t);
    if (it == alphabet.end()) {
      (T_1_transitions + i)->out = next_symbol;
      alphabet[t] = next_symbol++;
    } else {
      (T_1_transitions + i)->out = it->second;
    }
  }

  // XXX
  std::cout << std::endl << "Alphabet:" << std::endl;
  for (std::map<Trans, SigmaSymbol>::const_iterator it = alphabet.begin();
       it != alphabet.end(); ++it) {
    std::cout << "(" << it->first.source << ", " << it->first.in << ") -> "
              << it->second << std::endl;
  }

  /*
   * And now we overwrite the input and output labels in T_2, based on delta.
   * Each entry in delta is a transition in T_2; input is the T_1 output symbol
   * that corresponds to the A_1 state and input in the delta entry; output to
   * the w.
   */
  std::vector<Transition> edges;
  for (std::map<BiTrans, SigmaSymbol>::const_iterator it = bimachine.delta.begin();
       it != bimachine.delta.end(); ++it) {
    /* The usual UNKNOWN -> IDENTITY conversion. */
    const SigmaSymbol& in = it->first.a == UNKNOWN ? IDENTITY : it->first.a;
    const SigmaSymbol& out = it->second;
    const State& q_1 = it->first.q_1;
    const State& q_2 = it->first.q_2;
    /* We search in A_2, because it remains unchanged, while T_2 does not. */
    int trans_offset = find_transition(bimachine.A_2, q_2, in);
    struct fsm_state* T_2_trans = T_2->states + trans_offset;
    T_2_trans->in  = alphabet[Trans(q_1, in)];
    T_2_trans->out = out;
    edges.push_back(Transition(T_2_trans->state_no, T_2_trans->target,
                               alphabet[Trans(q_1, in)], out));
  }

  std::sort(edges.begin(), edges.end());
  std::cout << std::endl << "Edges:" << std::endl << join(edges, "\n")
            << std::endl << std::endl;

  xxfree(T_2->states);
  T_2->states = static_cast<fsm_state*>(xxmalloc(sizeof(struct fsm_state) * (edges.size() + 1)));
  T_2->arccount = edges.size();
  T_2->linecount = edges.size() + 1;
  T_2->finalcount = 1;  // For the time being -- we don't need these

  int offset = 0;
  for (std::vector<Transition>::const_iterator edge = edges.begin();
       edge != edges.end(); ++edge) {
    offset = add_fsm_arc(T_2->states, offset, edge->source, edge->in, edge->out,
                         edge->target, 0, edge->source == 0 ? 1 : 0);
  }
  add_fsm_arc(T_2->states, offset, -1, -1, -1, -1, -1, -1);

//  std::map<int, int> transes;
//  for (std::map<BiTrans, SigmaSymbol>::const_iterator it = bimachine.delta.begin();
//       it != bimachine.delta.end(); ++it) {
//    /* The usual UNKNOWN -> IDENTITY conversion. */
//    SigmaSymbol a = it->first.a;
//    if (a == UNKNOWN) {
//      a = IDENTITY;
//    }
//    /* We search in A_2, because it remains unchanged, while T_2 does not. */
//    int trans_offset = find_transition(bimachine.A_2, it->first.q_2, a);
//    transes[trans_offset]++;
//    std::cout << "q2: " << it->first.q_2 << ", a: " << a
//              << ", in: " << alphabet[Trans(it->first.q_1, a)]
//              << " => " << trans_offset << std::endl;
//    //struct fsm_state* A_2_trans = A_2->states + trans_offset;
//    struct fsm_state* T_2_trans = T_2->states + trans_offset;
//    T_2_trans->in  = alphabet[Trans(it->first.q_1, a)];
//    T_2_trans->out = it->second;
//  }
//  // TODO: what is T_2_trans for? I am not using it, but that must be an error.
//  std::cout << "Transes" << std::endl;
//  for (std::map<int, int>::const_iterator it = transes.begin(); it != transes.end(); ++it) {
//    std::cout << it->first << ": " << it->second << std::endl;
//  }

  /* Rounding up... */
  snprintf(T_1->name, 40, "%s_T_1", fst->name);
  T_1->arity = 2;
  snprintf(T_2->name, 40, "%s_T_2", fst->name);
  T_2->arity = 2;
  fsm_sigma_destroy(T_2);
  T_2->sigma = copy_sigma(fst->sigma);
  fsm_sort_arcs(T_1, 1);
  fsm_sort_arcs(T_2, 1);
}

std::vector<SigmaSymbol> LeftRightSequential::ts_apply_down(
    std::vector<SigmaSymbol> input) {
  std::cout << "Sentence is: " << std::endl;
  for (size_t i = 0; i < input.size(); i++) {
    std::cout << input[i] << " ";
  }
  std::cout << std::endl << std::endl;
  std::vector<SigmaSymbol> intermediate = fst_apply_down(T_1, input);
  std::cout << "Intermediate: ";
  for (size_t i = 0; i < intermediate.size(); i++) {
    std::cout << intermediate[i] << " ";
  }
  std::cout << std::endl << std::endl;
  std::reverse(intermediate.begin(), intermediate.end());
//  std::cout << "Intermediate reversed: ";
//  for (size_t i = 0; i < intermediate.size(); i++) {
//    std::cout << intermediate[i] << " ";
//  }
//  std::cout << std::endl;
  std::vector<SigmaSymbol> output = fst_apply_down(T_2, intermediate);
  std::reverse(output.begin(), output.end());
  std::cout << "Output is: " << std::endl;
  for (size_t i = 0; i < input.size(); i++) {
    std::cout << input[i] << " ";
  }
  std::cout << std::endl << std::endl;
  return output;
}

LeftRightSequential* fst_to_left_right(struct fsm* fst) {
  /* Sorting is needed because of the next step. */
  if (!fst->arcs_sorted_in) {
    fsm_sort_arcs(fst, 1);
  }
  /* The starting state should not be final. */
  struct fsm_state* curr_line;
  for (curr_line = fst->states; curr_line->state_no == 0; curr_line++) {
    curr_line->final_state = 0;
  }

  fst->finalcount--;
  BiMachine bi(fst);
  LeftRightSequential* lrs = new LeftRightSequential(fst, bi);
  fsm_destroy(fst);
  return lrs;
}
