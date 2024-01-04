#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#define SS_PIN D8
#define RST_PIN D0
#define BUZZER_PIN D3
#define STATUS_LED D4

MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);

const char *ssid = "NODEMCU";
const char *password = "test@nodemcu";
const char *server = "192.168.16.101"; // Replace with your laptop's IP address on the hotspot
const int port = 8000; // Replace with the port where your API is running
String validUser = "False";
WiFiClient client;

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();

  lcd.init();
  lcd.backlight();
  lcd.print("");

  pinMode(STATUS_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(STATUS_LED, HIGH);
  digitalWrite(BUZZER_PIN, LOW);

  displayStatus("Loading ... ", "");
  // Connect to Wi-Fi hotspot
  connectToWiFi();
}

void connectToWiFi() {
  displayStatus("Connecting to WiFi...", "");
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    displayStatus("Connected to WiFi", "");
    Serial.println("\nConnected to WiFi");
  } else {
    displayStatus("Failed to connect", "Please check credentials and try again.");
    Serial.println("\nFailed to connect to WiFi. Please check credentials and try again.");
  }
}

String readCardUID() {
  String cardUID = "";

  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      cardUID += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
      cardUID += String(mfrc522.uid.uidByte[i], HEX);
    }

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }

  return cardUID;
}

void displayStatus(const String &line1, const String &line2) {
  Serial.println(line1);
  Serial.println(line2);

  lcd.clear();

  // Display line1
  lcd.setCursor(0, 0);
  lcd.print(line1);

  // Check if line1 exceeds the LCD width
  if (line1.length() > 16) {  // Assuming your LCD has 16 columns, adjust accordingly
    // Scroll line1
    for (int position = 0; position < line1.length() + 16; position++) {
      lcd.scrollDisplayLeft();
      delay(300);  // Adjust the delay to control the scrolling speed
    }
    lcd.clear();
  }

  // Display line2
  lcd.setCursor(0, 1);
  lcd.print(line2);

  // Check if line2 exceeds the LCD width
  if (line2.length() > 16) {  // Assuming your LCD has 16 columns, adjust accordingly
    // Scroll line2
    for (int position = 0; position < line2.length() + 16; position++) {
      lcd.scrollDisplayLeft();
      delay(400);  // Adjust the delay to control the scrolling speed
    }
    lcd.clear();
  }
}

String checkValidUser(String uid) {
  String url = "http://" + String(server) + ":" + String(port) + "/valid/" + uid; // Use the correct IP address
  Serial.println(url);
  Serial.print("Connecting to: ");
  Serial.println(url);

  HTTPClient http;

  // Set the target URL
  http.begin(client, url);

  // Set the content type header for the GET request
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  // Send the GET request
  int httpResponseCode = http.GET();

  // Check the response code
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    // Get the response payload
    String response = http.getString();
    Serial.println("Received response: " + response);

    // Close the connection
    http.end();

    // Return the raw response
    return response;
  } else {
    Serial.print("HTTP GET request failed, error code: ");
    Serial.println(httpResponseCode);

    // Close the connection
    http.end();

    return "false";
  }
}




String get_valid_client(String uid) {
  String client_id;
  String url = "http://" + String(server) + ":" + String(port) + "/valid/rfid/" + uid; // Use the correct IP address
  Serial.print("Connecting to: ");
  Serial.println(url);

  DynamicJsonDocument doc(1024);  // Adjust the size based on your expected JSON response size

  if (client.connect(server, port)) {
    Serial.println("HTTP request started");
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + server + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(500);

    String jsonResponse = "";

    while (client.available()) {
      char c = client.read();
      jsonResponse += c;
    }

    int startIndex = jsonResponse.indexOf("{");
    int endIndex = jsonResponse.lastIndexOf("}");

    String jsonContent = jsonResponse.substring(startIndex, endIndex + 1);
    deserializeJson(doc, jsonContent);

    String client_name = doc["name"].as<String>();
    client_id = doc["cid"].as<String>();
    displayStatus("Welcome !", client_name);
    Serial.println("Name From JSON: " + client_name);

    client.stop();
  } else {
    Serial.println("Failed to connect to server");
  }

  return client_id;
}

String make_travel_log(String cid){
  WiFiClient client;
  HTTPClient http;

   // Specify the target URL for the POST request
    String url = "http://" + String(server) + ":" + String(port) + "/travellog/?cid=" + cid;
    Serial.print("Sending POST request to: ");
    Serial.println(url);

    // Set the target URL
    http.begin(client,url);

    // Set the content type header for the POST request
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    // Add the data to be sent in the POST request
    String postData = "cid=" + cid;
    Serial.print("Sending data: ");
    Serial.println(postData);

    // Send the POST request
    int httpResponseCode = http.POST(postData);

    // Check the response code
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);

      // Get the response payload
      String response = http.getString();
      Serial.println("POST RESPONSE: " + response);
      return response;
    } else {
      Serial.print("HTTP POST request failed, error code: ");
      Serial.println(httpResponseCode);
    }

    // Close the connection
    
    http.end();
    return "Error";
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    // Allow some time for the user to swipe the card
    delay(200);
    displayStatus("Welcome!", "Swipe Card");

    String uid = readCardUID();

    if (uid.length() > 0) {
      Serial.println("UID:" + uid);
      displayStatus("Wait", "");

      // Reduce unnecessary delays
      digitalWrite(STATUS_LED, LOW);
      delay(100);
      digitalWrite(STATUS_LED, HIGH);

      String isValidUser = checkValidUser(uid);

      if (isValidUser=="true") {
        String clientid = get_valid_client(uid);
        Serial.println(clientid);
        
        // Assuming checkValidUser is true, proceed to the next function
        String seat_data = make_travel_log(clientid);

        if (seat_data == "Seat Released") {
          displayStatus("Thank you!", "From Arniko College");
        } else {
          displayStatus("Your Seat is ", seat_data);
        }
      } else {
        displayStatus("Invalid Entry", "");
      }

      // Implement your logic here based on the card UID if needed
      Serial.println("Card UID: " + uid);

      delay(2000); // Adjust the delay based on your needs
      digitalWrite(STATUS_LED, HIGH);
    }
  } else {
    // Reconnect to Wi-Fi if not connected
    connectToWiFi();
  }
}


