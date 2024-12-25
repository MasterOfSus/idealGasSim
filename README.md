# idealGasSim

Ideally will simulate gas
Nicco vieni dietro col portatile
nah, I don't feel like it

ONLY MEANINGFUL DESCRIPTIONS TO PUSHES!!!!

OK RAGAA QUESTO ESAME SI DA A GENNAIO

Thermodinamic coordinates could be implemented as another class (similar to sample/statistics in pf_lab3) instead of having methods of the class gas, then have a method that takes a  gas object and returns the coordinates relative to that gas.
In alternative just a loose function "coordinates" that does roughly the same thing.

## To do

- [x] Unit test per for_any_couple()
- [x] Cambiare i nomi agli unitTest
- [ ] Exception handling
    - [ ] Throw quando c'è qualcosa che non va e fare il catch nel main
    - [x] std::invalid_argument per gli argomenti delle funzioni
- [x] Sistemare i test del costruttore del gas
- [x] Finire il secondo test carta e penna di collisionTime
- [x] Togliere i namespace extra
- [x] Implementare il gas loop con gli if
- [x] Scrivere firstWallCollision()
    * Implementare collisionTime con una sola particella
    * Implementare dei metodi getter delle coordinate dei muri e una funzione sempliche collisionTime che calcola, più bella ma da ottimizzzare di più
    * Implementare una funzione collisionTime che prende solo la lunghezza della scatola e trova i muri con una lambda
- [x] Scrivere test di firstWallCollision()
- [x] fix generazione particelle random
- [x] trasformare physvector in un template
- [ ] aggiungere una struct statistics che mi venga resistuita quando lancio il gasLoop
- [x] Valutare (e nel caso implementare) i membri pubblic const nella classe collision (const pointer) -> viene male
- [ ] Aggiungere uno sborobotto di test
- [ ] Pressione variazione quantità di moto su ogni muro
- [ ] Temperatura per il controllo
- [ ] CONTROLLA CHE L'ORDINE NON CAMBI se così tieni in memoria di gasLoop un vettore di lastPositions e calcola il libero cammino medio
- [ ] Aggiungere un gasLoop con un int tempo
- [ ] Aggiungere un gasLoop con un int tempo


## Da discutere nella prossima call

* Quando usare i const
* Quale regole di naming abbiamo deciso
* RandomVector


## Cosa discusse

* Usiamo per grafici e statistica
* NO BOTTONI in una user interface grafica
* Implementazione input con command line con librerie:
    * 
* Implementiamo scatola dentro classe gas
* Pubblic inheritance per la class collision


## Cosa da fare

Nicco input, output
Liam graphics, opzionalmente statistica
Diego physicsEngine, forse main

## Interface

using https://github.com/jarro2783/cxxopts library to manage command line options
* Una modalità tutti i parametri inseriti per quello che noi consideriamo standard
* Opzione --help, opzione di default
* Opzione --config, che fa partire il configuratore per la scelta dei dati di base
* Opzione --gui=false|true, per vedere tutta la roba di Liam
* Opzione --print, per printare in terminal con i valori
* Opzione --save=graphics,data , per salvare grafici e video?
