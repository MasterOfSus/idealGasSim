# idealGasSim

**idealGasSim** è un simulatore C++ di gas perfetto monoatomico in una scatola cubica, con visualizzazione 3D in tempo reale e analisi delle grandezze termodinamiche del sistema.

## Caratteristiche principali

- ⚛️ Simulazione di gas perfetto monoatomico con dinamica delle collisioni elastiche
- 🖥️ Visualizzazione grafica 3D tramite SFML
- 📈 Calcolo e salvataggio di statistiche termodinamiche (es. temperatura, energia, pressione)
- 🛠️ Configurazione parametrica tramite file `.ini`

## Compilazione

Assicurati di avere installato [SFML](https://www.sfml-dev.org/) nel tuo sistema.  
Per compilare il programma:

```bash
g++ -Wall -Wextra gasSim/graphics.cpp gasSim/physicsEngine.cpp gasSim/statistics.cpp gasSim/input.cpp gasSim/output.cpp main.cpp -o idealGasSim -lsfml-graphics -lsfml-window -lsfml-system
```

## Esecuzione

Esegui il simulatore con le opzioni desiderate. Un esempio consigliato:

```bash
./idealGasSim -t -c configs/gasSim_test.ini
```
