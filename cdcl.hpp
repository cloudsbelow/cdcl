#include<vector>
#include<queue>
#include "./cnf.hpp"
#include "ranges"

class CdclSolver{
  std::vector<std::vector<int>> watched;
  std::queue<int> toProcess;
  CNF cnf;
  Assignment a;

  void decide() {
    a.assignment.push_back(toProcess.front());
    toProcess.pop();
  }

  Clause explain() {
    Clause *conflict = cnf.getConflictClause(a);
    Literal l = a.assignment[a.assignment.size() - 1];
    for (Literal lit:a.assignment) {
      bool isNeg;
      if (conflict->HasLiteral(lit, isNeg) && isNeg) {
        l = lit;
        break;
      }
    }
    for (Clause cl:cnf.getClauses()) {
      if (cl.HasExactLiteral(l) && cexplains(&cl, l)) {
        return cl.Resolution(*conflict,l);
      }
    }
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