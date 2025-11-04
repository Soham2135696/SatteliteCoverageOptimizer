#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include <memory>
#include <limits>
#include <cmath>

// ==================== CoverageInterval Class ====================
class CoverageInterval {
private:
    double start;
    double end;

public:
    CoverageInterval(double s, double e) : start(s), end(e) {}
    
    double getStart() const { return start; }
    double getEnd() const { return end; }
    double getDuration() const { return end - start; }
    
    bool overlaps(const CoverageInterval& other) const {
        return !(end <= other.start || start >= other.end);
    }
    
    bool operator<(const CoverageInterval& other) const {
        return start < other.start;
    }
};

// ==================== Satellite Class ====================
class Satellite {
private:
    std::string name;
    CoverageInterval interval;
    double cost;
    std::string region;

public:
    Satellite(const std::string& n, const CoverageInterval& i, 
              double c = 1.0, const std::string& r = "Global")
        : name(n), interval(i), cost(c), region(r) {}
    
    std::string getName() const { return name; }
    CoverageInterval getInterval() const { return interval; }
    double getCost() const { return cost; }
    std::string getRegion() const { return region; }
    
    bool operator<(const Satellite& other) const {
        return interval.getStart() < other.interval.getStart();
    }
};

// ==================== Coverage Summary Structure ====================
struct CoverageSummary {
    double totalDuration;
    double coveredDuration;
    double coveragePercentage;
    std::vector<CoverageInterval> gaps;
    int satellitesUsed;
    double totalCost;
};

// ==================== Satellite Coverage Optimizer Class ====================
class SatelliteCoverageOptimizer {
private:
    std::vector<Satellite> satellites;
    double targetStart;
    double targetEnd;

    std::vector<Satellite> filterByRegion(const std::string& region) const {
        if (region == "All") {
            return satellites;
        }
        
        std::vector<Satellite> filtered;
        for (const auto& sat : satellites) {
            if (sat.getRegion() == region) {
                filtered.push_back(sat);
            }
        }
        return filtered;
    }

public:
    SatelliteCoverageOptimizer(double start = 0, double end = 24)
        : targetStart(start), targetEnd(end) {}
    
    void addSatellite(const std::string& name, double start, double end,
                     double cost = 1.0, const std::string& region = "Global") {
        CoverageInterval interval(start, end);
        satellites.emplace_back(name, interval, cost, region);
    }
    
    // Greedy Algorithm: Minimum Number of Satellites
    std::pair<std::vector<Satellite>, std::vector<CoverageInterval>>
    findMinimumSatellites(const std::string& region = "All") {
        std::vector<Satellite> filtered = filterByRegion(region);
        std::sort(filtered.begin(), filtered.end());
        
        std::vector<Satellite> selected;
        std::vector<CoverageInterval> gaps;
        double currentEnd = targetStart;
        size_t i = 0;
        
        while (currentEnd < targetEnd && i < filtered.size()) {
            std::vector<Satellite> candidates;
            
            // Collect all satellites that can cover current position
            while (i < filtered.size() && 
                   filtered[i].getInterval().getStart() <= currentEnd) {
                candidates.push_back(filtered[i]);
                i++;
            }
            
            if (candidates.empty()) {
                // Gap detected
                double gapStart = currentEnd;
                double gapEnd = (i < filtered.size()) ? 
                    std::min(filtered[i].getInterval().getStart(), targetEnd) : targetEnd;
                
                if (gapStart < gapEnd) {
                    gaps.emplace_back(gapStart, gapEnd);
                }
                
                if (i < filtered.size()) {
                    currentEnd = filtered[i].getInterval().getEnd();
                    selected.push_back(filtered[i]);
                    i++;
                } else {
                    break;
                }
            } else {
                // Select satellite with maximum end time
                auto bestCandidate = std::max_element(
                    candidates.begin(), candidates.end(),
                    [](const Satellite& a, const Satellite& b) {
                        return a.getInterval().getEnd() < b.getInterval().getEnd();
                    }
                );
                
                selected.push_back(*bestCandidate);
                currentEnd = bestCandidate->getInterval().getEnd();
            }
        }
        
        if (currentEnd < targetEnd) {
            gaps.emplace_back(currentEnd, targetEnd);
        }
        
        return {selected, gaps};
    }
    
