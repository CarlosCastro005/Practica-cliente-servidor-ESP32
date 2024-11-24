#include <WiFi.h>
#include <Stepper.h>

// Configuración de la red WiFi
const char* ssid = "D08D16";
const char* password = "82C26C2C40627";

WiFiServer server(80);

// Configuración del motor paso a paso
const int stepsPerRevolution = 2048; // Número de pasos por revolución
const int motSpeed = 20;             // Velocidad del motor en RPM
Stepper myStepper(stepsPerRevolution, 14, 27, 26, 25); // Pines del motor

// Configuración del LED con brillo ajustable
const int ledPin = 33; 
int brightness = 0; // Brillo inicial (0 a 255)

// Configuración del foco
const int focoPin = 12; 
bool focoState = false; // Estado inicial del foco (apagado)

// Función para generar PWM simulada
void writePWM(int pin, int dutyCycle) {
  int period = 1000; // Periodo en microsegundos (frecuencia: ~1 kHz)
  int onTime = (dutyCycle * period) / 255; // Tiempo en HIGH
  int offTime = period - onTime;           // Tiempo en LOW

  digitalWrite(pin, HIGH);
  delayMicroseconds(onTime);
  digitalWrite(pin, LOW);
  delayMicroseconds(offTime);
}

void setup() {
  Serial.begin(115200);

  // Configuración del motor paso a paso
  myStepper.setSpeed(motSpeed);

  // Configuración del LED
  pinMode(ledPin, OUTPUT);

  // Configuración del foco
  pinMode(focoPin, OUTPUT);
  digitalWrite(focoPin, LOW); // Foco apagado por defecto

  // Conexión a WiFi
  Serial.print("Conectando a WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConexión exitosa.");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  // Iniciar el servidor web
  server.begin();
  Serial.println("Servidor HTTP iniciado.");
}

void loop() {
  // Generar PWM continuamente en el LED
  writePWM(ledPin, brightness);

  WiFiClient client = server.available(); // Esperar conexión de un cliente

  if (client) {
    Serial.println("Nuevo cliente conectado");
    String request = client.readStringUntil('\r'); // Leer solicitud HTTP
    Serial.println(request);
    client.flush();

    String action; // Acción para notificar a la página web

    // Procesar la solicitud para el motor paso a paso
    if (request.indexOf("GET /?action=left") >= 0) {
      Serial.println("Girar a la izquierda");
      myStepper.step(stepsPerRevolution); // Girar una revolución a la izquierda
      action = "Motor girando a la izquierda.";
    } else if (request.indexOf("GET /?action=right") >= 0) {
      Serial.println("Girar a la derecha");
      myStepper.step(-stepsPerRevolution); // Girar una revolución a la derecha
      action = "Motor girando a la derecha.";
    }
    // Procesar la solicitud para el foco
    else if (request.indexOf("GET /?action=turnOn") >= 0) {
      Serial.println("Encender foco");
      digitalWrite(focoPin, HIGH); // Encender el foco
      focoState = true;
      action = "Foco encendido.";
    } else if (request.indexOf("GET /?action=turnOff") >= 0) {
      Serial.println("Apagar foco");
      digitalWrite(focoPin, LOW); // Apagar el foco
      focoState = false;
      action = "Foco apagado.";
    }
    // Procesar la solicitud para ajustar el brillo
    else if (request.indexOf("GET /?brightness=") >= 0) {
      int index = request.indexOf("brightness=") + 11;
      int newBrightness = request.substring(index).toInt();
      newBrightness = constrain(newBrightness, 0, 100); // Limitar entre 0 y 100
      brightness = map(newBrightness, 0, 100, 0, 255); // Convertir a rango 0-255
      Serial.print("Nuevo brillo: ");
      Serial.println(brightness);
      action = "Brillo ajustado.";
    }

    // Responder con la página HTML
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println("<!DOCTYPE HTML>");
    client.println("<html>");
    client.println("<head>");
    client.println("<meta charset='UTF-8'>");
    client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
    client.println("<title>Control de Motor, Foco y LED</title>");
    client.println("<style>");
    client.println("body { font-family: Arial, sans-serif; text-align: center; margin: 20px; }");
    client.println(".button { padding: 10px 20px; font-size: 16px; cursor: pointer; margin: 10px; }");
    client.println(".left { background-color: lightblue; }");
    client.println(".right { background-color: lightgreen; }");
    client.println(".on { background-color: orange; }");
    client.println(".off { background-color: gray; }");
    client.println("#slider { width: 80%; }");
    client.println("#notification { margin-top: 20px; font-size: 18px; font-weight: bold; color: darkblue; }");
    client.println("img { width: 200px; margin: 10px; }"); // Estilo para las imágenes
    client.println("</style>");
    client.println("</head>");
    client.println("<body>");
    client.println("<h1>Control de Motor, Foco y LED</h1>");

    // Agregar imágenes
    client.println("<div>");
    client.println("<img src='https://www.uppuebla.edu.mx/images/logo.png' alt='UP Puebla'>");
    client.println("<img src='https://fiction-champions.fandom.com/es/wiki/Fichero:Logotipo.png' alt='Fandom Logo'>");
    client.println("</div>");

    // Controles del motor
    client.println("<h2>Motor Paso a Paso</h2>");
    client.println("<button class='button left' onclick='sendCommand(\"left\")'>Girar a la Izquierda</button>");
    client.println("<button class='button right' onclick='sendCommand(\"right\")'>Girar a la Derecha</button>");

    // Controles del foco
    client.println("<h2>Foco</h2>");
    client.println("<button class='button on' onclick='sendCommand(\"turnOn\")'>Encender Foco</button>");
    client.println("<button class='button off' onclick='sendCommand(\"turnOff\")'>Apagar Foco</button>");

    // Control del LED con slider
    client.println("<h2>Brillo del LED</h2>");
    client.println("<input id='slider' type='range' min='0' max='100' value='" + String(map(brightness, 0, 255, 0, 100)) + "' oninput='updateBrightness(this.value)'>");
    client.println("<p id='brightness'>Brillo: " + String(map(brightness, 0, 255, 0, 100)) + "%</p>");

    client.println("<p id='notification'></p>");
    client.println("<script>");
    client.println("function sendCommand(action) {");
    client.println("  fetch('/?action=' + action).then(() => {");
    client.println("    document.getElementById('notification').innerText = 'Acción ejecutada: ' + action;");
    client.println("  });");
    client.println("}");
    client.println("function updateBrightness(value) {");
    client.println("  document.getElementById('brightness').innerText = 'Brillo: ' + value + '%';");
    client.println("  fetch('/?brightness=' + value);");
    client.println("}");
    client.println("</script>");
    client.println("</body>");
    client.println("</html>");

    delay(10);
    client.stop(); // Cerrar conexión
    Serial.println("Cliente desconectado.");
  }
}