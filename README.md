# idealGasSim

Ideally will simulate gas
Nicco vieni dietro col portatile
nah, I don't feel like it

ONLY MEANINGFUL DESCRIPTIONS TO PUSHES!!!!

OK RAGAA QUESTO ESAME SI DA A GENNAIO

Thermodinamic coordinates could be implemented as another class (similar to sample/statistics in pf_lab3) instead of having methods of the class gas, then have a method that takes a  gas object and returns the coordinates relative to that gas.
In alternative just a loose function "coordinates" that does roughly the same thing.

## To do

- [x] Aggiungere costruttori che inseriscono solo particelle e costruttori che deleghino
- [ ] Scrivere gli unit tests
- [x] Capire ed implementare la gestione degli urti a piu di due particelle (facoltativo) -> NO
- [ ] Inserire una svalangata di assert una volta completata l'implementazione di base
- [ ] Exception handling
- [ ] Migliorare find_iteration()

## Da discutere nella prossima call

* Per l'attuale implementazione funziona l'operazione vector * scalar, ma non scalar * vector
* Implementazione con struct collsion?
    + Se si, allora facciamo un unica funzione collisionTime e viene overloadata 2 volte per le pareti e per le particelle
    + Se no 1, bisogna fare due funzioni diverse e gestire in qualche modo il passaggio degli oggetti che hanno colliso
    + Se no 2, ci sarà un unica funzione che gestisce la collisione degli oggetti con 2 cicli/2 algoritmi
* Quando usare i const
* Quale regole di naming abbiamo deciso



