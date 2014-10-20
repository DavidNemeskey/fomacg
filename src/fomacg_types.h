#ifndef FOMACG_TYPES_H
#define FOMACG_TYPES_H

struct cg_sets {
  char *name;
  struct fsm *net;
  struct cg_sets *next;
};

/* Each rule is stored as it is built into the cg_rule_complex struct */

/* What do these fields mean? */
struct cg_rule_complex {
  struct fsm *wordform  ;
  struct fsm *target    ;
  struct fsm *condition ;
  struct fsm *barrier   ;
  int rule_not ;
  int scope_mode ;
  int scope_num  ;
  int num_links  ;                /* Number of LINKed conditions */
  struct cg_rule_complex *link;   /* Linked conditions   */
  struct cg_rule_complex *next;   /* multiple conditions */
};

/* We store each rule as a fsm + lineno combo */
struct cg_rules {
  int lineno;
  int section_no;
  struct fsm *rule;
  struct fsm *conditions;  /* Automaton that checks rule applicability. */
  struct fsm *filter;      /* Rule target filter. */
  struct cg_rules *next;
};

#endif
