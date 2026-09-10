/*
  Este programa conecta una tarjeta Arduino MKR WAN 1300/1310 a The Things Network (TTN) mediante LoRaWAN utilizando OTAA.

  Una vez conectado:
  - Envía un contador consecutivo mediante un payload estructurado.
  - El contador utiliza 2 bytes.
  - Cambia el estado del LED integrado en cada ciclo como indicador visual.
  - Comprueba si existe un mensaje de bajada (downlink).
  - Si recibe un downlink, muestra su contenido en hexadecimal.

  Estructura del payload enviado:
    [Canal] [Tipo] [Dato MSB] [Dato LSB]
  Ejemplo:
    01 C8 00 01
    Canal = 1
    Tipo = 200 (C8 hexadecimal)
    Valor = 1
*/

#include <MKRWAN.h>
#include "arduino_secrets.h"

// Objeto utilizado para controlar el módem LoRaWAN.
LoRaModem modem;

// Credenciales OTAA.
String appEui = SECRET_APP_EUI;
String appKey = SECRET_APP_KEY;

// Configuración del dato enviado.
const byte CANAL_CONTADOR = 1, TIPO_CONTADOR = 200;
// El contador ocupa 2 bytes: 0 a 65535.
uint16_t numero = 1;
// Control del LED integrado.
int flag = 0;

void setup() {
  // Inicia la comunicación serial.
  Serial.begin(115200);
  // 10 intentos para ingresar al monitor serial antes de continuar.
  for(int i = 0; i < 10 && !Serial; i++) {
    delay(1000);
  }
  // Configura el LED integrado como salida.
  pinMode(LED_BUILTIN, OUTPUT);

  // Inicia el módem LoRaWAN utilizando la región US915.
  while (!modem.begin(US915)) {
    Serial.println("Fallo al iniciar el modulo. Reintentando en 5s...");
    digitalWrite(LED_BUILTIN, HIGH);
    delay(5000);
    digitalWrite(LED_BUILTIN, LOW);
  }

  // Muestra información del módulo LoRaWAN.
  Serial.print("Version del modulo: ");
  Serial.println(modem.version());
  Serial.print("EUI del dispositivo: ");
  Serial.println(modem.deviceEUI());

  // Intenta realizar la conexión OTAA con TTN.
  int connected = 0, contador = 0;
  while (!connected) {
    Serial.println("Intentando conectar a TTN (OTAA)...");
    connected = modem.joinOTAA(appEui, appKey);
    if (!connected) {
      Serial.println("Fallo al conectar [" + String(++contador) + "]. Reintentando...");
      for (int i = 0; i < 5; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
        delay(500);
      }
    }
  }

  Serial.println();
  Serial.println("INICIO");
  Serial.println();
  modem.minPollInterval(60);
}


void loop() {
  // Alterna el LED integrado en cada ciclo.
  if (flag) {
    digitalWrite(LED_BUILTIN, LOW);
    flag = 0;
    Serial.println("LED APAGADO");
  } else {
    digitalWrite(LED_BUILTIN, HIGH);
    flag = 1;
    Serial.println("LED ENCENDIDO");
  }

  /*
    Construcción del payload:
      Byte 0 = Canal
      Byte 1 = Tipo de dato
      Byte 2 = Parte alta del número
      Byte 3 = Parte baja del número
  */
  byte payload[4];
  payload[0] = CANAL_CONTADOR;
  payload[1] = TIPO_CONTADOR;
  payload[2] = (numero >> 8) & 0xFF;
  payload[3] = numero & 0xFF;

  // Muestra el valor que se enviará.
  Serial.print("Enviando numero: ");
  Serial.println(numero);
  // Muestra el payload en hexadecimal.
  Serial.print("Payload: ");
  for (unsigned int i = 0; i < sizeof(payload); i++) {
    if (payload[i] < 0x10) Serial.print("0");
    Serial.print(payload[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // Crea el paquete LoRaWAN.
  modem.beginPacket();
  // Envía los bytes del payload.
  for (unsigned int i = 0; i < sizeof(payload); i++) {
    modem.write(payload[i]);
  }
  // Envía el paquete.
  int err = modem.endPacket(true);
  if (err > 0) {
    Serial.println("Mensaje enviado correctamente!");
    // Incrementa el contador únicamente si el envío fue exitoso.
    numero++;
  } else {
    Serial.println("Error enviando mensaje :(");
  }

  // Espera 60 segundos antes del siguiente envío.
  delay(60000);
}