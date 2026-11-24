// Pinos UART2 / MAX485
#define RXD2 16
#define TXD2 17
#define RS485_CTRL 4   // Pinos DE e RE unidos

String bufferRS485 = "";

uint8_t calcularChecksumXOR(const String &payload) {
  uint8_t cs = 0;
  for (size_t i = 0; i < payload.length(); i++) {
    cs ^= (uint8_t)payload[i];
  }
  return cs;
}

void processarQuadro(String quadro) {
  quadro.trim();

  // Verifica delimitadores de inicio ($) e checksum (*)
  int idxInicio = quadro.indexOf('$');
  int idxAst = quadro.lastIndexOf('*');

  if (idxInicio == -1 || idxAst == -1 || idxAst <= idxInicio + 1) {
    Serial.println("[RECEPTOR] ERR | FORMATO_INVALIDO");
    return;
  }

  String payload = quadro.substring(idxInicio + 1, idxAst);
  String csRecebidoStr = quadro.substring(idxAst + 1);
  csRecebidoStr.toUpperCase();

  if (csRecebidoStr.length() != 2) {
    Serial.println("[RECEPTOR] ERR | TAMANHO_CHECKSUM");
    return;
  }

  // Valida integridade via Checksum XOR
  uint8_t csCalculado = calcularChecksumXOR(payload);
  uint8_t csRecebido = (uint8_t)strtol(csRecebidoStr.c_str(), NULL, 16);

  if (csCalculado != csRecebido) {
    Serial.println("[RECEPTOR] ERR | CHECKSUM_DIVERGENTE");
    return;
  }

  // Separa identificadores T: e P:
  int idxSep = payload.indexOf(';');
  if (idxSep == -1 || !payload.startsWith("T:") || !payload.substring(idxSep + 1).startsWith("P:")) {
    Serial.println("[RECEPTOR] ERR | IDENTIFICADORES_AUSENTES");
    return;
  }

  float temperatura = payload.substring(2, idxSep).toFloat();
  float angulo = payload.substring(idxSep + 3).toFloat();

  // Valida faixa operacional dos sensores (DS18B20 retorna -127 se desconectado)
  if (temperatura < -55.0 || temperatura > 125.0) {
    Serial.println("[RECEPTOR] ERR | SENSOR_TEMP_INVALIDO");
    return;
  }
  if (angulo < 0.0 || angulo > 300.0) {
    Serial.println("[RECEPTOR] ERR | POT_FORA_FAIXA");
    return;
  }

  // Envia mensagem pela USB Serial para o terminal e para o Python
  Serial.print("[RECEPTOR] OK | Temp: ");
  Serial.print(temperatura, 2);
  Serial.print(" C | Angulo: ");
  Serial.print(angulo, 1);
  Serial.print(" graus | Mensagem: ");
  Serial.println(quadro);
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  pinMode(RS485_CTRL, OUTPUT);
  digitalWrite(RS485_CTRL, LOW); // Mantem MAX485 em modo recepcao

  Serial.println("[RECEPTOR] Aguardando mensagem...");
}

void loop() {
  while (Serial2.available()) {
    char c = Serial2.read();
    if (c == '\n') {
      processarQuadro(bufferRS485);
      bufferRS485 = "";
    } else if (c != '\r') {
      if (bufferRS485.length() < 64) {
        bufferRS485 += c;
      } else {
        bufferRS485 = ""; // Descarta se exceder tamanho maximo do quadro
      }
    }
  }
}