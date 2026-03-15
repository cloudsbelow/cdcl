#include <iostream>
#include <filesystem>
#include <chrono>
#include <minisat/core/Solver.h>
#include <minisat/core/Dimacs.h>
#include <zlib.h>

namespace fs = std::filesystem;

int main() {
  std::string dir = "cnfFiles/examples";
  std::vector<std::string> files;
  for (auto& entry : fs::directory_iterator(dir)) {
    if (entry.path().extension() == ".cnf") {
      files.push_back(entry.path().string());
    }
  }
  std::sort(files.begin(), files.end());

  auto totalStart = std::chrono::high_resolution_clock::now();

  for (const auto& filepath : files) {
    std::cout << "=== " << filepath << " ===" << std::endl;

    Minisat::Solver solver;
    gzFile in = gzopen(filepath.c_str(), "rb");
    if (in == NULL) {
      std::cerr << "Skipping " << filepath << std::endl;
      continue;
    }
    Minisat::parse_DIMACS(in, solver);
    gzclose(in);

    auto start = std::chrono::high_resolution_clock::now();
    bool sat = solver.solve();
    auto end = std::chrono::high_resolution_clock::now();

    double elapsed = std::chrono::duration<double>(end - start).count();
    std::cout << "Result: " << (sat ? "SAT" : "UNSAT") << std::endl;
    std::cout << "Time: " << elapsed << "s" << std::endl;
    std::cout << std::endl;
  }

  auto totalEnd = std::chrono::high_resolution_clock::now();
  double totalElapsed = std::chrono::duration<double>(totalEnd - totalStart).count();
  std::cout << "=== Total time: " << totalElapsed << "s ===" << std::endl;

  return 0;
}