    // Dynamic Programming: Minimum Cost Coverage
    std::tuple<std::vector<Satellite>, double, std::vector<CoverageInterval>>
    findMinimumCostCoverage(const std::string& region = "All") {
        std::vector<Satellite> filtered = filterByRegion(region);
        std::sort(filtered.begin(), filtered.end());
        
        int n = filtered.size();
        std::vector<double> dp(n + 1, std::numeric_limits<double>::infinity());
        std::vector<int> parent(n + 1, -1);
        dp[0] = 0;
        
        for (int i = 0; i < n; i++) {
            if (dp[i] == std::numeric_limits<double>::infinity()) continue;
            
            double currentEnd = (i == 0) ? targetStart : 
                filtered[parent[i]].getInterval().getEnd();
            
            for (int j = i; j < n; j++) {
                if (filtered[j].getInterval().getStart() > currentEnd) break;
                
                double newCost = dp[i] + filtered[j].getCost();
                if (newCost < dp[j + 1]) {
                    dp[j + 1] = newCost;
                    parent[j + 1] = j;
                }
            }
        }
        
        // Reconstruct solution
        std::vector<Satellite> selected;
        double totalCost = 0;
        int idx = n;
        
        while (idx > 0 && parent[idx] != -1) {
            selected.push_back(filtered[parent[idx]]);
            totalCost += filtered[parent[idx]].getCost();
            idx = parent[idx];
        }
        
        std::reverse(selected.begin(), selected.end());
        
        // Find gaps
        std::vector<CoverageInterval> gaps;
        double currentEnd = targetStart;
        
        for (const auto& sat : selected) {
            if (sat.getInterval().getStart() > currentEnd) {
                gaps.emplace_back(currentEnd, sat.getInterval().getStart());
            }
            currentEnd = std::max(currentEnd, sat.getInterval().getEnd());
        }
        
        if (currentEnd < targetEnd) {
            gaps.emplace_back(currentEnd, targetEnd);
        }
        
        return {selected, totalCost, gaps};
    }
    
    // Get comprehensive coverage summary
    CoverageSummary getCoverageSummary(const std::string& region = "All") {
        auto [selected, gaps] = findMinimumSatellites(region);
        
        CoverageSummary summary;
        summary.totalDuration = targetEnd - targetStart;
        summary.satellitesUsed = selected.size();
        summary.gaps = gaps;
        summary.totalCost = 0;
        
        double coveredTime = 0;
        for (const auto& sat : selected) {
            summary.totalCost += sat.getCost();
            double start = std::max(targetStart, sat.getInterval().getStart());
            double end = std::min(targetEnd, sat.getInterval().getEnd());
            coveredTime += (end - start);
        }
        
        summary.coveredDuration = coveredTime;
        summary.coveragePercentage = (coveredTime / summary.totalDuration) * 100.0;
        
        return summary;
    }
    
    void printAllSatellites() const {
        std::cout << "\n=== ALL REGISTERED SATELLITES ===" << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        std::cout << std::left << std::setw(12) << "Name" 
                  << std::setw(10) << "Start" 
                  << std::setw(10) << "End"
                  << std::setw(12) << "Duration" 
                  << std::setw(10) << "Cost"
                  << std::setw(15) << "Region" << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        
        for (const auto& sat : satellites) {
            std::cout << std::left << std::setw(12) << sat.getName()
                      << std::setw(10) << std::fixed << std::setprecision(1) 
                      << sat.getInterval().getStart()
                      << std::setw(10) << sat.getInterval().getEnd()
                      << std::setw(12) << sat.getInterval().getDuration()
                      << std::setw(10) << std::setprecision(2) << sat.getCost()
                      << std::setw(15) << sat.getRegion() << std::endl;
        }
        std::cout << std::string(80, '-') << std::endl;
    }
};

// ==================== Visualization Helper Class ====================
class CoverageVisualizer {
public:
    static void printSatelliteList(const std::vector<Satellite>& satellites) {
        std::cout << "\nSelected Satellites:" << std::endl;
        std::cout << std::string(70, '-') << std::endl;
        std::cout << std::left << std::setw(12) << "Name" 
                  << std::setw(10) << "Start" 
                  << std::setw(10) << "End"
                  << std::setw(12) << "Duration" 
                  << std::setw(10) << "Cost"
                  << std::setw(15) << "Region" << std::endl;
        std::cout << std::string(70, '-') << std::endl;
        
        for (const auto& sat : satellites) {
            std::cout << std::left << std::setw(12) << sat.getName()
                      << std::setw(10) << std::fixed << std::setprecision(1) 
                      << sat.getInterval().getStart()
                      << std::setw(10) << sat.getInterval().getEnd()
                      << std::setw(12) << sat.getInterval().getDuration()
                      << std::setw(10) << std::setprecision(2) << sat.getCost()
                      << std::setw(15) << sat.getRegion() << std::endl;
        }
        std::cout << std::string(70, '-') << std::endl;
    }
    
