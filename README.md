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
- [ ] Sistemare i test del costruttore del gas
- [x] Finire il secondo test carta e penna di collisionTime
- [x] Togliere i namespace extra
- [x] Implementare il gas loop con gli if
- [ ] Valutare (e nel caso implementare) i membri pubblic const nella classe collision (const pointer)
- [ ] Provare a implementare la const reference nella class collision che poi si copia l'indirizzo per metterlo nel puntatore
- [x] Scrivere firstWallCollision()
    * Implementare collisionTime con una sola particella
    * Implementare dei metodi getter delle coordinate dei muri e una funzione sempliche collisionTime che calcola, più bella ma da ottimizzzare di più
    * Implementare una funzione collisionTime che prende solo la lunghezza della scatola e trova i muri con una lambda
- [ ] Scrivere test di firstWallCollision()
- [ ] Cambiare i wall char con degli enum

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

* Una modalità tutti i parametri inseriti per quello che noi consideriamo standard
* Opzione --help, opzione di default
* Opzione --config, che fa partire il configuratore per la scelta dei dati di base
* Opzione --gui=false|true, per vedere tutta la roba di Liam
* Opzione --print, per printare in terminal con i valori
* Opzione --save=graphics,data , per salvare grafici e video?
