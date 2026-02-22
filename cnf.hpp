
#include <vector>
#include <algorithm>
#include <string>
#include <stack>
#include <iostream>

#pragma once
class Literal{
  unsigned int data;
  static const unsigned int NegatedMask = 0x80000000;
  Literal(){}
public:
  bool Negated() const {
    return (data&Literal::NegatedMask)!=0;
  } 
  unsigned int Idx() const {
    return data & ~Literal::NegatedMask;
  }
  Literal(unsigned int idx, bool negated){
    data = idx | (negated? Literal::NegatedMask:0);
  }
  Literal(int idx):Literal((unsigned int)(idx>0?idx:-idx), idx<0){}

  static bool Cmp(const Literal& a, const Literal& b){
    return a.Idx() < b.Idx();
  }
  static bool NegCmp(const Literal& a, const Literal& b){
    if(a.Idx()==b.Idx()) return a.data<b.data;
    return a.Idx() < b.Idx();
  }
  std::string toString(){
    return (Negated()? "~":"")+std::to_string(Idx());
  }
  bool operator ==(const Literal& o){
    return data==o.data;
  }
  Literal operator -(){
    auto l = Literal();
    l.data = data=data^NegatedMask;
    return l;
  }
};

class Assignment{
public:
  std::vector<char> assignment;
  std::vector<int> decisionLevel;
  std::vector<int> fromClause;
  int size;
  Assignment(int maxLiteral):
    assignment(maxLiteral + 1, 0),
    decisionLevel(maxLiteral + 1, 0),
    fromClause(maxLiteral + 1, 0)
  {size = maxLiteral+1;}
  void Assign(int idx, bool val, int level=1, int from=0){
    assignment[idx] = 2 | (val?1:0);
    decisionLevel[idx]=level;
    fromClause[idx]=from;
  }
  bool IsAssigned(int idx){return (assignment[idx]&2)!=0;}
  bool IsAssigned(int idx, bool &True){
    True = assignment[idx]&1;
    return (assignment[idx]&2)!=0;
  }
  bool IsTrue(int idx){assignment[idx]&1;}
  void RemoveAssignment(int idx){
    assignment[idx]=0;
  }
  void SetMaxDecisionLevel(int nlevel){
    for(int i=0; i<size; i++){
      if(decisionLevel[i]>nlevel) RemoveAssignment(nlevel);
    }
  }
};

class Clause{
  std::vector<Literal> lits;
  void Fill(std::vector<Literal> &literals){
    if(literals.empty()) return;
    std::sort(literals.begin(),literals.end(),Literal::Cmp);

    lits.push_back(literals[0]);
    for(int i=1; i<literals.size(); i++){
      if(lits[lits.size()-1].Idx() == literals[i].Idx()){
        if(lits[lits.size()-1].Negated() != literals[i].Negated()){
          valid=true;
        }
      } else{
        lits.push_back(literals[i]);
      }
    }
  }
public:
  bool valid = false;
  Clause(){}
  Clause(std::vector<Literal> &literals, bool &isValid){
    Fill(literals);
    isValid = valid;
  }
  Clause(std::vector<Literal> &literals){
    Fill(literals);
  }
  Clause(std::initializer_list<Literal> literals){
    std::vector<Literal> temp;
    for(Literal l:literals) temp.push_back(l);
    Fill(temp);
  }

  bool HasLiteral(Literal l, bool &negated){
    auto f = std::lower_bound(lits.begin(),lits.end(), l, Literal::Cmp);
    if(f!=lits.end() && f->Idx()!=l.Idx()) return false;
    negated = f->Negated()!=l.Negated();
    return true;
  }
  bool HasExactLiteral(Literal l){
    auto f = std::lower_bound(lits.begin(),lits.end(), l, Literal::Cmp);
    return f!=lits.end() && *f==l;
  }
  void RemoveLiterals(std::vector<Literal> &literals){
    auto oldLits = lits;
    std::sort(literals.begin(),literals.end(),Literal::NegCmp);
    lits.clear();
    int i=0;
    for(Literal l:oldLits){
      while(i<literals.size() && Literal::NegCmp(literals[i],l))i++;
      if(!(literals[i]==l)) lits.push_back(l);
    }
  }
  Clause Clone(){
    Clause c(lits);
    c.valid = valid;
    return c;
  }
  Clause Resolution(Clause &other, Literal pivot){
    //confirm valid usage of resolution
    bool n1,n2;
    if(!HasLiteral(pivot, n1) || !other.HasLiteral(pivot, n2) || n1==n2){
      throw "Bad usage of resolution";
    }
    std::vector<Literal> nlits;
    for(Literal l : lits) if(l.Idx()!=pivot.Idx()) nlits.push_back(l);
    for(Literal l: other.lits) if (l.Idx() != pivot.Idx()) nlits.push_back(l);
    return Clause(nlits);
  }
  std::string toString(){
    std::string str = "{";
    for(int i=0; i<lits.size()-1; i++){
      str+=lits[i].toString()+", ";
    }
    if(!lits.empty()) str+=lits[lits.size()-1].toString();
    str+="}";
    return str;
  }

  bool isSatisfied(Assignment &a){
    if(valid) return true;
    for(Literal l:lits){
      if(a.IsAssigned(l.Idx()) && l.Negated()!=a.IsTrue(l.Idx())) return true;
    }
    return false;
  }
  bool isUnit(Assignment &a, Literal &last){
    if(valid) return false;
    bool found=false;
    for(Literal l:lits){
      if(a.IsAssigned(l.Idx())) {
        if(l.Negated()!=a.IsTrue(l.Idx())) return false;
      } else {
        if(found) return false;
        last = l;
        found = true;
      }
    }
    return found;
  }
};

class CNF:std::vector<Clause>{
public:
  std::stack<int> history;
  CNF(){}

  void addClause(Clause c){
    if(c.valid) return;
    push_back(c);
  }
  void pushClauses(){
    history.push(size());
  }
  void popClauses(){
    if(history.empty()) throw "history empty; cannot pop clauses";
    else{
      int nlen = history.top();
      history.pop();
      resize(nlen);
    }
  }
  bool isSatisfied(Assignment &a){
    for(Clause c:*this) if(!c.isSatisfied(a)) return false;
    return true;
  }
};
