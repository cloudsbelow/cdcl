#include <iostream>
#include "./cnf.hpp"
#include <vector>

int main(){
  Clause c = {1,2,3,-7,-5,6};
  Clause c2 = {7,12,15};

  Clause c3 = c.Resolution(c2,7);
  c3.RemoveLiterals(std::vector<Literal>({1,3,10,15}));
  std::cout<<(c3).toString()<<std::endl;
}