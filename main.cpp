#include <iostream>
#include "./cnf.hpp"
#include <vector>
#include "./cdcl.hpp"

int main(){
  CNF cnf;
  Clause c = {1};
  Clause c2 = {-1, 2};
  Clause c3 = {-3, 4};
  Clause c4 = {-5, -6};
  Clause c5 = {-1,-5,7};
  Clause c6 = {-2,-5,6,-7};
  cnf.addClause(c);
  cnf.addClause(c2);
  cnf.addClause(c3);
  cnf.addClause(c4);
  cnf.addClause(c5);
  cnf.addClause(c6);
  CdclSolver solver(7, cnf);
  std::cout << "Satisfiable: " << solver.solve() << std::endl;
}