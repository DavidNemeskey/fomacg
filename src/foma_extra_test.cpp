#include <iostream>

#include "foma_extra.h"

int main(int argc, char* argv[]) {
  struct fsm* first = fsm_concat(
                        fsm_concat(
                          fsm_symbol("#BOC# "),
                          fsm_symbol("| ")),
                        fsm_concat(
                          fsm_symbol("#0# "),
                          fsm_symbol("kutya ")));
  struct fsm* second = fsm_concat(
                         fsm_concat(
                           fsm_symbol("#BOC# "),
                           fsm_symbol("| ")),
                         fsm_concat(
                           fsm_symbol("#X# "),
                           fsm_symbol("macska ")));
  struct apply_handle* sh = apply_init(second);
  std::cout << "Run before: " << apply_detmin_fsa(sh, "#BOC# | #X# macska ") << std::endl;
  apply_clear(sh);
  gzFile* outfile = (gzFile*)gzopen("sigma1.fst", "wb");
  foma_net_print(first, outfile);
  foma_net_print(second, outfile);
  gzclose(outfile);
  std::vector<struct fsm*> fsms;
  fsms.push_back(first);
  fsms.push_back(second);
  merge_sigma(fsms);
  sh = apply_init(second);
  std::cout << "Run after: " << apply_detmin_fsa(sh, "#BOC# | #X# macska ") << std::endl;
  apply_clear(sh);
  outfile = (gzFile*)gzopen("sigma2.fst", "wb");
  foma_net_print(first, outfile);
  foma_net_print(second, outfile);
  gzclose(outfile);
}
