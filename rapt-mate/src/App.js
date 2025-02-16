import React, { useState, useEffect } from 'react';
import { AppBar, Toolbar, Typography, Tabs, Tab, Box, TextField, Button, Container, Paper } from '@mui/material';
import Overview from './components/Overview'; // Adjust path as needed
import './App.css';

function App() {
  const [data, setData] = useState(null);
  const [ssid, setSsid] = useState('');
  const [password, setPassword] = useState('');
  const [tabIndex, setTabIndex] = useState(0);

  useEffect(() => {
    const fetchData = () => {
      fetch('/data')
        .then(response => response.json())
        .then(data => setData(data));
    };

    // Fetch data every second
    const interval = setInterval(fetchData, 1000);
    fetchData(); // initial fetch

    // Cleanup interval on component unmount
    return () => clearInterval(interval);
  }, []);

  const handleTabChange = (event, newValue) => {
    setTabIndex(newValue);
  };

  return (
    <div className="App">
      <AppBar position="static">
        <Toolbar>
          <Typography variant="h6">RaptMate</Typography>
        </Toolbar>
      </AppBar>
      <Tabs value={tabIndex} onChange={handleTabChange} centered>
        <Tab label="Overview" />
        <Tab label="Settings" />
      </Tabs>
      <Container>
        {tabIndex === 0 && (
          <Paper elevation={3} className="paper">
            <Overview data={data} />
          </Paper>
        )}
        {tabIndex === 1 && (
          <Paper elevation={3} className="paper">
            <Box id="config">
              <Typography variant="h5">Configuration</Typography>
              <TextField
                label="SSID"
                variant="outlined"
                fullWidth
                margin="normal"
                value={ssid}
                onChange={(e) => setSsid(e.target.value)}
              />
              <TextField
                label="Password"
                type="password"
                variant="outlined"
                fullWidth
                margin="normal"
                value={password}
                onChange={(e) => setPassword(e.target.value)}
              />
              <Button variant="contained" color="primary">Save</Button>
            </Box>
          </Paper>
        )}
      </Container>
    </div>
  );
}

export default App;
