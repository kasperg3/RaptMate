import React from 'react';
import { Typography, Box } from '@mui/material';

const Overview = ({ data }) => {
  return (
    <Box>
      <Typography variant="h5">Overview</Typography>
      {data ? (
        <>
          <Box className="data-item">Gravity Velocity: {data.gravity_velocity}</Box>
          <Box className="data-item">Temperature (Celsius): {data.temperature_celsius}</Box>
          <Box className="data-item">Specific Gravity: {data.specific_gravity}</Box>
          <Box className="data-item">Acceleration X: {data.accel_x}</Box>
          <Box className="data-item">Acceleration Y: {data.accel_y}</Box>
          <Box className="data-item">Acceleration Z: {data.accel_z}</Box>
          <Box className="data-item">Battery: {data.battery}</Box>
        </>
      ) : (
        <Typography>Loading...</Typography>
      )}
    </Box>
  );
};

export default Overview;