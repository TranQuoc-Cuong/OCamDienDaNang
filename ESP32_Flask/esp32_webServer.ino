#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WebServer.h>
#include <ArduinoJson.h>

const char* ap_ssid = "ESP32_AP";
const char* ap_password = "123456789";
const char* defaultFlaskServer = "http://192.168.4.2:5000/upload"; 

Preferences preferences;
WebServer server(80);
bool isConnected = false;
String flaskServer;
bool ledState = false; 

#define CMD_VOLTAGE "voltage"
#define CMD_CURRENT "current"
#define CMD_LIGHT "light"
#define RXD2 16 
#define TXD2 17 

float voltage, current;
uint16_t power;

long long timePrevious = millis();

String dataFromSTM32;

float cmd_handle(const String &str, const String &manhandang) {
    int index = str.indexOf(manhandang);
    if (index != -1) {
        int startIndex = index + manhandang.length() + 1; // Bỏ qua dấu ':'
        int endIndex = str.indexOf(';', startIndex); // Tìm dấu ';'

        if (endIndex == -1) {
            endIndex = str.length(); // Nếu không tìm thấy, đến cuối chuỗi
        }

        String valueString = str.substring(startIndex, endIndex); // Lấy giá trị giữa ':' và ';'
        return valueString.toFloat(); // Chuyển đổi thành số thực
    }
    return NULL; // Trả về 0 nếu không tìm thấy
}

void resetSTM32(){
  pinMode(25, OUTPUT);
  digitalWrite(25, 0);
  delay(1000);
  digitalWrite(25, 1);
}

void setup() {
  Serial.begin(115200);
  preferences.begin("wifi_config", false);
  Serial1.begin(115200, SERIAL_8N1, RXD2, TXD2);
  resetSTM32();

  String ssid = preferences.getString("ssid", "");
  String password = preferences.getString("password", "");
  flaskServer = preferences.getString("flaskServer", defaultFlaskServer);


  if (ssid.length() > 0 && password.length() > 0) {
    connectToWiFi(ssid, password);
  } else {
    startAP();
  }
}

void loop() {
  server.handleClient();
  if (Serial1.available()) {
      dataFromSTM32 = Serial1.readStringUntil('\n'); // Đọc chuỗi dữ liệu từ Arduino

      voltage = cmd_handle(dataFromSTM32, CMD_VOLTAGE);
      current = cmd_handle(dataFromSTM32, CMD_CURRENT);
      power = voltage * current;
  }
 
  if(millis() - timePrevious >= 1000)
  {
    if (isConnected) {
      sendDataToFlask(); 
    }
    timePrevious = millis();
  }

}

void startAP() {
  WiFi.softAP(ap_ssid, ap_password);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  startWebServer();
}

void connectToWiFi(String ssid, String password) {
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.print("Connecting to WiFi...");
  
  unsigned long startAttemptTime = millis();
  
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(1000);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    isConnected = true; 
  } else {
    Serial.println("WiFi connection failed!");
    startAP();
  }
}

void sendDataToFlask() {

  // Kiểm tra nếu người dùng chưa nhập phần "/upload" thì tự động thêm vào
  String completeFlaskServer = flaskServer;
  if (!flaskServer.endsWith("/upload")) {
    completeFlaskServer += "/upload";
  }

  // Lấy địa chỉ IP của ESP32
  String esp32_ip = WiFi.localIP().toString();

  // Thêm địa chỉ IP vào serverPath
  String serverPath = completeFlaskServer + 
                     "?voltage=" + String(voltage) + 
                     "&current=" + String(current) + 
                     "&power=" + String(power) +
                     "&ledState=" + String(ledState) +
                     "&esp32_ip=" + esp32_ip;  // Thêm địa chỉ IP
  
  HTTPClient http;
  http.begin(serverPath);
  
  int httpResponseCode = http.GET();
  
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    String payload = http.getString();  // Nhận dữ liệu từ Flask
    Serial.println(payload);            // In ra giá trị nút nhấn từ Flask

    // Phân tích JSON response
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    String ledStateFromServer = doc["ledState"].as<String>();  // Lấy giá trị ledState

    // Cập nhật trạng thái LED và ledState
    if (ledStateFromServer == "1") {
      Serial1.print("light:");
      Serial1.print(ledStateFromServer);
      Serial1.println(";");
      ledState = true;
    } else if (ledStateFromServer == "0") {
      Serial1.print("light:");
      Serial1.print(ledStateFromServer);
      Serial1.println(";");
      ledState = false;
    }
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
    
    if (httpResponseCode == -1) {
      Serial.println("HTTP request failed! Switching to AP mode...");
      startAP();
    }
  }
  
  http.end();
}

void startWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
  Serial.println("Web server started");
}

void handleRoot() {
  String html = "<html><body><h1>ESP32 Configuration</h1>";
  html += "<form action='/save' method='POST'>";
  html += "SSID: <input type='text' name='ssid'><br>";
  html += "Password: <input type='password' name='password'><br>";
  html += "Flask Server URL: <input type='text' name='flaskServer' value='" + flaskServer + "'><br>";
  html += "<input type='submit' value='Save'>";
  html += "</form>";

  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleSave() {
  String ssid = server.arg("ssid");
  String password = server.arg("password");
  flaskServer = server.arg("flaskServer");

  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.putString("flaskServer", flaskServer);
  
  server.send(200, "text/html", "<html><body><h1>Saved! Restarting...</h1></body></html>");
  
  delay(2000); 
  ESP.restart(); 
}