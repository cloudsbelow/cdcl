#pragma once
#include <vector>
#include "./cnf.hpp"
#include "ranges"
#include <map>

class CdclSolver {
public:
    CNF cnf;
    Assignment a;
    int decisionLevel;
    std::queue<int> toProcess;
    std::vector<int> trail;

    std::vector<std::pair<Literal, Literal>> clauseWatches;
    std::vector<std::vector<int>> watchList;
    std::vector<Literal> propQueue;
    int earlyConflict = -1;

    CdclSolver(int numVars) : a(numVars), decisionLevel(0), watchList(2 * (numVars + 1)) {}

    void addClause(Clause c) {
        if (c.valid) return;
        int ci = (int)cnf.size();
        cnf.addClause(c);
        if (cnf.size() <= (size_t)ci) return;
        auto& lits = cnf[ci].getLiterals();
        if (lits.size() >= 2) {
            clauseWatches.push_back({lits[0], lits[1]});
            watchList[lits[0].watchIdx()].push_back(ci);
            watchList[lits[1].watchIdx()].push_back(ci);
        } else if (lits.size() == 1) {
            clauseWatches.push_back({lits[0], lits[0]});
            if (!a.IsAssigned(lits[0].Idx())) {
                a.Assign(lits[0].Idx(), !lits[0].Negated(), 0, ci);
                trail.push_back(lits[0].Idx());
                propQueue.push_back(lits[0]);
            } else {
                bool isTrue = (lits[0].Negated() != a.IsTrue(lits[0].Idx()));
                if (!isTrue) earlyConflict = ci;
            }
        }
    }

    int propagate() {
        if (earlyConflict != -1) {
            int c = earlyConflict;
            earlyConflict = -1;
            return c;
        }
        while (!propQueue.empty()) {
            Literal p = propQueue.back();
            propQueue.pop_back();
            Literal falseLit = -p;
            auto& wlist = watchList[falseLit.watchIdx()];

            int i = 0;
            while (i < (int)wlist.size()) {
                int ci = wlist[i];
                auto& [w0, w1] = clauseWatches[ci];

                Literal wOther = (w0 == falseLit) ? w1 : w0;

                if (a.IsAssigned(wOther.Idx()) && (wOther.Negated() != a.IsTrue(wOther.Idx()))) {
                    i++;
                    continue;
                }

                auto& lits = cnf[ci].getLiterals();
                bool found = false;
                for (const Literal& lit : lits) {
                    if (lit == wOther) continue;
                    if (lit == falseLit) continue;
                    if (a.IsAssigned(lit.Idx()) && (lit.Negated() == a.IsTrue(lit.Idx()))) {
                        continue;
                    }
                    if (w0 == falseLit) w0 = lit;
                    else w1 = lit;
                    watchList[lit.watchIdx()].push_back(ci);
                    wlist[i] = wlist.back();
                    wlist.pop_back();
                    found = true;
                    break;
                }

                if (found) continue;

                if (!a.IsAssigned(wOther.Idx())) {
                    a.Assign(wOther.Idx(), !wOther.Negated(), decisionLevel, ci);
                    trail.push_back(wOther.Idx());
                    propQueue.push_back(wOther);
                    i++;
                } else {
                    return ci;
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

      void decide() {
        a.Assign(toProcess.front(), true, decisionLevel, -1);
        toProcess.pop();
      }

      int explain(Clause *conflict) {
        Literal l = a.order[a.order.size()-1];
        for (std::vector<Literal>::reverse_iterator it = a.order.rbegin(); it != a.order.rend(); it++) {
          bool _;
          if (conflict->HasLiteral(*it, _)) {
            l = *it;
            break;
          }
        }
        return a.fromClause[l.Idx()];
      }

      bool cexplains(Clause *c, Literal conflict_lit) {
        for (Literal l:a.assignment) {
          bool isNeg;
          bool hasLit  = c->HasLiteral(l, isNeg);
          if (hasLit && !isNeg) {
            return false;
          }
        }
        return true;
      }
};
