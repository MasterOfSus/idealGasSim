# idealGasSim

Ideally will simulate gas
Nicco vieni dietro col portatile
nah, I don't feel like it

ONLY MEANINGFUL DESCRIPTIONS TO PUSHES!!!!

OK RAGAA QUESTO ESAME SI DA A GENNAIO

Thermodinamic coordinates could be implemented as another class (similar to sample/statistics in pf_lab3) instead of having methods of the class gas, then have a method that takes a  gas object and returns the coordinates relative to that gas.
In alternative just a loose function "coordinates" that does roughly the same thing.

## To do

- [ ] Unit test per for_any_couple()
- [ ] Exception handling
    - [ ] Throw quando c'è qualcosa che non va e fare il catch nel main
    - [x] std::invalid_argument per gli argomenti delle funzioni
- [ ] Sistemare i test del costruttore del gas
- [ ] Togliere i namespace extra
- [ ] Implementare il gas loop con gli if


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
