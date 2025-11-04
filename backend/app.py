# app.py
from flask import Flask, jsonify, request
from flask_cors import CORS
import subprocess
import json
import tempfile
import os

app = Flask(__name__)
CORS(app)

# C++ executable wrapper
class SatelliteOptimizer:
    def __init__(self):
        self.executable_path = "./satellite_optimizer"
        
    def run_optimization(self, satellites_data=None):
        try:
            # If no custom data provided, use default
            if satellites_data is None:
                result = subprocess.run([self.executable_path], 
                                      capture_output=True, text=True)
                return self.parse_output(result.stdout)
            
            # Create temporary C++ file with custom data
            with tempfile.NamedTemporaryFile(mode='w', suffix='.cpp', delete=False) as f:
                f.write(self.generate_cpp_code(satellites_data))
                temp_file = f.name
            
            # Compile and run
            executable_name = temp_file.replace('.cpp', '')
            compile_result = subprocess.run([
                'g++', '-std=c++17', temp_file, '-o', executable_name
            ], capture_output=True, text=True)
            
            if compile_result.returncode != 0:
                return {"error": "Compilation failed", "details": compile_result.stderr}
            
            run_result = subprocess.run([executable_name], capture_output=True, text=True)
            
            # Cleanup
            os.unlink(temp_file)
            os.unlink(executable_name)
            
            return self.parse_output(run_result.stdout)
            
        except Exception as e:
            return {"error": str(e)}
    
    def generate_cpp_code(self, satellites_data):
        base_code = """
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include <memory>
#include <limits>
#include <cmath>

// ===================== CoverageInterval Class ========================
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

// ===================== Satellite Class ========================
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

// --- Coverage Summary Structure ---
struct CoverageSummary {
    double totalDuration;
    double coveredDuration;
    double coveragePercentage;
    std::vector<CoverageInterval> gaps;
    int satellitesUsed;
    double totalCost;
};

// --- Satellite Coverage Optimizer Class ---
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
        std::cout << "=== ALL REGISTERED SATELLITES ===" << std::endl;
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

int main() {
    SatelliteCoverageOptimizer optimizer(0, 24);
    """
        
        # Add satellite data
        satellite_code = ""
        for sat in satellites_data:
            satellite_code += f'    optimizer.addSatellite("{sat["name"]}", {sat["start"]}, {sat["end"]}, {sat["cost"]}, "{sat["region"]}");\n'
        
        end_code = """
    // Run analysis
    optimizer.printAllSatellites();
    
    auto [minSats, minGaps] = optimizer.findMinimumSatellites("All");
    
    std::cout << "=== MINIMUM SATELLITES RESULT ===" << std::endl;
    std::cout << "Selected Satellites: " << minSats.size() << std::endl;
    for (const auto& sat : minSats) {
        std::cout << sat.getName() << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Coverage Gaps: " << minGaps.size() << std::endl;
    for (const auto& gap : minGaps) {
        std::cout << gap.getStart() << "-" << gap.getEnd() << " ";
    }
    std::cout << std::endl;
    
    CoverageSummary summary = optimizer.getCoverageSummary("All");
    std::cout << "=== SUMMARY ===" << std::endl;
    std::cout << "Total Duration: " << summary.totalDuration << std::endl;
    std::cout << "Covered Duration: " << summary.coveredDuration << std::endl;
    std::cout << "Coverage Percentage: " << summary.coveragePercentage << std::endl;
    std::cout << "Satellites Used: " << summary.satellitesUsed << std::endl;
    std::cout << "Total Cost: " << summary.totalCost << std::endl;
    
    return 0;
}
        """
        
        return base_code + satellite_code + end_code
    
    def parse_output(self, output):
        lines = output.split('\n')
        result = {
            'all_satellites': [],
            'min_satellites': {'selected': [], 'gaps': []},
            'summary': {}
        }
        
        current_section = None
        
        for line in lines:
            if '=== ALL REGISTERED SATELLITES ===' in line:
                current_section = 'all_satellites'
                continue
            elif '=== MINIMUM SATELLITES RESULT ===' in line:
                current_section = 'min_satellites'
                continue
            elif '=== SUMMARY ===' in line:
                current_section = 'summary'
                continue
            
            if current_section == 'all_satellites' and '---' not in line and line.strip():
                parts = line.split()
                if len(parts) >= 6:
                    result['all_satellites'].append({
                        'name': parts[0],
                        'start': float(parts[1]),
                        'end': float(parts[2]),
                        'duration': float(parts[3]),
                        'cost': float(parts[4]),
                        'region': parts[5]
                    })
            
            elif current_section == 'min_satellites':
                if 'Selected Satellites:' in line:
                    sats = line.split(':')[1].strip().split()
                    result['min_satellites']['selected'] = sats
                elif 'Coverage Gaps:' in line:
                    gaps_text = line.split(':')[1].strip()
                    if gaps_text != '0':
                        gap_parts = gaps_text.split()
                        for i in range(0, len(gap_parts), 2):
                            if i+1 < len(gap_parts):
                                start_end = gap_parts[i].split('-')
                                if len(start_end) == 2:
                                    result['min_satellites']['gaps'].append({
                                        'start': float(start_end[0]),
                                        'end': float(start_end[1])
                                    })
            
            elif current_section == 'summary':
                if 'Total Duration:' in line:
                    result['summary']['total_duration'] = float(line.split(':')[1].strip())
                elif 'Covered Duration:' in line:
                    result['summary']['covered_duration'] = float(line.split(':')[1].strip())
                elif 'Coverage Percentage:' in line:
                    result['summary']['coverage_percentage'] = float(line.split(':')[1].strip())
                elif 'Satellites Used:' in line:
                    result['summary']['satellites_used'] = int(line.split(':')[1].strip())
                elif 'Total Cost:' in line:
                    result['summary']['total_cost'] = float(line.split(':')[1].strip())
        
        return result

optimizer = SatelliteOptimizer()

# API Routes
@app.route('/api/optimize', methods=['POST'])
def optimize_coverage():
    data = request.json
    satellites = data.get('satellites', [])
    
    result = optimizer.run_optimization(satellites)
    return jsonify(result)

@app.route('/api/default-optimization', methods=['GET'])
def default_optimization():
    result = optimizer.run_optimization()
    return jsonify(result)

@app.route('/api/health', methods=['GET'])
def health_check():
    return jsonify({"status": "healthy", "message": "Satellite Coverage Optimizer API is running"})

if __name__ == '__main__':
    app.run(debug=True, port=5000)