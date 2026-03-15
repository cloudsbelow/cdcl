#pragma once
#include <vector>
#include "./cnf.hpp"
#include "ranges"
#include <map>
#include <queue>

class CdclSolver{
  std::map<int, std::vector<int>> watched;
  std::queue<int> toProcess;
  CNF cnf;
  Assignment a;
  int decision_level;
  std::map<int, bool> trail;
  int numVars;
  public:
  CdclSolver(int numVars, CNF& expression) : a(numVars), decision_level(0) {
    cnf = expression;
    setup();
  }
  void addClause(Clause c) {
      cnf.addClause(c);
  }
  void setup(){
    watched.clear();
    while(!toProcess.empty())toProcess.pop();
    bool hasConflict=false;
    for(int i=0; i<cnf.size(); i++){
      Clause &c = cnf[i];
      Literal w1(0);
      Literal w2(0);
      int status = c.getStatus(a, w1, w2);
      if(status==-1) continue;
      else if(status==0) toProcess.push(i); //there will be a conflict
      else if(status==1) toProcess.push(i);
      else {
        watched[w1.Idx()].push_back(i);
        watched[w2.Idx()].push_back(i);
      }
    }
  }
  void assign(Literal l, int from){
    a.Assign(l.Idx(),!l.Negated(), decision_level, from);
    std::cout<<"assigned "<<l.Idx()<<" to "<<!l.Negated()<<std::endl;
    for(int i:watched[l.Idx()]){
      toProcess.push(i);
    }
    watched.erase(l.Idx());
  }
  int propagate(){
    while(!toProcess.empty()){
      int top = toProcess.front();
      toProcess.pop();
      Clause &c = cnf[top];
      Literal w1(0);
      Literal w2(0);
      int status = c.getStatus(a, w1, w2);
      if(status == -1){
        continue;
      } else if(status==0){
        return top;
      } else if(status == 1){
        assign(w1,top);
      } else {
        watched[w2.Idx()].push_back(top);
      }
    }
    //std::cout<<"done propegate"<<std::endl;
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
              assign(l,-1);
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
    setup();
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
