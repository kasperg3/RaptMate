import React, { useState, useEffect } from 'react';
import {
    Typography,
    Box,
    Table,
    TableBody,
    TableCell,
    TableContainer,
    TableRow,
    AppBar,
    Toolbar,
    Tabs,
    Tab,
    TextField,
    Button,
    Container,
    Paper
} from '@mui/material';

import { LineChart } from '@mui/x-charts/LineChart';

import './App.css';

function App() {
    const [data, setData] = useState(null);
    const [ssid, setSsid] = useState('');
    const [password, setPassword] = useState('');
    const [tabIndex, setTabIndex] = useState(0);
    const [chartData, setChartData] = useState({
        labels: [],
        gravity: [],
        temperature: [],
        battery: []
    });
    useEffect(() => {
        const fetchData = () => {
            fetch('/data', { headers: { 'Accept': 'text/csv' } })
                .then(response => response.text())
                .then(csvText => {
                    const rows = csvText.split('\n').filter(row => row.trim() !== '');
                    const newDataList = rows.slice(1).map(row => {
                        const [
                            timestamp,
                            gravity_velocity,
                            temperature_celsius,
                            specific_gravity,
                            accel_x,
                            accel_y,
                            accel_z,
                            battery
                        ] = row.split(',');
                        return {
                            timestamp: parseInt(timestamp, 10),
                            gravity_velocity: parseFloat(gravity_velocity),
                            temperature_celsius: parseFloat(temperature_celsius),
                            specific_gravity: parseFloat(specific_gravity),
                            accel_x: parseFloat(accel_x),
                            accel_y: parseFloat(accel_y),
                            accel_z: parseFloat(accel_z),
                            battery: parseFloat(battery),
                        };
                    });
                    setData(newDataList[newDataList.length - 1]); // Set the latest data point for display
                    setChartData({
                        labels: newDataList.map(newData => new Date(newData.timestamp * 1000)),
                        gravity: newDataList.map(newData => newData.specific_gravity / 1000),
                        temperature: newDataList.map(newData => newData.temperature_celsius),
                        battery: newDataList.map(newData => newData.battery),
                    });
                });
        };
        const interval = setInterval(fetchData, 10000);
        fetchData();
        return () => clearInterval(interval);
    }, []);


    const handleTabChange = (event, newValue) => {
        setTabIndex(newValue);
    };

    return (
        <div className="App">
            <AppBar position="static" sx={{ mb: 2 }}>
                <Toolbar>
                    <Typography variant="h6" component="div" sx={{ flexGrow: 1 }}>
                        RaptMate
                    </Typography>
                </Toolbar>
            </AppBar>
            <Tabs
                value={tabIndex}
                onChange={handleTabChange}
                centered
                textColor="primary"
                indicatorColor="primary"
                sx={{ mb: 3 }}
            >
                <Tab label="Overview" />
                <Tab label="Settings" />
            </Tabs>
            <Container maxWidth="lg">
                {tabIndex === 0 && (
                <Paper elevation={4} sx={{ p: 3, mb: 4 }}>
                    <Box display="flex" justifyContent="space-between" gap={2}>
                        <Paper elevation={2} sx={{ p: 2, flex: '1 1 30%', textAlign: 'center' }}>
                            <Typography variant="subtitle1">Specific Gravity</Typography>
                            <Typography variant="h6">{data?.specific_gravity / 1000 || 0}</Typography>
                        </Paper>
                        <Paper elevation={2} sx={{ p: 2, flex: '1 1 30%', textAlign: 'center' }}>
                            <Typography variant="subtitle1">Temperature (°C)</Typography>
                            <Typography variant="h6">{data?.temperature_celsius || 0}</Typography>
                        </Paper>
                        <Paper elevation={2} sx={{ p: 2, flex: '1 1 30%', textAlign: 'center' }}>
                            <Typography variant="subtitle1">Battery (%)</Typography>
                            <Typography variant="h6">{data?.battery || 0}</Typography>
                        </Paper>
                    </Box>
                        <Box display="flex" flexDirection={{ xs: 'column', md: 'row' }} gap={3} alignItems="stretch">
                            <Box flex={2} display="flex" flexDirection="column">
                                <Typography variant="h6" gutterBottom>
                                    Sensor Data Chart
                                </Typography>
                                <Paper elevation={2} sx={{ p: 2, height: '100%' }}>
                                    <LineChart
                                        xAxis={[{ data: chartData.labels, scaleType: 'time' }]}
                                        yAxis={[
                                            {
                                                id: 'leftAxis',
                                                label: 'Specific Gravity',
                                                min: 0.9,
                                                position: 'left'
                                            },
                                            {
                                                id: 'rightAxis',
                                                label: 'Temp / Battery',
                                                min: 0,
                                                max: 100,
                                                position: 'right'
                                            },
                                        ]}
                                        series={[
                                            {
                                                data: chartData.gravity,
                                                label: 'Specific Gravity',
                                                yAxisId: 'leftAxis',
                                                valueFormatter: (value) => `${value} SG`,
                                            },
                                            {
                                                data: chartData.temperature,
                                                label: 'Temperature (°C)',
                                                yAxisId: 'rightAxis',
                                                valueFormatter: (value) => `${value} °C`,
                                            },
                                            {
                                                data: chartData.battery,
                                                label: 'Battery (%)',
                                                yAxisId: 'rightAxis',
                                                valueFormatter: (value) => `${value} %`,
                                            },
                                        ]}

                                        height={400}
                                        slotProps={{
                                            tooltip: {
                                                axis: {
                                                    x: 'x',
                                                    yLeft: 'leftAxis',
                                                    yRight: 'rightAxis',
                                                },
                                            },
                                        }}
                                        rightAxis="rightAxis" // Explicitly enable right axis
                                    />
                                </Paper>
                            </Box>
                            <Box flex={1} display="flex" flexDirection="column">
                                <Typography variant="h6" gutterBottom>
                                    Sensor Data Table
                                </Typography>
                                <TableContainer component={Paper} elevation={2} sx={{ flexGrow: 1 }}>
                                    <Table>
                                        <TableBody>
                                            <TableRow>
                                                <TableCell>Gravity Velocity</TableCell>
                                                <TableCell align="right">{data?.gravity_velocity || 0}</TableCell>
                                            </TableRow>
                                            <TableRow>
                                                <TableCell>Temperature (°C)</TableCell>
                                                <TableCell align="right">{data?.temperature_celsius || 0}</TableCell>
                                            </TableRow>
                                            <TableRow>
                                                <TableCell>Specific Gravity</TableCell>
                                                <TableCell align="right">{data?.specific_gravity / 1000 || 0}</TableCell>
                                            </TableRow>
                                            <TableRow>
                                                <TableCell>Acceleration X</TableCell>
                                                <TableCell align="right">{data?.accel_x || 0}</TableCell>
                                            </TableRow>
                                            <TableRow>
                                                <TableCell>Acceleration Y</TableCell>
                                                <TableCell align="right">{data?.accel_y || 0}</TableCell>
                                            </TableRow>
                                            <TableRow>
                                                <TableCell>Acceleration Z</TableCell>
                                                <TableCell align="right">{data?.accel_z || 0}</TableCell>
                                            </TableRow>
                                            <TableRow>
                                                <TableCell>Battery</TableCell>
                                                <TableCell align="right">{data?.battery || 0}</TableCell>
                                            </TableRow>
                                            <TableRow>
                                                <TableCell>Timestamp</TableCell>
                                                <TableCell align="right">
                                                    {data?.timestamp ? new Date(data.timestamp * 1000).toLocaleString() : 'N/A'}
                                                </TableCell>
                                            </TableRow>
                                        </TableBody>
                                    </Table>
                                </TableContainer>
                            </Box>
                        </Box>
                    </Paper>
                )}
                {tabIndex === 1 && (
                    <Box display="flex" flexDirection="column" gap={3}>
                        <Paper elevation={4} sx={{ p: 3 }}>
                            <Typography variant="h6" gutterBottom align="left">
                                Wi-Fi Configuration
                            </Typography>
                            <Box component="form" noValidate autoComplete="off" align="left">
                                <TextField
                                    label="ssid"
                                    variant="outlined"
                                    fullWidth
                                    margin="normal"
                                    value={ssid}
                                    onChange={(e) => setSsid(e.target.value)}
                                />
                                <TextField
                                    label="password"
                                    type="password"
                                    variant="outlined"
                                    fullWidth
                                    margin="normal"
                                    value={password}
                                    onChange={(e) => setPassword(e.target.value)}
                                />
                                <Button
                                    variant="contained"
                                    color="primary"
                                    onClick={() => {
                                        fetch('/settings', {
                                            method: 'POST',
                                            headers: {
                                                'Content-Type': 'application/json',
                                            },
                                            body: JSON.stringify({ ssid, password }),
                                        })
                                            .then(response => {
                                                if (response.ok) {
                                                    alert('Settings saved successfully');
                                                } else {
                                                    alert('Failed to save settings');
                                                }
                                            })
                                            .catch(() => alert('Error occurred while saving settings'));
                                    }}
                                >
                                    Save
                                </Button>
                            </Box>
                        </Paper>
                        <Paper elevation={4} sx={{ p: 3 }}>
                            <Typography variant="h6" gutterBottom align="left">
                                Data Management
                            </Typography>
                            <Box display="flex" justifyContent="flex-start">
                                <Button
                                    variant="contained"
                                    color="error"
                                    onClick={() => {
                                        if (window.confirm('Are you sure you want to reset all data? This cannot be undone.')) {
                                            fetch('/reset')
                                                .then(response => {
                                                    if (response.ok) {
                                                        alert('Data reset successfully');
                                                    } else {
                                                        alert('Failed to reset data');
                                                    }
                                                })
                                                .catch(() => alert('Error occurred while resetting data'));
                                        }
                                    }}
                                    sx={{ mt: 1 }}
                                >
                                    Reset All Data
                                </Button>
                            </Box>
                        </Paper>
                    </Box>
                )}
            </Container>
        </div>
    );
}

export default App;