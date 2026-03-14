#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <AViShaESPCam.h>
#include <SPIFFS.h>

AViShaESPCam espcam;

//const char* ssid = "FiberHGW_ZTF7ZQ_2.4GHz";
//const char* password = "4Yax4c9zxKNH";
const char* ssid = "ONEPLUS_co_apxtmm";
const char* password = "xtmm9437";

WebServer server(80);

const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Hello OpenCV.js</title>
<style>
  body { font-family: sans-serif; }
  .row { display: flex; flex-direction: row; gap: 10px; flex-wrap: wrap; }
  .inputoutput { display: flex; flex-direction: column; align-items: center; }
  .caption { margin-top: 4px; font-weight: bold; }
</style>
</head>
<body>
<h2>Hello OpenCV.js</h2>
<p id="status">OpenCV.js is loading...</p>
<div class="row">
  <div class="inputoutput">
    <canvas id="cameraCanvas" width="320" height="240"></canvas>
    <div class="caption">Original</div>
  </div>
  <div class="inputoutput">
    <canvas id="grayCanvas" width="320" height="240"></canvas>
    <div class="caption">Grayscale</div>
  </div>
  <div class="inputoutput">
    <canvas id="filteredCanvas1" width="320" height="240"></canvas>
    <div class="caption">Low Pass Filtered</div>
  </div>
  <div class="inputoutput">
    <canvas id="filteredCanvas2" width="320" height="240"></canvas>
    <div class="caption">High Pass Filtered</div>
  </div>
</div>

<script async src="https://docs.opencv.org/3.4.0/opencv.js" type="text/javascript"></script>
<script type="text/javascript">

    function fetchImage() {
        fetch('/image')
            .then(response => response.text())
            .then(data => {
                const img = new Image();
                img.src = data;
                img.onload = () => {
                    const src = cv.imread(img);

                    // Grayscale
                    let gray = new cv.Mat();
                    cv.cvtColor(src, gray, cv.COLOR_RGBA2GRAY);

                    let anchor = new cv.Point(-1, -1);

                    // Low pass filter - 11x11 averaging kernel
                    let lowKernel = cv.Mat.ones(11, 11, cv.CV_32F);
                    lowKernel.convertTo(lowKernel, cv.CV_32F, 1.0 / 121.0);
                    let lowpassfiltered = new cv.Mat();
                    cv.filter2D(gray, lowpassfiltered, -1, lowKernel, anchor, 0, cv.BORDER_DEFAULT);

                    let highKernelData = new Float32Array([
                        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
                        0,  -0.3, -0.3, -0.3, -0.3, -0.3, -0.3, -0.3, -0.3, -0.3,     0,
                        0,  -0.3, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.3,     0,
                        0,  -0.3, -0.5, -0.8, -0.8, -0.8, -0.8, -0.8, -0.5, -0.3,     0,
                        0,  -0.3, -0.5, -0.8, -1.0, -1.0, -1.0, -0.8, -0.5, -0.3,     0,
                        0,  -0.3, -0.5, -0.8, -1.0, 24.0, -1.0, -0.8, -0.5, -0.3,     0,
                        0,  -0.3, -0.5, -0.8, -1.0, -1.0, -1.0, -0.8, -0.5, -0.3,     0,
                        0,  -0.3, -0.5, -0.8, -0.8, -0.8, -0.8, -0.8, -0.5, -0.3,     0,
                        0,  -0.3, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.3,     0,
                        0,  -0.3, -0.3, -0.3, -0.3, -0.3, -0.3, -0.3, -0.3, -0.3,     0,
                        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
                    ]);
                    let highKernel = cv.matFromArray(11, 11, cv.CV_32F, highKernelData);
                    let highpassfiltered = new cv.Mat();
                    cv.filter2D(gray, highpassfiltered, -1, highKernel, anchor, 0, cv.BORDER_DEFAULT);

                    cv.imshow('cameraCanvas', src);
                    cv.imshow('grayCanvas', gray);
                    cv.imshow('filteredCanvas1', lowpassfiltered);
                    cv.imshow('filteredCanvas2', highpassfiltered);

                    src.delete();
                    gray.delete();
                    lowKernel.delete();
                    lowpassfiltered.delete();
                    highKernel.delete();
                    highpassfiltered.delete();
                };
                img.onerror = () => {
                    console.error("Failed to load image");
                };
            })
            .catch(error => console.error('Error fetching image:', error));
    }

    var Module = {
        onRuntimeInitialized() {
            document.getElementById('status').innerHTML = 'OpenCV.js is ready.';
            setInterval(fetchImage, 1000);
        }
    };

</script>
</body>
</html>
)rawliteral";

void handleRoot() {
server.send(200, "text/html", htmlPage);
}

// Görüntüyü Base64 olarak gönder
// Görüntüyü Base64 olarak gönder
void handleImageRequest() {
  FrameBuffer* frame = espcam.capture();
  if (frame) {
    String base64Image = "data:image/jpeg;base64,";
    base64Image += espcam.frameToBase64(frame);
    server.send(200, "text/plain", base64Image);
    espcam.returnFrame(frame);
  } else {
    server.send(500, "text/plain", "Failed to capture image");
  }
}

// Favicon için
void handleFavicon() {
  server.send(204, "image/x-icon", "");
}

void setup() {
  Serial.begin(115200);
  if (!SPIFFS.begin(true)) {
  Serial.println("SPIFFS Mount Failed!");
  return;
}  // Kamera başlatma
  espcam.enableLogging(true);
  if (!espcam.init(AI_THINKER(), QVGA)) {
    Serial.println("Camera init failed");
    // Hata durumunda işlem yap
    while (true) {
      delay(1000);
      Serial.println("Camera init failed, retrying...");
      // Yeniden deneyebilirsiniz, ancak döngüye girmemek için bir kaç deneme yapıp durmak isteyebilirsiniz.
    }
  }
  Serial.println("Camera initialized");

  // WiFi bağlantısı
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP()); // ESP32'nin IP adresini yazdır

  // Web sunucusu rotaları
  server.on("/", HTTP_GET, handleRoot); // Ana sayfa
  server.on("/image", HTTP_GET, handleImageRequest); // Görüntü isteği
  server.on("/favicon.ico", HTTP_GET, handleFavicon); // Favicon

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient(); // İstemci isteklerini işle
}