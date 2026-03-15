#include <iostream>
#include "./cdcl.hpp"

void printClauses(CdclSolver &s) {
    for (int i = 0; i < (int)s.cnf.size(); i++) {
        std::cout << "  C" << i << ": " << s.cnf[i].toString() << std::endl;
    }
}

int main(){
  Clause c = {1,2,3,-7,-5,6};
  Clause c2 = {7,12,15};
}
