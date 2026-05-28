# Global Solution 2026 — COA (Cápsula Espacial IoT)

Sistema IoT simulado no **Tinkercad** para monitoramento das condições internas de uma cápsula espacial, com **Arduino Uno**, exibindo telemetria em tempo real em **LCD 16x2** e acionando **alertas** (LCD + LED).

## Integrantes

- **Gabriel Barbosa Furin** — RM: **572941**
- **Lucas Kiodi Moraca** — RM: **571004**
- **Renan Fracalossi Mano da Silva** — RM: **569610**

## Links (entregáveis)

- **Simulação (Tinkercad):** https://www.tinkercad.com/things/7ZBku7a92Tj-global-solution-coa?sharecode=Ag-auWkIeHudWy2uvOhL03sFWdusYM7DWpYb_4HR0Iw
- **Vídeo (YouTube):** https://youtu.be/1qs2sT09RGM
- **Repositório (GitHub):** https://github.com/gabrielbfurin/COA-Global_Solution-2026

## Objetivo

Desenvolver um sistema embarcado (IoT) capaz de **coletar, processar e exibir em tempo real** as seguintes grandezas físicas, simulando o ambiente interno de uma cápsula espacial:

- **Temperatura**
- **Luminosidade**
- **Vibração** (simulada)

Além disso, o sistema deve **indicar condições de risco** por meio de alertas visuais.

## Componentes utilizados

- **Arduino Uno**
- **LCD 16x2** (modo 4 bits)
- **TMP36** (sensor de temperatura)
- **LDR (fotoresistor)** + **resistor 10kΩ** (divisor de tensão para luminosidade)
- **Potenciômetro** para **contraste** do LCD
- **Potenciômetro** para **simulação de vibração** (entrada analógica)
- **LED de alerta** + **resistor (≈ 220Ω)**
- Protoboard e jumpers

> Observação: a **vibração** foi representada por um **potenciômetro**, funcionando como um equivalente simulado (no lugar de um sensor real como piezoelétrico/acelerômetro) para validar a lógica de leitura, análise e disparo de alertas.

## Funcionamento (resumo)

- O Arduino lê:
  - **Temperatura (TMP36)** em **A0**
  - **Luminosidade (LDR)** em **A1** (via divisor de tensão)
  - **Vibração (potenciômetro)** em **A2** (simulação)
- O **LCD 16x2** exibe:
  - **Tela 1:** valores numéricos (telemetria) + status geral
  - **Tela 2:** diagnóstico compacto por sensor (OK/ALRT) + status geral
- O **LED de alerta** acende quando qualquer variável ultrapassa o limite configurado.

### Conversões e lógica de alerta

- TMP36: conversão para °C usando `(V - 0.5) * 100`
- Limites (thresholds) usados para o status:
  - Temperatura: **>= 30°C**
  - Luminosidade: **<= 150** (quanto menor, mais escuro)
  - Vibração (nível): **>= 700** (escala 0..1023)

## Evidências (prints)

### Circuito completo

![Circuito completo](docs/img/circuito_completo.png)

### Sistema em operação — Status OK

![Status OK](docs/img/status_ok.png)

### Sistema em operação — Status ALERTA

![Status ALERTA](docs/img/status_alerta.png)

## Como executar (no Tinkercad)

1. Acesse o link do projeto no Tinkercad.
2. Clique em **Iniciar simulação**.
3. Ajuste os componentes para testar:
   - Altere a temperatura no **TMP36**.
   - Aumente/diminua a luz no **fotoresistor (LDR)**.
   - Gire o potenciômetro de **vibração** para simular níveis maiores.
4. Verifique:
   - Mudança dos valores no **LCD**;
   - Mudança do status para **OK/ALRT**;
   - **LED** acendendo quando houver alerta.

## Estrutura sugerida do repositório

- `README.md`
- `src/` (código do Arduino)
- `docs/img/` (prints do circuito e da simulação)
- `docs/` (relatório PDF)

---

**FIAP — Global Solution 2026 (1º semestre) — Disciplina COA**
