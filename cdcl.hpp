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
  CdclSolver(int numVars, CNF& expression) : a(numVars), decision_level(1) {
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
            CdclSolver temp(a.size - 1, cnf);
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
        cnf.resize(currentCnfSize);

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
              std::cout<<"Assigning "<<l.Idx()<<" to "<<!l.Negated()<<std::endl;
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
    std::cout<<"Backjumping from decision level "<<decision_level<<" to "<<max<<std::endl;
    decision_level = max;
    a.SetMaxDecisionLevel(max);
    setup();
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

  bool solve(int maxIter=10000) {
    vivify();
    std::cout << "vivified" << std::endl;
    for(int i=0; i<maxIter; i++){
      int conflict = propagate();
      if(cnf.isSatisfied(a)){
        return true;
      }
      if(conflict == -1){
        decide();
      } else {
        if(decision_level==1) return false;
        std::cout<<"Conflict on clause "<<conflict<<": "<<cnf[conflict].toString()<<std::endl;
        Clause res = explain(conflict);
        if(res.isEmpty()){
          return false;
        }
        addClause(res);
        vivifyClause(cnf.size()-1);
        std::cout<<"Add explain clause "<<res.toString()<<std::endl;
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
