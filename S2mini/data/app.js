let dataPoints = [];
let startTime = Date.now();
let soundVelocity = 343;
let timeWindow = 5;
let currentSteering = 0; // Track current steering for gauge
let currentSpeed = 0; // Track current speed for bar

const MAX_ECHO_TIME = 36000; // 36ms max distance
const controlCanvas = document.getElementById('controlChart');
const controlCtx = controlCanvas.getContext('2d');
const sonarCanvas = document.getElementById('sonarChart');
const sonarCtx = sonarCanvas.getContext('2d');
const steeringGauge = document.getElementById('steeringGauge');
const steeringCtx = steeringGauge.getContext('2d');

// Settings
document.getElementById('soundVelocity').addEventListener('change', (e) => {
  soundVelocity = parseFloat(e.target.value);
});

document.getElementById('timeWindow').addEventListener('change', (e) => {
  timeWindow = parseInt(e.target.value);
});

// Map functions
function mapSteering(val) {
  return ((val / 255) * 120) - 60; // 0-255 -> -60 to 60
}

function mapThrottle(val) {
  return ((val / 255) * 200) - 100; // 0-255 -> -100 to 100
}

function mapBrake(val) {
  return (val / 255) * 100; // 0-255 -> 0 to 100
}

function mapPWM(val) {
  return (val / 255) * 100; // 0-255 -> 0 to 100
}

function updateData() {
  fetch('/api/data')
    .then(r => r.json())
    .then(d => {
      const now = Date.now();
      const elapsedTime = (now - startTime) / 1000;
      
      // Map values
      const steeringDeg = mapSteering(d.st);
      const throttleUser = mapThrottle(d.ut);
      const throttleTrue = mapThrottle(d.tt);
      const brakePercent = mapBrake(d.br);
      const pwm1Percent = mapPWM(d.p1);
      const pwm2Percent = mapPWM(d.p2);
      
      // Store current steering for gauge
      currentSteering = steeringDeg;
      currentSpeed = d.sp; // Store current speed for bar
      
      // Check if out of range
      const isOutOfRange = d.di > MAX_ECHO_TIME;
      const distanceDisplay = isOutOfRange ? 'Unknown' : 
        (d.di * soundVelocity / 2 / 1000000 * 100).toFixed(1) + 'cm';
      
      // Update sonar info display
      const distanceValue = (d.di * soundVelocity / 2 / 1000000 * 100).toFixed(1);
      document.getElementById('sonarInfo').textContent = 
        `${d.di} us - ${distanceValue} cm`;
      
      // Store data point
      dataPoints.push({
        time: elapsedTime,
        echoTime: Math.min(d.di, MAX_ECHO_TIME),
        distance: d.di * soundVelocity / 2 / 1000000 * 100,
        isOutOfRange: isOutOfRange,
        throttleUser: throttleUser,
        throttleTrue: throttleTrue,
        brake: brakePercent
      });
      
      // Remove old data points
      dataPoints = dataPoints.filter(p => p.time >= elapsedTime - timeWindow);
      
      // Update telemetry
      document.getElementById('data').textContent = 
        `ST: ${steeringDeg.toFixed(1)}deg | UT: ${throttleUser.toFixed(0)}% | TT: ${throttleTrue.toFixed(0)}% | BR: ${brakePercent.toFixed(0)}% | SP: ${d.sp}km/h | P1: ${pwm1Percent.toFixed(0)}% | P2: ${pwm2Percent.toFixed(0)}% | DI: ${d.di}us (${distanceDisplay})`;
      
      // Draw charts
      drawSteeringGauge();
      drawSpeedBar();
      drawControlChart();
      drawSonarChart();
    })
    .catch(e => console.error(e));
}

