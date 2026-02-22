#include<vector>
#include<queue>
#include "./cnf.hpp"

class CdclSolver{
  std::vector<std::vector<int>> watched;
  std::queue<int> toProcess;
  CNF cnf;
  Assignment a;
};