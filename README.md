## WOOT framework
Provo a fare un piccolo riassunto sul funzionamento e il data model.

L'idea é avere un framework che rispetti le tre seguenti proprieta (detto consistency model):
(1) Convergence: applicare lo stesso set di operazioni ad ogni sito, comporta ottenere lo stesso buffer tra tutti
(2) Intention Preservation: per ogni operazione, applicarla in ogni sito é lo stesso che applicarla nel luogo di partenza
    (i.e. : se ho "ab" da una parte e aggiungo 1 tra "a" e "b", voglio che l'operazione applicata su un altro buffer
    rispetti il fatto che 1 é tra "a" e "b", quella é l'intenzione.)
(3) Casuality Preservation: data qualunque coppia di operazioni Oa, Ob, se sono casualmente relazionate
    Oa -> Ob, allora deve essere che Oa viene eseguita prima di Ob.
    (i.e. nel nostro caso molto semplice la relazione Oa -> Ob, é una "temporale", spiego dopo cosa si intende con "tempo")

Il framework é strutturato attorno a due operazioni fondamentali:
- `ins(a < c < b)` inserisci il carattere `c` tra il carattere `a` e il carattere `b`
- `del(c)` elimina il carattere `c`. (l'eliminazione é particolare, vedi sotto)

queste operazioni differiscono dalle solite operazioni basate sulla posizione `ins(p, c)` e `del(p)` dove specifichiamo
la posizione del buffer in cui inserire o rimuovere un carattere.
Grande causa di problemi, come abbiamo visto, perche dobbiamo assicurarci che il buffer non cambi durante queste operazioni.

il problema di operazioni tipo `ins(a < c < b)` é che specificare "tra `a` e `b`" offre solamente un ordine parziale
(i.e. non posso stabilire se qualcosa viene prima dell'altro per ogni coppia di elementi. Ci saranno alcune coppie per le quali non posso dire nulla)
Per sopperire a questo problema il framework utilizza dei caratteri speciali, dotati di una sorta di id univoco. Grazie al quale possiamo, partendo
dall'ordine parziale indicato dalle "intenzioni" possiamo ricostruire invece un ordine lineare uguale per tutti.

Qui il primo (forse) pain point:
Il framework non agisce su buffer standard, ma agisce su una rappresentazione piu complessa dello stesso
(nel nostro caso dovremmo tenere un mirror del buffer reale e passare dall'uno all'altro)
Ogni elemento del buffer é detto un `W-Character` ed é una tupla di 5 elementi:
`<id, char, v, cp, cn>` dove:
-`id` é l'id univoco del nostro W-character
-`char` é il carattere alphanumerico vero e proprio (quello che forse vedremo nel buffer reale)
-`v` é la visibility flag (vedi sotto per discorso tombstone/del)
-`cp` é l'id univoco del W-character precedente della nostra intention (la "a" in a < c < b)
-`cn` é l'id univoco del W-character successivo della nostra intention (la "b" in a < c < b) 

piccolo detour ora sul discorso sul tempo e sull'id: Cosa é l'id di un character?
l'id di un character é una tupla `<ns, ng>` che dipende dal tempo e dal sito in cui il character é stato generato.
Ogni sito (o client) avra' un id univoce rappresentativo, e quello sara' `ns` 
###### IMPORTANTE
dobbiamo essere in grado di fare comparison tra questi id!
possono essere random, e ho notato che viene comodo se il server ha l'id piu basso (0?)
per questioni moleste che posso spiegare a voce.

Secondo: il tempo, `ng` rappresenta il tempo in cui é stato generato quel carattere, e altro non é che un contatore che viene incrementato ogni volta che aggiungiamo un carattere 
al buffer.
questo assicura che i caratteri sono davvero univoci.

##### pain point numero dos (forse):
la questione del delete e dell'approccio tombstone. come avete forse intuito, in questo modello non si cancella davvero, ma si "nasconde".
infatti applicare l'operazione di delete altro non é che flippare la flag `v` del carattere desiderato.
Quindi, dopo una sessione di lavoro il WOOT buffer conterra tutti i caratteri mai digitati in quella sessione.

Secondo me non é troppo problematico dato che non stiamo parlando di milioni di caratteri, ed in ogni caso é sempre possibile fare del "garbage collection" sul buffer
una volta ogni tanto. (é un problma in contesto puramente peer 2 peer, ma noi abbiamo un central server che puo fare da manager)

Detto cio, in funzionamento é molto semplice:
- un sito genera un operazione, la applica al suo stesso buffer e poi la broadcasta.
- gli altri siti ricevono l'operazione e la storano in un pool.
- con tutta la calma del mondo possono poi iterare dentro questo pool e applicare tutte le operazioni, e questo é molto importante, CHE SONO APPLICABILI.
cosa intendo con "sono applicabili"? supponi di avere il tuo buffer "ab"
performo due operazioni `ins(a < 1 < b)` e `ins(a < 3 < 1)` una dopo l'altra ottenendo "a31b".
queste due operazioni vengono mandate ad un altro client, ma lui le riceve al contrario:
dato che il suo buffer é ancora "ab" l'operazione `ins(a < 3 < 1)` non ha senso, dato che 1 non esiste ancora.
nessun problema, si skippa e si aspetta fino a che non si ottiene un operazione che aggiunge quell'uno (e di sicuro ad un certo punto arriva)
a quel punto posso riprovare ad applicare la prima operazione e sta volta funziona e viene piazzato al posto giusto.

La figata quale é? la promessa che se mi arrivano tutte le operazioni generate dagli altri, non importa in che ordine o con quanto tempo in mezzo
io saro sempre in grado di ricostruire il buffer come inteso. Le operazioni sono completamente indipendenti dallo stato del buffer quando arrivano.

## Codice
scusate per il cpp e se é mezzo incasinato, lo ho spalmato assieme durante un allnighter e alla fine stavo perdendo gli occhi e il cervello.
se volete provare a giocarci dovrete scaricare il sorgente e compilarlo con il vostro buildtool preferito,
é un progetto molto semplice con solo un file `main.cpp` dove faccio dei test e un file `types.cpp` + header dove implemento il framework grossolanamente.
nel file `main.cpp` ho fatto anche dei diagrammini che spiegano quello che sta succedendo e che mostrano la commutativita tra le varie combinazioni di operazioni.

