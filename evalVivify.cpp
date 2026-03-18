#include <iostream>
#include <filesystem>
#include <chrono>
#include <cstdlib>
#include "./cnf.hpp"
#include "./cdcl.hpp"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
  int MAX_LBD = argc > 1 ? std::atoi(argv[1]) : 3;
  std::cout << "Using max_LBD = " << MAX_LBD << std::endl;
  std::string dir = "cnfFiles/examples";
  std::vector<std::string> files;
  for (auto& entry : fs::directory_iterator(dir)) {
    if (entry.path().extension() == ".cnf") {
      files.push_back(entry.path().string());
    }
  }
  std::sort(files.begin(), files.end());

  double totalSolveTime = 0.0;

  for (const auto& filepath : files) {
    std::cout << "=== " << filepath << " ===" << std::endl;

    CNF cnf;
    int nvars = ReadFromFile(filepath, cnf);
    if (nvars < 0) {
      std::cerr << "Skipping " << filepath << std::endl;
      continue;
    }

    CdclSolver solver(nvars, cnf, false);

    auto start = std::chrono::high_resolution_clock::now();
    bool sat = solver.solve(100000, MAX_LBD);
    auto end = std::chrono::high_resolution_clock::now();

    double elapsed = std::chrono::duration<double>(end - start).count();
    totalSolveTime += elapsed;
    std::cout << "Result: " << (sat ? "SAT" : "UNSAT") << std::endl;
    std::cout << "Iterations: " << solver.iterationCount << std::endl;
    std::cout << "Clauses vivified: " << solver.vivifiedCount << std::endl;
    std::cout << "Preprocess time: " << solver.preprocessTime << "s" << std::endl;
    std::cout << "Learned clauses vivified: " << solver.learnedVivifiedCount << std::endl;
    std::cout << "Time: " << elapsed << "s" << std::endl;
    std::cout << std::endl;
  }

  std::cout << "=== Total solver time: " << totalSolveTime << "s ===" << std::endl;

  return 0;
}
