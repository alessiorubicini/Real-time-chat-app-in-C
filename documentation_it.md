# Report di sviluppo e documentazione del software di chat in linguaggio C

## **Contenuti**

- [Requisiti del prodotto](#requisiti-del-prodotto)
- [Architettura del software](#Architettura-del-software)
- [Specifiche funzionali](#specifiche-funzionali)
- [Problemi riscontrati](#problemi-riscontrati)


## **Requisiti del prodotto**

È stato richiesto lo sviluppo di un software che permetta la chat in tempo reale tra due o più utenti in linguaggio C.


## **Mansionario**

**Alessio Rubicini**: sviluppo del server e delle routine per la gestione dei messaggi, pesanti bug fix al client e scrittura della documentazione   

**Andrea Malloni**: progettazione del software, sviluppo dell'interfaccia grafica, pesanti bug fix al client, scrittura della documentazione e ottimizzazioni varie

**Daniele Monaldi**: sviluppo del client, bug fix all'interfaccia e ottimizzazioni varie  

**Alessio Cinti**: sviluppo dell'interfaccia e bug fix vari

**Nicolas Burocchi**: stesura dei commenti e beta testing

---

## **Architettura del software**

Il software si basa sulla classica architettura Client/Server.  
Nel complesso, si compone di 3 file: *server.c*, *client.c* e *utilitychat.h*

### Server
Il Server gestisce l'intera piattaforma occupandosi della connessione/disconnessione dei client, del broadcast dei messaggi sulla chat e di parte della gestione dell'interfaccia del client.
Esso tiene traccia dei client collegati tramite una matrice 15x2, contenente i descrittori dei socket e i colori a loro assegnati (per un puro fattore estetico che verra' analizzato successivamente) espressi sotto forma di interi (1-15). Inoltre, una variabile intera memorizza il numero di client attualmente online.

Una volta inizializzato il socket del server su indirizzo IP locale e porta 8080, esso rimane in ascolto per la connessione dei client.
Quando un client si connette la matrice viene aggiornata memorizzando il descrittore della connessione e il suo corrispondente colore selezionato tra quelli disponibili. Viene poi avviato un thread il quale esegue la funzione di gestione del client *client_t*. In tal modo il server continua a rimanere in ascolto per ulteriori connessioni sulla porta 8080.

La funzione *client_t* esegue un ciclo infinito nel quale prova a leggere un messaggio in arrivo dal client.
In particolare, utilizzando la funzione *get_field* (contenuta nel file [utilityChat.h](#utilityChat.h)) effettua un controllo sul tipo di messaggio ricevuto.
- Se il messaggio è un comando di uscita (*exit*) il server procede con la disconnessione del client.
- Se il messaggio è invece un messaggio inviato sulla chat (*message*) lo reindirizza a tutti i client.


### Client
Il client è la parte del software che l'utente utilizza per collegarsi alla chat ed inviare/ricevere messaggi. Essendo l'input dei messaggi sul terminale un'operazione bloccante, è stata presa la decisione di adoperare dei thread per le operazioni di lettura e scrittura, in modo da rendere le due cose simultanee, e di sfruttare un'[interfaccia grafica](#Interfaccia-grafica) per la visualizzazione dei messaggi, poiche' in caso contrario il terminale classico avrebbe risscontrato dei bug.
Il codice si compone di 4 processi, il *main* e 3 *thread* (1 di scrittura, 1 di lettura e 1 di controllo). 

Il **main** è colui che si occupa di avviare l'interfaccia grafica e i 3 thread, per poi aspettarne il termine e ed uscire dal programma.

Il **sender** è invece colui che prende in input i messaggi dell'utente attraverso l'interfaccia grafica e li invia al server. Effettua inoltre controlli sulla grandezza del messaggio scritto e sul suo contenuto; se infatti esso è `/exit` il thread termina la sua esecuzione, dopo averlo comunicato al server.

Il **listener** legge continuamente messaggi dal server stampandoli nella finestra dell'interfaccia. Anch'esso effettua controlli sul tipo di messaggio e, come il sender, se riceve un messaggio di chiusura termina la sua esecuzione.

Il **watchdog** è infine un thread puramente di supporto in quanto l'unica cosa che fa è aggiornare la posizione del cursore durante l'input.

### Utilitychat.h
Il nome di questo file è autoesplicativo, infatti non è altro che una libreria contenente funzioni di utility utilizzate negli altri 2 file.  
Si compone di:
- Una sezione di inclusione di ulteriori librerie esterne
- Una sezione di definizione delle costanti.  
Tra queste troviamo i colori ANSI, i tipi di messaggio e la dimensione standard dei buffer.
- Una sezione di dichiarazione delle funzioni.  
Tra queste troviamo:  
 *get_msg*, la quale restituisce un messaggio nel formato **type|name|body|color** costruito sulla base delle stringhe date come parametri;   
*get_field*, la quale restituisce, dati un messaggio e una proprieta', 1 dei 4 campi del suddetto.

Informazioni piu' dettagliate riguardo il funzionamento e l'implementazione della struttura sono fornite nella sezione seguente.

---


## **Specifiche funzionali**

### Struttura dei messaggi

I messaggi in transito tra client e server sono stringhe suddivise in campi testuali da una pipeline (`|`). Sono generati dalla funzione *get_msg*, la quale necessita di 4 stringhe come parametri:  
1 tipo (type), 1 mittente (name), 1 corpo (body) e 1 colore (color). 
- **Tipo**: i due tipi di messaggio inviabili sono definiti in [utilityChat.h](#utilityChat.h) come `EXIT` e `MSG`, rispettivamente per messaggi di uscita o broadcast.
- **Mittente**: corrisponde al nome utente che il client inserisce al momento della connessione.
- **Corpo**: è la stringa che viene inserita dall'utente quando vuole scrivere un messaggio nella chat.
- **Colore**: è utilizzato durante la fase di stampa dei messaggi per fare in modo che sull'interfaccia i nomi utente dei client collegati vengano visualizzati con colori univoci. Ad ogni client viene assegnato un colore dal server, che lo sceglie tra quelli disponibili. 

La composizione avviene copiando tutti i parmaetri e le pipeline separatorie in un unico buffer tramite la funzione *strcat*. La dimensione del buffer è predefinita in [utilityChat.h](#utilityChat.h) ed è pari a 2KB. Inoltre, è stata fissata a priori la dimensione dello username, pari a 30 Byte, dei tipi di messaggio, pari a 15 Byte, del colore assegnato, pari a 2 Byte e infine del corpo, che si compone di tutto lo spazio rimanente.

### Connessione al server
La connessione è un'operazione piuttosto semplice da svolgere per il client in quanto non deve far altro che, dopo aver impostato tutti i relativi parametri del socket, eseguire la funzione *connect*, presupponendo che il server sia gia' in stato di ascolto. Se quest'ultima ha successo, il processo *main* del client chiede in input lo username dell'utente, che puo' essere al massimo di 30 caratteri, per poi avviare l'interfaccia grafica. Come ultima operazione vengono avviati i 3 thread che, utilizando la connessione appena generata, possono iniziare a svolgere il loro lavoro. Il *main* rimane quindi in attesa della loro terminazione.

### Disconnessione
Il client può disconnettersi dalla chat in 2 modi: 
- Digitando il comando `/exit` nella chat.
- Chiudendo forzatamente il terminale.

La seconda soluzione è molto estrema e ci si aspetta che venga poco utilizzata, tuttavia è funzionante. Chiudendo il terminale il processo client viene terminato forzatamente e la connessione chiusa, senza che il server venga avvertito, infatti quest'ultimo rimane perfettamente attivo. In fase di implementazione è stata riscontrata una problematica riguardante l'invio di un messaggio vuoto da parte del client in maniera involontaria durante la chiusura forzata che il server non era in grado di gestire, di conseguenza subiva un crash. Come soluzione, il client controlla la dimensione del corpo di qualsiasi messaggio in uscita ed impedisce l'invio di quelli vuoti, al fine di evitare equivoci accidentali. Come conseguenza, l'unico scenario plausibile in cui il server riceve un messaggio vuoto è quello di una chiusura forzata, che viene gestita disconnettendo automaticamente il client mittente di questo messaggio.


La prima soluzione è invece quella classica. L'utente non deve far altro che digitare `/exit` per terminare il programma. Il client, che controlla il corpo dei messaggi in uscita, si accorge di questo ed invia al server un messaggio di tipo `EXIT` (definito in [utilityChat.h](#utilityChat.h)) per comunicare la sua intenzione. Imposta inoltre la variabile globale `stop` a 1, cosa che fa automaticamente terminare il thread di monitoraggio *watchdog*, dopodiche' il thread mittente *sender* termina la sua esecuzione. Il server, ricevendo questo messaggio, realizza che un client si sta disconnettendo ed invia a sua volta un messaggio di uscita come conferma, per poi chiudere la connessione eliminando dalla matrice dei descrittori quello corrispondente al client disconnesso e rendendo di nuovo disponibile il colore che gli era stato assegnato. Dal lato client, il thread lettore *listener* riceve il messaggio di chiusura da parte del server e termina anch'esso la sua esecuzione. Terminati tutti i thread riprende l'attivita' del processo *main*, il quale distrugge le finestre dell'interfaccia grafica pulendo il terminale per poi ritornare 0. Questo scambio di conferme tra client e server è dovuto al fatto che i thread del processo client non comunicano fra di loro, quindi il server fa in un certo senso da "tramite".


Precisazione: la combinazione di tasti ctrl+c è stata disabilitata catturando il segnale `SIGINT` e gestendolo tramite una funzione vuota, rendendo disponibili per la chiusura solamente i 2 metodi sopra citati . 

### Scambio dei messaggi
A grandi linee lo scambio di messaggi tra i vari client avviene a partire da un utente, il quale manda un messaggio al server. Il server provvede poi a reindirizzare quel messaggio a tutti gli altri utenti connessi iterando sulla matrice dei descrittori dei socket.  
Lo svolgimento dettagliato di questa procedura è invece il seguente:
1. Il thread *sender* prende in input il messaggio dell'utente x attraverso l'interfaccia
2. Il thread effettua un controllo sull'input per verificare se il corpo inserito è vuoto o di uscita
3. Superati i controlli, il thread richiama la funzione di creazione del messaggio ed invia quest'ultimo al server.
4. Il server riceve il messaggio broadcast dall'utente x ed effettua anch'esso controlli sul tipo di messaggio e sulla sua dimensione
5. Se il messaggio ricevuto è valido viene iterata la matrice degli utenti connessi e il messaggio in questione viene reindirizzato a tutti
6. I *listener* dei client ricevono il messaggio reindirizzato ed effettuano controlli sul tipo
7. Se il messaggio è di broadcast, viene stampato sulla finestra di output


### Interfaccia grafica
Per la gestione dell'Input e dell'Output parallelo sono stati utilizzati dei thread, senza particolari problematiche. Tuttavia la visualizzazione dei messaggi nel terminale ha reso necessaria la presenza di un'interfaccia grafica che gestisse la cosa, dato che la concorrenza dei thread nell'utilizzo del terminale causava numerosi bug visivi. è stata utilizzata *curses.h*, una libreria per la gestione del terminale in sistemi UNIX-like.

In questo caso il terminale è stato "diviso" in 2 finestre:
- la parte superiore (*top*) dove vengono stampati i messaggi in arrivo dal server
- la parte inferiore (*bottom*) dove vengono presi in input i messaggi dell'utente

L'idea di base è la seguente: l'utente ha la possibilita' di scrivere i suoi messaggi nella finestra *bottom* quando vuole, utilizzando se necessario piu' di una riga, nel frattempo tutti i messaggi in transito nel gruppo vengono visualizzati nella finestra top in ordine di arrivo. Naturalmente, una volta inviato il messaggio, la finestra inferiore va pulita, mentre quella superiore, raggiunta la sua capienza massima, va fatta scorrere verso l'alto per consentire la stampa dei nuovi messaggi in arrivo.  

A livello di implementazione, l'interfaccia viene avviata e terminata nel processo *main*, rispettivamente con la creazione e la distruzione delle 2 finestre. Una volta create, queste vengono gestite da un unico thread ciascuno, che viene avviato successivamente nello stesso processo principale. Solamente quando i thread hanno tutti terminato la loro esecuzione il *main* è in grado di distruggere le finestre e terminare in seguito il programma.
- **gestione della finestra top**: la finestra superiore viene controllata unicamente dal thread *listener*. Una volta ricevuto un messaggio ed effettuati i vari controlli, questo viene stampato mediante la funzione *mvwprintw* nella finestra, specificando come coordinata y il valore della variabile `tline`, che rappresenta la linea vuota corrente. Una volta effettuata la stampa la variabile viene aggiornata, tuttavia, se il suo valore corrisponde alla fine della finestra, quest'ultima viene invece fatta scorrere tramite la funzione *scroll*. Inoltre, se il messaggio da stampare ha una dimensione maggiore della larghezza della finestra, viene segmentato in piu' righe stampate in successione secondo la medesima procedura. 
- **gestione della finestra bottom**: la finestra inferiore viene controllata invece dal thread *sender*. In questo caso pero' entra indirettamente in gioco anche il thread *watchdog*, in quanto: l'utente scrive il suo messaggio nella finestra *bottom* attraverso la funzione *mvwgetnstr*, la quale prende in input una stringa di dimensione pari a quella specificata, in questo caso 2KB. Essendo pero' una funzione bloccante, finche' l'utente non effettua l'invio del messaggio il *sender* non puo' effettuare altre operazioni, di conseguenza sta al *watchdog* controllare continuamente la posizione del cursore all'interno della finestra di inserimento e, se questo eccede la fine della riga, lo riporta a capo.
- **gestione dei colori**: i colori sono impiegati durante la stampa dei messaggi da parte del *listener*, per fare in modo che i nomi utente vengano visualizzati secondo il colore loro assegnato. Con la libreria *ncurses* i colori messi a disposizione sono 15, 7 sono i colori standard disponibili nella libreria mentre gli altri 7 sono le corrispettive versioni in grassetto, e vengono definiti nel *listener* attraverso la funzione *init_pair*, che associa un numero intero ad una coppia di colori (foreground/background). Quando il *listener* riceve un messaggio, il colore viene estratto e, dato che è di tipo char, viene effettuato un casting a intero, in maniera tale che quel numero possa essere utilizzato come riferimento alla coppia di colori. La libreria non permette tuttavia di stampare un messaggio parzialmente colorato, infatti occorre utilizzare la funzione *wattron* tramite la quale si può attivare/disattivare la modalità di stampa a colori. Mentre questa "modalità" è attiva ciò che viene stampato viene colorato secondo la coppia di colori specificata.

---

## **Problemi riscontrati**

### Ncurses e thread
Il problema maggiore nello sviluppo del software è stata la libreria *ncurses*.
La libreria, come scritto nella sezione precedente, non è thread-safe. Ciò crea dei conflitti nel client dato che è gestito da 2 thread diversi.
In particolare causava un'inizializzazione non corretta dell'interfaccia che appariva distorta all'avvio del programma per poi sistemarsi dopo il primo utilizzo.

