/*
  Este programa conecta una tarjeta Arduino MKR WAN 1300/1310 a The Things Network (TTN) mediante LoRaWAN utilizando OTAA.

  Una vez conectado:
  - Envía un número consecutivo como mensaje.
  - Cambia el estado del LED integrado en cada ciclo, sirviendo como indicativo visual de que manda correctamente.
  - Comprueba si existe un mensaje de bajada (downlink) desde TTN.
  - Si recibe un downlink, muestra su contenido en hexadecimal.

  Nota: Basado en el ejemplo "LoRa Send And Receive" de la librería MKRWAN.
*/

#include <MKRWAN.h>
#include "arduino_secrets.h"

// Objeto utilizado para controlar el módem LoRaWAN.
LoRaModem modem;

// Credenciales para realizar la conexión OTAA. Los valores se almacenan en arduino_secrets.h.
String appEui = SECRET_APP_EUI;
String appKey = SECRET_APP_KEY;

int numero = 1;   // Número que se enviará a TTN.
int flag = 0;     // Control del estado del LED.
int ciclo = 0;    // Contador de ciclos.

void setup() {
  // Inicia la comunicación serial para mostrar información de diagnóstico.
  Serial.begin(115200);
  // Configura el LED integrado como salida.
  pinMode(LED_BUILTIN, OUTPUT);

  //Inicia el módem LoRaWAN utilizando la región US915.
  while (!modem.begin(US915)) {
    Serial.println("Fallo al iniciar el módulo. Reintentando en 5s...");

    digitalWrite(LED_BUILTIN, HIGH);
    delay(5000);
    digitalWrite(LED_BUILTIN, LOW);
    //Si no logra iniciar, vuelve a intentarlo cada 5 segundos y muestra un indicativo visual con el LED. 
  }

  // Muestra información del módulo LoRaWAN.
  Serial.print("Su versión de módulo es: ");
  Serial.println(modem.version());
  Serial.print("Su Identificador de Hardware(EUI) del dispositivo es: ");
  Serial.println(modem.deviceEUI());

  //Intenta realizar la conexión OTAA con TTN.
  //Si la conexión falla, vuelve a intentarlo y utiliza el LED integrado como indicador visual.
  int connected = 0;
  while (!connected) {
    Serial.println("Intentando conectar a TTN (OTAA)...");
    connected = modem.joinOTAA(appEui, appKey); 
    //Para ello, utiliza las credenciales definidas en arduino_secrets.h. Aunque también se pueden definir directamente en el código.

    if (!connected) {
      Serial.println("Fallo al conectar. Reintentando...");
      for (int i = 0; i < 5; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
        delay(500);
      }
    }
  }

  // La conexión con TTN se realizó correctamente.
  Serial.println(" INICIO ");
  Serial.println("");
  //Define el intervalo mínimo de consulta del módem en 60 segundos.
  modem.minPollInterval(60);
}

//Ciclo principal del programa.
void loop() {
  // Alterna el estado del LED integrado en cada ciclo para indicar visualmente la ejecución del programa.
  if (flag) {
    digitalWrite(LED_BUILTIN, LOW);
    flag = 0;
    Serial.println(" ENCENDIDO ");
  }
  else {
    digitalWrite(LED_BUILTIN, HIGH);
    flag = 1;
    Serial.println(" APAGADO ");
  }

  // Convierte el número actual a texto para enviarlo por LoRaWAN.
  String msg = (String) numero;
  // Muestra el mensaje que será enviado.
  Serial.print("Enviando: " + msg + " - ");

  // Muestra cada byte del mensaje en formato hexadecimal.
  for (unsigned int i = 0; i < msg.length(); i++) {
    Serial.print(msg[i] >> 4, HEX);
    Serial.print(msg[i] & 0xF, HEX);
    Serial.print(" ");
  }
  Serial.println();

  //Crea y envía el paquete LoRaWAN.
  modem.beginPacket();
  modem.print(msg);

  int err = modem.endPacket(true);
  if (err > 0) {
    Serial.println("Mensaje enviado correctamente!");
    // Incrementa el número únicamente si el envío fue exitoso.
    numero++;
    // Espera 60 segundos antes de continuar.
    delay(60000);
  }
  else Serial.println("Error enviando mensaje:(");
  
  delay(1000);

  //Después del uplink se comprueba si TTN envió algún mensaje de bajada (downlink).
  if (!modem.available()) {
    Serial.println("No downlink message received at this time.");
    return;
  }

  // Buffer para almacenar el mensaje recibido.
  char rcv[64];
  int i = 0;
  // Lee todos los bytes disponibles del downlink.
  while (modem.available()) {
    rcv[i++] = (char)modem.read();
  }

  // Muestra el mensaje recibido en hexadecimal.
  Serial.print("Recibido: ");
  for (unsigned int j = 0; j < i; j++) {
    Serial.print(rcv[j] >> 4, HEX);
    Serial.print(rcv[j] & 0xF, HEX);
    Serial.print(" ");
  }

  Serial.println();
}