    static void printCoverageGaps(const std::vector<CoverageInterval>& gaps) {
        if (gaps.empty()) {
            std::cout << "\n✓ No coverage gaps! Full coverage achieved." << std::endl;
            return;
        }
        
        std::cout << "\n⚠ Coverage Gaps Detected:" << std::endl;
        for (size_t i = 0; i < gaps.size(); i++) {
            std::cout << "  Gap " << (i + 1) << ": " 
                      << std::fixed << std::setprecision(1)
                      << gaps[i].getStart() << "h to " << gaps[i].getEnd() 
                      << "h (duration: " << gaps[i].getDuration() 
                      << " hours)" << std::endl;
        }
    }
    
    static void printSummary(const CoverageSummary& summary) {
        std::cout << "\n=== COVERAGE SUMMARY ===" << std::endl;
        std::cout << "Total Duration:      " << std::fixed << std::setprecision(1) 
                  << summary.totalDuration << " hours" << std::endl;
        std::cout << "Covered Duration:    " << summary.coveredDuration << " hours" << std::endl;
        std::cout << "Coverage Percentage: " << std::setprecision(2) 
                  << summary.coveragePercentage << "%" << std::endl;
        std::cout << "Satellites Used:     " << summary.satellitesUsed << std::endl;
        std::cout << "Total Cost:          $" << summary.totalCost << std::endl;
        std::cout << "Coverage Gaps:       " << summary.gaps.size() << std::endl;
    }
};

// ==================== Main Function ====================
int main() {
    std::cout << "╔════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║        SATELLITE COVERAGE OPTIMIZATION SYSTEM                  ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════╝" << std::endl;
    
    // Initialize optimizer for 24-hour period
    SatelliteCoverageOptimizer optimizer(0, 24);
    
    // Add satellites with different regions and costs
    optimizer.addSatellite("Sat-Alpha", 0, 6, 1200, "Asia");
    optimizer.addSatellite("Sat-Beta", 4, 10, 1500, "Europe");
    optimizer.addSatellite("Sat-Gamma", 8, 14, 1800, "Asia");
    optimizer.addSatellite("Sat-Delta", 12, 18, 1300, "Americas");
    optimizer.addSatellite("Sat-Epsilon", 16, 22, 1600, "Europe");
    optimizer.addSatellite("Sat-Zeta", 20, 24, 1100, "Global");
    optimizer.addSatellite("Sat-Eta", 2, 8, 900, "Asia");
    optimizer.addSatellite("Sat-Theta", 10, 16, 1400, "Europe");
    optimizer.addSatellite("Sat-Iota", 14, 20, 1700, "Americas");
    optimizer.addSatellite("Sat-Kappa", 18, 23, 1000, "Global");
    
    // Display all registered satellites
    optimizer.printAllSatellites();
    
    // Algorithm 1: Minimum Satellites (Greedy Approach)
    std::cout << "\n\n╔════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  ALGORITHM 1: MINIMUM SATELLITES (GREEDY INTERVAL SCHEDULING) ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════╝" << std::endl;
    
    auto [minSats, minGaps] = optimizer.findMinimumSatellites("All");
    CoverageVisualizer::printSatelliteList(minSats);
    CoverageVisualizer::printCoverageGaps(minGaps);
    
    // Algorithm 2: Minimum Cost Coverage (Dynamic Programming)
    std::cout << "\n\n╔════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  ALGORITHM 2: MINIMUM COST COVERAGE (DYNAMIC PROGRAMMING)     ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════╝" << std::endl;
    
    auto [costSats, totalCost, costGaps] = optimizer.findMinimumCostCoverage("All");
    CoverageVisualizer::printSatelliteList(costSats);
    std::cout << "\n→ Total Cost: $" << std::fixed << std::setprecision(2) 
              << totalCost << std::endl;
    CoverageVisualizer::printCoverageGaps(costGaps);
    
    // Regional Analysis
    std::cout << "\n\n╔════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                    REGIONAL ANALYSIS                           ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════╝" << std::endl;
    
    std::vector<std::string> regions = {"Asia", "Europe", "Americas"};
    for (const auto& region : regions) {
        std::cout << "\n--- Region: " << region << " ---" << std::endl;
        CoverageSummary summary = optimizer.getCoverageSummary(region);
        CoverageVisualizer::printSummary(summary);
    }
    
    // Overall Summary
    std::cout << "\n\n╔════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                  GLOBAL COVERAGE SUMMARY                       ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════╝" << std::endl;
    
    CoverageSummary globalSummary = optimizer.getCoverageSummary("All");
    CoverageVisualizer::printSummary(globalSummary);
    
    std::cout << "\n\n✓ Optimization Complete!" << std::endl;
    std::cout << "════════════════════════════════════════════════════════════════\n" << std::endl;
    
    return 0;
}