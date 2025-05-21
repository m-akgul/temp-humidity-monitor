#include <WiFi.h>
#include <WebServer.h>
#include "DHT.h"

#define DHTPIN 4       // GPIO pin where the DHT11 is connected
#define DHTTYPE DHT11  // DHT11 sensor

DHT dht(DHTPIN, DHTTYPE);

// Replace with your network credentials
const char* ssid = "your-ssid";
const char* password = "your-password";

// Set up a web server on port 80
WebServer server(80);

void handleRoot() {
  String message = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>DHT11 Sensor Dashboard</title>
      <style>
        :root {
          --bg: #f0f0f0;
          --text: #222;
          --card: #fff;
        }
        body.dark {
          --bg: #1c1c1c;
          --text: #f0f0f0;
          --card: #2c2c2c;
        }
        body {
          background-color: var(--bg);
          color: var(--text);
          font-family: Arial, sans-serif;
          margin: 0;
          padding: 0;
          display: flex;
          flex-direction: column;
          align-items: center;
          justify-content: center;
          min-height: 100vh;
        }
        .card {
          background-color: var(--card);
          padding: 30px;
          border-radius: 15px;
          box-shadow: 0 8px 20px rgba(0, 0, 0, 0.2);
          text-align: center;
          max-width: 600px;
          width: 100%;
        }
        h1 {
          margin-bottom: 20px;
        }
        .reading {
          font-size: 1.5em;
          margin: 10px 0;
        }
        .label {
          font-weight: bold;
        }
        #status {
          color: red;
          font-weight: bold;
        }
        #last-updated {
          font-size: 0.9em;
          margin-top: 10px;
        }
        .toggle-btn {
          margin-top: 15px;
          padding: 8px 15px;
          background-color: #007bff;
          color: white;
          border: none;
          border-radius: 8px;
          cursor: pointer;
        }
        .chart-container {
          width: 100%;
          height: 50vh;
          max-height: 50vh;
          padding: 10px;
          box-sizing: border-box;
          position: relative;
        }
        .chart-container canvas {
          position: absolute;
          top: 0;
          left: 0;
          width: 100% !important;
          height: 100% !important;
        }
      </style>
    </head>
    <body>
      <div class="card">
        <h1>🌡️ DHT11 Sensor Dashboard</h1>
        <div class="reading"><span class="label">Temperature:</span> <span id="temperature">--</span> &deg;C</div>
        <div class="reading"><span class="label">Humidity:</span> <span id="humidity">--</span> %</div>
        <div id="status"></div>
        <div id="last-updated">Loading...</div>
        <h3>🌡️ Temperature Over Time</h3>
        <div class="chart-container">
          <canvas id="tempChart"></canvas>
        </div>
        <h3>💧 Humidity Over Time</h3>
        <div class="chart-container">
          <canvas id="humChart"></canvas>
        </div>
        <button class="toggle-btn" onclick="toggleTheme()">Toggle Light/Dark Mode</button>
        <button class="toggle-btn" onclick="downloadCSV()">📥 Download CSV</button>
      </div>

      <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
      <script>
        let tempData = [];
        let humData = [];
        let labels = [];
        let csvData = [];

        const tempCtx = document.getElementById("tempChart").getContext("2d");
        const humCtx = document.getElementById("humChart").getContext("2d");

        const tempChart = new Chart(tempCtx, {
          type: "line",
          data: {
            labels: labels,
            datasets: [{
              label: "Temperature (°C)",
              data: tempData,
              borderColor: "tomato",
              backgroundColor: "rgba(255,99,71,0.2)",
              tension: 0.3,
              fill: true
            }]
          },
          options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
              y: { beginAtZero: false }
            }
          }
        });

        const humChart = new Chart(humCtx, {
          type: "line",
          data: {
            labels: labels,
            datasets: [{
              label: "Humidity (%)",
              data: humData,
              borderColor: "dodgerblue",
              backgroundColor: "rgba(30,144,255,0.2)",
              tension: 0.3,
              fill: true
            }]
          },
          options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
              y: { beginAtZero: false }
            }
          }
        });

        function updateData() {
          fetch('/data')
            .then(res => res.json())
            .then(data => {
              const now = new Date();
              const timeStr = now.toLocaleTimeString();
              if (isNaN(data.temperature) || isNaN(data.humidity)) {
                document.getElementById('status').innerText = "⚠️ Sensor read error!";
                return;
              } else {
                document.getElementById('status').innerText = "";
              }

              document.getElementById('temperature').innerText = data.temperature.toFixed(2);
              document.getElementById('humidity').innerText = data.humidity.toFixed(2);
              document.getElementById('last-updated').innerText = `Last updated: ${timeStr}`;

              // Limit chart data to 10 entries
              if (labels.length >= 10) {
                labels.shift();
                tempData.shift();
                humData.shift();
              }

              labels.push(timeStr);
              tempData.push(data.temperature);
              humData.push(data.humidity);
              tempChart.update();
              humChart.update();

              csvData.push({
                time: timeStr,
                temperature: data.temperature.toFixed(2),
                humidity: data.humidity.toFixed(2)
              });
            });
        }

        function downloadCSV() {
          if (csvData.length === 0) return alert("No data yet!");

          let csv = "Time,Temperature (°C),Humidity (%)\n";
          csvData.forEach(row => {
            csv += `${row.time},${row.temperature},${row.humidity}\n`;
          });

          const blob = new Blob([csv], { type: "text/csv" });
          const url = URL.createObjectURL(blob);
          const a = document.createElement("a");
          a.href = url;
          a.download = "sensor_data.csv";
          a.click();
          URL.revokeObjectURL(url);
        }

        function toggleTheme() {
          document.body.classList.toggle("dark");
        }

        setInterval(updateData, 2000);
        updateData();
      </script>
    </body>
    </html>
  )rawliteral";

  server.send(200, "text/html", message);
}

void handleData() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  String json = "{\"temperature\":";
  json += isnan(temperature) ? "null" : String(temperature, 2);  // Format to two decimal places
  json += ", \"humidity\":";
  json += isnan(humidity) ? "null" : String(humidity, 2);     // Format to two decimal places
  json += "}";

  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Connect to Wi-Fi network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Start the web server
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  server.handleClient();
}
