1. target_filtering
  - managed to decrease the number of rule tests
  - however, one filtering FSA / rule is too much; we don't gain anything

1. target_filtering_trie
  - `cat grammars/apertium-sme-fin.fin-sme_cg2.rlx | egrep SELECT\|REMOVE | egrep -v "^\\s*#" | sed -e "s/ IF.*//" -e "s/;.*//" -e "s/.*SELECT \|REMOVE //" -e "s/ (.*//" > fin_targets`