// Steering Gauge (horizontal bar)
function drawSteeringGauge() {
  const width = steeringGauge.width;
  const height = steeringGauge.height;
  const barY = height * 0.2 + 25; // Offset by speed bar height
  const barX1 = 40;
  const barX2 = width - 40;
  const barHeight = 30;
  
  // Clear canvas
  steeringCtx.fillStyle = '#fff';
  steeringCtx.fillRect(0, 0, width, height);
  
  // Draw background bar
  steeringCtx.fillStyle = '#e0e0e0';
  steeringCtx.fillRect(barX1, barY - barHeight / 2, barX2 - barX1, barHeight);
  
  // Draw scale ticks and labels
  steeringCtx.strokeStyle = '#333';
  steeringCtx.lineWidth = 2;
  steeringCtx.fillStyle = '#333';
  steeringCtx.font = 'bold 11px Arial';
  steeringCtx.textAlign = 'center';
  steeringCtx.textBaseline = 'bottom';
  
  const ticks = [-60, -30, 0, 30, 60];
  for (let tick of ticks) {
    const fraction = (tick + 60) / 120; // 0 to 1
    const x = barX1 + fraction * (barX2 - barX1);
    
    // Draw tick
    steeringCtx.beginPath();
    steeringCtx.moveTo(x, barY - barHeight / 2 - 5);
    steeringCtx.lineTo(x, barY - barHeight / 2 - 15);
    steeringCtx.stroke();
    
    // Draw label (moved up by character height)
    steeringCtx.fillText(tick, x, barY - barHeight / 2 - 27);
  }
  
  // Draw center line (0°)
  steeringCtx.strokeStyle = '#999';
  steeringCtx.lineWidth = 2;
  steeringCtx.setLineDash([3, 3]);
  steeringCtx.beginPath();
  const centerX = barX1 + 0.5 * (barX2 - barX1);
  steeringCtx.moveTo(centerX, barY - barHeight / 2 - 5);
  steeringCtx.lineTo(centerX, barY + barHeight / 2 + 5);
  steeringCtx.stroke();
  steeringCtx.setLineDash([]);
  
  // Draw indicator (red vertical line)
  const fraction = (currentSteering + 60) / 120;
  const indicatorX = barX1 + fraction * (barX2 - barX1);
  
  steeringCtx.strokeStyle = '#ff0000';
  steeringCtx.lineWidth = 4;
  steeringCtx.beginPath();
  steeringCtx.moveTo(indicatorX, barY - barHeight / 2 - 10);
  steeringCtx.lineTo(indicatorX, barY + barHeight / 2 + 10);
  steeringCtx.stroke();
  
  // Draw value text (above the bar)
  steeringCtx.fillStyle = '#ff0000';
  steeringCtx.font = 'bold 18px Arial';
  steeringCtx.textAlign = 'center';
  steeringCtx.textBaseline = 'top';
  steeringCtx.fillText(currentSteering.toFixed(1) + '°', indicatorX, barY + barHeight / 2 + 5);
}

