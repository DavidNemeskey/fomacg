#include "fomacg_common.h"

//#include <cstring>
#include <stdexcept>
#include <sstream>
//#include <iostream>

FstPair::FstPair() : fst(NULL), ah(NULL) {}
FstPair::FstPair(struct fsm* fst, struct apply_handle* ah) : fst(fst), ah(ah) {}

void FstPair::fill_sigma(size_t sigma_size) {
  if (fst != NULL) {
    sigma.resize(sigma_size, IDENTITY);
    sigma[EPSILON] = sigma[EPSILON];
    sigma[UNKNOWN] = sigma[UNKNOWN];
    for (struct sigma* s = fst->sigma; s != NULL; s = s->next) {
      sigma[s->number] = s->number;
    }
  }
}

void FstPair::cleanup(bool only_handle) {
  if (ah != NULL) {
    apply_clear(ah);
  }
  if (fst != NULL && !only_handle) {
    fsm_destroy(fst);
  }
}

FstPair load_fst(const std::string& fst_file)
    throw (std::invalid_argument, std::runtime_error) {
  struct fsm* fst = fsm_read_binary_file(fst_file.c_str());
  if (fst == NULL) {
    throw std::invalid_argument("Could not load FST from " + fst_file);
  }
  /* Should be sorted already by fomacg. */
  if (!fst->arcs_sorted_in) {
//    std::cerr << "Sorting " << fst->name << std::endl;
    fsm_sort_arcs(fst, 1);
  }
  struct apply_handle* ah = apply_init(fst);
  if (ah == NULL) {
    fsm_destroy(fst);
    throw std::runtime_error("Failed to initialize apply handle for FST " + fst_file);
  }
  return FstPair(fst, ah);
}

FstVector load_fsts(const std::string& fst_file) throw (std::invalid_argument) {
  FstVector ret;
  fsm_read_binary_handle fsrh;
  if ((fsrh = fsm_read_binary_file_multiple_init(fst_file.c_str())) == NULL) {
    throw std::invalid_argument("Could not open FST file " + fst_file);
  }

  struct fsm* net;
  while ((net = fsm_read_binary_file_multiple(fsrh)) != NULL) {
    /* Should be sorted already by fomacg. */
    if (!net->arcs_sorted_in) {
//      std::cerr << "Sorting " << net->name << std::endl;
      fsm_sort_arcs(net, 1);
    }
    struct apply_handle* ah = apply_init(net);
    if (ah == NULL) {
      fsm_destroy(net);
    } else {
      ret.push_back(FstPair(net, ah));
    }
//    if (strcmp(net->name, "Cond_1_198") == 0) {
//      apply_print_sigma(net);
//    }
  }

  return ret;
}

RuleSetLoader::RuleSetLoader(const std::string& fst_file)
    throw (std::invalid_argument) {
  if ((fsrh = fsm_read_binary_file_multiple_init(fst_file.c_str())) == NULL) {
    throw std::invalid_argument("Could not open FST file " + fst_file);
  }
}

bool RuleSetLoader::load_fst(FstPair& fst) throw (std::runtime_error) {
  struct fsm* net;
  if ((net = fsm_read_binary_file_multiple(fsrh)) != NULL) {
    if (!net->arcs_sorted_in) {
//      std::cerr << "Sorting " << net->name << std::endl;
      fsm_sort_arcs(net, 1);
    }

    struct apply_handle* ah = apply_init(net);
    if (ah == NULL) {
      fsm_destroy(net);
      std::stringstream ss("FST ");
      ss << net->name << " could not be initialized.";
      throw std::runtime_error(ss.str());
    } else {
      fst = FstPair(net, ah);
      return true;
    }
  } else {
    return false;
  }
}

