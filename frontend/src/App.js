// App.js
import React, { useState, useEffect } from 'react';
import './App.css';

const App = () => {
  const [satellites, setSatellites] = useState([]);
  const [optimizationResult, setOptimizationResult] = useState(null);
  const [loading, setLoading] = useState(false);
  const [newSatellite, setNewSatellite] = useState({
    name: '',
    start: '',
    end: '',
    cost: '',
    region: 'Asia'
  });

  // Default satellites
  const defaultSatellites = [
    { name: 'Sat-Alpha', start: 0, end: 6, cost: 1200, region: 'Asia' },
    { name: 'Sat-Beta', start: 4, end: 10, cost: 1500, region: 'Europe' },
    { name: 'Sat-Gamma', start: 8, end: 14, cost: 1800, region: 'Asia' },
    { name: 'Sat-Delta', start: 12, end: 18, cost: 1300, region: 'Americas' },
    { name: 'Sat-Epsilon', start: 16, end: 22, cost: 1600, region: 'Europe' }
  ];

  useEffect(() => {
    setSatellites(defaultSatellites);
  }, []);

  const runOptimization = async (customSatellites = null) => {
    setLoading(true);
    try {
      const payload = {
        satellites: customSatellites || satellites
      };

      const response = await fetch('http://localhost:5000/api/optimize', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify(payload),
      });

      const result = await response.json();
      setOptimizationResult(result);
    } catch (error) {
      console.error('Optimization error:', error);
      alert('Error running optimization. Make sure the backend is running.');
    }
    setLoading(false);
  };

  const runDefaultOptimization = async () => {
    setLoading(true);
    try {
      const response = await fetch('http://localhost:5000/api/default-optimization');
      const result = await response.json();
      setOptimizationResult(result);
    } catch (error) {
      console.error('Default optimization error:', error);
    }
    setLoading(false);
  };

  const addSatellite = () => {
    if (newSatellite.name && newSatellite.start !== '' && newSatellite.end !== '' && newSatellite.cost !== '') {
      const satellite = {
        ...newSatellite,
        start: parseFloat(newSatellite.start),
        end: parseFloat(newSatellite.end),
        cost: parseFloat(newSatellite.cost)
      };
      setSatellites([...satellites, satellite]);
      setNewSatellite({ name: '', start: '', end: '', cost: '', region: 'Asia' });
    }
  };

  const removeSatellite = (index) => {
    const updatedSatellites = satellites.filter((_, i) => i !== index);
    setSatellites(updatedSatellites);
  };

  const TimelineVisualization = ({ satellites, gaps, title }) => {
    const totalHours = 24;
    const scale = 100 / totalHours;

    return (
      <div className="timeline-section">
        <h3>{title}</h3>
        <div className="timeline-container">
          <div className="timeline-axis">
            {Array.from({ length: totalHours + 1 }, (_, i) => (
              <div key={i} className="timeline-hour-marker">
                <span>{i}h</span>
              </div>
            ))}
          </div>
          
          <div className="timeline-content">
            {satellites.map((sat, index) => (
              <div
                key={index}
                className="satellite-timeline"
                style={{
                  left: `${sat.start * scale}%`,
                  width: `${(sat.end - sat.start) * scale}%`,
                  backgroundColor: getRegionColor(sat.region)
                }}
                title={`${sat.name} (${sat.region})`}
              >
                <span className="satellite-label">{sat.name}</span>
              </div>
            ))}
            
            {gaps && gaps.map((gap, index) => (
              <div
                key={index}
                className="coverage-gap"
                style={{
                  left: `${gap.start * scale}%`,
                  width: `${(gap.end - gap.start) * scale}%`
                }}
                title={`Gap: ${gap.start}h - ${gap.end}h`}
              >
                <span>Gap</span>
              </div>
            ))}
          </div>
        </div>
      </div>
    );
  };

  const getRegionColor = (region) => {
    const colors = {
      'Asia': '#ff6b6b',
      'Europe': '#4ecdc4',
      'Americas': '#45b7d1',
      'Global': '#96ceb4'
    };
    return colors[region] || '#cccccc';
  };

  return (
    <div className="App">
      <header className="app-header">
        <h1>🛰️ Satellite Coverage Optimizer</h1>
        <p>Visual optimization tool for satellite coverage planning</p>
      </header>

      <div className="container">
        {/* Satellite Management */}
        <div className="section">
          <h2>Satellite Management</h2>
          
          <div className="add-satellite-form">
            <h3>Add New Satellite</h3>
            <div className="form-row">
              <input
                type="text"
                placeholder="Satellite Name"
                value={newSatellite.name}
                onChange={(e) => setNewSatellite({...newSatellite, name: e.target.value})}
              />
              <input
                type="number"
                placeholder="Start Hour"
                min="0"
                max="24"
                step="0.5"
                value={newSatellite.start}
                onChange={(e) => setNewSatellite({...newSatellite, start: e.target.value})}
              />
              <input
                type="number"
                placeholder="End Hour"
                min="0"
                max="24"
                step="0.5"
                value={newSatellite.end}
                onChange={(e) => setNewSatellite({...newSatellite, end: e.target.value})}
              />
              <input
                type="number"
                placeholder="Cost"
                min="0"
                step="100"
                value={newSatellite.cost}
                onChange={(e) => setNewSatellite({...newSatellite, cost: e.target.value})}
              />
              <select
                value={newSatellite.region}
                onChange={(e) => setNewSatellite({...newSatellite, region: e.target.value})}
              >
                <option value="Asia">Asia</option>
                <option value="Europe">Europe</option>
                <option value="Americas">Americas</option>
                <option value="Global">Global</option>
              </select>
              <button onClick={addSatellite}>Add Satellite</button>
            </div>
          </div>

          <div className="satellites-list">
            <h3>Current Satellites ({satellites.length})</h3>
            <div className="satellites-grid">
              {satellites.map((sat, index) => (
                <div key={index} className="satellite-card">
                  <div className="satellite-header">
                    <span className="satellite-name">{sat.name}</span>
                    <span className="satellite-region" style={{backgroundColor: getRegionColor(sat.region)}}>
                      {sat.region}
                    </span>
                    <button className="remove-btn" onClick={() => removeSatellite(index)}>×</button>
                  </div>
                  <div className="satellite-details">
                    <div>Time: {sat.start}h - {sat.end}h</div>
                    <div>Duration: {sat.end - sat.start}h</div>
                    <div>Cost: ${sat.cost}</div>
                  </div>
                </div>
              ))}
            </div>
          </div>
        </div>

        {/* Optimization Controls */}
        <div className="section">
          <h2>Optimization</h2>
          <div className="optimization-controls">
            <button 
              onClick={() => runOptimization()}
              disabled={loading}
              className="primary-btn"
            >
              {loading ? 'Running...' : 'Run Optimization'}
            </button>
            <button 
              onClick={runDefaultOptimization}
              disabled={loading}
              className="secondary-btn"
            >
              {loading ? 'Running...' : 'Use Default Data'}
            </button>
          </div>
        </div>

        {/* Results Visualization */}
        {optimizationResult && (
          <div className="section">
            <h2>Optimization Results</h2>
            
            {/* Summary */}
            {optimizationResult.summary && (
              <div className="summary-section">
                <h3>Coverage Summary</h3>
                <div className="summary-grid">
                  <div className="summary-item">
                    <label>Total Duration</label>
                    <span>{optimizationResult.summary.total_duration}h</span>
                  </div>
                  <div className="summary-item">
                    <label>Covered Duration</label>
                    <span>{optimizationResult.summary.covered_duration}h</span>
                  </div>
                  <div className="summary-item">
                    <label>Coverage Percentage</label>
                    <span>{optimizationResult.summary.coverage_percentage.toFixed(2)}%</span>
                  </div>
                  <div className="summary-item">
                    <label>Satellites Used</label>
                    <span>{optimizationResult.summary.satellites_used}</span>
                  </div>
                  <div className="summary-item">
                    <label>Total Cost</label>
                    <span>${optimizationResult.summary.total_cost.toFixed(2)}</span>
                  </div>
                </div>
              </div>
            )}

            {/* Timeline Visualizations */}
            <div className="timeline-comparison">
              <TimelineVisualization
                satellites={optimizationResult.all_satellites}
                title="All Available Satellites"
              />
              
              {optimizationResult.min_satellites && (
                <TimelineVisualization
                  satellites={optimizationResult.all_satellites.filter(sat => 
                    optimizationResult.min_satellites.selected.includes(sat.name)
                  )}
                  gaps={optimizationResult.min_satellites.gaps}
                  title="Optimized Coverage (Minimum Satellites)"
                />
              )}
            </div>

            {/* Satellite Details */}
            <div className="results-details">
              <div className="results-column">
                <h3>All Satellites</h3>
                <div className="satellite-table">
                  <div className="table-header">
                    <span>Name</span>
                    <span>Start</span>
                    <span>End</span>
                    <span>Duration</span>
                    <span>Cost</span>
                    <span>Region</span>
                  </div>
                  {optimizationResult.all_satellites.map((sat, index) => (
                    <div key={index} className="table-row">
                      <span>{sat.name}</span>
                      <span>{sat.start}h</span>
                      <span>{sat.end}h</span>
                      <span>{sat.duration}h</span>
                      <span>${sat.cost}</span>
                      <span>{sat.region}</span>
                    </div>
                  ))}
                </div>
              </div>

              {optimizationResult.min_satellites && (
                <div className="results-column">
                  <h3>Selected Satellites (Optimized)</h3>
                  <div className="optimized-list">
                    {optimizationResult.min_satellites.selected.map((satName, index) => {
                      const sat = optimizationResult.all_satellites.find(s => s.name === satName);
                      return sat ? (
                        <div key={index} className="optimized-satellite">
                          <span className="sat-name">{sat.name}</span>
                          <span className="sat-time">{sat.start}h-{sat.end}h</span>
                          <span className="sat-cost">${sat.cost}</span>
                          <span className="sat-region">{sat.region}</span>
                        </div>
                      ) : null;
                    })}
                  </div>
                  
                  {optimizationResult.min_satellites.gaps.length > 0 && (
                    <div className="gaps-section">
                      <h4>Coverage Gaps</h4>
                      {optimizationResult.min_satellites.gaps.map((gap, index) => (
                        <div key={index} className="gap-item">
                          {gap.start}h - {gap.end}h (Duration: {gap.end - gap.start}h)
                        </div>
                      ))}
                    </div>
                  )}
                </div>
              )}
            </div>
          </div>
        )}
      </div>
    </div>
  );
};

export default App;