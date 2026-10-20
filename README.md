# ESP32 AI Video Blackout Monitor 🎥🔍

[🇺🇸 Read in English](#english) | [🇧🇷 Ler em Português](#portuguese)

---

<a id="english"></a>
## 🇺🇸 English

### Overview
**ESP32 AI Video Blackout Monitor** (`ESP_AI_VIDEO`) is an embedded real-time computer vision and monitoring system built for the ESP32 microcontroller.

It operates as its own Wi-Fi Access Point and Web Server (with optional local Wi-Fi Station connectivity). An operator connects a smartphone directly to the ESP32's network and accesses the web dashboard via the mobile browser—**no internet connection or external server required.**

Through the web interface, the system streams live frames from the smartphone's rear camera to the ESP32. The microcontroller decodes and analyzes each frame in real time: if it detects a contiguous black region (such as a screen blackout of `50x50` up to `100x100+` pixels), it automatically triggers a **Stop**, records the exact stop timestamp, logs the event to a persistent CSV history, and saves the **last clean image captured immediately before the black square appeared**.

### Key Features
*   **📡 100% Standalone (Access Point Mode):** Broadcasts its own Wi-Fi network (`ESP32-Video-Monitor`) at `192.168.4.1`, while also supporting optional local Wi-Fi connection.
*   **📱 Mobile Rear Camera Integration:** Streams real-time video frames (`320x240` JPEG) directly from the smartphone's rear camera (`facingMode: "environment"`), with an interactive Start/Stop control panel.
*   **🧠 On-Device Computer Vision (`VideoAnalyzer`):** Uses the ESP32's native ROM/SDK JPEG decoder (`esp_jpg_decode`) combined with a static 1-bit blackness mask (`9.6 KB` RAM) and an $O(W \times H)$ Dynamic Programming (Maximal Square) algorithm to detect contiguous black squares (`50x50`, `100x100`, or custom sizes) in milliseconds.
*   **🖼️ Pre-Blackout Frame Preservation (`FrameStorage`):** Maintains a double-buffered copy of the latest valid (clean) frame in RAM and commits it to SPIFFS (`/last_valid.jpg`) only when a blackout occurs—preserving the exact image prior to the fault while protecting Flash memory from wear.
*   **⏱️ Automatic Browser RTC Synchronization (`TimeUtils`):** Synchronizes the ESP32 internal clock directly with the smartphone's browser timestamp (`/sync_time`), ensuring accurate Start and Stop timestamps even completely offline.
*   **📊 Persistent Stop History (`HistoryStorage`):** Records every automatic and manual stop in SPIFFS (`/stop_history.csv`), displaying a live history table on the dashboard with one-click Excel-compatible `.CSV` export and history clearing.
*   **🧪 Built-in Blackout Simulator:** Includes an interactive test toggle on the dashboard to inject a synthetic black square into the live video stream for instant validation.

### Clean Code Architecture
Following strict modular Clean Code principles, `main.cpp` is reserved exclusively for `setup()` and `loop()` orchestration, with each domain responsibility isolated in its own module:
*   `Config.h` — Centralized hardware pins, network constants, video thresholds, and SPIFFS paths.
*   `NetworkManager` — Wi-Fi Access Point and Station initialization.
*   `TimeUtils` — NTP and browser timestamp RTC synchronization.
*   `Logger` — Concurrent Serial and Telnet (port 23) logging.
*   `VideoAnalyzer` — JPEG decoding and Dynamic Programming maximal black square detection.
*   `FrameStorage` — In-memory pre-fault frame buffering and SPIFFS image persistence.
*   `SessionState` — Session state machine, timestamps, and sensitivity settings.
*   `HistoryStorage` — Persistent CSV stop log in SPIFFS and in-memory cache.
*   `WebRoutes` — HTTP endpoints and multipart frame ingestion.

### Technical Stack
*   **Hardware:** ESP32 (e.g., ESP32 DevKit V1, 4MB Flash).
*   **Firmware:** C++17 / Arduino Framework (PlatformIO).
*   **Web Server:** ESP32 `WebServer` (Port 80) & Telnet Server (Port 23).
*   **Storage:** SPIFFS for static web assets, pre-fault JPEG capture (`/last_valid.jpg`), and CSV history database (`/stop_history.csv`).
*   **Frontend:** Vanilla HTML5, CSS3, and JavaScript (Canvas API & MediaDevices API). Zero external CDNs.

### Installation (PlatformIO)
1. Clone this repository.
2. (Optional) Configure your Wi-Fi or Access Point credentials in `credentials.ini` (see `credentials.example`).
3. Upload the web interface files (`data/`) to SPIFFS:
   ```bash
   pio run --target uploadfs
   ```
4. Build and upload the firmware to the ESP32:
   ```bash
   pio run --target upload
   ```
5. Connect your smartphone to the Wi-Fi network `ESP32-Video-Monitor` (Password: `12345678`).
6. Open `http://192.168.4.1` in your mobile browser.
   * *Note for Android Chrome over HTTP:* Navigate to `chrome://flags/#unsafely-treat-insecure-origin-as-secure`, add `http://192.168.4.1`, set to **Enabled**, and relaunch Chrome to allow camera access on a local IP.

---

<a id="portuguese"></a>
## 🇧🇷 Português

### Visão Geral
O **ESP32 AI Video Blackout Monitor** (`ESP_AI_VIDEO`) é um sistema embarcado de visão computacional e monitoramento de vídeo em tempo real desenvolvido para o microcontrolador ESP32.

Ele atua como seu próprio Ponto de Acesso Wi-Fi (Access Point) e Servidor Web (com suporte opcional a conexão Wi-Fi local). O operador conecta o celular diretamente na rede Wi-Fi do ESP32 e acessa a interface pelo navegador — **sem depender de internet ou servidor externo.**

Pela interface web, o sistema acessa a câmera traseira do celular e transmite os frames em tempo real para o ESP32. O microcontrolador decodifica e interpreta o vídeo continuamente: caso identifique uma região em que a tela se apaga (um quadrado inteiramente preto de `50x50` até `100x100+` pixels), ele aciona o **Stop** automaticamente, marca o horário exato de parada, registra o evento no histórico persistente em CSV e salva a **última imagem limpa capturada imediatamente antes de o quadrado preto aparecer**.

### Principais Funcionalidades
*   **📡 100% Independente (Modo Access Point):** O ESP32 cria sua própria rede Wi-Fi (`ESP32-Video-Monitor`) no IP `192.168.4.1`, suportando também conexão simultânea a um roteador local.
*   **📱 Acesso à Câmera Traseira do Celular:** Captura e transmite frames em tempo real (`320x240` JPEG) diretamente da câmera traseira (`facingMode: "environment"`), com botões de **Iniciar** e **Parar** e indicadores de horário.
*   **🧠 Visão Computacional no ESP32 (`VideoAnalyzer`):** Utiliza o decodificador JPEG nativo do SDK do ESP32 (`esp_jpg_decode`) aliado a uma máscara binária estática (`9,6 KB` de RAM) e um algoritmo de Programação Dinâmica (Maximal Square) em tempo $O(W \times H)$ para detectar quadrados pretos contínuos (`50x50`, `100x100` ou tamanho configurável) em poucos milissegundos.
*   **🖼️ Preservação da Última Imagem Pré-Falha (`FrameStorage`):** Mantém em RAM o último frame válido (sem falha) e o grava na memória Flash SPIFFS (`/last_valid.jpg`) apenas no instante em que o apagão é detectado — preservando a imagem exata anterior ao problema e evitando desgaste da memória Flash.
*   **⏱️ Sincronização Automática de Horário (`TimeUtils`):** Sincroniza o relógio interno (RTC) do ESP32 diretamente com o horário do navegador do celular (`/sync_time`), garantindo precisão nos horários de Início e Parada mesmo em operação 100% offline.
*   **📊 Histórico Persistente de Paradas (`HistoryStorage`):** Salva todas as paradas automáticas e manuais no SPIFFS (`/stop_history.csv`), exibindo uma tabela na interface com opção de **Baixar CSV** (compatível com Excel) e **Limpar Histórico**.
*   **🧪 Simulador de Falha Integrado:** Conta com uma opção de teste na própria interface para injetar um quadrado preto no vídeo ao vivo e validar o disparo automático de parada a qualquer momento.

### Arquitetura Clean Code
Seguindo diretrizes estritas de Clean Code, o arquivo `main.cpp` é reservado exclusivamente para a orquestração do `setup()` e `loop()`, separando cada responsabilidade em seu próprio módulo:
*   `Config.h` — Centralização de pinos de hardware, constantes de rede, limites de vídeo e caminhos do SPIFFS.
*   `NetworkManager` — Gerenciamento da rede Wi-Fi (Access Point e Station).
*   `TimeUtils` — Sincronização de data e hora via NTP e via navegador.
*   `Logger` — Saída simultânea de logs via Serial e servidor Telnet (porta 23).
*   `VideoAnalyzer` — Decodificação JPEG e detecção do maior quadrado preto via Programação Dinâmica.
*   `FrameStorage` — Buffer em RAM do último frame limpo e persistência da imagem no SPIFFS.
*   `SessionState` — Máquina de estados da sessão, horários de início/parada e configuração de sensibilidade.
*   `HistoryStorage` — Armazenamento persistente do histórico de paradas em CSV no SPIFFS.
*   `WebRoutes` — Registro de todas as rotas HTTP e recepção de frames.

### Stack Tecnológica
*   **Hardware:** ESP32 (ex: ESP32 DevKit V1, mínimo de 4MB Flash).
*   **Firmware:** C++17 / Arduino Framework (PlatformIO).
*   **Servidor Web:** ESP32 `WebServer` (Porta 80) e Servidor Telnet (Porta 23).
*   **Armazenamento:** SPIFFS para arquivos da interface web, imagem pré-falha (`/last_valid.jpg`) e banco de dados CSV de histórico (`/stop_history.csv`).
*   **Frontend:** HTML5, CSS3 e JavaScript puros (Vanilla — Canvas API & MediaDevices API). Zero dependência de CDNs externas.

### Instalação (PlatformIO)
1. Clone este repositório.
2. (Opcional) Configure as credenciais de rede em `credentials.ini` (veja `credentials.example`).
3. Grave os arquivos da interface web (`data/`) na memória SPIFFS:
   ```bash
   pio run --target uploadfs
   ```
4. Compile e grave o firmware no ESP32:
   ```bash
   pio run --target upload
   ```
5. Conecte o celular à rede Wi-Fi `ESP32-Video-Monitor` (Senha: `12345678`).
6. Acesse `http://192.168.4.1` no navegador do celular.
   * *Dica para Chrome no Android via HTTP:* Acesse `chrome://flags/#unsafely-treat-insecure-origin-as-secure`, adicione `http://192.168.4.1`, altere para **Enabled** e reinicie o navegador para liberar o acesso à câmera em IP local.
