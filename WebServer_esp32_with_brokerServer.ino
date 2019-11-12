// Load Wi-Fi library
#include <WiFi.h>
#include <PubSubClient.h>
#include <Ultrasonic.h>

// Replace with your network credentials
const char* ssid     = "SSID";
const char* password = "PASSWORD";

// Bluetooth
const char* mqttServer = "mqtt.eclipse.org";
const int mqttPort = 1883;
const char* mqttUser = "abcdefg";
const char* mqttPassword = "123456";

char mensagem[50];

WiFiClient espClient;
PubSubClient client(espClient);

// Set web server port number to 80
WiFiServer server(80); 

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String estadoLED = "off";
String estadoRELE = "off";
String estadoBUZZER = "off";

// Assign output variables to GPIO pins
const int LED = 18;
const int RELE = 17;
const int LDR = 34;

// LDR
float Luminosidade = 0;

// Buzzer
const int buzzer = 25;

unsigned int distancia = 0;
//conexão dos pinos para o sensor ultrasonico
#define PIN_TRIGGER   5
#define PIN_ECHO      4
//Inicializa o sensor nos pinos definidos acima
Ultrasonic ultrasonic(PIN_TRIGGER, PIN_ECHO);

int getDistance()
{
    //faz a leitura das informacoes do sensor (em cm)
    int distanciaCM;
    long microsec = ultrasonic.timing();
    // pode ser um float ex: 20,42 cm se declarar a var float 
    distanciaCM = ultrasonic.convert(microsec, Ultrasonic::CM);
  
    return distanciaCM;
}

void setup() {
  Serial.begin(9600);
  // Initialize the output variables as outputs
  pinMode(LDR, INPUT);
  pinMode(LED, OUTPUT);
  pinMode(RELE, OUTPUT);
  // Set outputs to LOW
  digitalWrite(LED, LOW);
  digitalWrite(RELE, HIGH);

  // Buzzer
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("Iniciando conexao com a rede WiFi...");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi conectado.");
  Serial.println("Endereço IP: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void loop(){
  Luminosidade = analogRead(LDR);
  distancia = getDistance();

  // Conexao Bluetooth
  reconectabroker();
  sprintf(mensagem, "LDR %f", Luminosidade);

  client.publish("TOPIC", mensagem);
  
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // Buzzer
            if (Luminosidade >= 1000) {
              digitalWrite(buzzer, HIGH);
              estadoBUZZER = "on";
            } else if (Luminosidade < 1000) {
              digitalWrite(buzzer, LOW);
              estadoBUZZER = "off";
            }
            
            // turns the GPIOs on and off
            if (header.indexOf("GET /26/on") >= 0) {
              Serial.println("LED on");
              estadoLED = "on";
              digitalWrite(LED, HIGH);
            } else if (header.indexOf("GET /26/off") >= 0) {
              Serial.println("LED off");
              estadoLED = "off";
              digitalWrite(LED, LOW);
            } else if (header.indexOf("GET /27/on") >= 0) {
              Serial.println("RELE on");
              estadoRELE = "on";
              digitalWrite(RELE, LOW);
            } else if (header.indexOf("GET /27/off") >= 0) {
              Serial.println("RELE off");
              estadoRELE = "off";
              digitalWrite(RELE, HIGH);
            }
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta http-equiv='refresh' content='3' name=\"viewport\" content=\"width=device-width, initial-scale=1\">"); // Botar IP Local atual
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>ESP32 Web Server</h1>");
            
            // Display current state, and ON/OFF buttons for LED  
            client.println("<p>LED - Estado " + estadoLED + "</p>");
            // If the estadoLED is off, it displays the ON button       
            if (estadoLED=="off") {
              client.println("<p><a href=\"/26/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/26/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
               
            // Display current state, and ON/OFF buttons for RELE  
            client.println("<p>RELE - Estado " + estadoRELE + "</p>");
            // If the estadoRELE is off, it displays the ON button       
            if (estadoRELE=="off") {
              client.println("<p><a href=\"/27/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/27/off\"><button class=\"button button2\">OFF</button></a></p>");
            }

            // Ultrassonico
            client.println("<p>Ultrassonico valor: " + String(distancia) + "</p>");

            // LDR
            client.println("<p>LDR valor: " + String(Luminosidade) + "</p>");
            
            client.println("<p>Buzzer - Estado " + estadoBUZZER + "</p>");
            
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

void reconectabroker()
{
  //Conexao ao broker MQTT
  client.setServer(mqttServer, mqttPort);
  while (!client.connected())
  {
    Serial.println("Conectando ao broker MQTT...");
    if (client.connect("ESP32Client", mqttUser, mqttPassword ))
    {
      Serial.println("Conectado ao broker!");
    }
    else
    {
      Serial.print("Falha na conexao ao broker - Estado: ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}
