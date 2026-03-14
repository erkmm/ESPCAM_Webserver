#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <AViShaESPCam.h>

AViShaESPCam espcam;

const char* ssid = "FiberHGW_ZTF7ZQ_2.4GHz";
const char* password = "4Yax4c9zxKNH";

WebServer server(80);

// HTML sayfası
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Hello OpenCV.js</title>
</head>
<body>
<h2>Hello OpenCV.js</h2>
<p id="status">OpenCV.js is loading...</p>
<div>
  <div class="inputoutput">
    <canvas id="cameraCanvas" width="320" height="240"></canvas>
    <div class="caption">cameraCanvas</div>
  </div>
  <div id="result"></div>
</div>
<!-- Fazladan bir </div> kaldırıldı -->
<script async src="https://docs.opencv.org/3.4.0/opencv.js" type="text/javascript"></script>
<script  type="text/javascript">
   
    // var cv; // Bu satıra gerek yok, çünkü OpenCV.js cv'yi globalde oluşturur.

    function fetchImage() {
        fetch('/image')
            .then(response => response.text())
            .then(data => {
                const img = new Image();
                img.src = data;
                img.onload = () => {
                    // Canvas'tan alınan ImageData'yı OpenCV Mat formatına dönüştür
                    const src = cv.imread(img);

                    let matC3 = new cv.Mat(src.rows, src.cols, cv.CV_8UC3);
                    cv.cvtColor(src, matC3, cv.COLOR_RGBA2GRAY);

                    // İşlenmiş görüntüyü canvas'a geri yükle
                    cv.imshow('cameraCanvas', matC3);

                    // Belleği temizle
                    src.delete();
                    matC3.delete();
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

// Ana sayfayı gönder
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
  
  // Kamera başlatma
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