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
- [x] aggiungere una struct statistics che mi venga resistuita quando lancio il gasLoop
- [ ] Pressione variazione quantità di moto su ogni muro
- [ ] implementare il calcolo della pressione su ogni lato
- [x] cambiare il modo in cui memoriziamo i muri enum class
- [x] Valutare (e nel caso implementare) i membri pubblic const nella classe collision (const pointer) -> viene male
- [ ] Aggiungere uno sborobotto di test
- [ ] Temperatura per il controllo
- [ ] CONTROLLA CHE L'ORDINE NON CAMBI se così tieni in memoria di gasLoop un vettore di lastPositions e calcola il libero cammino medio
- [ ] Aggiungere un gasLoop con un int tempo
- [ ] Decidere cosa fare con i metodi set, se per esempio mettere un setBoxSide in statistic o lasciarlo solo nel costruttore


Liam deve fare:

- [ ] Tests:
     - [x] Unit tests per RenderStyle
     - [x] Unit tests per Camera
     - [x] Unit tests per le funzioni di rendering
     - [ ] Unit tests per le funzioni di drawing - forse non fattibili ne' necessari
- [ ] Implementazioni:
     - [ ] Sistemare il disegno dei muri
     - [ ] Aggiungere gli assi (?)
     - [ ] Aggiungere la griglia (?)
     - [ ] Statistica e misure
     - [ ] Disegnare una finestra grafica
         

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

Using https://github.com/jarro2783/cxxopts library to manage command line options
follow https://github.com/jarro2783/cxxopts/wiki to implement
Using https://github.com/jtilly/inih/blob/master/test.ini library to manage config file input
* Una modalità tutti i parametri inseriti per quello che noi consideriamo standard
* Opzione -h|--help, mostra il messaggio di help
* (Opzione --usage, mostra il messaggio di usage)
* Opzione -c|--config, che fa partire il configuratore per la scelta dei dati di base   def OFF
* Opzione -g|--graphics, per vedere tutta la roba di Liam                               def OFF
* Opzione -p|--print, per printare in terminal con i valori                             def ON
* Opzione -s|--save=graphics,data , per salvare grafici e video?                        def OFF

launching with no options runs --help option

## Cose da chiedere al GIACOZ

- [ ] Mettere funzioni libere inline nei cpp da usare come pezzo di codice che sostituisce una espressione che verrebbe utilizzata piu volte per rendere il codice piu leggibile?
- [ ] Pratica di fare un header separato solo per i test in modo da riuscire a testare con funzioni che non servirebbero all'utente ma che servono a noi?
- [ ] Vuole uscire con noi a cena?
