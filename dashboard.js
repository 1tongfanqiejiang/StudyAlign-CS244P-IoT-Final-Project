async function fetchData() {
    try {
        const response = await fetch("/telemetry");
        const json = await response.json();
        updateCharts(json.data, json.logs, json.last_calibration);
    } catch (e) {
        console.error("Fetch failed:", e);
    }
}

setInterval(fetchData, 1000);

const angleCtx = document.getElementById("angleChart").getContext("2d");
const lightCtx = document.getElementById("lightChart").getContext("2d");

let angleChart = new Chart(angleCtx, {
    type: "line",
    data: {
        labels: [],
        datasets: [
            { label: "Tilt Angle (°)", borderColor: "blue", data: [] },
            { label: "Calibration", borderColor: "green", pointRadius: 6, showLine: false, data: [] },
            { label: "Buzzer", borderColor: "red", pointRadius: 6, showLine: false, data: [] }
        ]
    },
    options: { responsive: true }
});

let lightChart = new Chart(lightCtx, {
    type: "line",
    data: {
        labels: [],
        datasets: [
            { label: "Light Level", borderColor: "orange", data: [] },
            { label: "Calibration", borderColor: "green", pointRadius: 6, showLine: false, data: [] },
            { label: "Buzzer", borderColor: "red", pointRadius: 6, showLine: false, data: [] }
        ]
    },
    options: { responsive: true }
});

function updateCharts(data, logs, calibrationTime) {
    if (!data || data.length === 0) return;

    let labels = data.map(d => new Date(d.timestamp * 1000).toLocaleTimeString());

    let angles = data.map(d => d.tilt_angle ?? 0);
    let lights = data.map(d => d.light ?? 0);

    let calAngleMarkers = data.map(d => d.calibration === 1 ? d.tilt_angle : null);
    let buzAngleMarkers = data.map(d => d.buzzer === 1 ? d.tilt_angle : null);

    let calLightMarkers = data.map(d => d.calibration === 1 ? d.light : null);
    let buzLightMarkers = data.map(d => d.buzzer === 1 ? d.light : null);


    angleChart.data.labels = labels;
    angleChart.data.datasets[0].data = angles;
    angleChart.data.datasets[1].data = calAngleMarkers;
    angleChart.data.datasets[2].data = buzAngleMarkers;
    angleChart.update();

    lightChart.data.labels = labels;
    lightChart.data.datasets[0].data = lights;
    lightChart.data.datasets[1].data = calLightMarkers;
    lightChart.data.datasets[2].data = buzLightMarkers;
    lightChart.update();

    const logBox = document.getElementById("logBox");
    logBox.innerHTML = logs.map(l => `<div>${l}</div>`).join("");
}

