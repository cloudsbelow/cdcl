#include <iostream>
#include "./cdcl.hpp"

void printClauses(CdclSolver &s) {
    for (int i = 0; i < (int)s.cnf.size(); i++) {
        std::cout << "  C" << i << ": " << s.cnf[i].toString() << std::endl;
    }
}

int main(){
    // Test 1: Unit propagation with conflict
    // (x1) & (~x1 | x2) & (~x2 | x3) & (~x3)
    std::cout << "=== Test 1: Unit prop with conflict ===" << std::endl;
    {
        CdclSolver s(3);
        s.addClause({1});
        s.addClause({-1, 2});
        s.addClause({-2, 3});
        s.addClause({-3});

        int c = s.propagate();
        if (c >= 0)
            std::cout << "Conflict at clause " << c << std::endl;
        else
            std::cout << "No conflict" << std::endl;
        for (int i = 1; i <= 3; i++)
            if (s.a.IsAssigned(i))
                std::cout << "x" << i << " = " << (s.a.IsTrue(i) ? "true" : "false") << std::endl;
    }

    // Test 2: Unit propagation without conflict
    // (x1) & (~x1 | x2) & (~x2 | x3)
    std::cout << "\n=== Test 2: Unit prop without conflict ===" << std::endl;
    {
        CdclSolver s(3);
        s.addClause({1});
        s.addClause({-1, 2});
        s.addClause({-2, 3});

        int c = s.propagate();
        if (c >= 0)
            std::cout << "Conflict at clause " << c << std::endl;
        else
            std::cout << "No conflict" << std::endl;
        for (int i = 1; i <= 3; i++)
            if (s.a.IsAssigned(i))
                std::cout << "x" << i << " = " << (s.a.IsTrue(i) ? "true" : "false") << std::endl;
    }

    // Test 3: Vivification - subsumption shortening
    // C0: (x1 | x2), C1: (x1 | x2 | x3)
    // Under ~x1, C0 forces x2=true, making x3 unnecessary in C1
    std::cout << "\n=== Test 3: Vivification (subsumption) ===" << std::endl;
    {
        CdclSolver s(3);
        s.addClause({1, 2});
        s.addClause({1, 2, 3});
        std::cout << "Before:" << std::endl;
        printClauses(s);

        int n = s.vivify();
        std::cout << "Clauses shortened: " << n << std::endl;
        std::cout << "After:" << std::endl;
        printClauses(s);
    }

    // Test 4: Vivification - literal removal
    // C0: (x1 | ~x3), C1: (x1 | x2 | x3)
    // Under ~x1, C0 forces x3=false, so x3 is useless in C1
    std::cout << "\n=== Test 4: Vivification (literal removal) ===" << std::endl;
    {
        CdclSolver s(3);
        s.addClause({1, -3});
        s.addClause({1, 2, 3});
        std::cout << "Before:" << std::endl;
        printClauses(s);

        int n = s.vivify();
        std::cout << "Clauses shortened: " << n << std::endl;
        std::cout << "After:" << std::endl;
        printClauses(s);
    }

    // Test 5: Vivification - conflict shortening
    // C0: (x1), C1: (x2), C2: (x1 | x2 | x3 | x4)
    // x1 and x2 are forced true by the formula alone,
    // so C2 is immediately satisfied and shortened to {x1}
    std::cout << "\n=== Test 5: Vivification (implied true at start) ===" << std::endl;
    {
        CdclSolver s(4);
        s.addClause({1});
        s.addClause({2});
        s.addClause({1, 2, 3, 4});
        std::cout << "Before:" << std::endl;
        printClauses(s);

        int n = s.vivify();
        std::cout << "Clauses shortened: " << n << std::endl;
        std::cout << "After:" << std::endl;
        printClauses(s);
    }

    return 0;
}
