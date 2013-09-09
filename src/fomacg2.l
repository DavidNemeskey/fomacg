/*     Foma: a finite-state toolkit and library.                             */
/*     Copyright © 2008-2012 Mans Hulden                                     */

/*     This file is part of foma.                                            */

/*     Foma is free software: you can redistribute it and/or modify          */
/*     it under the terms of the GNU General Public License version 2 as     */
/*     published by the Free Software Foundation.                            */

/*     Foma is distributed in the hope that it will be useful,               */
/*     but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         */
/*     GNU General Public License for more details.                          */

/*     You should have received a copy of the GNU General Public License     */
/*     along with foma.  If not, see <http://www.gnu.org/licenses/>.         */

/*     Rudimentary CG2 rule => FST compiler                                  */
/*     Compiles .rle files as documented in the paper:                       */
/*     Hulden, M. (2011). Constraint Grammar parsing with left and right     */
/*     sequential finite transducers. In Proceedings of the 9th FSMNLP.      */

/*     Compile with: flex -8 src/fomacg2.l ;                                 */
/*                   g++ -Wno-write-strings -o bin/fomacg2 lex.yy.c          */
/*                       src/rule_condition_tree.c -lfoma -lz                */
/*     Run: fomacg2 file.rle                                                 */
/*     Currently only outputs a .prolog file for each rule in the .rle file  */
/*     where the filename is RXXX.prolog where XXX is the line number the    */
/*     rule is found on.                                                     */


%option noyywrap
%option nounput
%option yylineno

