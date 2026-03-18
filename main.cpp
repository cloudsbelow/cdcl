#include <iostream>
#include "./cnf.hpp"
#include <vector>
#include "./cdcl.hpp"


void testWatchedLits(){
  CNF cnf;
  cnf.addClause({1,2,-3});
  cnf.addClause({2,3});
  cnf.addClause({-2});
  CdclSolver solver(3,cnf);
  solver.propagate();
}
void testUNSAT() {
  CNF cnf;
  cnf.addClause({1,2});
  cnf.addClause({-1,-2});
  cnf.addClause({-1,2});
  cnf.addClause({1,-2});
  CdclSolver solver(2,cnf,true);
  std::cout << solver.solve();
  solver.printAssignment();
}

int main(){
  testWatchedLits();
  CNF cnf;
  Clause c1 = {1};
  Clause c2 = {-1, 2};
  Clause c3 = {-3, 4};
  Clause c4 = {-5, -6};
  Clause c5 = {-1,-5,7};
  Clause c6 = {-2,-5,6,-7};
  cnf.addClause(c1);
  cnf.addClause(c2);
  cnf.addClause(c3);
  cnf.addClause(c4);
  cnf.addClause(c5);
  cnf.addClause(c6);
  CdclSolver solver(7, cnf);
  std::cout << "Satisfiable: " << solver.solve() << std::endl;
  solver.printAssignment();
  
  CNF c;
  int nlits = ReadFromFile("cnfFiles/examples/CBS_k3_n100_m403_b10_0.cnf", c);
  CdclSolver solver2(nlits, c);
  std::cout << "Satisfiable: " << solver2.solve(10000) << std::endl;
  solver2.printAssignment();
  testUNSAT();
}

