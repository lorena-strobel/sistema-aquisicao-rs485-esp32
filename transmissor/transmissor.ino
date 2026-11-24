#include <OneWire.h>
#include <DallasTemperature.h>

// Pinos UART2 / MAX485
#define RXD2 16
#define TXD2 17
#define RS485_CTRL 4   // Pinos DE e RE unidos

// Pinos dos sensores
#define ONE_WIRE_BUS 18
#define POT_PIN 34

// Curso angular mecanico do potenciometro (padrao 270 graus)
const float ANGULO_MAXIMO = 270.0;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

uint8_t calcularChecksumXOR(const String &payload) {
  uint8_t cs = 0;
  for (size_t i = 0; i < payload.length(); i++) {
    cs ^= (uint8_t)payload[i];
  }
  return cs;
}

float lerPosicaoAngular() {
  long soma = 0;
  for (int i = 0; i < 10; i++) {
    soma += analogRead(POT_PIN);
    delay(2);
  }
  float media = soma / 10.0;
  return (media / 4095.0) * ANGULO_MAXIMO;
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  pinMode(RS485_CTRL, OUTPUT);
  digitalWrite(RS485_CTRL, LOW);

  analogReadResolution(12);
  sensors.begin();
  sensors.setResolution(10); // Conversao em ~190 ms com precisao de 0.25 C

  Serial.println("[TRANSMISSOR] Sistema iniciado. Lendo sensores a cada ~530 ms...");
}

void loop() {
  sensors.requestTemperatures();
  float temperatura = sensors.getTempCByIndex(0);
  float angulo = lerPosicaoAngular();

  // Monta payload com identificadores explicitos
  String payload = "T:" + String(temperatura, 2) + ";P:" + String(angulo, 1);

  // Calcula checksum XOR e formata em hexadecimal de 2 digitos
  uint8_t cs = calcularChecksumXOR(payload);
  char hexCS[3];
  snprintf(hexCS, sizeof(hexCS), "%02X", cs);

  String quadro = "$" + payload + "*" + String(hexCS);

  // Transmite via RS485
  digitalWrite(RS485_CTRL, HIGH); // Habilita driver de transmissao
  delay(2);
  Serial2.println(quadro);
  Serial2.flush();                // Aguarda envio completo antes de liberar o barramento
  delay(2);
  digitalWrite(RS485_CTRL, LOW);  // Retorna ao modo recepcao

  // Exibe status legivel no Monitor Serial do Transmissor
  Serial.print("[TRANSMISSOR] Temp: ");
  Serial.print(temperatura, 2);
  Serial.print(" C | Angulo: ");
  Serial.print(angulo, 1);
  Serial.print(" graus | Mensagem: ");
  Serial.println(quadro);

  delay(300); // Pausa (ciclo total de ~530 ms)
}