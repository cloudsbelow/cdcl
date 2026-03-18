#pragma once
#include <chrono>
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
  bool verbose;
  public:
  int iterationCount;
  int vivifiedCount;
  int learnedVivifiedCount;
  double preprocessTime;
  CdclSolver(int numVars, CNF& expression, bool verbose=false) : a(numVars), decision_level(1), verbose(verbose), iterationCount(0), vivifiedCount(0), learnedVivifiedCount(0), preprocessTime(0.0) {
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
    //std::cout<<"assigned "<<l.Idx()<<" to "<<!l.Negated()<<std::endl;
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
        int currentCnfSize = cnf.size();

        for (int i = 0; i < (int)lits.size(); i++) {
            // save current state
            cnf.pushClauses();
            for (int j = 0; j < i; j++) {
                cnf.addClause({-lits[j]});
            }

            // temp fresh assignment
            Assignment tempA(a.size - 1);
            bool conflict = false;
            bool progress = true;
            while (progress) {
                progress = false;
                // naivie prop b/c watched literals not valid
                for (int ci = 0; ci < (int)cnf.size(); ci++) {
                    if (cnf[ci].isConflict(tempA)) { conflict = true; break; }
                    Literal unit(1);
                    if (cnf[ci].isUnit(tempA, unit) && !tempA.IsAssigned(unit.Idx())) {
                        tempA.Assign(unit.Idx(), !unit.Negated(), 0, ci);
                        progress = true;
                    }
                }
                if (conflict) break;
            }

            // restore
            cnf.popClauses();

            if (conflict) break;

            if (tempA.IsAssigned(lits[i].Idx())) {
                bool litIsTrue = lits[i].Negated() != tempA.IsTrue(lits[i].Idx());
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
    decision_level += 1;
    for (Clause c:cnf) {
      if (!c.isSatisfied(a)) {
        for (Literal l:c.getLiterals()) {
          if (!a.IsAssigned(l.Idx())) {
              assign(l,-1);
              if(verbose) std::cout<<"Assigning "<<l.Idx()<<" to "<<!l.Negated()<<std::endl;
              return;
          }
        }
      }
    }
  }
  void backjump(Clause &res) {
    int max = 0;
    for (Literal l:res.getLiterals()) {
      if (a.decisionLevel[l.Idx()] > max && a.decisionLevel[l.Idx()] < decision_level) {
        max = a.decisionLevel[l.Idx()];
      }
    }
    if(verbose) std::cout<<"Backjumping from decision level "<<decision_level<<" to "<<max<<std::endl;
    decision_level = max;
    a.SetMaxDecisionLevel(max);
    setup();
  }

  
  int LBD(Clause c) {
    int lbd = 0;
    std::vector<int> decision_levels(decision_level + 1, false);
    for (Literal l:c.lits) {
      if (!decision_levels[a.decisionLevel[l.Idx()]]) {
        lbd += 1;
        decision_levels[a.decisionLevel[l.Idx()]] = true;
      }
    }
    return lbd;
  }

  Clause explain(int conflictIndex) {
    bool cont = true;
    Clause res = cnf[conflictIndex].Clone();
    while (true) {
      int num=0;
      Literal pivot(0);
      for(Literal l:res.lits) if(a.decisionLevel[l.Idx()] == decision_level){
        num++;
        if(a.fromClause[l.Idx()]!=-1) pivot=l;
      }
      if(num<=1){
        if(num==0) std::cerr<<"Error in explain; none from current level"<<std::endl;
        break;
      }
      if(pivot.Idx()==0) std::cerr<<"Error in explain; no pivot"<<std::endl;
      res = res.Resolution(cnf[a.fromClause[pivot.Idx()]],pivot);
    }
    if(res.valid) std::cerr<<"Error in explain; conflict clause valid"<<std::endl;
    return res;
  }

  bool solve(int maxIter=100000, int max_LBD=3, bool enableVivify=true, bool enablePreprocess=true) {
    if (enableVivify && enablePreprocess) {
      auto vstart = std::chrono::high_resolution_clock::now();
      vivifiedCount = vivify();
      if (vivifiedCount > 0) setup();
      auto vend = std::chrono::high_resolution_clock::now();
      preprocessTime = std::chrono::duration<double>(vend - vstart).count();
      if(verbose) std::cout << "vivified" << std::endl;
    }
    for(int i=0; i<maxIter; i++){
      iterationCount = i;
      int conflict = propagate();
      if(cnf.isSatisfied(a)){
        return true;
      }
      if(conflict == -1){
        decide();
      } else {
        if(decision_level==1) return false;
        if(verbose) std::cout<<"Conflict on clause "<<conflict<<": "<<cnf[conflict].toString()<<std::endl;
        Clause res = explain(conflict);
        if(res.isEmpty() || decision_level == 0){
          return false;
        }
        addClause(res);
        if (enableVivify && LBD(res) < max_LBD) {
          if(vivifyClause(cnf.size()-1)) learnedVivifiedCount++;
          if(verbose) std::cout << "vivified" << std::endl;
        }
        if(verbose) std::cout<<"Add explain clause "<<res.toString()<<std::endl;
        backjump(res);
      }
    }
    std::cout<<"Ran out of iterations"<<std::endl;
    return false;
  }
  void printAssignment(){
    std::cout<<"Current assignment: (satisfiable "<<cnf.isSatisfied(a)<<")"<<std::endl;
    for(int i=1; i<a.size; i++){
      std::cout<<i<<" "<<a.IsTrue(i)<<std::endl;
    }
  }
};
