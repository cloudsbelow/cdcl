#include<vector>
#include<queue>
#include "./cnf.hpp"
#include "ranges"
#include <map>

class CdclSolver{
  std::map<Clause *, std::vector<Literal>> watched;
  std::queue<int> toProcess;
  CNF cnf;
  Assignment a;
  int decision_level = 0;
  void decide() {
    a.Assign(toProcess.front(), true, decision_level, -1);
    toProcess.pop();
  }

  Clause explain(Clause *conflict) {
    Literal l = a.order[a.order.size()-1];
    for (std::vector<Literal>::reverse_iterator it = a.order.rbegin(); it != a.order.rend(); it++) {
      bool _;
      if (conflict->HasLiteral(*it, _)) {
        l = *it;
        break;
      }
    }
    Clause c = cnf.getClauses()[a.fromClause[l.Idx()]];
    return c.Resolution(*conflict, l);
  }

  bool cexplains(Clause *c, Literal conflict_lit) {
    for (Literal l:a.assignment) {
      bool isNeg;
      bool hasLit  = c->HasLiteral(l, isNeg);
      if (hasLit && !isNeg) {
        return false;
      }
    }
    return true;
  } 
};