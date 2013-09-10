#include <iostream>
#include <string>

#include "foma_extra.h"
#include "fomacg_common.h"

int main(int argc, char* argv[]) {
/*  struct fsm* first = fsm_concat(
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
  struct fsm* allsigma = merge_sigma(fsms);

  sh = apply_init(second);
  std::cout << "Run after: " << apply_detmin_fsa(sh, "#BOC# | #X# macska ") << std::endl;
  apply_clear(sh);
  outfile = (gzFile*)gzopen("sigma2.fst", "wb");
  foma_net_print(first, outfile);
  foma_net_print(second, outfile);
  foma_net_print(allsigma, outfile);
  gzclose(outfile);*/

  gzFile* outfile = (gzFile*)gzopen((std::string(argv[2]) + ".fst").c_str(), "wb");
  FstVector fsts = load_fsts(argv[1]);
  FstPair fsp = fsts[0];
  fsts.pop_front();
  foma_net_print(fsp.fst, outfile);
  std::vector<struct fsm*> fsms;
  for (size_t i = 0; i < fsts.size(); i++) {
    fsms.push_back(fsts[i].fst);
  }
  struct fsm* allsigma = merge_sigma(fsms);
  foma_net_print(allsigma, outfile);
  for (size_t i = 0; i < fsms.size(); i++) {
    foma_net_print(fsms[i], outfile);
  }
  gzclose(outfile);
}
