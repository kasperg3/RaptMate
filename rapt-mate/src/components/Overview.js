import React from 'react';
import { Typography, Box} from '@mui/material';
import { LineChart } from '@mui/x-charts/LineChart';

import { useState } from 'react';
import 'chart.js/auto';
const Overview = ({ data }) => {
    const [selectedMetrics, setSelectedMetrics] = useState({
        gravity_velocity: true,
        temperature_celsius: true,
        specific_gravity: true,
        accel_x: true,
        accel_y: true,
        accel_z: true,
        battery: true,
    });

    return (
        <Box>
            <Typography variant="h5">Overview</Typography>
            {data && data.timestamps && data.gravity_velocity && data.temperature_celsius && data.specific_gravity && data.accel_x && data.accel_y && data.accel_z && data.battery ? (
                <>
                    <LineChart
                        xAxis={[{ data: data.timestamps }]}
                        series={[
                            {
                                label: 'Gravity Velocity',
                                data: data.gravity_velocity,
                                borderColor: 'rgb(9, 12, 12)',
                                hidden: !selectedMetrics.gravity_velocity,
                            },
                            {
                                label: 'Temperature (Celsius)',
                                data: data.temperature_celsius,
                                borderColor: 'rgba(255,99,132,1)',
                                hidden: !selectedMetrics.temperature_celsius,
                            },
                            {
                                label: 'Specific Gravity',
                                data: data.specific_gravity,
                                borderColor: 'rgba(54,162,235,1)',
                                hidden: !selectedMetrics.specific_gravity,
                            },
                            {
                                label: 'Acceleration X',
                                data: data.accel_x,
                                borderColor: 'rgba(255,206,86,1)',
                                hidden: !selectedMetrics.accel_x,
                            },
                            {
                                label: 'Acceleration Y',
                                data: data.accel_y,
                                borderColor: 'rgba(75,192,192,1)',
                                hidden: !selectedMetrics.accel_y,
                            },
                            {
                                label: 'Acceleration Z',
                                data: data.accel_z,
                                borderColor: 'rgba(153,102,255,1)',
                                hidden: !selectedMetrics.accel_z,
                            },
                            {
                                label: 'Battery',
                                data: data.battery,
                                borderColor: 'rgba(255,159,64,1)',
                                hidden: !selectedMetrics.battery,
                            },
                        ]}
                        width={500}
                        height={300}
                    />
                </>
            ) : (
                <Typography>Loading...</Typography>
            )}
        </Box>
    );
};

export default Overview;