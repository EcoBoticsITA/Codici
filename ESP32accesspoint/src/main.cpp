#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <IniFile.h> // Libreria per leggere file .ini
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>

const int chipSelect = 5; // Porta per il chip select della microSD

// Variabili per i dati WiFi
char personal_ssid[32];
char personal_password[32];

// Variabili per i dati WiFi dell'access point
const char *ap_ssid = "EcoBotics-ESP32-Access-Point";
const char *ap_password = "123456789";

// Configurazione AP
IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

// Server asincrono
AsyncWebServer webServer(80);

// Funzione per leggere il file di configurazione dalla microSD
String readConfig() {
  File file = SD.open("/config.ini"); // Apre il file config.ini
  if (!file) {
    // Se non riesce ad aprire config.ini (perchè, ad esempio, non lo trova)
    Serial.println("Impossibile aprire config.ini");
    return "";
  }

  String config =
      ""; // Crea una variabile config dove verrà salvato il valore del file
  while (file.available()) {
    config += (char)file.read();
  }
  file.close();  // Chiude il config
  return config; // Restituisce il valore del file
}

// Funzione per ottenere un valore dal file di configurazione
String getConfigValue(String config, String key) {
  int startIdx = config.indexOf(key); // Cerca la posizione della key
  if (startIdx == -1) {               // Non la trova...
    return ""; // Se non la trova la funzione resistuisce un valore nullo
  }
  // La ha trovata
  int endIdx = config.indexOf(
      "\n", startIdx);        // Cerca dove si vada a capo (dove finisce la key)
  if (endIdx == -1) {         // Se non trova dove si vada a capo...
    endIdx = config.length(); // Vuol dire che la key è l'ultima riga del file
  }
  // Restituisce il valore della key senza "="
  return config.substring(startIdx + key.length() + 1, endIdx);
}

// Variabile con i valori del file config.ini
String config = ""; // Verrà inizializzata nel setup()

// Funzione che prova a connettersi al wifi
bool TryToConnect() {
  unsigned long startMillis = millis(); // Salva il tempo di partenza
  unsigned long timeout = 10000;        // Timeout in millisecondi (10 secondi)

  if (!config.isEmpty()) { // Se la variabile config non è vuota...
    // Lettura SSID e password dalla configurazione
    String ssid = getConfigValue(config, "ssid");
    String password = getConfigValue(config, "password");

    if (ssid != "" && password != "") { // Prova a connettersi al wifi
      WiFi.begin(ssid.c_str(), password.c_str());
      while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
        if (millis() - startMillis >= timeout) {
          Serial.println("");
          Serial.println("Tentativo di connessione fallito!");
          return false; // Se il timeout scade, restituisce false
        }
      }
      Serial.println("Connesso al WiFi");
      Serial.println(WiFi.localIP());
      return true;
    } else {
      Serial.println("SSID e/o password non trovati");
      return false;
    }
  } else {
    Serial.println("File di configurazione vuoto o non trovato");
    return false;
  }
}

void setup() {
  Serial.begin(115200); // Attivo la comunicazione seriale

  // Dati AP
  WiFi.softAPConfig(local_ip, gateway, subnet);

  if (!SD.begin(chipSelect)) { // Provo a connettermi alla SD
    Serial.println("Inizializzazione SD fallita!");
    return;
  }
  Serial.println("MicroSD inizializzata.");

  config = readConfig(); // Prendo i valori del config.ini

  // Prova a connettersi
  if (TryToConnect()) { // Connessione riuscita
    Serial.println("Connessione riuscita!");
  } else { // Connessione fallita, avvio l'access point
    // Fa partire l'access point per ricevere le info di config
    WiFi.softAP(ap_ssid, ap_password);
    Serial.println("Avvio Access Point...");

    // Configurazione del server Web per inserire i dati
    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      String html = "<html><head><title>Config Editor</title></head><body>";
      html += "<h1>Modifica il file di configurazione WiFi</h1>";
      html += "<form action=\"/save_config\" method=\"POST\">";
      html += "<label for=\"ssid\">SSID:</label><input type=\"text\" "
              "id=\"ssid\" name=\"ssid\" value=\"" +
              getConfigValue(config, "ssid") + "\"><br><br>";
      html += "<label for=\"password\">Password:</label><input type=\"text\" "
              "id=\"password\" name=\"password\" value=\"" +
              getConfigValue(config, "password") + "\"><br><br>";
      html += "<input type=\"submit\" value=\"Salva\">";
      html += "</form></body></html>";
      request->send(200, "text/html", html);
    });

    // Quando vengono inviati i dati di configurazione
    webServer.on("/save_config", HTTP_POST, [](AsyncWebServerRequest *request) {
      String ssid = request->getParam("ssid", true)->value();
      String password = request->getParam("password", true)->value();

      // Salvataggio nel file di configurazione
      File file = SD.open("/config.ini", FILE_WRITE);
      if (file) {
        file.print("[WiFi]\n");
        file.print("ssid=" + ssid + "\n");
        file.print("password=" + password + "\n");
        file.close();
        request->send(200, "text/html", "<h1>Configurazione salvata!</h1>");
        config = readConfig(); // Rileggi il file di configurazione per ottenere
                               // i nuovi dati
        if (TryToConnect()) {  // Riprova a connetterti con i nuovi dati
          Serial.println("Connessione WiFi riuscita con i nuovi dati!");
        } else {
          Serial.println("Connessione WiFi fallita con i nuovi dati.");
        }
      } else {
        request->send(500, "text/html",
                      "<h1>Errore nel salvataggio della configurazione</h1>");
      }
    });

    // Avvia il server web
    webServer.begin();
    Serial.println("Web server avviato. Link: http://192.168.4.1");
  }
}

void loop() {
  if (TryToConnect()) {
    // Fermare il server
    server.end();

    // Disconnettere l'Access Point
    WiFi.softAPdisconnect(true);
  }
}