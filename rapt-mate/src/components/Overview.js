import React from 'react';
import { Typography, Box, Table, TableBody, TableCell, TableContainer, TableHead, TableRow, Paper } from '@mui/material';

const Overview = ({ data }) => {
    return (
        <Box>
            <Typography variant="h5">Overview</Typography>
            <Box mt={4}>
                <Typography variant="h6">Sensor Data Table</Typography>
                <TableContainer component={Paper}>
                    <Table>
                        <TableHead>
                            <TableRow>
                                <TableCell>Metric</TableCell>
                                <TableCell align="right">Value</TableCell>
                            </TableRow>
                        </TableHead>
                        <TableBody>
                            <TableRow>
                                <TableCell>Gravity Velocity</TableCell>
                                <TableCell align="right">{data?.gravity_velocity || 0}</TableCell>
                            </TableRow>
                            <TableRow>
                                <TableCell>Temperature (Celsius)</TableCell>
                                <TableCell align="right">{data?.temperature_celsius || 0}</TableCell>
                            </TableRow>
                            <TableRow>
                                <TableCell>Specific Gravity</TableCell>
                                <TableCell align="right">{data?.specific_gravity || 0}</TableCell>
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
                                    {data?.timestamp 
                                        ? new Date(data.timestamp * 1000).toLocaleString() 
                                        : 'N/A'}
                                </TableCell>
                            </TableRow>
                        </TableBody>
                    </Table>
                </TableContainer>
            </Box>
        </Box>
    );
};

export default Overview;
