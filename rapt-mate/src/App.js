// src/App.js
import React, { useState, useEffect } from 'react';
import './App.css';

function App() {
  const [data, setData] = useState(null);

  useEffect(() => {
    const fetchData = () => {
      fetch('/data')
        .then((response) => response.json())
        .then((json) => setData(json))
        .catch((err) => console.error('Error:', err));
    };

    // Fetch data every second
    const interval = setInterval(fetchData, 1000);
    fetchData(); // initial fetch

    // Cleanup interval on component unmount
    return () => clearInterval(interval);
  }, []);

  return (
    <div className="App">
      <h1>RaptMate BLE Data</h1>
      <div id="data">
        {data ? (
          <>
            <div className="data-item">Gravity Velocity: {data.gravity_velocity}</div>
            <div className="data-item">Temperature (Celsius): {data.temperature_celsius}</div>
            <div className="data-item">Specific Gravity: {data.specific_gravity}</div>
            <div className="data-item">Acceleration X: {data.accel_x}</div>
            <div className="data-item">Acceleration Y: {data.accel_y}</div>
            <div className="data-item">Acceleration Z: {data.accel_z}</div>
            <div className="data-item">Battery: {data.battery}</div>
          </>
        ) : (
          "Loading..."
        )}
      </div>
    </div>
  );
}

export default App;