%{

#ifndef FOMA_INCLUDED
#define FOMA_INCLUDED
#include <stdbool.h>  // to avoid errors in fomalib.h
#include "fomalib.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "zlib.h"

#include "fomacg_types.h"
#include "rule_condition_tree.h"
#include "foma_extra.h"

#define MODE_CAREFUL  1
#define MODE_STAR     2
#define MODE_TWOSTAR  4
#define MODE_ABSOLUTE 8
#define MODE_NOT      16

#define WHERE_TARGET    1
#define WHERE_CONDITION 2
#define WHERE_BARRIER   3

#define TYPE_SET      1
#define TYPE_TAG      2

#define OP_NONE       0
#define OP_OR         1
#define OP_PLUS       2
#define OP_MINUS      3

#define OP_SELECT     1
#define OP_REMOVE     2

#define VACUOUS_STATECOUNT 7

  int g_debug = 1;
  int lineno_begin_rule;
  /** The number of the current section. */
  int section_no = 0;
  /** The prefix of the binary output files. */
  char* binary_prefix;

  int total_rules = 0 ;
  int total_states = 0;
  int total_arcs = 0;

  int rule_operation;
  int curr_op;

  char *current_error;
  int current_error_line;
  char *first_error_sym;
  char *cg_list_name = NULL;   /* Name of current list */

  struct fsm *CG_DEF_NODEL, *CG_DEF_ConstrainS, *CG_DEF_ConstrainR,
             *CG_DEF_Rollback, *CG_DEF_Cleanup, *CG_DEF_InitialFilter,
             *CG_DEF_MWFPROLOG, *CG_DEF_MWFEPILOG, *CG_DEF_ANYCOHORT,
             *CG_DEF_MATCHCOHORT1, *CG_DEF_MATCHCOHORT2, *CG_DEF_DEAD,
             *CG_DEF_ALIVE, *CG_DEF_DEL, *CG_DEF_DeleteX, *CG_DEF_Modified;

  /* For the DELIMITERS. */
  struct fsm *cg_delim_wordforms = NULL;
  struct fsm* delimiter_rule;

  struct fsm *cg_list_currelement = NULL;
  struct fsm *cg_list_currcelement = NULL;

  struct cg_sets *cg_sets = NULL;

  /* Each rule is stored as it is built into the cg_rule_complex struct */

  struct cg_rule_complex *cg_rule_complex, *cg_rule_complex_current,
                         *cg_rule_complex_current_head;

  /* We store each rule as a fsm + lineno combo */
  struct cg_rules *cg_rules = NULL;

void dprint(const char* format, ...) {
  va_list args;
  if (g_debug == 1) {
    va_start(args, format);       	
    vfprintf(stdout, format, args);
    va_end(args);
  }
}

/**
 * Parses the scope string and fills the scope-related fields of cg_rule_complex
 * accordingly.
 */
void cg_parse_scope(char *string, struct cg_rule_complex *cgr) {
  size_t skipnum;
  if (strncmp(string,"**",2)==0) {
    cgr->scope_mode = MODE_TWOSTAR;
  } else if  (strncmp(string,"*",1)==0) {
    cgr->scope_mode = MODE_STAR;
  }
  if (strchr(string,'@') != NULL) {
    cgr->scope_mode = cgr->scope_mode + MODE_ABSOLUTE;
  }
  if (strchr(string,'C') != NULL) {
    cgr->scope_mode = cgr->scope_mode + MODE_CAREFUL;
  }
  skipnum = strcspn(string, "-0123456789");
  cgr->scope_num = (int) strtol(string+skipnum, NULL, 10);

  /* Sanity check */
  if (cgr->scope_num == 0 && (cgr->scope_mode & MODE_ABSOLUTE)) {
    printf("Error: position @0 is not possible in scope on line %i.\n",yylineno);
    exit(0);
  }
}

void cg_init(void) {
  printf("Loading auxiliary networks...\n");
  /* Consumes everything. */
  CG_DEF_NODEL = fsm_read_binary_file("data/cg_NODEL.foma");
  CG_DEF_ConstrainS = fsm_read_binary_file("data/cg_ConstrainS.foma");
  CG_DEF_ConstrainR = fsm_read_binary_file("data/cg_ConstrainR.foma");
  CG_DEF_Rollback = fsm_read_binary_file("data/cg_Rollback.foma");  // = invCR
  CG_DEF_Cleanup = fsm_read_binary_file("data/cg_Cleanup.foma");
  CG_DEF_InitialFilter = fsm_read_binary_file("data/cg_InitialFilter.foma");
  CG_DEF_MWFPROLOG = fsm_read_binary_file("data/cg_MWFPROLOG.foma");
  CG_DEF_MWFEPILOG = fsm_read_binary_file("data/cg_MWFEPILOG.foma");
  CG_DEF_ANYCOHORT = fsm_read_binary_file("data/cg_ANYCOHORT.foma");
  CG_DEF_MATCHCOHORT1 = fsm_read_binary_file("data/cg_MATCHCOHORT1.foma");
  CG_DEF_MATCHCOHORT2 = fsm_read_binary_file("data/cg_MATCHCOHORT2.foma");
  CG_DEF_DEAD = fsm_read_binary_file("data/cg_DEAD.foma");
  CG_DEF_ALIVE = fsm_read_binary_file("data/cg_ALIVE.foma");
  CG_DEF_DEL = fsm_read_binary_file("data/cg_DEL.foma");
  CG_DEF_DeleteX = fsm_read_binary_file("data/cg_DeleteX.foma");
  CG_DEF_Modified = fsm_read_binary_file("data/cg_Modified.foma");
  cg_rule_complex =
  cg_rule_complex_current =
  cg_rule_complex_current_head = (struct cg_rule_complex*)calloc(1,sizeof(struct cg_rule_complex));
}

void cg_print_rule_info(struct fsm *net) {
  printf("%i: %i states, %i arcs.\n",lineno_begin_rule,net->statecount, net->arccount);
  if (net->statecount == VACUOUS_STATECOUNT) {
    printf("WARNING: rule is vacuous!\n");
  }
}

struct fsm *cg_coerce_rules(struct fsm *rawrules) {
  /* rawrules .o. ~[rawrules .o. Rollback].l .o. Cleanup */

  struct fsm *result;
  result = fsm_minimize(fsm_compose(
               fsm_copy(rawrules),
               fsm_compose(
                   fsm_complement(fsm_lower(fsm_compose(
                       fsm_copy(rawrules),
                       fsm_copy(CG_DEF_Rollback)))),
                   fsm_copy(CG_DEF_Cleanup))));
  return(result);
}

/** @note destroys both parameters. */
struct fsm *cg_mark_form_target(struct fsm *wordform, struct fsm *target) {
  struct fsm *result, *constrain1, *constrain2;

  /* prolog .o. constrain .o. epilog */

  /* prolog =~$["$00$"|"#00#"|"$1$ "] .o. "$0$ " -> ["$1$ "|"$00$"] , "#0# " -> ["#1# "|"#00#"]        */
  /* constrain = ~[?* ["$1$ " ~[WORDFORM ?*]|"#1# " ~[TARGET ?*]]] & ~$["$00$" WORDFORM|"#00#" TARGET] */
  /* epilog = "$00$" -> "$0$ " , "#00#" -> "#0# "                                                      */

  if (wordform == NULL) {
    wordform = fsm_universal();
  }
  /* remove auxiliaries for speed, i.e. target & ~$["#1# "|"#00#"] */
  target = fsm_intersect(
               target,
               fsm_complement(fsm_contains(fsm_union(
                   fsm_symbol("#1# "),
                   fsm_symbol("#00#")))));

  constrain1 = fsm_minimize(fsm_complement(fsm_concat(
                   fsm_universal(),
                   fsm_union(
                       fsm_concat(
                           fsm_symbol("$1$ "),
                           fsm_complement(fsm_concat(
                               fsm_copy(wordform),
                               fsm_universal()))),
                       fsm_concat(
                           fsm_symbol("#1# "),
                           fsm_complement(fsm_concat(
                               fsm_copy(target),
                               fsm_universal())))))));

  constrain2 = fsm_complement(fsm_contains(fsm_union(
                   fsm_concat(fsm_symbol("$00$"), fsm_copy(wordform)),
                   fsm_concat(fsm_symbol("#00#"), fsm_copy(target)))));

  result = fsm_minimize(fsm_compose(
               fsm_compose(
                   fsm_copy(CG_DEF_MWFPROLOG),
                   fsm_intersect(constrain1, constrain2)),
               fsm_copy(CG_DEF_MWFEPILOG)));
  fsm_destroy(wordform);
  fsm_destroy(target);
  return(result);
}

/**
 * Returns the FSM of the set called @p string, or @c NULL, if no set by that
 * name is defined.
 */
struct fsm *cg_set_find(char *string) {
  struct cg_sets *cgs;
  for (cgs = cg_sets; cgs != NULL; cgs = cgs->next) {
    if (strcmp(string, cgs->name) == 0) {
      return cgs->net;
    }
  }
  return NULL;
}

/** What CompileRule(X) does in the paper. */
struct fsm *cg_rule_wrapup(struct fsm *rawrule) {
  /* [Rawrule .o. ~[Rawrule .o. Rollback].l .o. Cleanup]; */

  struct fsm *result;
  result = fsm_topsort(
             fsm_minimize(
               fsm_compose(
                 fsm_compose(
                   fsm_compose(
                     fsm_copy(rawrule),
                     fsm_complement(fsm_lower(fsm_compose(
                       fsm_copy(rawrule),
                       fsm_copy(CG_DEF_Rollback))))),
                   fsm_copy(CG_DEF_Cleanup)),
                 fsm_copy(CG_DEF_DeleteX))));
  fsm_sort_arcs(result, 1);
  return result;
}

/**
 * Creates an automaton that accepts the sentence if the rule transducer
 * created from @p rawrule would change it.
 */
struct fsm *cg_create_checker(struct fsm* rawrule) {
  struct fsm *result;
  result = fsm_topsort(
             fsm_minimize(
               fsm_determinize(
                 fsm_upper(
                   fsm_compose(
                     fsm_copy(rawrule),
                     fsm_copy(CG_DEF_Modified))))));
  fsm_sort_arcs(result, 1);
  return result;
}

void cg_add_rule_to_chain(struct fsm *rule, struct fsm *conditions) {
  struct cg_rules *newrule;
  char rulename[20];
  sprintf(rulename, "R_%d_%i", section_no, lineno_begin_rule);
  strcpy(rule->name, rulename);
  sprintf(rulename, "C_%d_%i", section_no, lineno_begin_rule);
  strcpy(conditions->name, rulename);

  newrule = (struct cg_rules*)malloc(sizeof(struct cg_rules));
  if (cg_rules == NULL) {
    cg_rules = newrule;
    cg_rules->next = NULL;
  } else {
    newrule->next = cg_rules;
    cg_rules = newrule;
  }
  cg_rules->rule = rule;
  cg_rules->conditions = conditions;
  cg_rules->lineno = lineno_begin_rule;
  cg_rules->section_no = section_no;
  total_states += rule->statecount;
  total_arcs += rule->arccount;
  total_rules++;
}

struct fsm *cg_match_cohort(struct fsm *net) {
  /* [\"#BOC# "* "#BOC# " \"#EOC# "* DEL ALIVE \DEL* X \"#EOC# "* "#EOC# " ~$"#BOC# "] */
  return fsm_minimize(fsm_concat(
             fsm_copy(CG_DEF_MATCHCOHORT1),
             fsm_concat(
                 net,
                 fsm_copy(CG_DEF_MATCHCOHORT2))));
}

struct fsm *cg_match_cohort_careful(struct fsm *net) {
  struct fsm *A,*B,*Center,*Pre,*Post,*result;
  /* ~$"#BOC# " "#BOC# " [[DEL DEAD \DEL*]* [DEL ALIVE \DEL* "DET " \DEL*] [DEL DEAD \DEL*]*]+ DEL "#EOC# " ~$"#BOC# " */
  /* Pre                   A                            B                      A                    Post */
  /* Pre [A B A]+ Post */

  Pre = fsm_concat(
            fsm_complement(fsm_contains(fsm_symbol("#BOC# "))),
            fsm_symbol("#BOC# "));
  Post = fsm_concat(
             fsm_symbol("| "),
             fsm_concat(
                 fsm_symbol("#EOC# "),
                 fsm_complement(fsm_contains(fsm_symbol("#BOC# ")))));
  A = fsm_kleene_star(fsm_concat(
          fsm_copy(CG_DEF_DEL),
          fsm_concat(
              fsm_copy(CG_DEF_DEAD),
              fsm_copy(CG_DEF_NODEL))));
  B = fsm_concat(
          fsm_copy(CG_DEF_DEL),
          fsm_concat(
              fsm_copy(CG_DEF_ALIVE),
              fsm_concat(
                  fsm_copy(CG_DEF_NODEL),
                  fsm_concat(net, fsm_copy(CG_DEF_NODEL)))));
  Center = fsm_kleene_plus(fsm_concat(
               fsm_copy(A),
               fsm_concat(
                   fsm_copy(B),
                   fsm_copy(A))));
  result = fsm_minimize(fsm_concat(Pre, fsm_concat(Center, Post)));
  fsm_destroy(A);
  fsm_destroy(B);
  return(result);
}

struct fsm *cg_simple_crr(struct fsm *C, struct fsm *R) {
  /* C => _ R = ~[?* C ~[R ?*]] */
  return fsm_minimize(fsm_complement(fsm_concat(
             fsm_universal(),
             fsm_concat(
                 C,
                 fsm_complement(fsm_concat(R, fsm_universal()))))));
}

struct fsm *cg_simple_crrnot(struct fsm *C, struct fsm *R) {
  /* ~$[C R] */
  return fsm_minimize(fsm_complement(fsm_contains(fsm_concat(C, R))));
}

struct fsm *cg_simple_crl(struct fsm *C, struct fsm *L) {
  /* C => L _ = ~[~[?* L] C ?*] */
  return fsm_minimize(fsm_complement(fsm_concat(
             fsm_complement(fsm_concat(fsm_universal(), L)),
             fsm_concat(C, fsm_universal()))));
}

struct fsm *cg_simple_crlnot(struct fsm *C, struct fsm *L) {
  /* ~$[L C] */
  return fsm_minimize(fsm_complement(fsm_contains(fsm_concat(L, C))));
}

struct fsm *cg_insertaux(int count) {
  char linknumstring[10];
  int i;
  struct fsm *L, *R1, *R2, *result;
  /* [?-"$0$ "-"$A$ "-"$R$ "-"$1$ "-"$2$ "-...] | ["$0$ "|"$A$ "|"$R$ "] 0:[("$1$ ") ("$2$ ") ... ]]* */
  /* L                                              R1                       R2                          */
  L = fsm_identity();
  for (i = 1; i <= count; i++) {
    sprintf(linknumstring, "$%i$ ", i);
    L = fsm_minus(L, fsm_symbol(linknumstring));	
  }
  R2 = fsm_empty_string();
  for (i = 1; i <= count; i++) {
    sprintf(linknumstring, "$%i$ ", i);
    R2 = fsm_concat(R2, fsm_optionality(fsm_symbol(linknumstring)));
  }
  R1 = fsm_union(
           fsm_symbol("$0$ "),
           fsm_union(fsm_symbol("$A$ "), fsm_symbol("$R$ ")));
  /* [L | | R1 0:R2]* */
  result = fsm_minimize(fsm_kleene_star(fsm_union(
           L,
           fsm_concat(
               R1,
               fsm_cross_product(fsm_empty_string(), R2)))));
  return(result);    
}

struct fsm *cg_removeaux(int count) {
  char linknumstring[10];
  int i;
  struct fsm *L, *R, *result;
  /* [ [?-"$1$ "-"$2$ "-...] | ["$1$ "|"$2$ "|...]:0 ]* */

  L = fsm_identity();
  for (i = 1; i <= count; i++) {
    sprintf(linknumstring, "$%i$ ", i);
    L = fsm_minus(L, fsm_symbol(linknumstring));	
  }
  R = fsm_empty_set();
  for (i = 1; i <= count; i++) {
    sprintf(linknumstring, "$%i$ ", i);
    R = fsm_union(R, fsm_symbol(linknumstring));	
  }
  R = fsm_cross_product(R, fsm_empty_string());
  result = fsm_minimize(fsm_kleene_star(fsm_union(L, R)));
  return(result);
}

void cg_free_rule_complex() {
  struct cg_rule_complex* to_del = cg_rule_complex_current_head;
  while (to_del != NULL) {
    /* Delete fsms. */
    if (to_del->wordform != NULL)  fsm_destroy(to_del->wordform);
    if (to_del->target != NULL)    fsm_destroy(to_del->target);
    if (to_del->condition != NULL) fsm_destroy(to_del->condition);
    if (to_del->barrier != NULL)   fsm_destroy(to_del->barrier);
    /* Delete linked conditions. */
    struct cg_rule_complex* to_del_link = to_del->link;
    while (to_del_link != NULL) {
      struct cg_rule_complex* to_del_link_next = to_del_link->link;
      free(to_del_link);
      to_del_link = to_del_link_next;
    }
    /* Delete this object and go to the next. */
    struct cg_rule_complex* to_del_next = to_del->next;
    free(to_del);
    to_del = to_del_next;
  }
}

/** Compiles the rule. */
/* TODO: understand this. */
void cg_compile_rule() {
  char linknumstring[10], headnumstring[10];  // for sprintf
  int numskip,                      
      linkcount;        // Counts the number of links in the inner loop
  struct cg_rule_complex *rules,    // Running variable in the outer loop
                         *linkrule; // The currently analysed linked rule
  struct fsm *rawrules,
             *constrain,    // SELECT or REMOVE (ConstrainS or ConstrainR)
             *tests,        // TODO: should be "test"? The test part in the rule
             *thisrule,
             *rule,         // The "wrapped up" (complete) rule
             *checker,      // The rule checker FSA
             *match,
             *skip,
             *scan,
             *thistest;
  if (rule_operation == OP_SELECT) {
    constrain = fsm_copy(CG_DEF_ConstrainS);
  } else if (rule_operation == OP_REMOVE) {
    constrain = fsm_copy(CG_DEF_ConstrainR);
  } else {
    constrain = fsm_copy(CG_DEF_ConstrainS);
  }

  for (rawrules = NULL, rules = cg_rule_complex; rules != NULL; rules = rules->next) {
    /* Compile rule */

    /* "$A " => _ skip^(n) scan* (linknum) match         (scope >=0)      */
    /* "$A " => (linknum) match scan* skip^(abs(s+1)) _  (scope < 0)      */
    /* ~$["$A " (linknum) skip^(n) scan* (linknum) match (scope >=0, NOT) */
    /* ~$[(linknum) match scan* skip^(abs(s+1)) "$A$ "   (scope < 0, NOT) */

    /* skip = ANYCOHORT                                       */
    /* scan = ANYCOHORT - MC(barrier) - MC(match) [in *-mode] */
    /* match = MC(condition)                                  */

    for (linkcount = 1, linkrule = rules; linkrule != NULL; linkrule = linkrule->link, linkcount++) {
      if (linkrule->condition != NULL) {
        /* MATCH */

        /* Normal / safe matching. */
        if (linkrule->scope_mode & MODE_CAREFUL) {
          match = cg_match_cohort_careful(fsm_copy(linkrule->condition));
        } else {
          match = cg_match_cohort(fsm_copy(linkrule->condition));
        }
        /* TODO: add $linkcount$ to the above? */
        if (linkrule->link != NULL) {
          sprintf(linknumstring,"$%i$ ",linkcount);
          match = fsm_minimize(fsm_concat(fsm_symbol(linknumstring), match));
        }
        /* TODO: the head: $linkcount-1$ or $A$. */
        if (linkcount > 1) {
          sprintf(headnumstring,"$%i$ ", linkcount-1);
        } else {
          sprintf(headnumstring,"$A$ ");
        }

        /* SCAN */

        /* Create an ANYCOHORT "train" of scope length. */
        numskip = linkrule->scope_num >= 0 ? linkrule->scope_num
                                           : abs(linkrule->scope_num + 1);
        skip = fsm_concat_n(fsm_copy(CG_DEF_ANYCOHORT), numskip);

        if (linkrule->scope_mode & (MODE_STAR | MODE_TWOSTAR)) {
          /* Scan */
          scan = fsm_copy(CG_DEF_ANYCOHORT);
          if (linkrule->barrier != NULL) {
            scan = fsm_minimize(fsm_minus(
                       scan,
                       cg_match_cohort(fsm_copy(linkrule->barrier))));
          }
          /* Only matters in the presence of LINKed contexts */
          /* Q: what is this? Also: cg_match_cohor_careful? */
          if ((linkrule->scope_mode & MODE_STAR) && (rules->num_links > 0)) {
            scan = fsm_minimize(fsm_minus(
                       scan,
                       cg_match_cohort(fsm_copy(linkrule->condition))));
          }
          scan = fsm_minimize(fsm_kleene_star(scan));
        } else {
          scan = fsm_empty_string();
          /* We remove the BARRIER from skip only if no scanning is used */
          /* Q: why? Shouldn't this be done above too? */
          if (linkrule->barrier != NULL) {
            skip = fsm_minimize(fsm_minus(
                       skip,
                       cg_match_cohort(fsm_copy(linkrule->barrier))));
          }
        }
        /* + or - scope. */
        if (linkrule->scope_num >=0) {
          scan = fsm_minimize(fsm_concat(skip, scan));
        } else {
          scan = fsm_minimize(fsm_concat(scan, skip));
        }

        /* TESTS: ABSOLUTE SCOPE (scope = absolute word index) */
        if (linkrule->scope_mode & MODE_ABSOLUTE) {
          if (linkrule->scope_num >=0) {
            /* Look right */
            if (linkrule->rule_not) {
              /* Q: why the first part? [~$"$i$ " || ~[scan match ?*] ]  */
              tests = fsm_minimize(fsm_union(
                          fsm_complement(fsm_contains(fsm_symbol(headnumstring))),
                          fsm_complement(fsm_concat(
                              fsm_minimize(fsm_concat(scan, match)),
                              fsm_universal()))));
            } else {
              /* [~$"$i$ " || scan match ?*] */
              tests = fsm_minimize(fsm_union(
                          fsm_complement(fsm_contains(fsm_symbol(headnumstring))),
                          fsm_concat(
                              fsm_minimize(fsm_concat(scan, match)),
                              fsm_universal())));
            }
          } else {	
            /* Look left */
            if (linkrule->rule_not) {
              /* [ ~$"$i$ " || ~[?* match scan] ] */
              tests = fsm_minimize(fsm_union(
                          fsm_complement(fsm_contains(fsm_symbol(headnumstring))),
                          fsm_complement(fsm_concat(
                              fsm_universal(),
                              fsm_minimize(fsm_concat(match, scan))))));
            } else {
              /* [ ~$"$i$ " || ?* match scan] */
              tests = fsm_minimize(fsm_union(
                          fsm_complement(fsm_contains(fsm_symbol(headnumstring))),
                          fsm_concat(
                              fsm_universal(),
                              fsm_minimize(fsm_concat(match, scan)))));
            }
          }
          /* TESTS: RELATIVE SCOPE */
        } else {
          if (linkrule->scope_num >=0) {
            /* Look right */
            if (linkrule->rule_not) {
              /* ~$["$i$ " [scan match]] */
              tests = cg_simple_crrnot(fsm_symbol(headnumstring),
                                       fsm_minimize(fsm_concat(scan, match)));
            } else {
              /* "$i$ " => _ [scan match]] */
              tests = cg_simple_crr(fsm_symbol(headnumstring),
                                    fsm_minimize(fsm_concat(scan, match)));
            }
          } else {
            /* Look left */
            if (linkrule->rule_not) {
              /* ~$[ [match scan] "$i$ "] */
              tests = cg_simple_crlnot(fsm_symbol(headnumstring),
                                       fsm_minimize(fsm_concat(match, scan)));
            } else {
              /* "$i$ " => [match scan] _] */
              tests = cg_simple_crl(fsm_symbol(headnumstring),
                                    fsm_minimize(fsm_concat(match, scan)));
            }
          }
        }
      } else {
        tests = fsm_universal();
      }

      /* InitialFilter .o. mark_form_target(wordform, target) .o. constrain .o. tests */
      if (linkcount == 1) {
        /* The original condition. */
        thistest = fsm_minimize(fsm_compose(
                       fsm_compose(
                           fsm_copy(CG_DEF_InitialFilter),
                           cg_mark_form_target(fsm_copy(cg_rule_complex->wordform),
                                               fsm_copy(cg_rule_complex->target))),
                       fsm_copy(constrain)));
        /* need to add insertaux if needed */
        if (linkrule->num_links > 0) {
          thistest = fsm_minimize(fsm_compose(
                         thistest,
                         cg_insertaux(linkrule->num_links)));
        }
      }
      /* Add the linked test to the list of current tests (with .o.). */
      thistest = fsm_minimize(fsm_compose(thistest, tests));
    }  // end for LINKs

    /* Add removeaux if needed */
    if (rules->num_links > 0) {
      thisrule = fsm_minimize(fsm_compose(
                     thistest,
                     cg_removeaux(rules->num_links)));
    } else {
      thisrule = thistest;
    }

    /* Intersect all separate conditions */
    if (rawrules == NULL) {
      rawrules = thisrule;
    } else {
      rawrules = fsm_minimize(fsm_intersect(rawrules, thisrule));
    }
  }

  rule    = cg_rule_wrapup(rawrules);
  checker = cg_create_checker(rawrules);
  rule = fsm_topsort(rule);
  checker = fsm_topsort(checker);
  cg_print_rule_info(rule);
  cg_add_rule_to_chain(rule, checker);

  /* Cleanup mem */
  fsm_destroy(constrain);
  fsm_destroy(rawrules);
  cg_free_rule_complex();
  cg_rule_complex =
  cg_rule_complex_current =
  cg_rule_complex_current_head = (struct cg_rule_complex*)calloc(1,sizeof(struct cg_rule_complex));
}

/**
 * Returns a symbol for a tag. To ensure that tags have the "postfix property",
 * the symbol is not "tag ", as in the paper and the original version, but
 * "<tag> ". Tags starting with a quotation mark are exempt from this change to
 * prevent clutter, and also, they are already "safe".
 */
struct fsm* cg_tag_symbol(char* tag) {
  char* symbol;
  if (tag[0] == '\"') {
    symbol = (char*)malloc((strlen(tag) + 2) * sizeof(char));
    sprintf(symbol, "%s ", tag);
  } else {
    symbol = (char*)malloc((strlen(tag) + 4) * sizeof(char));
    sprintf(symbol, "<%s> ", tag);
  }
  struct fsm* ret = fsm_symbol(symbol);
  free(symbol);
  return ret;
  /* Doesn't help the speed problems. */
  // return fsm_explode(symbol);
}

/* Delimiters. For tags, we reuse the infrastructure for the LIST command
   (cg_list_currelement, etc). */

/** Registers a delimiter word form. */
void cg_delim_wordform_add(char* string) {
  if (cg_delim_wordforms == NULL) {
    cg_delim_wordforms = fsm_symbol(string);
  } else {
    cg_delim_wordforms = fsm_union(cg_delim_wordforms, fsm_symbol(string));
  }
}

/**
 * Writes the delimiters "rule" to a file, and frees the resources associated
 * with it.
 */
void cg_delim_write() {
  delimiter_rule = fsm_empty_set();
  if (cg_delim_wordforms != NULL) {
    // TODO: nope, must embed in $0$ X #BOC# ...
    delimiter_rule = fsm_union(delimiter_rule,
                               fsm_concat(fsm_universal(),
                                          fsm_concat(cg_delim_wordforms,
                                                     fsm_universal())));
    cg_delim_wordforms = NULL;
  }
  if (cg_list_currelement != NULL) {
    delimiter_rule = fsm_union(delimiter_rule,
                               cg_match_cohort(cg_list_currelement));
    cg_list_currelement = NULL;
  }
  delimiter_rule = fsm_minimize(fsm_determinize(delimiter_rule));
  fsm_sort_arcs(delimiter_rule, 1);
  strcpy(delimiter_rule->name, "DELIMITERS");
}

void cg_wordform_add(char *string) {
  cg_rule_complex->wordform = cg_tag_symbol(string);
}

void cg_fsm_add(char *string, int where, int type) {
  struct fsm *net, **combinewith;
  if (type == TYPE_SET) {
    if ((net = cg_set_find(string)) == NULL) {
      printf("Error on line %i: set %s not defined\n",lineno_begin_rule, string);
      exit(0);
    }
  } else if (type == TYPE_TAG) {
    net = cg_tag_symbol(string);
  }

  /* @*fsm@* */
  net = fsm_minimize(fsm_concat(
            fsm_copy(CG_DEF_NODEL),
            fsm_concat(
                type == TYPE_SET ? fsm_copy(net) : net,  // clean up tag
                fsm_copy(CG_DEF_NODEL))));

  if (where == WHERE_TARGET)
    combinewith = &(cg_rule_complex_current->target);
  if (where == WHERE_CONDITION)
    combinewith = &(cg_rule_complex_current->condition);
  if (where == WHERE_BARRIER)
    combinewith = &(cg_rule_complex_current->barrier);

  if (*combinewith == NULL) {
    *combinewith = net;
    return;
  }

  if (curr_op == OP_PLUS) {
    *combinewith = fsm_minimize(fsm_concat(*combinewith, net));
  }
  if (curr_op == OP_OR) {
    *combinewith = fsm_minimize(fsm_union(*combinewith, net));
  }
  if (curr_op == OP_MINUS) {
    *combinewith = fsm_minimize(fsm_minus(*combinewith, net));    
  }
}

/**
 * Adds the current set to the linked list that stores sets (cg_sets). Called
 * at the end of both the LIST and SET commands. 
 */
void cg_list_add(void) {
  struct cg_sets *cgs;
  if (cg_list_name == NULL) {
    printf("Something wrong. No list name\n"); exit(0);
  }
  cgs = (struct cg_sets*)malloc(sizeof(struct cg_sets));
  cgs->name = cg_list_name;
  cgs->net = fsm_minimize(cg_list_currelement);
  printf("SET/LIST \"%s\": ",cg_list_name);
  cg_print_rule_info(cg_list_currelement);
  if (cg_sets == NULL) {
    cg_sets = cgs;
    cgs->next = NULL;
  } else {
    cgs->next = cg_sets;
    cg_sets = cgs;
  }
  cg_list_name = NULL;
  cg_list_currelement = NULL;
  cg_list_currcelement = NULL;
}

void cg_set_element_add(struct fsm *list_fsm) { 
  if (cg_list_currelement == NULL) {
    cg_list_currelement = fsm_copy(list_fsm);
  } else {
    /* + operator: concats @*name@* to the end of the current element. */
    if (curr_op == OP_PLUS) {
      cg_list_currelement = fsm_minimize(
          fsm_concat(cg_list_currelement, fsm_copy(list_fsm)));
    }
    /* - operator: current element - @*name@*. Not the opposite of +! */
    if (curr_op == OP_MINUS) {
      cg_list_currelement = fsm_minimize(
          fsm_minus(cg_list_currelement, fsm_copy(list_fsm)));
    }
    /* OR operator: union */
    if (curr_op == OP_OR) {
      cg_list_currelement = fsm_minimize(
          fsm_union(cg_list_currelement, fsm_copy(list_fsm)));
    }
  }
  /* TODO: else if */
}

bool cg_set_list_add(char* name) {
  struct fsm *list_fsm = cg_set_find(name);
  if (list_fsm == NULL) return false;
  cg_set_element_add(list_fsm);
  return true;
}

void cg_set_celement_end() {
  struct fsm *list_fsm = cg_list_currcelement;
  cg_set_element_add(list_fsm);
  cg_list_currcelement = NULL;
}

/* Set definition functions, that tackle the LIST command. The resulting FSM is
 * a union of the list's
 * elements, with the exception of elements in parentheses, which are
 * concatenated. A space is inserted to the end of all elements. */
/* TODO: why? */
/* */

void cg_list_element_add(char *name) { 
  if (cg_list_currelement == NULL) {
    cg_list_currelement = cg_tag_symbol(name);
  } else {
    cg_list_currelement = fsm_minimize(fsm_union(
                              cg_list_currelement,
                              cg_tag_symbol(name)));
  }
}

void cg_list_celement_add(char *name) {
  if (cg_list_currcelement == NULL) {
    //	cg_list_currcelement = fsm_minimize(fsm_concat(fsm_copy(CG_DEF_NODEL), fsm_concat(fsm_symbol(cg_string_addspace(name)), fsm_copy(CG_DEF_NODEL))));
    cg_list_currcelement = cg_tag_symbol(name);
  } else {
    //	cg_list_currcelement = fsm_minimize(fsm_concat(cg_list_currcelement, fsm_minimize(fsm_concat(fsm_copy(CG_DEF_NODEL), fsm_concat(fsm_symbol(cg_string_addspace(name)), fsm_copy(CG_DEF_NODEL))))));
    cg_list_currcelement = fsm_minimize(fsm_concat(
                               cg_list_currcelement,
                               cg_tag_symbol(name)));
  }
}

void cg_list_celement_end() { 
  if (cg_list_currelement == NULL) {
    cg_list_currelement = cg_list_currcelement;
  } else {
    cg_list_currelement = fsm_minimize(fsm_union(
                              cg_list_currelement,
                              cg_list_currcelement));
  }
  cg_list_currcelement = NULL;
}


void yerror(char *s1, char *s2, int line) {
  current_error = strdup(s1);
  first_error_sym = strdup(s2);
  current_error_line = line;
}

void yerrorprint(char *s) {
  printf("\nSyntax error: expected %s at \"%s%s\" on line %i.\n",
         current_error, first_error_sym, s, current_error_line);    
  exit(1);
}

%}