// Speed Bar (horizontal fill bar)
function drawSpeedBar() {
  const width = steeringGauge.width;
  const height = steeringGauge.height;
  const barY = height * 0.6 + 25; // Offset by speed bar height
  const barX1 = 40;
  const barX2 = width - 40;
  const barHeight = 25;
  const maxSpeed = 150; // Max speed 150 km/h
  
  // Draw background bar (light gray)
  steeringCtx.fillStyle = '#e0e0e0';
  steeringCtx.fillRect(barX1, barY - barHeight / 2, barX2 - barX1, barHeight);
  
  // Draw filled bar (green fill from left to right)
  const fillFraction = Math.min(currentSpeed / maxSpeed, 1);
  const fillWidth = fillFraction * (barX2 - barX1);
  steeringCtx.fillStyle = '#4CAF50';
  steeringCtx.fillRect(barX1, barY - barHeight / 2, fillWidth, barHeight);
  
  // Draw border
  steeringCtx.strokeStyle = '#333';
  steeringCtx.lineWidth = 2;
  steeringCtx.strokeRect(barX1, barY - barHeight / 2, barX2 - barX1, barHeight);
  
  // Draw scale ticks and labels
  steeringCtx.strokeStyle = '#333';
  steeringCtx.lineWidth = 1;
  steeringCtx.fillStyle = '#333';
  steeringCtx.font = 'bold 10px Arial';
  steeringCtx.textAlign = 'center';
  steeringCtx.textBaseline = 'bottom';
  
  const speedTicks = [0, 30, 60, 90, 120, 150];
  for (let tick of speedTicks) {
    const fraction = tick / maxSpeed;
    const x = barX1 + fraction * (barX2 - barX1);
    
    // Draw tick
    steeringCtx.beginPath();
    steeringCtx.moveTo(x, barY - barHeight / 2 - 2);
    steeringCtx.lineTo(x, barY - barHeight / 2 - 8);
    steeringCtx.stroke();
    
    // Draw label
    steeringCtx.fillText(tick, x, barY - barHeight / 2 - 12);
  }
  
  // Draw value text
  steeringCtx.fillStyle = '#4CAF50';
  steeringCtx.font = 'bold 16px Arial';
  steeringCtx.textAlign = 'left';
  steeringCtx.textBaseline = 'bottom';
  steeringCtx.fillText('Speed: ' + currentSpeed.toFixed(0) + ' km/h', barX1, barY + barHeight / 2 + 20);
}
function drawControlChart() {
  const width = controlCanvas.width;
  const height = controlCanvas.height;
  const padding = 50;
  const chartWidth = width - 2 * padding;
  const chartHeight = height - 2 * padding;
  
  // Clear canvas
  controlCtx.fillStyle = '#fff';
  controlCtx.fillRect(0, 0, width, height);
  
  // Draw grid
  controlCtx.strokeStyle = '#e0e0e0';
  controlCtx.lineWidth = 1;
  for (let i = 0; i <= 5; i++) {
    const y = padding + (chartHeight / 5) * i;
    controlCtx.beginPath();
    controlCtx.moveTo(padding, y);
    controlCtx.lineTo(width - padding, y);
    controlCtx.stroke();
  }
  
  // Axis labels
  controlCtx.fillStyle = '#333';
  controlCtx.font = '12px Arial';
  controlCtx.textAlign = 'center';
  
  // Time axis (X)
  controlCtx.fillText('Time (s)', width / 2, height - 10);
  for (let i = 0; i <= timeWindow; i++) {
    const x = padding + (chartWidth / timeWindow) * i;
    controlCtx.fillText(i, x, height - padding + 20);
  }
  
  // Y axis (throttle/brake) - left: -100 to 100
  controlCtx.textAlign = 'left';
  controlCtx.fillText('Throttle (%)', 5, 20);
  for (let i = 0; i <= 5; i++) {
    const val = 100 - (i * 40); // 100, 60, 20, -20, -60, -100
    const y = padding + (chartHeight / 5) * i;
    controlCtx.fillText(val, padding - 40, y + 5);
  }
  
  // Y axis (brake) - right: 0 to 100
  controlCtx.textAlign = 'right';
  controlCtx.fillText('Brake (%)', width - 5, 20);
  for (let i = 0; i <= 5; i++) {
    const val = 100 - (i * 20); // 100, 80, 60, 40, 20, 0
    const y = padding + (chartHeight / 5) * i;
    controlCtx.fillText(val, width - padding + 20, y + 5);
  }
  
  // Draw axes
  controlCtx.strokeStyle = '#000';
  controlCtx.lineWidth = 2;
  controlCtx.beginPath();
  controlCtx.moveTo(padding, padding);
  controlCtx.lineTo(padding, height - padding);
  controlCtx.lineTo(width - padding, height - padding);
  controlCtx.stroke();
  
  // Draw center line (0% throttle)
  controlCtx.strokeStyle = '#ccc';
  controlCtx.lineWidth = 1;
  controlCtx.setLineDash([5, 5]);
  const zeroY = height - padding - (100 / 200) * chartHeight; // middle of -100 to 100
  controlCtx.beginPath();
  controlCtx.moveTo(padding, zeroY);
  controlCtx.lineTo(width - padding, zeroY);
  controlCtx.stroke();
  controlCtx.setLineDash([]);
  
  // Draw lines
  if (dataPoints.length > 0) {
    const minTime = timeWindow > 0 ? Math.max(...dataPoints.map(p => p.time)) - timeWindow : 0;
    
    // Throttle user line (blue)
    controlCtx.strokeStyle = '#0066cc';
    controlCtx.lineWidth = 2;
    controlCtx.beginPath();
    let first = true;
    for (let p of dataPoints) {
      const x = padding + ((p.time - minTime) / timeWindow) * chartWidth;
      const y = height - padding - ((p.throttleUser + 100) / 200) * chartHeight;
      if (first) {
        controlCtx.moveTo(x, y);
        first = false;
      } else {
        controlCtx.lineTo(x, y);
      }
    }
    controlCtx.stroke();
    
    // Throttle true line (cyan, dashed)
    controlCtx.strokeStyle = '#00bfff';
    controlCtx.lineWidth = 2;
    controlCtx.setLineDash([5, 5]);
    controlCtx.beginPath();
    first = true;
    for (let p of dataPoints) {
      const x = padding + ((p.time - minTime) / timeWindow) * chartWidth;
      const y = height - padding - ((p.throttleTrue + 100) / 200) * chartHeight;
      if (first) {
        controlCtx.moveTo(x, y);
        first = false;
      } else {
        controlCtx.lineTo(x, y);
      }
    }
    controlCtx.stroke();
    controlCtx.setLineDash([]);
    
    // Brake line (red)
    controlCtx.strokeStyle = '#ff6b6b';
    controlCtx.lineWidth = 2;
    controlCtx.beginPath();
    first = true;
    for (let p of dataPoints) {
      const x = padding + ((p.time - minTime) / timeWindow) * chartWidth;
      const y = height - padding - (p.brake / 100) * chartHeight;
      if (first) {
        controlCtx.moveTo(x, y);
        first = false;
      } else {
        controlCtx.lineTo(x, y);
      }
    }
    controlCtx.stroke();
  }
  
  // Legend
  controlCtx.fillStyle = '#0066cc';
  controlCtx.fillRect(width - 200, 30, 15, 15);
  controlCtx.fillStyle = '#000';
  controlCtx.textAlign = 'left';
  controlCtx.font = 'bold 12px Arial';
  controlCtx.fillText('User Throttle', width - 175, 42);
  
  controlCtx.fillStyle = '#00bfff';
  controlCtx.fillRect(width - 200, 50, 15, 15);
  controlCtx.fillStyle = '#000';
  controlCtx.fillText('True Throttle', width - 175, 62);
  
  controlCtx.fillStyle = '#ff6b6b';
  controlCtx.fillRect(width - 200, 70, 15, 15);
  controlCtx.fillStyle = '#000';
  controlCtx.fillText('Brake', width - 175, 82);
}

