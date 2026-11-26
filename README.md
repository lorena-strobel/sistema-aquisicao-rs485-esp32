# Monitoramento de Temperatura e Posição Angular com ESP32 e RS-485

![ESP32](https://img.shields.io/badge/Hardware-ESP32%20DevKit%20V1-blue)
![Protocol](https://img.shields.io/badge/Barramento-RS--485%20(MAX485)-orange)
![Firmware](https://img.shields.io/badge/Firmware-C%2B%2B%20%2F%20Arduino-00979D)
![Frontend](https://img.shields.io/badge/Interface-Web%20Serial%20API%20%2B%20Chart.js-yellow)

Sistema de aquisição, transmissão serial diferencial e monitoramento gráfico em tempo real de **temperatura** (`DS18B20`) e **posição angular** (potenciômetro linear de `5 kΩ`), utilizando dois microcontroladores **DOIT ESP32 DevKit V1** interligados via barramento **RS-485 (módulos MAX485)**.

> Projeto desenvolvido para a disciplina de **Microcontroladores** do curso de Bacharelado em Engenharia da Computação — **IFMT (Instituto Federal de Mato Grosso)**.

---

## Índice

- [Monitoramento de Temperatura e Posição Angular com ESP32 e RS-485](#monitoramento-de-temperatura-e-posição-angular-com-esp32-e-rs-485)
  - [Índice](#índice)
  - [Visão Geral](#visão-geral)
  - [Arquitetura do Sistema](#arquitetura-do-sistema)
  - [Componentes Utilizados](#componentes-utilizados)
  - [Esquema de Ligações (Pinout)](#esquema-de-ligações-pinout)
    - [Unidade Transmissora](#unidade-transmissora)
    - [Unidade Receptora](#unidade-receptora)
    - [Interligação do Barramento RS-485](#interligação-do-barramento-rs-485)
    - [Equações de Projeto](#equações-de-projeto)
  - [Protocolo de Comunicação](#protocolo-de-comunicação)
    - [Formato do Quadro ASCII](#formato-do-quadro-ascii)
    - [Códigos de Diagnóstico do Receptor](#códigos-de-diagnóstico-do-receptor)
  - [Estrutura do Repositório](#estrutura-do-repositório)
  - [Como Executar](#como-executar)
    - [1. Pré-requisitos](#1-pré-requisitos)
    - [2. Gravação dos Firmwares](#2-gravação-dos-firmwares)
    - [3. Monitoramento em Tempo Real](#3-monitoramento-em-tempo-real)
  - [Resultados Obtidos](#resultados-obtidos)
  - [Autores](#autores)

---

## Visão Geral

O projeto implementa um enlace de telemetria *half-duplex* unidirecional sobre o padrão físico **TIA/EIA-485** (imune a ruídos em modo comum) combinado a um **protocolo ASCII próprio com verificação de integridade por Checksum XOR**:

- **Unidade Transmissora (`transmissor.ino`):** lê o sensor digital `DS18B20` (resolução de 10 bits) e a média de 10 amostras do potenciômetro de `5 kΩ` (ADC de 12 bits convertido para `0°–270°`), empacota os dados com checksum XOR e transmite via UART2 (`9600 bps`) pelo módulo MAX485 a cada `~532 ms`.
- **Unidade Receptora (`receptor.ino`):** mantém o MAX485 em escuta contínua (`DE` e `/RE` em `LOW`), protege o pino `RX2` (`GPIO 16`) com divisor de tensão (`5 V → ~3,44 V`), valida delimitadores, checksum XOR e faixas físicas dos sensores, e encaminha os quadros aprovados via USB Serial (`115200 bps`).
- **Interface Web (`grafico.html`):** aplicação independente executada direto no navegador via **Web Serial API** e **Chart.js**, exibindo leitura instantânea, mínima/média/máxima térmica, gráficos deslizantes (últimos 30 pontos) e contador de pacotes válidos e erros.

---

## Arquitetura do Sistema

```text
[DS18B20 + Potenciômetro 5kΩ]
              │
              ▼
   ┌─────────────────────┐
   │  ESP32 Transmissor  │ (UART2: 9600 bps, 8N1)
   └──────────┬──────────┘
              │ TX2 (GPIO 17) / CTRL (GPIO 4)
              ▼
         ┌────────┐
         │ MAX485 │
         └────┬───┘
              │ Barramento RS-485 (Par Trançado: A, B + GND)
              ▼
         ┌────────┐
         │ MAX485 │
         └────┬───┘
              │ RO → Divisor de Tensão (1kΩ / 2,2kΩ → ~3,44 V) → RX2 (GPIO 16)
              ▼
   ┌─────────────────────┐
   │   ESP32 Receptor    │ (Validação XOR + Faixas Físicas)
   └──────────┬──────────┘
              │ USB Serial (115200 bps)
              ▼
   ┌─────────────────────┐
   │    grafico.html     │ (Web Serial API + Chart.js)
   └─────────────────────┘
```

---

## Componentes Utilizados

| Qtd. | Componente | Especificação / Função |
| :---: | :--- | :--- |
| 02 | DOIT ESP32 DevKit V1 (30 pinos) | Microcontroladores (Transmissor e Receptor) |
| 02 | Módulo TTL para RS-485 (MAX485) | Transceptores diferenciais *half-duplex* (alimentados em `5 V`) |
| 01 | Sensor de temperatura DS18B20 | Sonda digital 1-Wire (10 bits, precisão de `0,25 °C`, faixa `-55 °C` a `+125 °C`) |
| 01 | Potenciômetro rotativo linear `5 kΩ` | Curso mecânico de `270°` conectado ao `ADC1_CH6` |
| 01 | Resistor `4,7 kΩ` (`R3`) | *Pull-up* da linha de dados do DS18B20 para `3,3 V` |
| 01 | Resistor `1 kΩ` (`R1`) | Resistor série do divisor de tensão na entrada `RX2` do receptor |
| 01 | Resistor `2,2 kΩ` (`R2`) | Resistor derivado ao `GND` no divisor de tensão do receptor |
| 02 | Protoboards + cabos jumper | Montagem física e barramento diferencial (`A`, `B` e `GND`) |

---

## Esquema de Ligações (Pinout)

> [!WARNING]
> Os GPIOs do ESP32 operam estritamente em **3,3 V** e **não são tolerantes a 5 V**. Mantenha o barramento `VIN` (`5 V` do MAX485) eletricamente isolado da linha `3V3` dos sensores e utilize obrigatoriamente o divisor de tensão no pino `RO → RX2` do receptor.

### Unidade Transmissora

| Componente | Pino | ESP32 Transmissor | Observação |
| :--- | :---: | :---: | :--- |
| **DS18B20** | `VCC` / `GND` | `3V3` / `GND` | Alimentação em `3,3 V` |
| **DS18B20** | `DQ` (Dados) | `GPIO 18` | Resistor *pull-up* `R3` (`4,7 kΩ`) entre `GPIO 18` e `3V3` |
| **Potenciômetro 5 kΩ** | Extremos | `3V3` e `GND` | Divisor analógico de `0 V` a `3,3 V` |
| **Potenciômetro 5 kΩ** | Cursor central | `GPIO 34` (`ADC1_CH6`) | Entrada analógica de 12 bits (`0–4095`) |
| **MAX485** | `VCC` / `GND` | `VIN` (`5 V`) / `GND` | Alimentação exclusiva de `5 V` |
| **MAX485** | `DI` | `GPIO 17` (`TX2`) | Ligação direta (sem divisor) |
| **MAX485** | `DE` e `/RE` (unidos) | `GPIO 4` | Controle de direção (`HIGH` transmite, `LOW` libera) |
| **MAX485** | `RO` | *Livre* | Não utilizado no transmissor |

### Unidade Receptora

| Componente | Pino | ESP32 Receptor | Observação |
| :--- | :---: | :---: | :--- |
| **MAX485** | `VCC` / `GND` | `VIN` (`5 V`) / `GND` | Pino `3V3` do ESP32 permanece livre |
| **MAX485** | `RO` | `GPIO 16` (`RX2`) | Via divisor resistivo: `RO → R1 (1 kΩ) → RX2 → R2 (2,2 kΩ) → GND` |
| **MAX485** | `DE` e `/RE` (unidos) | `GPIO 4` | Mantido em `LOW` (recepção permanente) |
| **MAX485** | `DI` | *Livre* | Não utilizado no receptor |

### Interligação do Barramento RS-485

| MAX485 Transmissor | ↔ | MAX485 Receptor |
| :---: | :---: | :---: |
| `A` | ↔ | `A` |
| `B` | ↔ | `B` |
| `GND` | ↔ | `GND` |

### Equações de Projeto

- **Divisor de tensão na entrada `RX2` (receptor):**

  $$V_{\text{RX2}} = 5{,}0 \times \frac{2200}{1000 + 2200} \approx 3{,}44\text{ V}$$

- **Sobreamostragem e conversão angular (transmissor):**

  $$\text{ADC}_{\text{médio}} = \frac{1}{10} \sum_{i=1}^{10} \text{ADC}_i \qquad \text{e} \qquad \text{Ângulo} = \left(\frac{\text{ADC}_{\text{médio}}}{4095{,}0}\right) \times 270{,}0^\circ$$

---

## Protocolo de Comunicação

- **Camada física:** RS-485 (*half-duplex*)
- **UART2 (entre ESP32s):** `9600 bps`, `8N1`
- **USB Serial (Receptor → Web):** `115200 bps`, `8N1`

### Formato do Quadro ASCII

```text
$T:<Temperatura>;P:<Angulo>*<Checksum_HEX>\r\n
```

**Exemplos válidos:**

```text
$T:24.75;P:135.2*3E
$T:29.00;P:116.5*37
```

O **Checksum XOR** é calculado aplicando a operação lógica `XOR` (`^`) byte a byte sobre todos os caracteres do *payload* compreendidos entre `$` e `*` (excluindo os delimitadores), formatado em **2 dígitos hexadecimais maiúsculos** (`%02X`).

### Códigos de Diagnóstico do Receptor

| Saída Serial (USB) | Significado |
| :--- | :--- |
| `[RECEPTOR] OK \| Temp: ... \| Angulo: ...` | Pacote íntegro e dentro das faixas físicas |
| `[RECEPTOR] ERR \| FORMATO_INVALIDO` | Delimitadores `$` ou `*` ausentes ou fora de ordem |
| `[RECEPTOR] ERR \| TAMANHO_CHECKSUM` | Checksum recebido não possui exatamente 2 caracteres |
| `[RECEPTOR] ERR \| CHECKSUM_DIVERGENTE` | Checksum recalculado difere do recebido (erro no barramento) |
| `[RECEPTOR] ERR \| IDENTIFICADORES_AUSENTES` | Campos `T:`, `;` ou `P:` ausentes no *payload* |
| `[RECEPTOR] ERR \| SENSOR_TEMP_INVALIDO` | Temperatura fora de `-55 °C` a `125 °C` (ex.: `-127 °C` com sensor solto) |
| `[RECEPTOR] ERR \| POT_FORA_FAIXA` | Ângulo fora da margem válida (`0°` a `300°`) |

---

## Estrutura do Repositório

```text
.
├── transmissor/
│   └── transmissor.ino      # Firmware da unidade transmissora (DS18B20 + ADC + RS-485)
├── receptor/
│   └── receptor.ino         # Firmware da unidade receptora (validação XOR + ponte USB)
├── web/
│   └── grafico.html         # Interface de monitoramento em tempo real (Web Serial API + Chart.js)
└── README.md
```

---

## Como Executar

### 1. Pré-requisitos

- **Arduino IDE** (ou PlatformIO) com suporte à placa **DOIT ESP32 DEVKIT V1** instalado.
- Bibliotecas instaladas no Gerenciador de Bibliotecas (para o transmissor):
  - `OneWire`
  - `DallasTemperature`
- Navegador baseado em Chromium (**Google Chrome** ou **Microsoft Edge**) com suporte à **Web Serial API**.

### 2. Gravação dos Firmwares

1. Conecte o **ESP32 Transmissor** via USB, abra `transmissor/transmissor.ino`, selecione a placa e a porta COM correspondente e faça o upload.
2. Conecte o **ESP32 Receptor** via USB, abra `receptor/receptor.ino` e faça o upload.
3. Feche o **Monitor Serial** da Arduino IDE na porta do receptor (a porta precisa estar livre para o navegador conectá-la).

### 3. Monitoramento em Tempo Real

1. Abra o arquivo `web/grafico.html` diretamente no Google Chrome ou Microsoft Edge.
2. Clique em **Conectar** e selecione a porta serial (`COM` / `/dev/ttyUSB*`) correspondente ao **ESP32 Receptor**.
3. Acompanhe em tempo real os cartões numéricos, estatísticas térmicas (mínima, média e máxima) e os gráficos de temperatura e posição angular.

---

## Resultados Obtidos

| Métrica | Valor Medido |
| :--- | :--- |
| **Tempo médio de ciclo** | `~532 ms` (`~1,88 amostras/s`) |
| **Tempo físico de envio (21 bytes a 9600 bps)** | `~21,9 ms` |
| **Ocupação do barramento RS-485 (com 4 ms de guarda)** | `< 26 ms` por ciclo (`~4,9%`) |
| **Teste de estabilidade contínua** | `1.353` pacotes válidos consecutivos com `0` erros |
| **Resolução térmica aplicada (DS18B20 10 bits)** | Passos de `0,25 °C` (`~187,5 ms` de conversão) |

---

## Autores

- **Lorena Strobel Campos**
- **Gabriel Siqueira Matias**

**Orientação:** Prof. Alberto Willian Mascarenhas — Disciplina de Microcontroladores, IFMT.