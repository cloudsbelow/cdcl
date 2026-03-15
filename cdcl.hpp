#pragma once
#include <vector>
#include "./cnf.hpp"

class CdclSolver {
public:
    CNF cnf;
    Assignment a;
    int decisionLevel;
    std::vector<int> trail;

    CdclSolver(int numVars) : a(numVars), decisionLevel(0) {}

    void addClause(Clause c) {
        cnf.addClause(c);
    }

    int propagate() {
        bool progress = true;
        while (progress) {
            progress = false;
            for (int i = 0; i < (int)cnf.size(); i++) {
                if (cnf[i].isConflict(a)) {
                    return i;
                }
                Literal unit(1);
                if (cnf[i].isUnit(a, unit)) {
                    a.Assign(unit.Idx(), !unit.Negated(), decisionLevel, i);
                    trail.push_back(unit.Idx());
                    progress = true;
                }
            }
        }
        return -1;
    }

    bool vivifyClause(int clauseIdx) {
        auto lits = cnf[clauseIdx].getLiterals();
        if ((int)lits.size() <= 1) return false;

        std::vector<Literal> surviving;

        for (int i = 0; i < (int)lits.size(); i++) {
            CdclSolver temp(a.size - 1);
            for (int j = 0; j < (int)cnf.size(); j++) {
                if (j != clauseIdx) temp.addClause(cnf[j]);
            }
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
};