// Sonar Chart (Echo Time + Distance)
function drawSonarChart() {
  const width = sonarCanvas.width;
  const height = sonarCanvas.height;
  const padding = 50;
  const chartWidth = width - 2 * padding;
  const chartHeight = height - 2 * padding;
  
  // Clear canvas
  sonarCtx.fillStyle = '#fff';
  sonarCtx.fillRect(0, 0, width, height);
  
  // Draw grid
  sonarCtx.strokeStyle = '#e0e0e0';
  sonarCtx.lineWidth = 1;
  for (let i = 0; i <= 5; i++) {
    const y = padding + (chartHeight / 5) * i;
    sonarCtx.beginPath();
    sonarCtx.moveTo(padding, y);
    sonarCtx.lineTo(width - padding, y);
    sonarCtx.stroke();
  }
  
  // Axis labels
  sonarCtx.fillStyle = '#333';
  sonarCtx.font = '12px Arial';
  sonarCtx.textAlign = 'center';
  
  // Time axis (X)
  sonarCtx.fillText('Time (s)', width / 2, height - 10);
  for (let i = 0; i <= timeWindow; i++) {
    const x = padding + (chartWidth / timeWindow) * i;
    sonarCtx.fillText(i, x, height - padding + 20);
  }
  
  // Echo time axis (Y left) - FIXED at 36ms max
  sonarCtx.textAlign = 'left';
  sonarCtx.fillText('Echo Time (us)', 5, 20);
  const maxEchoTime = MAX_ECHO_TIME;
  for (let i = 0; i <= 5; i++) {
    const val = Math.round(maxEchoTime / 5 * (5 - i));
    const y = padding + (chartHeight / 5) * i;
    sonarCtx.fillText(val, padding - 40, y + 5);
  }
  
  // Distance axis (Y right)
  sonarCtx.textAlign = 'right';
  sonarCtx.fillText('Distance (cm)', width - 5, 20);
  const maxDistance = (maxEchoTime * soundVelocity / 2 / 1000000 * 100);
  for (let i = 0; i <= 5; i++) {
    const val = (maxDistance / 5 * (5 - i)).toFixed(0);
    const y = padding + (chartHeight / 5) * i;
    sonarCtx.fillText(val, width - padding + 30, y + 5);
  }
  
  // Draw axes
  sonarCtx.strokeStyle = '#000';
  sonarCtx.lineWidth = 2;
  sonarCtx.beginPath();
  sonarCtx.moveTo(padding, padding);
  sonarCtx.lineTo(padding, height - padding);
  sonarCtx.lineTo(width - padding, height - padding);
  sonarCtx.stroke();
  
  // Draw lines
  if (dataPoints.length > 0) {
    const minTime = timeWindow > 0 ? Math.max(...dataPoints.map(p => p.time)) - timeWindow : 0;
    const maxEchoTimeFixed = MAX_ECHO_TIME;
    const maxDistanceFixed = (maxEchoTimeFixed * soundVelocity / 2 / 1000000 * 100);
    
    // Echo time line
    sonarCtx.strokeStyle = '#0066cc';
    sonarCtx.lineWidth = 2;
    sonarCtx.beginPath();
    let first = true;
    for (let p of dataPoints) {
      const x = padding + ((p.time - minTime) / timeWindow) * chartWidth;
      const y = height - padding - (p.echoTime / maxEchoTimeFixed) * chartHeight;
      if (first) {
        sonarCtx.moveTo(x, y);
        first = false;
      } else {
        sonarCtx.lineTo(x, y);
      }
      
      // Mark out-of-range points with red dot
      if (p.isOutOfRange) {
        sonarCtx.fillStyle = '#ff0000';
        sonarCtx.beginPath();
        sonarCtx.arc(x, y, 3, 0, 2 * Math.PI);
        sonarCtx.fill();
      }
    }
    sonarCtx.stroke();
    
    // Distance line
    sonarCtx.strokeStyle = '#ff6b6b';
    sonarCtx.lineWidth = 2;
    sonarCtx.beginPath();
    first = true;
    for (let p of dataPoints) {
      const x = padding + ((p.time - minTime) / timeWindow) * chartWidth;
      const cappedDist = Math.min(p.distance, maxDistanceFixed);
      const y = height - padding - (cappedDist / maxDistanceFixed) * chartHeight;
      if (first) {
        sonarCtx.moveTo(x, y);
        first = false;
      } else {
        sonarCtx.lineTo(x, y);
      }
    }
    sonarCtx.stroke();
  }
  
  // Legend
  sonarCtx.fillStyle = '#0066cc';
  sonarCtx.fillRect(width - 180, 30, 15, 15);
  sonarCtx.fillStyle = '#000';
  sonarCtx.textAlign = 'left';
  sonarCtx.font = 'bold 12px Arial';
  sonarCtx.fillText('Echo Time', width - 155, 42);
  
  sonarCtx.fillStyle = '#ff6b6b';
  sonarCtx.fillRect(width - 180, 50, 15, 15);
  sonarCtx.fillStyle = '#000';
  sonarCtx.fillText('Distance', width - 155, 62);
}

setInterval(updateData, 100);
updateData();
