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
            <Box>
                <LineChart
                    xAxis={[{ data: data?.timestamps?.length ? data.timestamps : new Array(data?.gravity_velocity?.length || 1).fill(0) }]}
                    series={[
                        {
                            label: 'Gravity Velocity',
                            data: data?.gravity_velocity?.length ? data.gravity_velocity : [0],
                            borderColor: 'rgb(9, 12, 12)',
                            hidden: !selectedMetrics.gravity_velocity,
                        },
                        {
                            label: 'Temperature (Celsius)',
                            data: data?.temperature_celsius?.length ? data.temperature_celsius : [0],
                            borderColor: 'rgba(255,99,132,1)',
                            hidden: !selectedMetrics.temperature_celsius,
                        },
                        {
                            label: 'Specific Gravity',
                            data: data?.specific_gravity?.length ? data.specific_gravity : [0],
                            borderColor: 'rgba(54,162,235,1)',
                            hidden: !selectedMetrics.specific_gravity,
                        },
                        {
                            label: 'Acceleration X',
                            data: data?.accel_x?.length ? data.accel_x : [0],
                            borderColor: 'rgba(255,206,86,1)',
                            hidden: !selectedMetrics.accel_x,
                        },
                        {
                            label: 'Acceleration Y',
                            data: data?.accel_y?.length ? data.accel_y : [0],
                            borderColor: 'rgba(75,192,192,1)',
                            hidden: !selectedMetrics.accel_y,
                        },
                        {
                            label: 'Acceleration Z',
                            data: data?.accel_z?.length ? data.accel_z : [0],
                            borderColor: 'rgba(153,102,255,1)',
                            hidden: !selectedMetrics.accel_z,
                        },
                        {
                            label: 'Battery',
                            data: data?.battery?.length ? data.battery : [0],
                            borderColor: 'rgba(255,159,64,1)',
                            hidden: !selectedMetrics.battery,
                        },
                    ]}
                />
            </Box>
        </Box>
    );
};

export default Overview;