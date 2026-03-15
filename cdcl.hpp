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
  int decision_level;
  std::map<int, bool> trail;
  int numVars;
  public:
  CdclSolver(int numVars, CNF expression) : a(numVars), decision_level(0) {cnf = expression;}
  void addClause(Clause c) {
      cnf.addClause(c);
  }
  int propagate() {
        bool progress = true;
        while (progress) {
            progress = false;
            for (int i = 0; i < (int)cnf.size(); i++) {
                if (cnf[i].isConflict(a)) {
                    return i;
                }
                Literal unit(1);
                if (cnf[i].isUnit(a, unit) && trail.find(unit.Idx()) == trail.end()) {
                    a.Assign(unit.Idx(), !unit.Negated(), decision_level, i);
                    trail[unit.Idx()] = true;
                    progress = true;
                }
            }
        }
        return -1;
    }

    bool vivifyClause(int clauseIdx) {
        auto lits = cnf[clauseIdx].lits;
        if ((int)lits.size() <= 1) return false;

        std::vector<Literal> surviving;

        for (int i = 0; i < (int)lits.size(); i++) {
            CNF expression;
            CdclSolver temp(a.size - 1, expression);
            for (int j = 0; j < (int)cnf.size(); j++) {
                if (j != clauseIdx) temp.addClause(cnf[j]);
            }
            for (int j = 0; j < i; j++) {
                temp.addClause({-lits[j]});
            }

            int conflict = temp.propagate();

            if (conflict >= 0) {
                break;
            }

            if (temp.a.IsAssigned(lits[i].Idx())) {
                bool litIsTrue = lits[i].Negated() != temp.a.IsTrue(lits[i].Idx());
                if (litIsTrue) {
                    surviving.push_back(lits[i]);
                    break;
                }
            } else {
                surviving.push_back(lits[i]);
            }
        }
        if (surviving.size() < lits.size()) {
            cnf[clauseIdx] = surviving.empty() ? Clause() : Clause(surviving);
            return true;
        }
        return false;
    }
  
  int vivify() {
      int count = 0;
      for (int i = 0; i < (int)cnf.size(); i++) {
          if (vivifyClause(i)) count++;
      }
      return count;
  }
  void decide() {
    for (Clause c:cnf) {
      if (!c.isSatisfied(a)) {
        for (Literal l:c.getLiterals()) {
          if (!a.IsAssigned(l.Idx())) {
              decision_level += 1;
              a.Assign(l.Idx(), !l.Negated(), decision_level, -1);
              break;
          }
        }
      }
    }
  }
  void backjump(Clause res) {
    int max = 0;
    for (Literal l:res.getLiterals()) {
      if (a.decisionLevel[l.Idx()] > max && a.decisionLevel[l.Idx()] < decision_level) {
        max = a.decisionLevel[l.Idx()];
      }
    }
    a.SetMaxDecisionLevel(max);
  }

  Clause explain(Clause *conflict) {
    bool cont = true;
    Clause res;
    Literal l = a.order[a.order.size()-1];
    while (cont) {
    l = a.order[a.order.size()-1];
    for (std::vector<Literal>::reverse_iterator it = a.order.rbegin(); it != a.order.rend(); it++) {
      bool _;
      if (conflict->HasLiteral(*it, _)) {
        l = *it;
        break;
      }
    }
    Clause c = cnf[a.fromClause[l.Idx()]];
    res = c.Resolution(*conflict, l);
    cnf.addClause(res);
    }
    cont = false;
    for (Literal ltl:res.getLiterals()) {
      if (a.decisionLevel[ltl.Idx()] == decision_level && ltl.Idx() != l.Idx()) {
        cont = true;
        break;
      }
    }
    return res;
  }
  bool allDifferent() {
    std::vector<bool> ls(numVars, false);
    std::cout << cnf.size() << std::endl;
    for (Clause c:cnf) {
      std::cout << c.lits.size() << std::endl;
      for (Literal l:c.lits) {
        std::cout << l.Idx() << std::endl;
        if (ls[l.Idx()]) {
          std::cout << "not all different" << std::endl;
          return false;
        }
        ls[l.Idx()] = true;
      }
    }
    std::cout << "all different" << std::endl;
    return true;
  }
  bool solve() {
    vivify();
    std::cout << "vivified" << std::endl;
    int conflict = propagate();
    std::cout << conflict << std::endl;
    while (!allDifferent()) {
      if (conflict == -1) {
        decide();
        std::cout << "decided" << std::endl;
        conflict = propagate();
        std::cout << conflict << std::endl;
      }

      if (conflict != -1) {
        std::cout << "explaining" << std::endl;
        Clause res = explain(&cnf[conflict]);
        std::cout << res.toString() << std::endl;
        if (res.isEmpty()) {
          return false;
        }
        backjump(res);
        std::cout << "backjumped" << std::endl;
      }
    }
    return true;
  }
};