%x s0 s1 s2 s3 s4 s5 s6 s7 s8 s9 s10 s11 s12 s13 s14 s15 s16 s17 s18 sIF
%x DELIMCOLLECT DELIMLISTING DELIMCONCAT
%x LISTCOLLECT LISTEQ LISTING LISTCONCAT
%x SETCOLLECT SETEQ SETLISTING SETCONCAT
%x ERROR 

DIGIT    [0-9]
    /* ID: [\@_:<>*+,?=$!%^.-[]' or alnum], then the same or unicode characters */
    /* anything between double quotes. */
    /* ID       ([\\@_:"<>"a-zA-Z0-9*+,?=$!%\^.\-\[\]\']([\\"<>"/\-_@:A-Za-z0-9*+,?=$!%\^.\[\]\']|[\300-\337].|[\340-\357]..|[\360-\367]...)*)|["][^"]*["] */
ID       ([\\"<>"/\-_@:A-Za-z0-9*+,?=$!%\^.\[\]\']|[\300-\337].|[\340-\357]..|[\360-\367]...)+|["][^"]*["] 
RULESCOPE ("*"|"*""*")?"@"?"-"?{DIGIT}+C?
    /* e.g. *-1 */
WHITESPACE [[:blank:]\n]
%%
<ERROR>[^\n]*       { yerrorprint(yytext);  }

<*>"#"[^\n]*         /* eat up one-line comments */
<*>[ \t\n]+          /* eat up whitespace        */

(?i:LIST)           { dprint("LIST "); lineno_begin_rule = yylineno; BEGIN(LISTCOLLECT); }
(?i:SET)            { dprint("SET ");   lineno_begin_rule = yylineno;BEGIN(SETCOLLECT);  }
(?i:DELIMITERS)     { dprint("DELIMITERS "); lineno_begin_rule = yylineno; BEGIN(DELIMCOLLECT); }
(?i:SECTION)        { dprint("SECTION "); lineno_begin_rule = yylineno; section_no++;    }
(?i:CONSTRAINTS)    { dprint("CONSTRAINTS "); lineno_begin_rule = yylineno; section_no++;}
(?i:MAPPINGS)       { dprint("MAPPINGS "); lineno_begin_rule = yylineno;                 }
    /* TODO: what are SECTION, CONSTRAINTS, MAPPINGS? */

<DELIMCOLLECT>"="   { dprint("DELIMEQ");
                      cg_list_name = "DELIMITERS";
                      BEGIN(DELIMLISTING); }
<DELIMCOLLECT>.     { yerror("delimiter", yytext, yylineno);
                      BEGIN(ERROR); }

<DELIMLISTING>{ID}  { dprint("DELEMENT:[%s] ", yytext);
                      if (strncmp(yytext, "\"<", 2) == 0) {
                        dprint("WORD FORM ");
                        cg_delim_wordform_add(yytext);
                      } else {
                        dprint("TAG ");
                        cg_list_element_add(yytext);
                      }
                    }
<DELIMLISTING>"("   { dprint("DCONC ");
                      BEGIN(DELIMCONCAT); }
<DELIMLISTING>";"   { dprint("DEND\n");
                      cg_delim_write();
                      BEGIN(0); }
<DELIMLISTING>.     { yerror("delim element or ; ", yytext, yylineno);
                      BEGIN(ERROR); }

<DELIMCONCAT>{ID}    { dprint("DCELEMENT:[%s] ", yytext);
                       cg_list_celement_add(yytext); }
<DELIMCONCAT>")"     { dprint("DNORMAL ");
                       cg_list_celement_end();
                       BEGIN(DELIMLISTING); }
<DELIMCONCAT>.       { yerror("list element or ) ",yytext,yylineno);
                       BEGIN(ERROR); }
 
<LISTCOLLECT>{ID}   { dprint("LNAME:[%s] ",yytext); cg_list_name = strdup(yytext); BEGIN(LISTEQ);       }
<LISTCOLLECT>.      { yerror("list name",yytext,yylineno); BEGIN(ERROR); }

<LISTEQ>"="         { dprint("LISTEQ "); BEGIN(LISTING);          }
<LISTEQ>.           { yerror("=",yytext,yylineno); BEGIN(ERROR); }

<LISTING>{ID}       { dprint("LELEMENT:[%s] ",yytext); cg_list_element_add(yytext); }
<LISTING>"("        { dprint("LCONC "); BEGIN(LISTCONCAT);                          }
<LISTING>";"        { dprint("LEND\n"); cg_list_add(); BEGIN(0);                    }
<LISTING>.          { yerror("list element or ; ",yytext,yylineno); BEGIN(ERROR);   }

<LISTCONCAT>{ID}    { dprint("LCELEMENT:[%s] ",yytext); cg_list_celement_add(yytext);     }
<LISTCONCAT>")"     { dprint("LNORMAL "); cg_list_celement_end(); BEGIN(LISTING);   }
<LISTCONCAT>.       { yerror("list element or ) ",yytext,yylineno); BEGIN(ERROR);         }

<SETCOLLECT>{ID}   { dprint("SNAME:[%s] ",yytext); cg_list_name = strdup(yytext); BEGIN(SETEQ);    }
<SETCOLLECT>.      { yerror("set name",yytext,yylineno); BEGIN(ERROR);                             }

<SETEQ>"="         { dprint("SETEQ "); curr_op = OP_NONE; BEGIN(SETLISTING);          }
<SETEQ>.           { yerror("=",yytext,yylineno); BEGIN(ERROR); }

<SETLISTING>{ID}   {
  dprint("SELEMENT:[%s] ",yytext);
  if (!cg_set_list_add(yytext)) {
    yerror("list doesn't exist", yytext, yylineno);
    BEGIN(ERROR);
  }
}
<SETLISTING>"("           { dprint("SCONC "); BEGIN(SETCONCAT);                          }
<SETLISTING>"+"{WHITESPACE}        { dprint("SPLUS "); curr_op = OP_PLUS;                          }
<SETLISTING>"-"{WHITESPACE}        { dprint("SMINUS "); curr_op = OP_MINUS;                        }
<SETLISTING>(?i:OR){WHITESPACE}    { dprint("OR "); curr_op = OP_OR;                               }
<SETLISTING>";"        { dprint("SEND\n"); cg_list_add(); BEGIN(0);                    }
<SETLISTING>.          { yerror("list element or ; ",yytext,yylineno); BEGIN(ERROR);   }

<SETCONCAT>{ID}    { dprint("SCELEMENT:[%s] ",yytext); cg_list_celement_add(yytext);     }
<SETCONCAT>")"     { dprint("SNORMAL "); cg_set_celement_end(); BEGIN(SETLISTING);   }
<SETCONCAT>.       { yerror("list element or ) ",yytext,yylineno); BEGIN(ERROR);         }

(?i:SELECT)      { dprint("SELECT ");  lineno_begin_rule = yylineno; rule_operation = OP_SELECT; curr_op = OP_NONE; BEGIN(s2); }
(?i:REMOVE)      { dprint("REMOVE ");  lineno_begin_rule = yylineno; rule_operation = OP_REMOVE; curr_op = OP_NONE; BEGIN(s2); }
^{ID}            { dprint("WORDFORM ");  lineno_begin_rule = yylineno; cg_wordform_add(yytext); curr_op = OP_NONE; BEGIN(s1);  }

   /* TODO: rename s1 and s2 to meaningful names. */

<s1>(?i:SELECT)   { dprint("SELECT "); rule_operation = OP_SELECT; BEGIN(s2);  }
<s1>(?i:REMOVE)   { dprint("REMOVE "); rule_operation = OP_REMOVE; BEGIN(s2);  }
<s1>.             { yerror("SELECT or REMOVE", yytext, yylineno); BEGIN(ERROR); }

<s2>{ID}          { dprint("SETNAME:[%s] ",yytext); cg_fsm_add(yytext, WHERE_TARGET, TYPE_SET); BEGIN(s4); } /* Collect set for TARGET */
<s2>"("           { dprint("OPENPAR "); BEGIN(s3); }
<s2>.             { yerror("set or (", yytext, yylineno); BEGIN(ERROR); }

<s3>{ID}          { dprint("TAGNAME:[%s] ",yytext); cg_fsm_add(yytext, WHERE_TARGET, TYPE_TAG); curr_op = OP_PLUS; BEGIN(s18); } /* Collect tag for TARGET */
<s3>.             { yerror("tag", yytext, yylineno); BEGIN(ERROR); }

<s18>{ID}         { dprint("TAGNAME:[%s] ",yytext); cg_fsm_add(yytext, WHERE_TARGET, TYPE_TAG); }
<s18>")"          { dprint("CLOSEPAR "); BEGIN(s4); }
<s18>.            { yerror("tag or )", yytext, yylineno); BEGIN(ERROR); }

<s4>(?i:OR)          { dprint("OR ");     curr_op = OP_OR;    BEGIN(s2);    }
<s4>"+"              { dprint("PLUS ");   curr_op = OP_PLUS;  BEGIN(s2);    }
<s4>"-"              { dprint("MINUS ");  curr_op = OP_MINUS; BEGIN(s2);    }
<s4>(?i:IF)          { dprint("IF ");                         BEGIN(sIF);   }
<s4>"("              { dprint("OPENPAR ");                    BEGIN(s5);    }
<s4>";"              { dprint("END-OF-RULE\n"); cg_compile_rule(); BEGIN(0); }
<s4>.                { yerror("OR, +, -, IF, (, or ;", yytext, yylineno); BEGIN(ERROR); }

<sIF>"("             { dprint("OPENPAR ");                    BEGIN(s5);    }
<sIF>.               { yerror("(", yytext, yylineno); BEGIN(ERROR);         }

<s5>(?i:NOT)     { dprint("NOT "); cg_rule_complex_current->rule_not = 1;  BEGIN(s7);   }
<s5>{RULESCOPE}  { dprint("RULESCOPE "); cg_parse_scope(yytext, cg_rule_complex_current); BEGIN(s8); }
<s5>.            { yerror("rule scope or NOT", yytext, yylineno); BEGIN(ERROR); }

<s7>{RULESCOPE}  { dprint("RULESCOPE "); cg_parse_scope(yytext, cg_rule_complex_current); BEGIN(s8); }
<s7>.            { yerror("rule scope", yytext, yylineno); BEGIN(ERROR); }

<s8>"("          { dprint("OPENPAR "); BEGIN(s9); }
<s8>{ID}         { dprint("SETNAME:[%s] ",yytext); cg_fsm_add(yytext, WHERE_CONDITION, TYPE_SET); BEGIN(s10);    }
<s8>.            { yerror("( or set", yytext, yylineno); BEGIN(ERROR);}

<s9>{ID}         { dprint("TAGNAME:[%s] ",yytext); cg_fsm_add(yytext, WHERE_CONDITION, TYPE_TAG); curr_op = OP_PLUS; BEGIN(s17); }
<s9>.            { yerror("tag", yytext, yylineno); BEGIN(ERROR);                       }

<s17>{ID}        { dprint("TAGNAME:[%s] ",yytext); cg_fsm_add(yytext, WHERE_CONDITION, TYPE_TAG); curr_op = OP_PLUS; }
<s17>")"         { dprint("CLOSEPAR "); BEGIN(s10); }
<s17>.           { yerror("tag or )", yytext, yylineno); BEGIN(ERROR);}

<s10>(?i:OR)      { dprint("OR ");    curr_op = OP_OR;   BEGIN(s8);        }
<s10>"+"          { dprint("PLUS ");  curr_op = OP_PLUS; BEGIN(s8);      }
<s10>"-"          { dprint("MINUS "); curr_op = OP_MINUS; BEGIN(s8);     }
<s10>(?i:LINK)    { 
    dprint("LINK ");
    cg_rule_complex_current_head->num_links++;
    cg_rule_complex_current->link = (struct cg_rule_complex*)calloc(1, sizeof(struct cg_rule_complex));
    cg_rule_complex_current = cg_rule_complex_current->link;
    BEGIN(s5);
}  /* TODO: understand linking. */

<s10>")"          { dprint("CLOSEPAR "); BEGIN(s11); }
<s10>(?i:BARRIER) { dprint("BARRIER "); BEGIN(s12);  }
<s10>(?i:AND)     {
                    dprint("AND ");
                    cg_rule_complex_current_head->next = (struct cg_rule_complex*)calloc(1, sizeof(struct cg_rule_complex));
                    cg_rule_complex_current_head->next->scope_mode = cg_rule_complex_current_head->scope_mode;
                    cg_rule_complex_current_head->next->scope_num = cg_rule_complex_current_head->scope_num;
                    cg_rule_complex_current_head =
                    cg_rule_complex_current = cg_rule_complex_current_head->next;
                    BEGIN(s13);      }
<s10>.            { yerror("OR, +, -, LINK, ), BARRIER or AND", yytext, yylineno); BEGIN(ERROR); }

<s11>"("         {
                   dprint("OPENPAR ");
                   cg_rule_complex_current_head->next = (struct cg_rule_complex*)calloc(1, sizeof(struct cg_rule_complex));
                   cg_rule_complex_current =
                   cg_rule_complex_current_head = cg_rule_complex_current_head->next;
                   BEGIN(s5);
                 }
<s11>";"         { dprint("END-OF-RULE\n"); cg_compile_rule(); BEGIN(0); } /* Join rules here FR(R1 & R2 ... Rn) */
<s11>.           { yerror("( or ;", yytext, yylineno);         BEGIN(ERROR);}

<s12>"("         { dprint("OPENPAR "); BEGIN(s14); }
<s12>{ID}        { dprint("SETNAME:[%s] ",yytext); cg_fsm_add(yytext, WHERE_BARRIER, TYPE_SET); BEGIN(s15); }
<s12>.           { yerror("set or (", yytext, yylineno); BEGIN(ERROR);}

<s14>{ID}        {  dprint("TAGNAME:[%s] ",yytext); cg_fsm_add(yytext, WHERE_BARRIER, TYPE_TAG); curr_op = OP_PLUS; BEGIN(s16);}
<s14>.           { yerror("tag", yytext, yylineno); BEGIN(ERROR);}

<s16>{ID}        { dprint("TAGNAME:[%s] ",yytext); cg_fsm_add(yytext, WHERE_BARRIER, TYPE_TAG); }
<s16>")"         { dprint("CLOSEPAR "); BEGIN(s15); }
<s16>.           { yerror("tag or )", yytext, yylineno); BEGIN(ERROR);}

<s15>(?i:OR)     { dprint("OR ");  curr_op = OP_OR; BEGIN(s12); }
<s15>"+"         { dprint("PLUS "); curr_op = OP_PLUS; BEGIN(s12); }
<s15>"-"         { dprint("MINUS "); curr_op = OP_MINUS; BEGIN(s12); }
<s15>(?i:AND)    { 
    dprint("AND ");
    cg_rule_complex_current_head->next =
        (struct cg_rule_complex*)calloc(1, sizeof(struct cg_rule_complex));
    cg_rule_complex_current_head->next->scope_mode = cg_rule_complex_current_head->scope_mode;
    cg_rule_complex_current_head->next->scope_num = cg_rule_complex_current_head->scope_num;
    cg_rule_complex_current_head = cg_rule_complex_current = cg_rule_complex_current_head->next; 
    BEGIN(s13); }
<s15>(?i:LINK)  { 
    dprint("LINK "); 
    cg_rule_complex_current_head->num_links++;
    cg_rule_complex_current->link =
        (struct cg_rule_complex*)calloc(1, sizeof(struct cg_rule_complex));
    cg_rule_complex_current = cg_rule_complex_current->link ;
    BEGIN(s5);
}
<s15>")"         { dprint("CLOSEPAR "); BEGIN(s11); }  /* Compile single rule here */
<s15>.           { yerror("OR, +, -, AND, LINK or )", yytext, yylineno); BEGIN(ERROR);}

<s13>(?i:NOT)    { dprint("NOT "); cg_rule_complex_current->rule_not = 1;  BEGIN(s8); }
<s13>{ID}        { dprint("SETNAME:[%s] ",yytext); cg_fsm_add(yytext, WHERE_CONDITION, TYPE_SET);  BEGIN(s10); }
<s13>"("         { dprint("OPENPAR "); BEGIN(s9); }
<s13>.           { yerror("set, NOT, or (", yytext, yylineno); BEGIN(ERROR); }

"="              { dprint("EQUALS\n");   }

.                { yerror("WORDFORM or SELECT OR REMOVE",yytext,yylineno); }
%%

/** Reverses the order of cg_rules to regain the original rule order. */
void reverse_rule_order() {
  struct cg_rules* last = cg_rules;
  struct cg_rules* curr = last->next;
  last->next = NULL;
  while (curr != NULL) {
    struct cg_rules* next = curr->next;
    curr->next = last;
    last = curr;
    curr = next;
  }
  cg_rules = last;
}

int save_rules_fsm(char *filename) {
  char prolog_name[30];
  struct cg_rules *rules;
  int section = -1;
  char* outfile_name = (char*)calloc(strlen(filename) + 5, sizeof(char));
  sprintf(outfile_name, "%s.fst", filename);
  gzFile* outfile = (gzFile*)gzopen(outfile_name, "wb");
  printf("Writing rules to file %s.\n", outfile_name);

  /* Return to the original rule order. */
  reverse_rule_order();

  SmallestFirstTreeMerger merger;

  struct Node* trees = merger.merge(cg_rules, total_rules);
  size_t num_rules;
  struct fsm** to_print = merger.serialize(trees, &num_rules);
  printf("num_rules: %zu\n", num_rules);
  size_t i;
  for (i = 0; i < num_rules; i++) {
    printf("FSM %s\n", to_print[i]->name);
  }
  struct fsm* allsigma = merge_sigma(to_print, num_rules);

  /* First the DELIMITERS... */
  foma_net_print(delimiter_rule, outfile);

  /* ... then the deserialized rule tree. */
  foma_net_print(allsigma, outfile);
  for (i = 0; i < num_rules; i++) {
    foma_net_print(to_print[i], outfile);
  }

//  /* ... then all the rules. */
//  for (rules = cg_rules ; rules != NULL; rules = rules->next) {
//    if (rules->section_no != section) {
//      struct fsm* delim = fsm_empty_set();
//      strcpy(delim->name, "---");
//      foma_net_print(delim, outfile);
//      section = rules->section_no;
//    }
//    foma_net_print(rules->rule, outfile);
//    foma_net_print(rules->conditions, outfile);
//    sprintf(prolog_name,"%s.prolog", rules->rule->name);
//    write_prolog(rules->rule, prolog_name);
//    sprintf(prolog_name,"%s.prolog", rules->conditions->name);
//    write_prolog(rules->conditions, prolog_name);
//  }
  gzclose(outfile);
  free(outfile_name);
  return 1;
}

int main(int argc, char **argv) {
  ++argv, --argc;  /* skip over program name */
  char* grammar_file = argv[0];
  if (argc > 0) {
    binary_prefix = (char*)calloc(strlen(grammar_file) + 1, sizeof(char));
    strcpy(binary_prefix, grammar_file);
    char* extension = strrchr(binary_prefix, '.');
    if (extension != NULL) {
      *extension = 0;
    }
    yyin = fopen(grammar_file, "r");
  } else {
    binary_prefix = (char*)calloc(strlen("outfile") + 1, sizeof(char));
    strcpy(binary_prefix, "outfile");
    yyin = stdin;
  }

  cg_init();
  yylex();
  printf("Done. Rule statistics:\n");
  printf("Number of rules compiled: %i\n",total_rules);
  printf("Average number of states: %i\n",total_states/total_rules);
  printf("Total number of states: %i\n",total_states);
  printf("Average number of transitions: %i\n",total_arcs/total_rules);
  printf("Total number of transitions: %i\n",total_arcs);

  if (cg_rules != NULL)
	save_rules_fsm(binary_prefix);
  free(binary_prefix);
  cg_free_rule_complex();
  fsm_destroy(delimiter_rule);
  return 1;
}
