# ERB PowerGuard • Sistema RTOS de Descarte de Cargas e Contingência Atmosférica

> **Projeto de Tempo Real para Estações Rádio-Base (ERBs)**  
> Disciplina de **Tópicos Especiais em Engenharia da Computação II (TEEC II)**  
> **Facens — Faculdade de Engenharia de Sorocaba**  
> **Orientador:** Prof. Marcos F. Jardini

[](https://www.st.com)
[](https://www.freertos.org)
[](https://developer.mozilla.org/en-US/docs/Web/API/Web_Serial_API)
[]()

---

## 📌 Visão Geral do Projeto

As Estações Rádio-Base (ERBs) de telecomunicações representam infraestruturas de missão crítica frequentemente instaladas em topos de morros e torres elevadas desassistidas, sujeitas tanto a **quedas na rede da concessionária de energia** quanto a **descargas atmosféricas diretas (raios)**.

Este projeto implementa um controlador redundante em tempo real baseado no microcontrolador **STM32F401CEU6 (Black Pill)** com o kernel **FreeRTOS (CMSIS-RTOS v2)**, capaz de executar simultaneamente:
1. **Descarte Escalonado de Cargas (*Soft Real-Time*):** monitoramento da tensão do banco de baterias (-48 V nominal) com corte ordenado de subsistemas para prolongar a sobrevida da antena de transmissão.
2. **Contingência e Proteção contra Surtos (*Hard Real-Time*):** preempção instantânea por interrupção de hardware do Dispositivo de Proteção contra Surtos (DPS), isolando todas as cargas em **Modo Casulo** por 3 segundos e gravando um snapshot na memória não-volátil do RTC.
3. **Telemetria Web em Tempo Real:** transmissão de pacotes JSON via porta serial nativa (**USB CDC**) diretamente para um navegador web via **Web Serial API**, dispensando drivers ou backends intermediários.

---

## ⚙️ Arquitetura de Hardware & Bancada

O circuito foi implementado em bancada didática com os seguintes periféricos:

| Pino STM32 | Periférico | Função no Sistema | Carga Simulada / Componente |
| :--- | :--- | :--- | :--- |
| **PA0** | `EXTI0` (Falling Edge) | Gatilho de Raio / Sensor DPS | Botão Push-Button com Pull-Up interno |
| **PA1** | `ADC1_IN1` (12 bits) | Sensor de Tensão de Bateria | Potenciômetro linear de 10 kΩ |
| **PA2** | `GPIO Output` | Carga 1: Antena de RF (Prioridade Máxima) | **LED Azul** + Resistor 1 kΩ |
| **PA3** | `GPIO Output` | Carga 2: Climatização / HVAC (Prioridade Média) | **LED Amarelo** + Resistor 1 kΩ |
| **PA4** | `GPIO Output` | Carga 3: Iluminação de Pátio (Prioridade Baixa) | **LED Vermelho** + Resistor 1 kΩ |
| **PA11 / PA12** | `USB_OTG_FS` | Comunicação USB CDC (Virtual COM Port) | Cabo USB Tipo-C nativo |
| **SWDIO / SWCLK**| `SWD` | Gravação e Depuração de Firmware | Gravador ST-Link V2 |

---

## ⚡ Matriz de Descarte de Cargas (*Load Shedding*)

A comutação das saídas obedece estritamente aos patamares de capacidade residual calculados a cada 200 ms:

| Nível de Bateria | Tensão Equiv. (-48V) | Estado do Sistema | Antena (PA2 - Azul) | Climatização (PA3 - Amarelo) | Iluminação (PA4 - Vermelho) | Comportamento Operacional |
| :---: | :---: | :---: | :---: | :---: | :---: | :--- |
| **> 50%** | > -48,0 V | **Regime Pleno** | 🟢 **ON** | 🟡 **ON** | 🔴 **ON** | Operação regular em 100% de capacidade. |
| **16% a 50%** | -44,0 V a -48,0 V | **Degrau Alerta** | 🟢 **ON** | 🟡 **ON** | ⚪ **OFF** | Desliga iluminação externa para economizar energia. |
| **4% a 15%** | -42,5 V a -44,0 V | **Degrau Crítico** | 🟢 **ON** | ⚪ **OFF** | ⚪ **OFF** | Desliga ar-condicionado; preserva apenas enlace de rádio. |
| **≤ 3%** | ≤ -42,4 V | **Desarme LVD** | ⚪ **OFF** | ⚪ **OFF** | ⚪ **OFF** | Desconexão por Subtensão (*Low Voltage Disconnect*). |
| **Sob Surto (PA0)**| Qualquer | **Modo Casulo** | ⚪ **OFF** | ⚪ **OFF** | ⚪ **OFF** | Isolamento total por 3 s para proteção contra transientes. |

---

## 🧵 Arquitetura do Kernel FreeRTOS

O firmware divide a CPU ARM Cortex-M4 (84 MHz) em três threads concorrentes com prioridades estritas:

```text
       [ Sensor DPS (PA0) ]              [ Potenciômetro (PA1) ]
                 |                                  |
            Interrupção EXTI0                  Amostragem ADC1 (200 ms)
                 |                                  |
                 v                                  v
       +--------------------+             +--------------------+
       |   taskSurge (RT)   |             |  taskBattery (Norm)|
       | Priority: Realtime |             +---------+----------+
       +---------+----------+                       |
                 |                       osMessageQueuePut (queueBattery)
                 |                                  |
                 |                                  v
                 |                        +--------------------+
                 +----------------------->|  taskPower (Above) |
                 | (surge_active flag)    | Atualiza Cargas    |
                 v                        +---------+----------+
       [ Caixa-Preta RTC BKP ]                      |
       [ Isolamento por 3 s  ]            CDC_Transmit_FS (USB)
                                                    |
                                                    v
                                         [ Dashboard Web Serial ]
```

### Detalhamento das Tarefas:

1. **`taskSurge` (Prioridade `osPriorityRealtime`, Stack: 1024 bytes):**
   * Desbloqueada imediatamente via `osThreadFlagsWait(0x01)` pela interrupção `EXTI0`.
   * Força desarme total preventivo de todos os pinos de saída em nível de hardware.
   * Incrementa o contador não-volátil nos registradores de backup (`RTC_BKP_DR1`).
   * Mantém o sítio em isolamento por 3.000 ms antes de devolver o controle operacional.
   * **Deadline garantido:** resposta medida de **2,58 µs** contra um deadline de **1,00 ms** (margem de segurança > 99,7%).

2. **`taskPower` (Prioridade `osPriorityAboveNormal`, Stack: 1024 bytes):**
   * Aguarda mensagens da fila `queueBatteryHandleHandle`.
   * Avalia a flag de contingência e aciona a matriz de chaveamento das cargas.
   * Formata e envia a string de telemetria JSON pela interface USB CDC.

3. **`taskBattery` (Prioridade `osPriorityNormal`, Stack: 512 bytes):**
   * Dispara a conversão de 12 bits do ADC1 a cada 200 ms (Soft Real-Time).
   * Normaliza o valor de 0 a 4095 para porcentagem (0 a 100%) e posta na fila do RTOS.

---

## 💻 Dashboard Web (Web Serial API)

A interface de monitoramento em tempo real roda diretamente no navegador, dispensando servidores backend ou instalação de drivers.

### Como Acessar:
* **Via GitHub Pages:** abra o link gerado pelo repositório (`https://<seu-usuario>.github.io/<repositorio>/`).
* **Localmente:** basta abrir o arquivo `index.html` no **Google Chrome**, **Microsoft Edge** ou **Opera**.

### Recursos da Interface:
* **Conexão Direta:** botão "Conectar STM32" usando a Web Serial API nativa a 115200 bps.
* **Modo Simulador Integrado:** permite testar e demonstrar toda a dinâmica de descarte e disparo de surtos sem necessidade da placa física conectada.
* **Curva Temporal Contínua:** gráfico interativo com histórico de descarga e demarcação de limiares.
* **Espelhamento Visual dos LEDs:** cards com cores fidedignas à bancada física (**Azul PA2**, **Amarelo PA3** e **Vermelho PA4**).
* **Terminal de Telemetria:** console com decodificação contínua dos pacotes JSON recebidos da controladora.

```json
// Pacote de telemetria periódico transmitido pela STM32:
{"bat": 75, "l1": 1, "l2": 1, "l3": 1, "casulo": 0}

// Pacote prioritário emitido sob descarga atmosférica:
{"event": "SURGE", "count": 2, "casulo": 1}
```

---

## 🛠️ Como Compilar e Gravar o Firmware

1. Abra o projeto no **STM32CubeIDE** (versão 1.14 ou superior).
2. Conecte o programador **ST-Link V2** nos pinos de depuração da Black Pill (`SWDIO`, `SWCLK`, `GND`, `3.3V`).
3. Conecte o cabo **USB Tipo-C** da placa em uma porta USB do computador.
4. Pressione **`Ctrl + B`** para compilar o projeto (verifique `0 errors, 0 warnings`).
5. Clique em **Run** (`Ctrl + F11`) para gravar o binário na memória Flash.
6. Pressione o botão físico **NRST** na Black Pill para iniciar a execução.

---

## 👥 Equipe de Desenvolvimento

* **André Siqueira**
* **Felipe Rusig de Paiva**
* **Guilherme Massayuki Yokoda de Moraes**
* **Leonardo Rossi de Oliveira**
* **Rafael Rocha Leite**
* **Tiago Tavares de Lima Gonçalves**

*Faculdade de Engenharia de Sorocaba (Facens) — 2026*
