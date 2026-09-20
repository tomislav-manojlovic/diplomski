# 1. Uvod

Performanse računarskih programa ne zavise isključivo od broja operacija koje algoritam izvršava. Iako se u klasičnoj analizi najčešće posmatraju vremenska i prostorna složenost, na savremenim računarima značajan uticaj ima i način na koji program pristupa memoriji. Razlika između brzine procesora i glavne memorije ublažava se višeslojnom memorijskom hijerarhijom, u kojoj keš memorija omogućava da često korišćeni podaci budu dostupni uz manju latenciju [1], [2].

Efikasnost keša u velikoj meri zavisi od lokalnosti pristupa. Vremenska lokalnost podrazumeva ponovno korišćenje nedavno korišćenih podataka, dok prostorna lokalnost podrazumeva pristup podacima na bliskim memorijskim adresama [1]. Dve implementacije zato mogu imati istu asimptotsku složenost i veoma sličan broj aritmetičkih operacija, a ipak ostvarivati značajno različita vremena izvršavanja zbog različitog rasporeda i redosleda pristupa memoriji.

Ova činjenica motiviše primenu algoritama prilagođenih keš memoriji, odnosno *cache-aware* algoritama, i odgovarajućih tehnika optimizacije. Kod takvih pristupa uzimaju se u obzir karakteristike memorijske hijerarhije, kao što su veličina keša, veličina keš linije i raspored podataka. Cilj nije nužno smanjenje ukupnog broja računskih operacija, već njihova organizacija tako da se već učitani podaci bolje iskoriste i smanji potreba za pristupom sporijim nivoima memorije.

Predmet ovog rada je analiza uticaja memorijske hijerarhije i organizacije podataka na performanse algoritama, sa posebnim fokusom na *cache-aware* tehnike. Teorijski deo obuhvata osnovne principe rada keš memorije i tehnike kao što su promena redosleda pristupa, blokiranje, rekurzivna dekompozicija, reorganizacija podataka i unapredno učitavanje. Eksperimentalni deo najpre koristi jednostavne mikrotestove kako bi izolovano prikazao uticaj koraka pristupa, veličine radnog skupa i konfliktnih promašaja. Zatim se isti principi analiziraju na složenijim primerima: množenju matrica, algoritmima pretrage, heš tabelama i BFS obilasku grafova.

Eksperimenti su izvedeni na dve procesorske platforme različitih generacija. Pored vremena izvršavanja, za reprezentativne slučajeve korišćeni su i hardverski brojači performansi, uključujući broj procesorskih ciklusa i instrukcija, IPC, događaje vezane za keš memoriju i pogrešne predikcije grananja. Na taj način moguće je posmatrati ne samo koja je implementacija brža, već i koje promene u izvršavanju prate uočene razlike.

Cilj rada je da se pokaže da asimptotska složenost sama po sebi nije dovoljna za procenu praktičnih performansi na savremenom hardveru i da se utvrdi u kojim situacijama organizacija memorijskih pristupa donosi značajno poboljšanje. Nakon teorijske osnove i opisa metodologije, centralni deo rada predstavlja eksperimentalnu analizu, nakon koje se rezultati zajednički razmatraju i izvode zaključci.

# 2. Memorijska hijerarhija i keš memorija

Performanse savremenih računarskih sistema u velikoj meri zavise od brzine kojom procesor može da dobije podatke potrebne za izvršavanje instrukcija. Tokom razvoja računarskih sistema brzina procesora rasla je znatno brže od brzine pristupa glavnoj memoriji. Zbog toga direktan pristup operativnoj memoriji za svaku potrebnu vrednost ne bi omogućio efikasno korišćenje procesorskih resursa.

Ovaj problem ublažava se organizovanjem memorijskog sistema u više nivoa različite brzine, kapaciteta i cene [1], [2]. Manje i brže memorije nalaze se bliže procesoru, dok su veće i sporije memorije udaljenije. Takva organizacija naziva se **memorijska hijerarhija**.

Keš memorija ima centralnu ulogu u ovoj hijerarhiji. Njena osnovna svrha je da podatke koji će verovatno uskoro biti potrebni zadrži bliže procesoru i time smanji prosečno vreme pristupa memoriji. Efikasnost keš memorije zasniva se prvenstveno na principima vremenske i prostorne lokalnosti.

## 2.1. Memorijska hijerarhija

Tipičan savremeni računarski sistem sadrži više nivoa memorije. Posmatrano od procesora ka sporijim i većim memorijama, hijerarhija se može približno predstaviti sledećim redosledom:

1. registri procesora;
2. L1 keš memorija;
3. L2 keš memorija;
4. L3 keš memorija;
5. glavna memorija;
6. sekundarna memorija, kao što su SSD ili HDD uređaji.

Registri predstavljaju najbrži oblik memorije dostupan procesoru, ali je njihov broj veoma ograničen. U njima se čuvaju podaci koji se neposredno koriste prilikom izvršavanja instrukcija.

Ispod registara nalazi se keš memorija, koja je obično organizovana u nekoliko nivoa. L1 keš je najmanji i najbrži, dok naredni nivoi imaju veći kapacitet, ali i veću latenciju. U savremenim procesorima L1 keš se često deli na poseban keš za instrukcije i keš za podatke, dok L2 i L3 keš mogu imati drugačiju organizaciju u zavisnosti od arhitekture procesora.

Glavna memorija ima znatno veći kapacitet od keš memorije, ali je pristup podacima koji se u njoj nalaze sporiji. Sekundarna memorija nudi još veći kapacitet, ali sa još većom latencijom.

Osnovna ideja memorijske hijerarhije je da se najčešće korišćeni podaci drže u bržim nivoima memorije. Na taj način većina memorijskih pristupa može biti zadovoljena bez pristupa sporijim nivoima.

Memorijska hijerarhija prikazana je na slici 1.

[Ovde umetnuti odgovarajući grafički prikaz.]

**Slika 1. Memorijska hijerarhija savremenog računarskog sistema**

Ukoliko se traženi podatak nalazi u određenom nivou keša, kaže se da je došlo do **pogotka u kešu**. Ukoliko podatak nije prisutan, nastaje **promašaj u kešu**, nakon čega je potrebno potražiti ga u narednom nivou memorijske hijerarhije. Takav pristup ima veću cenu, posebno ukoliko je potrebno pristupiti glavnoj memoriji.

Zbog toga prosečno vreme pristupa memoriji ne zavisi samo od latencije pojedinačnih nivoa, već i od učestalosti pogodaka i promašaja u kešu. Program koji ostvaruje visok procenat pogodaka može znatno bolje da iskoristi procesor od programa koji često izaziva promašaje u kešu.

## 2.2. Princip lokalnosti

Efikasnost keš memorije zasniva se na činjenici da pristupi memoriji u realnim programima uglavnom nisu potpuno slučajni [1]. Programi imaju tendenciju da u kratkom vremenskom periodu ponovo koriste iste podatke ili podatke koji se nalaze na obližnjim memorijskim lokacijama. Ova osobina naziva se **lokalnost pristupa memoriji**.

Razlikuju se dva osnovna oblika lokalnosti: vremenska i prostorna lokalnost.

### 2.2.1. Vremenska lokalnost

**Vremenska lokalnost** podrazumeva da postoji velika verovatnoća da će podatak koji je nedavno korišćen ponovo biti korišćen u bliskoj budućnosti.

Jednostavan primer predstavlja promenljiva koja se više puta koristi unutar petlje. Nakon prvog učitavanja iz memorije ona može ostati u registru ili kešu, pa naredni pristupi ne zahtevaju ponovno čitanje iz sporijeg nivoa memorijske hijerarhije.

Sličan efekat pojavljuje se kada se isti deo niza ili druge strukture podataka više puta obrađuje pre nego što se pređe na drugi deo strukture. Upravo na tome se zasnivaju brojne *cache-aware* optimizacije.

### 2.2.2. Prostorna lokalnost

**Prostorna lokalnost** označava tendenciju programa da, nakon pristupa određenoj memorijskoj lokaciji, uskoro pristupi i lokacijama koje se nalaze u njenoj neposrednoj blizini.

Keš memorija ne prenosi pojedinačne bajtove ili elemente nezavisno, već podatke prenosi u blokovima fiksne veličine koji se nazivaju **keš linije**. Kada se jedan podatak učita u keš, zajedno sa njim se učitavaju i susedni podaci koji pripadaju istoj keš liniji.

Zbog toga sekvencijalni prolazak kroz niz obično veoma dobro koristi prostornu lokalnost. Nakon što je jedna keš linija učitana, više uzastopnih elemenata može se obraditi pre nego što bude potrebno učitati sledeću liniju.

Nasuprot tome, pristup elementima koji su međusobno udaljeni u memoriji može dovesti do toga da se iz svake učitane keš linije iskoristi samo mali deo podataka. Time se povećava broj memorijskih pristupa i smanjuje efikasnost keša.

Ovaj efekat posebno je značajan kod višedimenzionalnih nizova. U implementacijama korišćenim u ovom radu matrice su smeštene po vrstama u kontinualnom memorijskom prostoru. Prolazak kroz elemente jednog reda zato koristi susedne memorijske lokacije i ostvaruje dobru prostornu lokalnost, dok prolazak kroz kolone može dovesti do velikih skokova između uzastopnih adresa i većeg broja promašaja u kešu.

Princip lokalnosti predstavlja osnovu *cache-aware* optimizacija analiziranih u ovom radu. Cilj takvih optimizacija je da se redosled operacija ili raspored podataka prilagodi tako da se već učitani podaci što bolje iskoriste pre nego što budu izbačeni iz keša.

## 2.3. Keš linije

Osnovna jedinica prenosa između keša i narednog nivoa memorije nije pojedinačni podatak, već **keš linija** [1], [2]. Njena veličina zavisi od arhitekture procesora, a kod velikog broja savremenih procesora iznosi 64 bajta.

Ako program, na primer, pristupi jednom četvorobajtnom celom broju koji trenutno nije prisutan u kešu, procesor neće učitati samo ta četiri bajta. Umesto toga, učitava se čitava keš linija koja sadrži traženi element.

Kod niza elemenata tipa `int` veličine četiri bajta, jedna keš linija od 64 bajta može sadržati 16 uzastopnih elemenata. Ukoliko program pristupa elementima redom, jedno učitavanje keš linije može omogućiti pristup većem broju elemenata bez dodatnog pristupa narednom nivou memorijske hijerarhije.

Sa druge strane, ako program iz svake keš linije koristi samo jedan element, većina prenetih podataka ostaje neiskorišćena. Zbog toga **korak pristupa** kroz niz može imati veliki uticaj na performanse.

Na primer, kod sekvencijalnog pristupa `A[0], A[1], A[2], A[3], ...` uzastopni pristupi dugo koriste podatke iz iste keš linije. Kod većeg koraka, kao u obrascu `A[0], A[16], A[32], A[48], ...`, svaki pristup može zahvatiti novu keš liniju, u zavisnosti od veličine elementa i linije.

Povećanje koraka pristupa zbog toga može značajno smanjiti prostornu lokalnost. Ovaj efekat će kasnije biti eksperimentalno analiziran pomoću mikrotesta koraka pristupa.

## 2.4. Organizacija i mapiranje keš memorije

Keš memorija ima znatno manji kapacitet od glavne memorije. Zbog toga nije moguće proizvoljno smestiti svaki blok glavne memorije na bilo koju lokaciju u kešu. Neophodan je mehanizam kojim se određuje gde određeni memorijski blok može biti smešten.

Prema načinu mapiranja razlikuju se tri osnovne organizacije:

* direktno mapirani keš;
* potpuno asocijativni keš;
* skupovno asocijativni keš.

### 2.4.1. Direktno mapirani keš

Kod direktno mapiranog keša svaki blok glavne memorije može biti smešten na samo jednu određenu poziciju u kešu. Pozicija se određuje na osnovu adrese bloka.

Prednost ovakve organizacije je jednostavnost i brzo određivanje mesta na kome treba tražiti podatak. Međutim, dva različita memorijska bloka mogu biti mapirana na istu poziciju. Ako program naizmenično pristupa takvim blokovima, oni mogu neprestano izbacivati jedan drugog iz keša čak i kada u ostatku keša postoji dovoljno slobodnog prostora.

Takvi promašaji nazivaju se **konfliktni promašaji**.

### 2.4.2. Potpuno asocijativni keš

Na drugom kraju spektra nalazi se potpuno asocijativni keš, kod koga svaki memorijski blok može biti smešten na bilo koju lokaciju.

Ovakav pristup značajno smanjuje problem konflikata, ali zahteva složeniji hardver jer je prilikom pristupa potrebno proveriti više mogućih lokacija. Zbog toga potpuna asocijativnost nije praktična za velike nivoe keš memorije.

### 2.4.3. Skupovno asocijativni keš

Savremeni procesori najčešće koriste kompromis između prethodna dva pristupa, odnosno **skupovno asocijativni keš** (*set-associative cache*) [2].

Keš je podeljen na određeni broj **skupova**, a svaki skup sadrži više keš linija. Memorijski blok se mapira na tačno jedan skup, ali unutar njega može biti smešten u bilo koju od raspoloživih linija.

Ako svaki skup sadrži \(k\) linija, kaže se da je keš \(k\)-struko skupovno asocijativan.

Na primer, kod osmostruko skupovno asocijativnog keša svaki memorijski blok pripada jednom određenom skupu, ali može zauzeti jednu od osam linija u tom skupu. To znači da do osam blokova koji se mapiraju u isti skup mogu istovremeno biti prisutni u kešu. Kada je potrebno smestiti još jedan blok u već popunjen skup, jedna od postojećih linija mora biti zamenjena.

Broj skupova može se izračunati na osnovu ukupne veličine keša, veličine keš linije i stepena asocijativnosti:

$$
N_{skup} =
\frac{C}
{L \cdot A}
$$

gde je:

* \(C\) ukupni kapacitet keša,
* \(L\) veličina keš linije,
* \(A\) stepen asocijativnosti.

Na primer, za keš veličine 32 KiB, sa keš linijom od 64 bajta i asocijativnošću 8, broj skupova iznosi:

$$
N_{skup} =
\frac{32 \cdot 1024}
{64 \cdot 8}
= 64.
$$

Svaka memorijska adresa može se logički podeliti na tri dela:

* **pomak** (*offset*), kojim se bira bajt unutar keš linije;
* **indeks** (*index*), kojim se određuje skup;
* **oznaka** (*tag*), kojom se razlikuju različiti memorijski blokovi mapirani u isti skup.

Podela memorijske adrese na oznaku, indeks skupa i pomak prikazana je na slici 2.

[Ovde umetnuti odgovarajući grafički prikaz.]

**Slika 2. Podela memorijske adrese na oznaku, indeks skupa i pomak unutar keš linije**

Za liniju od 64 bajta potrebno je šest bitova za pomak, jer je \(64 = 2^6\). Ako keš ima 64 skupa, još šest bitova koristi se za indeks skupa. Preostali viši bitovi adrese čine oznaku.

Ovakva organizacija ima važnu posledicu za performanse programa. Ako se često koriste memorijske lokacije koje se mapiraju u isti skup, a njihov broj premaši stepen asocijativnosti, može doći do čestog međusobnog izbacivanja linija. Tada nastaju konfliktni promašaji čak i kada ukupan radni skup nije veći od ukupnog kapaciteta keša.

Ovaj fenomen je posebno značajan kod pravilnih obrazaca pristupa memoriji, gde određeni razmaci između adresa mogu dovesti do sistematskog mapiranja podataka u iste skupove. U eksperimentalnom delu rada ovaj efekat biće demonstriran posebnim mikrotestom kojim se povećava broj memorijskih lokacija mapiranih u isti skup.

## 2.5. Vrste promašaja u keš memoriji

Nisu svi promašaji u kešu posledica istog uzroka. Klasično se razlikuju tri osnovne kategorije [2]: **obavezni promašaji**, **promašaji kapaciteta** i **konfliktni promašaji**.

### 2.5.1. Obavezni promašaji

**Obavezni promašaj** (*compulsory miss*) nastaje prilikom prvog pristupa određenoj keš liniji. Pošto podatak prethodno nije korišćen, ne može već biti prisutan u kešu i mora biti učitan iz nižeg nivoa memorijske hijerarhije.

Ovakvi promašaji ne mogu se u potpunosti eliminisati povećanjem kapaciteta ili asocijativnosti keša. Njihov efekat, međutim, može biti ublažen dobrim iskorišćavanjem prostorne lokalnosti. Ako se nakon prvog promašaja iskoristi više podataka iz upravo učitane keš linije, cena njenog učitavanja raspoređuje se na više korisnih pristupa.

### 2.5.2. Promašaji kapaciteta

**Promašaji kapaciteta** (*capacity misses*) nastaju kada količina aktivno korišćenih podataka premašuje kapacitet određenog nivoa keša. Podatak koji je ranije bio učitan tada može biti izbačen pre nego što program ponovo pristupi njemu.

Skup podataka koji je programu aktivno potreban tokom određenog dela izvršavanja često se naziva **radni skup** (*working set*). Ako se radni skup uklapa u određeni nivo keša, veliki deo pristupa može biti zadovoljen iz tog nivoa. Kada njegova veličina pređe kapacitet keša, broj promašaja može značajno porasti.

Zbog toga performanse nekog algoritma mogu ostati relativno stabilne za manje veličine problema, a zatim se pogoršati kada struktura podataka prestane da staje u L1, L2 ili neki drugi nivo keša. Upravo se ovaj efekat ispituje mikrotestom radnog skupa u eksperimentalnom delu rada.

### 2.5.3. Konfliktni promašaji

**Konfliktni promašaji** (*conflict misses*) posledica su načina mapiranja memorijskih blokova u skupove keša. Oni mogu nastati čak i kada ukupan broj korišćenih podataka ne premašuje kapacitet keša.

Ako se više aktivno korišćenih blokova mapira u isti skup, a njihov broj premaši asocijativnost tog skupa, blokovi počinju međusobno da se izbacuju. Na primer, kod osmostruko skupovno asocijativnog keša u jednom skupu istovremeno može biti smešteno najviše osam odgovarajućih linija. Pristup devetom aktivnom bloku koji pripada istom skupu zahteva izbacivanje jedne od prethodnih linija.

Ovaj problem pokazuje da nije dovoljna samo ukupna veličina radnog skupa. Važan je i način na koji su podaci raspoređeni u memoriji i način na koji se pristupa njihovim adresama.

Podela promašaja na ove kategorije korisna je prilikom analize performansi, jer različite *cache-aware* tehnike utiču na različite uzroke promašaja. Blokiranje, na primer, može smanjiti probleme vezane za kapacitet tako što ograničava aktivni radni skup, dok promena rasporeda podataka može smanjiti konfliktne promašaje i poboljšati prostornu lokalnost.

## 2.6. Politike zamene keš linija

Kada je potrebno učitati novu liniju u skup koji je već popunjen, hardver mora da odluči koja od postojećih linija će biti izbačena. Mehanizam kojim se donosi ova odluka naziva se **politika zamene** (*replacement policy*).

Idealna politika bi izbacila podatak koji najduže neće biti ponovo korišćen [2]. Takva odluka zahtevala bi poznavanje budućeg ponašanja programa, što u praksi nije moguće. Zbog toga procesori koriste različite heuristike koje pokušavaju da približno odrede koji podatak je najmanje koristan zadržati.

Jedna od najpoznatijih politika je **LRU** (*Least Recently Used*), kod koje se bira linija koja najduže nije korišćena. Ova politika se zasniva na principu vremenske lokalnosti: ako podatak dugo nije korišćen, pretpostavlja se da je manja verovatnoća da će uskoro ponovo biti potreban.

Precizna implementacija LRU politike postaje složena kada skup sadrži veći broj linija, jer hardver mora da prati njihov relativni redosled korišćenja. Zbog toga stvarni procesori često koriste aproksimacije LRU-a ili druge politike čije je ponašanje moguće implementirati uz manji hardverski trošak.

Druga jednostavna mogućnost je **FIFO** (*First In, First Out*), kod koje se izbacuje linija koja se najduže nalazi u skupu, bez obzira na to koliko je nedavno korišćena. Moguće su i pseudo-nasumične politike, kao i složeniji mehanizmi prilagođeni karakteristikama konkretne mikroarhitekture.

Za *cache-aware* programiranje nije uvek potrebno poznavati tačnu politiku zamene određenog procesora. Mnogo je važnije izbegavati obrasce pristupa kod kojih veliki broj aktivnih linija konkuriše za mali broj pozicija u istom skupu. Ako algoritam zavisi od vrlo specifičnog ponašanja politike zamene, njegove performanse mogu biti manje predvidive na različitim procesorima.

## 2.7. Čitanje i pisanje podataka

Prilikom čitanja podatka procesor najpre proverava da li je odgovarajuća keš linija prisutna u kešu. Ako jeste, podatak se dobija iz keša. U suprotnom dolazi do promašaja i potrebna linija se učitava iz nižeg nivoa memorijske hijerarhije.

Pisanje je složenije, jer promenjena vrednost u jednom trenutku mora biti usklađena sa kopijama podatka u drugim nivoima memorijskog sistema [2]. Dva osnovna pristupa poznata su kao **write-through** i **write-back**.

Kod pristupa *write-through* svaka promena u kešu istovremeno se prosleđuje i sledećem nivou memorije. Ovakav pristup pojednostavljuje održavanje konzistentnosti, ali može proizvesti veliki broj upisa u sporije nivoe memorijske hijerarhije.

Kod pristupa *write-back* promena se najpre vrši samo u kešu. Keš linija se označava kao izmenjena, odnosno *dirty*, a njen sadržaj se upisuje u niži nivo memorije tek kada linija treba da bude izbačena. Na taj način više uzastopnih promena iste linije može biti objedinjeno u jedan kasniji memorijski upis.

Pored toga, prilikom promašaja kod operacije pisanja potrebno je odlučiti da li će linija prvo biti učitana u keš. Kod politike *write-allocate*, promašaj pri pisanju dovodi do učitavanja odgovarajuće linije u keš, nakon čega se promena izvršava nad njom. Kod politike *no-write-allocate* zapis se prosleđuje ka nižem nivou bez obaveznog smeštanja linije u trenutni keš.

Konkretne kombinacije ovih politika zavise od nivoa keša i procesorske arhitekture. Sa stanovišta performansi algoritma, međutim, najvažnije je da veliki broj memorijskih upisa takođe može generisati značajan memorijski saobraćaj. Optimizacije tipa *cache-aware* zbog toga ne razmatraju samo redosled čitanja podataka, već i način na koji se rezultati menjaju i ponovo koriste.

Na primer, ako se određeni element rezultujuće matrice više puta ažurira dok se još nalazi u kešu, vremenska lokalnost omogućava da veliki broj operacija bude izvršen bez ponovnog pristupa glavnoj memoriji. Ako se između dva ažuriranja izvrši obrada velike količine drugih podataka, odgovarajuća linija može biti izbačena i kasnije ponovo učitana.

## 2.8. Hardversko unapredno učitavanje podataka

Do sada je razmatran slučaj u kome procesor reaguje tek kada program zatraži podatak koji se ne nalazi u kešu. Savremeni procesori, međutim, pokušavaju da predvide buduće memorijske pristupe i potrebne podatke učitaju unapred [2]. Ovaj mehanizam naziva se **unapredno učitavanje** (*prefetching*).

Hardverski mehanizam za unapredno učitavanje prati obrasce memorijskih pristupa koje program generiše. Kada prepozna dovoljno pravilan obrazac, može unapred zatražiti naredne keš linije i time započeti njihovo učitavanje pre nego što procesoru zaista budu potrebne. Sekvencijalni ili pravilno razmaknuti pristupi posebno su pogodni za ovakav mehanizam.

Unapredno učitavanje ne smanjuje nužno latenciju samog pristupa memoriji. Njegov cilj je da tu latenciju sakrije tako što učitavanje započinje dovoljno rano i preklapa ga sa drugim operacijama procesora. Kod nepravilnih pristupa, naročito kada naredna adresa zavisi od prethodno pročitanog podatka, predviđanje je znatno teže.

Pogrešno unapredno učitavanje može nepotrebno povećati memorijski saobraćaj i izbaciti korisne linije iz keša. Pored automatskog hardverskog mehanizma, program može eksplicitno zatražiti ranije učitavanje određene adrese. Takvo **softversko unapredno učitavanje** koristi se i u eksperimentu sa Eytzinger rasporedom, gde je buduću lokaciju moguće izračunati dovoljno rano.

## 2.9. Značaj memorijskog ponašanja za performanse algoritama

Prethodno opisani mehanizmi pokazuju da cena memorijskog pristupa nije konstantna. Ista instrukcija učitavanja podatka može imati veoma različitu cenu u zavisnosti od toga da li se podatak nalazi u L1, L2 ili L3 kešu, ili ga je potrebno dobaviti iz glavne memorije.

Zbog toga broj memorijskih pristupa sam po sebi ne opisuje u potpunosti ponašanje algoritma. Važno je i koje adrese se posećuju, kojim redosledom, koliko često se isti podaci ponovo koriste i koliko efikasno algoritam koristi podatke koji su preneti u jednoj keš liniji.

Na performanse dodatno utiču kapacitet pojedinačnih nivoa keša, njihova asocijativnost, politika zamene i sposobnost procesora da unapred predvidi buduće pristupe memoriji. Posledica je da dve implementacije sa istom asimptotskom složenošću, pa čak i sa veoma sličnim brojem izvršenih instrukcija, mogu imati značajno različito vreme izvršavanja.

Pristup *cache-aware* projektovanju algoritama pokušava da iskoristi ove osobine memorijskog sistema. Umesto posmatranja memorije kao uniformnog skladišta sa približno jednakom cenom svakog pristupa, raspored podataka i redosled operacija prilagođavaju se karakteristikama memorijske hijerarhije.

U narednom poglavlju predstavljene su najvažnije tehnike kojima se to može postići, uključujući promenu redosleda pristupa podacima, blokiranje, reorganizaciju rasporeda podataka, rekurzivnu dekompoziciju i unapredno učitavanje.

# 3. *Cache-aware* algoritmi i tehnike optimizacije

Klasična analiza algoritama uglavnom pretpostavlja da svaki pristup memoriji ima približno istu cenu. Takav model je koristan za procenu asimptotske složenosti, ali ne opisuje u potpunosti ponašanje programa na savremenim računarima. Kao što je prikazano u prethodnom poglavlju, memorijski sistem je hijerarhijski organizovan, pa cena pristupa podatku zavisi od toga na kom nivou hijerarhije se on trenutno nalazi.

Algoritmi tipa *cache-aware* projektuju se uz eksplicitno uzimanje u obzir karakteristika keš memorije i memorijske hijerarhije. Cilj je da se poveća lokalnost pristupa podacima, smanji broj skupih memorijskih transfera i već učitani podaci iskoriste što veći broj puta pre nego što budu izbačeni iz keša.

Takve optimizacije ne moraju menjati asimptotsku složenost algoritma. Dve implementacije mogu izvršavati isti red veličine operacija, pa čak i gotovo isti broj aritmetičkih operacija, ali ostvarivati različite performanse zbog drugačijeg rasporeda i redosleda pristupa memoriji.

U ovom poglavlju predstavljene su najvažnije tehnike koje se koriste za poboljšanje ponašanja algoritama u memorijskoj hijerarhiji. Poseban akcenat stavljen je na tehnike koje će kasnije biti analizirane kroz eksperimente u ovom radu.

## 3.1. *Cache-aware* pristup

Kod *cache-aware* algoritma odluke o organizaciji računanja donose se uz poznavanje ili pretpostavku određenih karakteristika memorijskog sistema. To mogu biti, na primer:

* veličina keš memorije;
* veličina keš linije;
* broj nivoa keša;
* stepen asocijativnosti;
* način rasporeda podataka u memoriji.

Jedan od najjednostavnijih primera je izbor veličine bloka kod blokiranog množenja matrica. Blokovi se biraju tako da odgovarajući delovi matrica mogu dovoljno dugo da ostanu u kešu i budu ponovo korišćeni pre nego što ih zamene drugi podaci.

Osnovna ideja *cache-aware* optimizacije može se svesti na nekoliko opštih principa:

1. pristupati podacima što je moguće sekvencijalnije;
2. ponovo koristiti podatke dok su još prisutni u kešu;
3. ograničiti veličinu aktivnog radnog skupa;
4. izbegavati nepotrebne skokove između udaljenih memorijskih lokacija;
5. organizovati podatke tako da elementi koji se zajedno koriste budu blizu jedni drugima u memoriji.

Ne postoji jedna tehnika koja daje najbolje rezultate za sve probleme. Efekat optimizacije zavisi od algoritma, veličine podataka, arhitekture procesora i načina na koji kompajler generiše mašinski kod.

## 3.2. Promena redosleda pristupa podacima

Jedna od najjednostavnijih *cache-aware* tehnika jeste promena redosleda kojim se podaci obrađuju.

Ako su elementi strukture raspoređeni uzastopno u memoriji, algoritam koji ih posećuje istim redosledom bolje koristi prostornu lokalnost od algoritma koji između pristupa pravi velike skokove. Ova promena često ne utiče na rezultat izračunavanja niti na broj osnovnih operacija, ali može značajno promeniti broj promašaja u kešu.

Tipičan primer predstavlja množenje matrica. Za matrice \(A\), \(B\) i \(C\), standardni izraz je

$$
C_{ij} = \sum_{k=0}^{N-1} A_{ik} B_{kj}.
$$

Implementacija sa redosledom petlji \(i\)-\(j\)-\(k\) za svaki element rezultata prolazi kroz jednu vrstu matrice \(A\) i jednu kolonu matrice \(B\). Ako su matrice u memoriji smeštene po vrstama, pristup elementima matrice \(B\) tada uključuje skokove između udaljenih memorijskih lokacija.

Promenom redosleda petlji, na primer na \(i\)-\(k\)-\(j\), moguće je postići da se unutrašnja petlja kreće kroz susedne elemente matrica. Time se znatno bolje koristi sadržaj već učitanih keš linija.

Važno je primetiti da obe implementacije i dalje imaju vremensku složenost

$$
O(N^3),
$$

ali njihovo stvarno vreme izvršavanja može biti veoma različito.

Ovaj primer ilustruje važnu osobinu *cache-aware* optimizacija: poboljšanje performansi često se postiže ne smanjenjem broja računskih operacija, već promenom redosleda u kome se postojeće operacije izvršavaju.

## 3.3. Blokiranje

Jedna od najpoznatijih *cache-aware* tehnika je **blokiranje** (*blocking* ili *tiling*) [1]. Osnovna ideja je da se veliki problem podeli na manje delove koji mogu efikasnije da stanu u keš.

Umesto da se čitava struktura podataka obilazi u jednom prolazu, algoritam najpre obrađuje mali deo podataka i pokušava da ga maksimalno iskoristi pre nego što pređe na sledeći deo.

Kod množenja matrica matrice se mogu podeliti na kvadratne blokove dimenzija \(B \times B\). Računanje se zatim organizuje tako da se manji blokovi matrica više puta koriste dok su još prisutni u kešu.

U pojednostavljenom obliku, umesto rada nad celim matricama dimenzija \(N \times N\), obrada se vrši nad podmatricama:

$$
A_{block}, B_{block}, C_{block}.
$$

Ako je veličina bloka dobro odabrana, odgovarajući delovi tri matrice mogu istovremeno da se nalaze u brzom nivou keša. Time se povećava vremenska lokalnost, jer se isti elementi koriste više puta pre nego što budu izbačeni.

Izbor veličine bloka predstavlja važan deo ove optimizacije. Premali blokovi mogu dovesti do dodatnog troška petlji i ograničiti mogućnosti optimizacije kompajlera. Preveliki blokovi mogu premašiti kapacitet keša i time umanjiti osnovnu prednost blokiranja.

Optimalna veličina bloka zato zavisi od hardvera, veličine elemenata, broja istovremeno aktivnih nizova i drugih detalja implementacije.

Blokiranje nije ograničeno na matrice. Sličan princip može se koristiti kod obrade slika, numeričkih simulacija, baza podataka i drugih algoritama koji više puta pristupaju velikim skupovima podataka.

## 3.4. Rekurzivna dekompozicija

Sličan cilj može se postići i rekurzivnom podelom problema na manje celine.

Kod rekurzivne dekompozicije veliki problem deli se na nekoliko manjih potproblema, koji se zatim dalje dele sve dok njihova veličina ne postane dovoljno mala za direktnu obradu.

Na primer, matrica može biti podeljena na četiri podmatrice:

$$
A =
\begin{bmatrix}
A_{11} & A_{12} \\
A_{21} & A_{22}
\end{bmatrix}.
$$

Isti postupak zatim se rekurzivno primenjuje na podmatrice.

Kako rekurzija napreduje, radni skup pojedinačnog poziva postaje sve manji. U određenom trenutku podproblem postaje dovoljno mali da njegovi podaci mogu da stanu u određeni nivo keša. Dalja obrada tada može imati vrlo dobru vremensku lokalnost.

U praktičnim implementacijama često se koristi **granična vrednost** vrednost. Kada dimenzija problema padne ispod određenog praga, rekurzija se prekida i koristi se jednostavnija iterativna implementacija.

Na taj način izbegava se nepotreban trošak velikog broja rekurzivnih poziva nad veoma malim problemima.

Rekurzivna dekompozicija može pružiti dobru lokalnost na više nivoa memorijske hijerarhije. Veliki potproblemi mogu odgovarati višim nivoima keša, dok manji potproblemi prirodno postaju pogodni za L2 i L1 keš.

Rezultat ipak zavisi od konkretne implementacije. Rekurzija uvodi dodatni trošak poziva funkcija i upravljanja potproblemima, pa nije garantovano da će rekurzivni algoritam uvek biti brži od dobro optimizovanog blokiranog algoritma.

## 3.5. Raspored podataka u memoriji

Performanse algoritma ne zavise samo od redosleda instrukcija, već i od načina na koji su podaci fizički raspoređeni u memoriji.

Ako se elementi koji se često koriste zajedno nalaze blizu jedni drugih, jedno učitavanje keš linije može obezbediti više korisnih podataka. Ako su rasuti po memoriji, svaki pristup može zahtevati učitavanje nove linije.

Ovo je posebno značajno kod struktura sa pokazivačima. Klasična struktura stabla, na primer, može sadržati čvorove koji su dinamički alocirani na međusobno udaljenim adresama. Logički susedni čvorovi tada ne moraju biti fizički susedni u memoriji.

Alternativni pristup je smeštanje elemenata u kompaktan niz. Time se smanjuje količina memorije potrebna za pokazivače i povećava mogućnost da više korisnih elemenata bude prisutno u istoj ili susednim keš linijama.

Jedan primer je **Eytzinger raspored** elemenata binarnog stabla u nizu [4]. Elementi se raspoređuju nivo po nivo, tako da za čvor na poziciji \(i\) njegove potomke možemo odrediti aritmetički, bez eksplicitnih pokazivača.

Takav raspored menja obrazac memorijskih pristupa u odnosu na klasičnu binarnu pretragu nad sortiranom nizom ili stablo sa pokazivačima. Cilj je da se ponašanje memorijskog sistema bolje uklopi sa načinom na koji se stablo obilazi.

Sličan princip pojavljuje se kod reprezentacije grafova. Klasična lista susedstva može koristiti veliki broj odvojenih dinamičkih struktura, dok format **CSR** (*Compressed Sparse Row*) smešta susedstva u nekoliko kompaktnih nizova [7].

Kompaktna reprezentacija obično smanjuje broj indirekcija i poboljšava prostornu lokalnost prilikom obilaska suseda jednog čvora.

Dodatna reorganizacija redosleda čvorova može dovesti do toga da čvorovi koji se često posećuju u bliskom vremenskom intervalu budu smešteni bliže jedni drugima. Time se može poboljšati lokalnost i za druge strukture povezane sa čvorovima, kao što je niz koji beleži da li je čvor već posećen.

## 3.6. unapredno učitavanje kao tehnika optimizacije

unapredno učitavanje, opisan u prethodnom poglavlju sa stanovišta hardvera, može se posmatrati i kao tehnika optimizacije algoritma.

Osnovna ideja je da se zahtev za podatkom pokrene pre trenutka u kome je taj podatak neophodan. Procesor zatim može nastaviti da izvršava druge instrukcije dok se podatak prenosi kroz memorijsku hijerarhiju.

Da bi unapredno učitavanje bio koristan, buduća memorijska adresa mora biti poznata dovoljno rano. Ako se zahtev izda prekasno, podatak neće stići na vreme. Ako se izda previše rano, postoji mogućnost da bude izbačen iz keša pre nego što se upotrebi.

Sekvencijalni obrasci često ne zahtevaju eksplicitni softversko unapredno učitavanje, jer ih hardverski mehanizam za unapredno učitavanje već dobro prepoznaje. Veću korist moguće je ostvariti kod obrazaca kod kojih program može unapred izračunati buduću adresu, ali je pristup sa stanovišta hardvera manje očigledan.

U ovom radu softversko unapredno učitavanje se razmatra kod Eytzinger pretrage. Tokom obilaska moguće je unapred proceniti memorijsku lokaciju do koje će pretraga uskoro stići i zatražiti njeno učitavanje pre trenutka kada je vrednost zaista potrebna.

Dobitak od unaprednog učitavanja uglavnom postaje značajniji kada struktura podataka više ne može da stane u najbrže nivoe keša i kada latencija pristupa memoriji postaje važniji deo ukupnog vremena izvršavanja.

## 3.7. Smanjenje radnog skupa i povećanje ponovne upotrebe

Zajednički cilj velikog broja *cache-aware* tehnika jeste kontrola **aktivnog radnog skupa** algoritma.

Ako algoritam u kratkom vremenskom periodu koristi veoma veliku količinu podataka, linije koje će uskoro ponovo biti potrebne mogu biti izbačene pre ponovne upotrebe. Ako se računanje reorganizuje tako da u datom trenutku aktivno koristi manji deo podataka, povećava se verovatnoća da će oni ostati u kešu dovoljno dugo.

Blokiranje predstavlja direktan primer ovoga, ali isti princip može biti prisutan i kod drugih optimizacija. Reorganizacija grafa može grupisati povezane podatke, promena redosleda petlji može omogućiti ponovno korišćenje iste vrednosti, a kompaktna heš tabela može smanjiti količinu memorije koja se mora posećivati tokom pretrage.

Važno je razlikovati **količinu ukupnih podataka** od veličine aktivnog radnog skupa. Algoritam može raditi nad skupom podataka koji je višestruko veći od keša, a ipak dobro koristiti memorijsku hijerarhiju ako u svakom trenutku intenzivno koristi samo mali deo tog skupa.

## 3.8. *Cache-aware* i *cache-oblivious* pristup

Pored *cache-aware* algoritama postoji i povezan pristup poznat kao projektovanje *cache-oblivious* algoritama [3].

Algoritam tipa *cache-aware* eksplicitno koristi parametre memorijskog sistema, na primer izborom veličine bloka na osnovu kapaciteta keša. Algoritam tipa *cache-oblivious*, sa druge strane, pokušava da ostvari dobru lokalnost bez direktnog poznavanja veličine keša ili keš linije.

Rekurzivna podela problema često se povezuje sa *cache-oblivious* pristupom. Uzastopnim deljenjem problema na manje delove prirodno se dolazi do podproblema koji mogu stati u različite nivoe memorijske hijerarhije, bez eksplicitnog zadavanja njihovih veličina.

Prednost takvog pristupa je potencijalno bolje prilagođavanje različitim nivoima keša i različitim hardverskim platformama. Sa druge strane, eksplicitno podešen *cache-aware* algoritam može ostvariti veoma dobre rezultate kada su karakteristike ciljne platforme poznate.

U ovom radu fokus je na *cache-aware* tehnikama i praktičnoj analizi lokalnosti. Rekurzivno množenje matrica biće korišćeno kao primer pristupa koji kroz hijerarhijsku dekompoziciju problema može ostvariti dobru lokalnost na više nivoa keša.

## 3.9. Ograničenja *cache-aware* optimizacija

Iako *cache-aware* tehnike mogu značajno poboljšati performanse, njihova primena nije besplatna niti univerzalno korisna.

Prvo, optimizovana implementacija često je složenija od osnovnog algoritma. Blokiranje zahteva dodatne petlje i izbor odgovarajuće veličine bloka, rekurzivne implementacije zahtevaju upravljanje granicama potproblema, dok reorganizacija strukture podataka može zahtevati dodatnu fazu pripreme.

Drugo, optimalni parametri mogu zavisiti od konkretne arhitekture. Veličina bloka koja daje dobre rezultate na jednom procesoru ne mora biti optimalna na drugom zbog različitih veličina keša, asocijativnosti, hardverskog unaprednog učitavanja ili drugih mikroarhitekturnih karakteristika.

Treće, performanse programa nisu određene samo memorijskim sistemom. Na rezultat mogu uticati i predikcija grananja, paralelizam na nivou instrukcija, mogućnost vektorizacije, broj izvršenih instrukcija, frekvencija procesora i optimizacije kompajlera.

Zbog toga se efekat *cache-aware* optimizacije ne može pouzdano proceniti samo teorijskim razmatranjem. Potrebno je izvršiti merenja na realnom hardveru i posmatrati ne samo ukupno vreme izvršavanja već, gde je moguće, i relevantne hardverske brojače performansi.

Eksperimentalni deo ovog rada upravo zbog toga kombinuje jednostavne mikrotestove sa složenijim algoritmima. Cilj je da se najpre izolovano pokažu osnovni efekti memorijske hijerarhije, a zatim ispita kako se isti principi ispoljavaju u realnijim algoritamskim strukturama.

# 4. Eksperimentalna metodologija

Eksperimentalni deo rada zasniva se na implementaciji i merenju više algoritama i struktura podataka sa različitim obrascima pristupa memoriji. Cilj eksperimenata nije samo poređenje ukupnog vremena izvršavanja, već i povezivanje uočenih razlika sa karakteristikama memorijske hijerarhije i načinom na koji pojedine implementacije pristupaju podacima.

Kako bi merenja bila što uporedivija, za sve eksperimente korišćeno je isto osnovno softversko okruženje, iste opcije kompajlera i konzistentan način pokretanja programa. Većina testova ponavljana je više puta, dok su za reprezentativne slučajeve prikupljani i hardverski brojači performansi pomoću alata `perf`.

Eksperimenti su izvođeni na dve računarske platforme različitih generacija. Time je omogućeno da se proveri u kojoj meri se uočeni efekti zadržavaju i na hardveru sa drugačijom mikroarhitekturom i memorijskom hijerarhijom.

## 4.1. Hardverske platforme

Primarna eksperimentalna platforma zasnovana je na procesoru **AMD Ryzen 5 3500U**. Procesor poseduje četiri fizička jezgra i osam logičkih niti. Njegova memorijska hijerarhija obuhvata L1 keš za podatke veličine 32 KiB po jezgru, L1 keš za instrukcije veličine 64 KiB, L2 keš veličine 512 KiB po jezgru i zajednički L3 keš kapaciteta 4 MiB. Veličina keš linije iznosi 64 bajta.

L1 keš podataka je osmostruko skupovno asocijativan, dok je L3 keš šesnaestostruko asocijativan. Ove karakteristike posebno su relevantne za mikrotest eksperimente kojima se ispituju efekti radnog skupa i konfliktnih promašaja.

Prva platforma poseduje približno 6 GiB operativne memorije. Ova količina bila je dovoljna za većinu eksperimenata, ali je predstavljala jedno od praktičnih ograničenja pri izboru maksimalnih veličina pojedinih skupova podataka.

Druga platforma zasnovana je na procesoru **Intel Core i7-13700H**. Reč je o znatno novijem procesoru sa hibridnom organizacijom jezgara. Sistem raspolaže sa ukupno 14 fizičkih jezgara i 20 logičkih procesora, dok zajednički L3 keš ima kapacitet od 24 MiB. Maksimalna deklarisana radna frekvencija procesora dostiže približno 5 GHz.

Korišćenje druge platforme omogućava proveru da li optimizacije koje daju određeno ponašanje na procesoru AMD Ryzen 5 3500U pokazuju slične tendencije i na modernijoj Intel arhitekturi. Pri tome se ne očekuje da apsolutna vremena izvršavanja budu direktno uporediva između dve mašine. Cilj poređenja jeste prvenstveno posmatranje relativnog ponašanja različitih implementacija na svakoj platformi zasebno.

Osnovne karakteristike dve platforme prikazane su u tabeli 1.

**Tabela 1. Osnovne karakteristike hardverskih platformi**

| Karakteristika    | Platforma 1        | Platforma 2                       |
| ----------------- | ------------------ | --------------------------------- |
| Procesor          | AMD Ryzen 5 3500U  | Intel Core i7-13700H              |
| Fizička jezgra    | 4                  | 14                                |
| Logički procesori | 8                  | 20                                |
| L1 keš za podatke | 32 KiB po jezgru   | zavisi od tipa jezgra             |
| L2 keš            | 512 KiB po jezgru  | hijerarhija zavisi od tipa jezgra |
| L3 keš            | 4 MiB              | 24 MiB                            |
| Keš linija        | 64 B               | 64 B                              |
| Operativni sistem | *Ubuntu 22.04.5 LTS* | *Ubuntu 22.04.5 LTS*                |

Za drugu platformu u tabeli nisu navedeni pojedinačni L1 i L2 kapaciteti, jer hibridna arhitektura procesora koristi različite tipove jezgara. U eksperimentima je zbog toga važnije kontrolisati na kom logičkom procesoru se program izvršava nego posmatrati samo zbirne podatke o kešu.

## 4.2. Softversko okruženje

Na obe platforme korišćen je operativni sistem *Ubuntu 22.04.5 LTS* sa *Linux* kernelom verzije `6.8.0-138-generic`. Programi su implementirani u programskom jeziku *C++* i kompajlirani pomoću `g++ 11.4.0`, dok je za hardverske brojače korišćen `perf 6.8.12`.

Izvorni kod svih programa za testiranje performansi kompajliran je komandom prikazanom u kodu 1.

```bash
g++ -O3 -std=c++17 -march=native
```

**Kod 1. Komanda za kompajliranje programa za testiranje performansi**

Opcija `-O3` uključuje dodatne optimizacije kompajlera, dok `-march=native` omogućava generisanje instrukcija prilagođenih procesoru na kome se kompajliranje izvršava [8]. Zbog toga izvršni fajlovi za dve platforme nisu nužno identični na nivou mašinskih instrukcija. Takav pristup je nameran, jer cilj nije poređenje identičnog binarnog koda, već procena algoritamskih pristupa kada je svaki kompajliran za ciljnu platformu.

## 4.3. Kontrola procesorskog jezgra

Savremeni operativni sistemi mogu tokom izvršavanja premeštati proces između različitih procesorskih jezgara. Takva migracija može da uvede dodatnu varijabilnost u merenja, između ostalog i zbog toga što različita jezgra ne moraju u datom trenutku imati isti sadržaj privatnih nivoa keša.

Kako bi se ovaj faktor ograničio, programi za testiranje performansi pokretani su pomoću komande `taskset`, kojom se proces vezuje za određeni logički procesor.

Tipična komanda imala je oblik `taskset -c 0 ./build/program argumenti`, čime se izvršavanje ograničava na procesor sa logičkim identifikatorom 0. Isti pristup korišćen je i kada su programi pokretani pomoću alata `perf`.

Vezivanje za jedno jezgro posebno je korisno u ovom radu zato što se ispituju sekvencijalne implementacije. Cilj nije analiza skaliranja sa brojem jezgara, već izolovanje uticaja rasporeda podataka i memorijskih pristupa na performanse jedne niti izvršavanja.

Kod druge platforme, koja koristi hibridnu organizaciju jezgara, fiksiranje procesora dodatno je važno kako testovi ne bi tokom jednog izvršavanja prelazili između jezgara različitih karakteristika.

## 4.4. Način merenja vremena

Za većinu eksperimenata osnovna metrika je ukupno vreme potrebno za izvršavanje dela programa koji predstavlja posmatrani algoritam.

U zavisnosti od vrste testa performansi, iz ukupnog vremena izračunavane su i specifičnije metrike. Kod algoritama pretrage i heš tabela koristi se vreme po upitu izraženo u nanosekundama:

$$
t_q = \frac{t}{Q},
$$

gde je \(t\) ukupno vreme, a \(Q\) broj izvršenih upita.

Kod mikrotesta pristupa memoriji koristi se vreme po pojedinačnom memorijskom pristupu:

$$
t_a = \frac{t}{A},
$$

gde \(A\) predstavlja ukupan broj posmatranih pristupa.

Kod obilaska grafova pogodna metrika je vreme po pregledanoj grani:

$$
t_e = \frac{t}{E},
$$

gde je \(E\) broj grana pregledanih tokom jednog izvršavanja algoritma.

Za množenje matrica, pored vremena izvršavanja, koristi se i propusnost izražena u **GFLOP/s**. Za klasično množenje dve kvadratne matrice dimenzije \(N\) približan broj operacija sa pokretnim zarezom iznosi:

$$
2N^3.
$$

Na osnovu toga performanse se računaju kao:

$$
GFLOP/s = \frac{2N^3}{t \cdot 10^9}.
$$

Ova metrika omogućava jednostavnije poređenje različitih implementacija istog matričnog problema.

## 4.5. Ponavljanje eksperimenata

Jedno izvršavanje testa performansi može biti pod uticajem procesa operativnog sistema, promena frekvencije procesora, početnog stanja keša i drugih izvora varijabilnosti. Zbog toga su ključni eksperimenti izvođeni više puta.

Za glavna poređenja tipično je korišćeno **pet ponavljanja** iste konfiguracije. Kada se u završnim tabelama navodi jedna reprezentativna vrednost za takvu konfiguraciju, korišćena je aritmetička sredina ponovljenih merenja, osim gde je u tekstu drugačije naznačeno. Na taj način je moguće proveriti da li se rezultat stabilno ponavlja i da li pojedinačno merenje značajno odstupa od ostalih.

Na primer, petostruko ponavljanje eksperimenta sa matricama prikazano je u kodu 2.

```bash
for r in 1 2 3 4 5; do
    taskset -c 0 ./build/matrix_loop_order ...
done
```

**Kod 2. Primer petostrukog ponavljanja eksperimenta sa matricama**

Sličan postupak primenjen je kod pretrage, heš tabela i grafova kada je bilo potrebno dobiti pouzdanije reprezentativno vreme.

Prilikom analize rezultata ne posmatra se samo najbrže pojedinačno izvršavanje, već celokupan skup ponovljenih merenja. Time se smanjuje rizik da zaključak bude zasnovan na slučajnom odstupanju.

Kod pojedinih istraživačkih eksperimenata čiji je cilj bio samo da se utvrdi opšti trend korišćen je manji broj ponavljanja. Završne tabele i grafikoni zasnivaju se prvenstveno na rezultatima koji su naknadno provereni kroz više pokretanja.

## 4.6. Hardverski brojači performansi

Samo vreme izvršavanja pokazuje koja implementacija je brža, ali ne objašnjava nužno uzrok razlike. Zbog toga su za odabrane konfiguracije prikupljani hardverski brojači pomoću alata `perf stat`, koji pokreće program i prikuplja statistiku izabranih događaja [9]. Korišćeni su događaji `cycles`, `instructions`, `cache-references`, `cache-misses`, `branches` i `branch-misses`.

Iz odnosa broja instrukcija i procesorskih ciklusa dobija se broj instrukcija po ciklusu:

$$
IPC = \frac{\text{instructions}}{\text{cycles}}.
$$

Veća vrednost IPC-a može ukazivati na bolje iskorišćenje izvršnih jedinica, dok niska vrednost može biti posledica čekanja na podatke, zavisnosti između instrukcija ili drugih ograničenja. Događaji `cache-references` i `cache-misses` korišćeni su kao dodatni pokazatelji memorijskog ponašanja, a `branches` i `branch-misses` za analizu grananja.

Tipična komanda za prikupljanje podataka prikazana je u kodu 3.

```bash
perf stat \
    -e cycles,instructions,cache-references,cache-misses,branches,branch-misses \
    taskset -c 0 ./build/program ...
```

**Kod 3. Primer prikupljanja hardverskih brojača pomoću alata `perf stat`**

Hardverski brojači koriste se kao dopuna vremenskim merenjima, a ne kao njihova zamena.

## 4.7. Ograničenja hardverskih brojača

Pri interpretaciji rezultata alata `perf` potrebno je uzeti u obzir da generički događaji, kao što su `cache-references` i `cache-misses`, ne moraju imati potpuno isto mikroarhitekturno značenje na različitim procesorima. Njihovo mapiranje zavisi od hardvera, zbog čega se apsolutne vrednosti ne koriste za direktno poređenje dve procesorske arhitekture bez dodatnog opreza.

Kada se istovremeno zahteva više događaja nego što procesor može direktno da prati, `perf` može koristiti multipleksiranje brojača i skaliranje rezultata [9]. Generički brojači takođe ne određuju neposredno na kom nivou memorijske hijerarhije je svaki zahtev zadovoljen. Zbog toga se u ovom radu koriste prvenstveno za poređenje implementacija na istoj platformi i za objašnjavanje većih, ponovljivih trendova.

## 4.8. Organizacija eksperimentalnog dela

Eksperimentalna analiza organizovana je od jednostavnijih ka složenijim obrascima pristupa memoriji. Najpre se mikrotestovima ispituju korak pristupa, veličina radnog skupa i konfliktno mapiranje u kešu. Zatim se posmatraju množenje matrica, algoritmi pretrage, heš tabele i BFS nad grafovima. Za svaki problem porede se implementacije sa različitim rasporedom ili redosledom pristupa podacima, a reprezentativni slučajevi dopunjeni su hardverskim brojačima performansi.

## 4.9. Faktori koji utiču na ponovljivost

Potpuno determinističko merenje performansi na opštem operativnom sistemu nije moguće. Na rezultat mogu uticati pozadinski procesi, prekidi, upravljanje frekvencijom procesora, temperatura sistema i trenutno stanje memorijske hijerarhije.

Procesori korišćeni u eksperimentima koriste dinamičko upravljanje frekvencijom, pa frekvencija tokom izvršavanja nije nužno konstantna. Takođe, na obe platforme *Linux* sistem može prilagođavati radnu frekvenciju u zavisnosti od trenutnog opterećenja i energetskih politika.

Uticaj ovih faktora ublažen je fiksiranjem izvršavanja na jedno logičko jezgro, izvođenjem više ponavljanja i fokusiranjem na stabilne i dovoljno velike razlike između implementacija.

Zbog toga male razlike između dva pojedinačna merenja ne treba automatski tumačiti kao značajnu prednost jedne implementacije. Veći značaj imaju obrasci koji se ponavljaju kroz više izvršavanja, više veličina problema ili obe hardverske platforme.

Ovakva metodologija omogućava da eksperimentalni rezultati budu dovoljno stabilni za analizu glavnog predmeta rada, odnosno uticaja lokalnosti i organizacije memorijskih pristupa na performanse algoritama.

# 5. Eksperimentalna analiza *cache-aware* tehnika

U ovom poglavlju predstavljeni su rezultati eksperimenata i njihova analiza. Rezultati se najpre posmatraju kroz jednostavne mikrotestove memorijskog sistema, a zatim kroz četiri grupe složenijih problema. Za svaku grupu izdvojeni su rezultati koji najbolje pokazuju uticaj lokalnosti, radnog skupa i rasporeda podataka. Vrednosti dobijene hardverskim brojačima koriste se kao dopuna vremenskim merenjima, uz ograničenja opisana u prethodnom poglavlju.

## 5.1. Mikrotestovi memorijskog sistema

Pre analize složenijih algoritama sprovedeno je nekoliko jednostavnih mikrotestova sa ciljem da se izolovano prikažu osnovni efekti memorijske hijerarhije. Posmatrani su uticaj koraka pristupa kroz niz, veličine aktivnog radnog skupa i konflikata pri mapiranju više memorijskih blokova u isti L1 keš skup.

Ovi eksperimenti izvedeni su na primarnoj platformi sa procesorom AMD Ryzen 5 3500U. Za njihovu interpretaciju posebno su značajne karakteristike L1 keša za podatke: kapacitet od 32 KiB, veličina keš linije od 64 bajta i osmostruka skupovna asocijativnost.

Cilj mikrotestova nije precizno određivanje latencije svakog nivoa memorijske hijerarhije, već praktična demonstracija principa koji će se kasnije pojaviti kod složenijih algoritama.

### 5.1.1. Uticaj koraka pristupa

Prvi eksperiment ispituje uticaj koraka između dva uzastopna pristupa elementima niza. Kod malog koraka program koristi veliki deo elemenata iz svake učitane keš linije, dok se povećanjem koraka smanjuje količina korisnih podataka iskorišćenih iz jedne linije.

Vremena za različite korake pristupa data su u tabeli 2.

**Tabela 2. Uticaj koraka pristupa na vreme po memorijskom pristupu**

| Korak pristupa | Vreme [ns/pristupu] |
| -----: | ------------------: |
|      1 |               0,285 |
|      2 |               0,617 |
|      4 |               1,060 |
|      8 |               1,965 |
|     16 |               4,213 |
|     32 |               ≈ 5,6 |

Zavisnost vremena pristupa od koraka prikazana je na slici 3.

[Ovde umetnuti odgovarajući grafički prikaz.]

**Slika 3. Vreme pristupa u zavisnosti od koraka pristupa**

Rezultati pokazuju jasan porast vremena po pristupu sa povećanjem koraka. Kod `korak pristupa = 1` elementi se posećuju sekvencijalno i jedna učitana keš linija može biti iskorišćena za više uzastopnih pristupa. To predstavlja veoma povoljan slučaj prostorne lokalnosti.

Povećanjem koraka na 2, 4 i 8 koristi se sve manji deo elemenata koji su preneti u keš zajedno sa traženim podatkom. Iako se čitava keš linija i dalje učitava, sve veći deo njenog sadržaja ostaje neiskorišćen pre nego što se pristupi drugoj liniji.

Posebno izražen porast pojavljuje se za veće korake. Vreme raste sa približno 0,285 ns po pristupu za `korak pristupa = 1` na više od 4 ns za `korak pristupa = 16`, dok se za `korak pristupa = 32` približava vrednosti od 5,6 ns.

Ovaj rezultat praktično demonstrira da broj pristupa elementima nije dovoljan za procenu njihove cene. Dva prolaska kroz memoriju mogu izvršiti isti broj pristupa, ali sekvencijalni prolazak može znatno bolje iskoristiti sadržaj svake keš linije.

Treba uzeti u obzir i mogućnost delovanja hardverskog mehanizma za unapredno učitavanje, koji pravilne obrasce pristupa može unapred da prepozna. Zbog toga dobijene vrednosti ne predstavljaju jednostavno latencije pojedinih nivoa memorije, već ukupno ponašanje konkretnog obrasca pristupa na posmatranoj mikroarhitekturi.

### 5.1.2. Uticaj veličine radnog skupa

Drugi mikrotest ispituje ponašanje pri promeni veličine skupa podataka koji se aktivno koristi tokom izvršavanja.

Ideja eksperimenta je da se za različite veličine radnog skupa više puta pristupa podacima i posmatra prosečno vreme po pristupu. Dok se podaci uklapaju u određeni nivo keša, očekuje se relativno stabilno ponašanje. Kada radni skup premaši kapacitet tog nivoa, deo podataka mora češće da se dobavlja iz narednog, sporijeg nivoa memorijske hijerarhije.

Rezultati za različite veličine radnog skupa prikazani su u tabeli 3.

**Tabela 3. Uticaj veličine radnog skupa na vreme po memorijskom pristupu**

| Veličina radnog skupa | Vreme [ns/pristupu] |
| --------------------: | ------------------: |
|                 4 KiB |               2,535 |
|                 8 KiB |               2,432 |
|                16 KiB |               2,429 |
|                32 KiB |               2,535 |
|                64 KiB |               4,502 |
|               128 KiB |               5,451 |
|               256 KiB |               5,960 |

Promena vremena pristupa sa veličinom radnog skupa prikazana je na slici 4.

[Ovde umetnuti odgovarajući grafički prikaz.]

**Slika 4. Vreme pristupa u zavisnosti od veličine radnog skupa**

Za radne skupove od 4 do 32 KiB vreme po pristupu ostaje približno stabilno, između 2,4 i 2,6 ns. Zatim se pri prelasku na 64 KiB pojavljuje izrazit porast na približno 4,5 ns po pristupu.

Ovakav rezultat je u skladu sa očekivanim ponašanjem L1 keša za podatke kapaciteta 32 KiB. Dok je radni skup dovoljno mali da se najvećim delom zadrži u L1 kešu, ponovljeni pristupi imaju relativno nisku cenu. Kada se veličina udvostruči na 64 KiB, radni skup više ne može u celosti da stane u L1, pa veći deo pristupa zavisi od nižih nivoa memorijske hijerarhije.

Daljim povećanjem radnog skupa na 128 i 256 KiB vreme nastavlja da raste, ali manje naglo. Ove veličine su i dalje manje od kapaciteta L2 keša posmatranog procesora, ali ponašanje ne zavisi samo od nominalnog kapaciteta. Na rezultat utiču raspored pristupa, asocijativnost, zamena linija, unapredno učitavanje i drugi mikroarhitekturni mehanizmi.

Zbog toga ovaj eksperiment ne treba tumačiti kao precizno merenje granica pojedinih keš nivoa. Njegov važniji rezultat je jasno uočavanje promene ponašanja kada aktivni radni skup prestane da se uklapa u najbrži nivo keša.

Ovaj princip biće posebno važan kod blokiranog i rekurzivnog množenja matrica, gde se algoritam reorganizuje upravo sa ciljem da aktivni skup podataka tokom određenog dela računanja bude dovoljno mali za efikasno korišćenje keša.

### 5.1.3. Konflikti pri mapiranju u L1 keš

Treći mikrotest ispituje efekat skupovne asocijativnosti L1 keša.

L1 keš za podatke na korišćenom procesoru ima kapacitet 32 KiB, keš liniju od 64 bajta i osmostruku skupovnu asocijativnost. Broj skupova zato iznosi

$$
\frac{32 \cdot 1024}{64 \cdot 8} = 64.
$$

Jedan kompletan prolaz kroz svih 64 skupa obuhvata:

$$
64 \cdot 64 = 4096 \text{ B}.
$$

Zbog toga memorijske lokacije čije se adrese razlikuju za odgovarajuće višekratnike od 4096 bajta mogu imati iste bitove indeksa skupa i time konkurisati za pozicije unutar istog L1 skupa.

U eksperimentu se postepeno povećava broj aktivno korišćenih lokacija koje su namenski izabrane tako da se mapiraju u isti skup. Rezultati su prikazani u tabeli 4.

**Tabela 4. Uticaj broja lokacija mapiranih u isti L1 skup**

| Broj lokacija | Vreme [ns/pristupu] |
| ------------: | ------------------: |
|             1 |                2,61 |
|             2 |                2,44 |
|             4 |                2,43 |
|             6 |                2,71 |
|             7 |                3,56 |
|             8 |                3,40 |
|             9 |               12,22 |
|            10 |                6,74 |
|            12 |                7,18 |

Efekat povećanja broja lokacija mapiranih u isti L1 skup prikazan je na slici 5.

[Ovde umetnuti odgovarajući grafički prikaz.]

**Slika 5. Uticaj broja lokacija mapiranih u isti L1 skup**

Za mali broj lokacija vreme pristupa ostaje približno između 2,4 i 2,7 ns. Nešto veća varijabilnost pojavljuje se kod sedam i osam lokacija, ali najznačajnija promena nastaje nakon prelaska granice od osam aktivnih lokacija.

Kod devet lokacija izmereno vreme naglo raste na približno 12,2 ns po pristupu. Rezultati za deset i dvanaest lokacija nisu monotono veći od rezultata za devet lokacija, ali ostaju značajno iznad vrednosti dobijenih za mali broj lokacija.

Ovakvo ponašanje odgovara očekivanom efektu osmostruke skupovne asocijativnosti. Dok u istom skupu postoji najviše osam relevantnih keš linija, sve mogu istovremeno da budu prisutne. Dodavanjem devete aktivne linije kapacitet pojedinačnog skupa postaje nedovoljan i pri daljim pristupima dolazi do izbacivanja i ponovnog učitavanja linija.

Posebno visoku vrednost za devet lokacija ne treba tumačiti tako da je devet lokacija nužno uvek nepovoljnije od deset ili dvanaest. Stvarno ponašanje zavisi od politike zamene, tačnog redosleda pristupa, mikroarhitekture i varijabilnosti merenja. Za potrebe ovog eksperimenta bitniji je kvalitativni rezultat: nakon prekoračenja asocijativnosti dolazi do značajnog pogoršanja performansi.

Eksperiment takođe pokazuje zašto ukupan kapacitet keša nije jedini relevantan parametar. Posmatrani skup podataka može biti znatno manji od ukupnih 32 KiB, a da ipak izazove veliki broj promašaja ukoliko se njegove linije nepovoljno mapiraju u isti skup.

### 5.1.4. Zajednička analiza mikrotestova

Tri izvedena mikrotesta prikazuju različite, ali međusobno povezane aspekte rada keš memorije.

Eksperiment sa korakom pristupa demonstrira značaj **prostorne lokalnosti**. Što se bolje koriste susedni podaci iz već učitane keš linije, manja je prosečna cena pojedinačnog pristupa.

Eksperiment sa radnim skupom pokazuje značaj **kapaciteta keša i vremenske lokalnosti**. Ponovno korišćenje podataka daje najveću korist dok aktivni skup može dovoljno dugo da ostane u brzom nivou memorijske hijerarhije.

Conflict eksperiment pokazuje da čak ni mali radni skup nije dovoljan za dobre performanse ako se njegove memorijske linije nepovoljno mapiraju. **Asocijativnost i raspored adresa** predstavljaju dodatno ograničenje pored samog ukupnog kapaciteta keša.

Važno je da sva tri eksperimenta pokazuju promene performansi bez promene osnovnog računskog zadatka. Razlika nastaje prvenstveno zbog načina pristupa memoriji.

Time se potvrđuje osnovna motivacija za *cache-aware* optimizacije: organizacija podataka i redosled njihovog korišćenja mogu imati značajan uticaj na stvarno vreme izvršavanja čak i kada broj korisnih računskih operacija ostane isti.

Rezultati mikrotestova zato predstavljaju osnovu za analizu složenijih algoritama u nastavku rada. Kod množenja matrica posebno će biti važni radni skup i ponovno korišćenje podataka, kod pretrage raspored elemenata i predvidljivost memorijskih pristupa, dok će kod heš tabela i grafova značajnu ulogu imati kompaktan raspored struktura i broj memorijskih lokacija koje se posećuju tokom izvršavanja.

## 5.2. Množenje matrica

Množenje matrica predstavlja jedan od klasičnih primera kod kojih organizacija memorijskih pristupa može imati veliki uticaj na performanse. Sve analizirane implementacije izvršavaju isti osnovni račun i imaju vremensku složenost \(O(N^3)\), ali se razlikuju u redosledu kojim pristupaju elementima matrica i načinu na koji ponovo koriste već učitane podatke.

Za kvadratne matrice \(A\), \(B\) i \(C\), svaki element rezultujuće matrice računa se prema izrazu

$$
C_{ij} = \sum_{k=0}^{N-1} A_{ik}B_{kj}.
$$

Ukupan broj aritmetičkih operacija zato raste približno kao \(2N^3\). Kod dovoljno velikih matrica podaci višestruko premašuju kapacitete najbržih nivoa keša, pa način njihovog obilaska postaje posebno značajan.

U eksperimentima su analizirane tri glavne implementacije:

* iterativno množenje sa redosledom petlji \(i\)-\(k\)-\(j\);
* blokirano množenje sa veličinom bloka 64;
* rekurzivno množenje sa graničnom vrednošću 64.

Pre izbora ovih varijanti ispitivani su i drugi redosledi petlji i različite vrednosti parametara. Cilj završnog poređenja nije obuhvatanje svake moguće implementacije, već poređenje jednostavne lokalnosti pristupa, eksplicitnog blokiranja i hijerarhijske dekompozicije problema.

### 5.2.1. Uticaj redosleda petlji

Najjednostavnija implementacija direktno sledi matematičku definiciju i koristi redosled petlji \(i\)-\(j\)-\(k\). Problem ovog pristupa je način pristupanja matrici \(B\).

U korišćenoj implementaciji elementi matrica smešteni su po vrstama u kontinualnom memorijskom prostoru, pa se elementi iste vrste nalaze jedan pored drugog. U unutrašnjoj \(k\) petlji implementacija \(i\)-\(j\)-\(k\) pristupa elementima

$$
B_{0j}, B_{1j}, B_{2j}, \ldots,
$$

odnosno kreće se kroz jednu kolonu matrice. Uzastopni elementi zato nisu susedni u memoriji, već su razdvojeni za približno jednu celu vrstu matrice.

Takav obrazac slabo koristi prostornu lokalnost i sadržaj učitanih keš linija.

Promenom redosleda petlji na \(i\)-\(k\)-\(j\), unutrašnja petlja pristupa elementima

$$
B_{k0}, B_{k1}, B_{k2}, \ldots
$$

sekvencijalno. Istovremeno se sekvencijalno ažuriraju i elementi vrste matrice \(C\), dok se vrednost \(A_{ik}\) ponovo koristi tokom čitave unutrašnje petlje.

Time se znatno poboljšavaju i prostorna i vremenska lokalnost.

Razlika je veoma velika iako obe varijante imaju istu asimptotsku složenost. Na primarnoj platformi, za \(N=1024\), ranije poređenje redosleda petlji pokazalo je da je \(i\)-\(j\)-\(k\) implementaciji bilo potrebno približno 12,8 s, dok je \(i\)-\(k\)-\(j\) varijanta izvršavala isti račun za približno 0,6 s.

Ovaj rezultat predstavlja jedan od najjednostavnijih primera osnovne ideje rada: poznavanje samo broja aritmetičkih operacija nije dovoljno za procenu stvarnih performansi algoritma.

Zbog izrazito lošeg ponašanja \(i\)-\(j\)-\(k\) varijante, za dalja poređenja kao osnovna iterativna implementacija koristi se \(i\)-\(k\)-\(j\).

### 5.2.2. Blokirano množenje

Kod blokiranog množenja matrice se dele na manje kvadratne regione. U eksperimentima je korišćena veličina bloka

$$
B = 64.
$$

Cilj blokiranja je da se obrada organizuje tako da se manji delovi matrica više puta koriste dok se još nalaze u kešu.

Umesto prelaska preko čitavih matrica pre ponovnog korišćenja određenog podatka, računanje se ograničava na nekoliko manjih podmatrica. Nakon njihove obrade prelazi se na sledeću grupu blokova.

U idealnom slučaju, radni skup potreban za obradu jednog bloka dovoljno je mali da ostane u brzom nivou memorijske hijerarhije. Na taj način se smanjuje broj ponovnih prenosa istih podataka iz sporijih nivoa memorije.

Međutim, performanse ne zavise samo od toga da li blok približno odgovara kapacitetu nekog nivoa keša. Na rezultat utiču i asocijativnost, prisustvo više istovremeno aktivnih blokova, način generisanja koda, vektorizacija, unapredno učitavanje i dodatna kontrola petlji.

Zbog toga veličinu bloka nije moguće pouzdano izabrati samo na osnovu nominalne veličine keša. U ovom radu vrednost 64 izabrana je na osnovu preliminarnih eksperimenata, a zatim je zadržana u završnom poređenju.

### 5.2.3. Rekurzivno množenje

Treća implementacija koristi rekurzivnu dekompoziciju matrica.

Velika matrica se deli na manje podmatrice, nakon čega se isti postupak primenjuje rekurzivno. Kako se dimenzije potproblema smanjuju, njihov radni skup postaje dovoljno mali da se efikasnije uklopi u pojedine nivoe memorijske hijerarhije.

Rekurzija se ne nastavlja sve do pojedinačnih elemenata. Kada dimenzija potproblema dostigne definisanu **granična vrednost** vrednost, dalja obrada se izvršava jednostavnijom iterativnom implementacijom.

Preliminarni eksperiment na primarnoj platformi za \(N=1024\) pokazao je približno sledeće ponašanje:

Rezultati izbora granične vrednosti prikazani su u tabeli 5.
**Tabela 5. Uticaj granične vrednosti na rekurzivno množenje matrica za N = 1024**

| Granična vrednost |   Vreme [s] |
| -----: | ----------: |
|      8 |      ≈ 0,90 |
|     16 |      ≈ 0,70 |
|     32 | ≈ 0,54–0,56 |
|     64 | ≈ 0,48–0,51 |
|    128 | ≈ 0,63–0,67 |

Najbolje ponašanje u ovom skupu testova dobijeno je za granična vrednost približno jednak 64, zbog čega je ta vrednost korišćena u daljim eksperimentima.

Premala granična vrednost dovodi do velikog broja rekurzivnih poziva i dodatnog kontrolnog troška. Sa druge strane, kod prevelike vrednosti potproblem koji se obrađuje iterativno postaje dovoljno velik da se izgubi deo prednosti rekurzivne lokalnosti.

Vrednost 64 zato predstavlja kompromis specifičan za testiranu implementaciju i hardver, a ne univerzalnu optimalnu vrednost.

### 5.2.4. Rezultati na primarnoj platformi

Na platformi sa procesorom AMD Ryzen 5 3500U izvršeno je poređenje za više veličina matrica, uključujući \(N=1024\), \(2048\), \(3072\) i \(4096\).

Promena vremena množenja sa dimenzijom matrice prikazana je na slici 6.

[Ovde umetnuti odgovarajući grafički prikaz.]

**Slika 6. Vreme množenja matrica na AMD Ryzen 5 3500U za različite vrednosti N**

Glavni trend je da rekurzivna implementacija sa granična vrednost vrednošću 64 ostvaruje najbolje vreme za sve posmatrane veće matrice.

Za \(N=1024\) reprezentativna vremena su približno:

Vremena za \(N=1024\) prikazana su u tabeli 6.
**Tabela 6. Vreme izvršavanja implementacija množenja matrica za N = 1024 na primarnoj platformi**

| Implementacija | Vreme [s] |
| -------------- | --------: |
| i-k-j            |      0,63 |
| blokirano, B = 64     |      0,64 |
| rekurzivno, granica = 64   | 0,47–0,49 |

U ovom slučaju blokiranje ne donosi poboljšanje u odnosu na već povoljan \(i\)-\(k\)-\(j\) redosled, dok rekurzivna implementacija smanjuje vreme izvršavanja za približno četvrtinu.

Kod većih matrica prednost rekurzivnog pristupa ostaje prisutna, ali nije monotona. Za \(N=3072\), na primer, završna merenja daju približno:

Vremena za \(N=3072\) prikazana su u tabeli 7.
**Tabela 7. Vreme izvršavanja implementacija množenja matrica za N = 3072 na primarnoj platformi**

| Implementacija | Vreme [s] |
| -------------- | --------: |
| i-k-j            |     17,75 |
| blokirano, B = 64     |     17,05 |
| rekurzivno, granica = 64   |     12,95 |

Rekurzivna varijanta u ovom slučaju zahteva približno 27% manje vremena od \(i\)-\(k\)-\(j\) implementacije i približno 24% manje vremena od blokirane implementacije.

Za \(N=4096\) razlika se ponovo smanjuje. Rekurzivna implementacija i dalje ostvaruje najbolje rezultat, ali njena prednost u odnosu na \(i\)-\(k\)-\(j\) iznosi svega nekoliko procenata.

Ovo pokazuje da korist od *cache-aware* optimizacije ne mora ravnomerno da raste sa veličinom problema. Različite dimenzije matrice mogu drugačije da interaguju sa kapacitetima keša, mapiranjem linija, mehanizmom za unapredno učitavanje, optimizacijama kompajlera i memorijskim podsistemom.

Posebno je zanimljivo ponašanje blokirane implementacije. Iako je blokiranje teorijski direktno motivisano boljim korišćenjem keša, veličina bloka 64 na ovoj platformi ne daje doslednu prednost u odnosu na jednostavniji \(i\)-\(k\)-\(j\) raspored.

To ne znači da blokiranje kao tehnika nije korisno. \(i\)-\(k\)-\(j\) implementacija već poseduje povoljan sekvencijalan obrazac pristupa i omogućava procesoru i kompajleru da dobro iskoriste hardversko unapredno učitavanje, registre i vektorske instrukcije. Dodatna hijerarhija petlji kod blokirane implementacije zato ne mora automatski da nadoknadi svoj kontrolni trošak.

### 5.2.5. Poređenje sa drugom platformom

Reprezentativni testovi performansi ponovljeni su na platformi sa procesorom Intel Core i7-13700H. Posmatrane su veličine \(N=1024\) i \(N=3072\), koje omogućavaju poređenje ponašanja kod manjeg i znatno većeg radnog skupa.

Poređenje implementacija množenja matrica na dve platforme prikazano je na slici 7.

[Ovde umetnuti odgovarajući grafički prikaz.]

**Slika 7. Poređenje implementacija množenja matrica na dve procesorske platforme**

Kod \(N=1024\) dobijena su približno sledeća vremena:

Poređenje dve platforme za \(N=1024\) dato je u tabeli 8.
**Tabela 8. Poređenje implementacija množenja matrica za N = 1024 na dve platforme**

| Implementacija | Ryzen 5 3500U [s] | Core i7-13700H [s] |
| -------------- | ----------------: | -----------------: |
| i-k-j            |              0,63 |               0,23 |
| blokirano, B = 64     |              0,64 |               0,25 |
| rekurzivno, granica = 64   |         0,47–0,49 |               0,25 |

Na novijoj Intel platformi za ovu veličinu problema jednostavna \(i\)-\(k\)-\(j\) implementacija je najbrža. Razlike između tri varijante su relativno male, što pokazuje da dodatna organizacija računanja ne mora biti korisna kada osnovna implementacija već dobro koristi raspoloživi hardver i kada je problem relativno mali.

Slika se značajno menja za \(N=3072\):

Poređenje dve platforme za \(N=3072\) dato je u tabeli 9.
**Tabela 9. Poređenje implementacija množenja matrica za N = 3072 na dve platforme**

| Implementacija | Ryzen 5 3500U [s] | Core i7-13700H [s] |
| -------------- | ----------------: | -----------------: |
| i-k-j            |       ≈ 17,7–18,3 |        ≈ 12,1–12,4 |
| blokirano, B = 64     |       ≈ 17,1–17,6 |              ≈ 6,8 |
| rekurzivno, granica = 64   |       ≈ 12,9–13,1 |              ≈ 7,1 |

Na Intel platformi blokirana implementacija postaje najbrža, dok rekurzivna implementacija zaostaje samo približno 5%. Obe su, međutim, znatno brže od jednostavne \(i\)-\(k\)-\(j\) varijante.

Kod završnih vremenskih merenja blokirana implementacija smanjuje vreme u odnosu na \(i\)-\(k\)-\(j\) za približno 45%, dok rekurzivna ostvaruje smanjenje od približno 42%.

Ovaj rezultat je važan jer pokazuje da ne postoji jedna implementacija koja je optimalna nezavisno od arhitekture.

Na Ryzen platformi rekurzivna dekompozicija pokazuje najbolje ponašanje, dok na Intel platformi pri većem problemu eksplicitno blokiranje daje malu prednost. Razlog ne mora biti samo veličina keša. Na rezultat mogu uticati i organizacija L1 i L2 nivoa, hardverski mehanizam za unapredno učitavanjei, širina izvršnih jedinica, mogućnosti vektorizacije i način na koji `-march=native` prilagođava generisani kod konkretnom procesoru.

Zato je preciznije reći da *cache-aware* tehnike stvaraju povoljnije uslove za memorijski sistem, ali njihov konačni efekat zavisi od konkretne mikroarhitekture.

### 5.2.6. Analiza hardverskih brojača

Za reprezentativnu veličinu \(N=3072\) izvršena su dodatna merenja pomoću alata `perf`.

Na AMD platformi dobijeni su sledeći ključni pokazatelji:

Reprezentativni hardverski pokazatelji na AMD platformi prikazani su u tabeli 10.
**Tabela 10. Hardverski pokazatelji množenja matrica na AMD Ryzen 5 3500U za N = 3072**

| Implementacija | Vreme [s] | GFLOP/s |  IPC | Promašaji među generičkim keš događajima [%] |
| -------------- | --------: | ------: | ---: | -------------: |
| i-k-j            |     18,30 |    3,17 | 2,39 |           0,96 |
| blokirano, B = 64     |     17,36 |    3,34 | 2,70 |          11,72 |
| rekurzivno, granica = 64   |     13,17 |    4,40 | 3,58 |           5,94 |

IPC vrednosti za reprezentativni matrični eksperiment prikazane su na slici 8.

[Ovde umetnuti odgovarajući grafički prikaz.]

**Slika 8. IPC implementacija množenja matrica za N = 3072**

Rekurzivna implementacija ostvaruje najveći IPC, približno 3,58 instrukcija po ciklusu, dok \(i\)-\(k\)-\(j\) ostvaruje približno 2,39. To je u skladu sa znatno kraćim vremenom izvršavanja rekurzivne varijante i pokazuje da procesor tokom njenog izvršavanja uspeva da ostvari viši stepen korisnog rada po ciklusu.

Na prvi pogled može delovati iznenđujuće što \(i\)-\(k\)-\(j\) ima najnižu prikazanu stopu generičkih `cache-misses`, iako je sporiji od rekurzivne implementacije. Ovaj rezultat pokazuje zašto se generički `perf` brojači ne smeju tumačiti izolovano.

`cache-references` i `cache-misses` predstavljaju hardverske događaje čije konkretno značenje zavisi od procesora. Njihov odnos nije jednostavna mera „koliko je algoritam povoljan za keš memoriju“, niti direktno odgovara ukupnom broju promašaja na svim nivoima memorijske hijerarhije.

Pored toga, rekurzivna i blokirana implementacija izvršavaju drugačiji tok instrukcija i generišu drugačiji ukupan broj događaja. Zbog toga je za zaključak važna kombinacija vremena, IPC-a i ostalih pokazatelja, a ne samo jedna stopa promašaja.

Stope pogrešne predikcije grananja kod sve tri implementacije veoma su male, približno reda desetog dela procenta ili manje. To ukazuje da grananje nije dominantno objašnjenje velikih razlika u vremenu izvršavanja.

Na Intel platformi `perf` merenja za \(N=3072\) daju sledeću sliku:

Reprezentativni hardverski pokazatelji na Intel platformi prikazani su u tabeli 11.
**Tabela 11. Hardverski pokazatelji množenja matrica na Intel Core i7-13700H za N = 3072**

| Implementacija | Vreme [s] | GFLOP/s |  IPC | Promašaji među generičkim keš događajima [%] |
| -------------- | --------: | ------: | ---: | -------------: |
| i-k-j            |     12,02 |    4,82 | 1,14 |          82,37 |
| blokirano, B = 64     |      6,73 |    8,61 | 2,08 |          23,72 |
| rekurzivno, granica = 64   |      7,12 |    8,14 | 2,08 |          13,46 |

Razlike su ovde veoma izražene. \(i\)-\(k\)-\(j\) ostvaruje IPC od samo približno 1,14, dok obe *cache-aware* varijante ostvaruju vrednost od približno 2,08.

Istovremeno, generički događaj `cache-misses` koji `perf` prijavljuje na ovoj platformi znatno je nepovoljniji za \(i\)-\(k\)-\(j\) varijantu nego za blokiranu i rekurzivnu implementaciju. Ovaj rezultat prati veliku razliku u vremenu izvršavanja.

Ipak, vrednosti brojača keš događaja sa Intel platforme ne treba direktno numerički porediti sa vrednostima sa AMD platforme. Mapiranje generičkih `perf` događaja na stvarne mikroarhitekturne događaje razlikuje se između procesora. Zbog toga su ove vrednosti prvenstveno korisne za poređenje implementacija **unutar iste platforme**.

Zanimljivo je i da rekurzivna implementacija ima nižu prijavljenu stopu promašaja u kešu od blokirane, ali je blokirana ipak nešto brža. To dodatno potvrđuje da krajnje performanse nisu određene samo jednim aspektom memorijskog ponašanja. Rekurzija uvodi dodatne instrukcije i kontrolni tok, dok blokirana implementacija može biti pogodnija za optimizacije koje ovaj procesor i kompajler primenjuju nad unutrašnjim petljama.

### 5.2.7. Diskusija rezultata

Rezultati množenja matrica pokazuju da isti račun može imati veoma različite performanse u zavisnosti od redosleda pristupa memoriji. Promena redosleda petlji donosi veliko poboljšanje u odnosu na nepovoljan obilazak, dok dodatna korist blokiranja i rekurzivne dekompozicije zavisi od veličine problema i procesorske arhitekture.

Na AMD platformi najbolje rezultate za veće matrice daje rekurzivna implementacija, dok na Intel platformi za \(N=3072\) malu prednost ima blokirana varijanta. Rezultat zato ne ukazuje na jednu univerzalno najbolju implementaciju, već na značaj prilagođavanja radnog skupa i redosleda pristupa konkretnom memorijskom sistemu.

## 5.3. Algoritmi pretrage

Algoritmi pretrage predstavljaju pogodan primer za analizu uticaja rasporeda podataka na performanse. Za razliku od množenja matrica, gde se optimizacijom menja prvenstveno redosled izvršavanja operacija, kod ovog eksperimenta posebna pažnja posvećena je načinu na koji je ista logička struktura predstavljena u memoriji.

U završnom poređenju analizirane su četiri implementacije:

* klasična binarna pretraga nad sortiranim nizom;
* binarno stablo pretrage sa čvorovima povezanim pokazivačima;
* Eytzinger raspored elemenata;
* Eytzinger raspored sa eksplicitnim unaprednim učitavanjem.

Za reprezentativni eksperiment korišćeno je

$$
N = 1\,048\,576
$$

elemenata i

$$
Q = 5\,000\,000
$$

upita.

Sve implementacije rešavaju isti problem i imaju logaritamski broj koraka po pretrazi u uravnoteženom slučaju. Razlike u performansama zato prvenstveno nastaju zbog načina pristupa memoriji, predikcije grananja i mogućnosti procesora da unapred pripremi podatke koji će biti potrebni.

### 5.3.1. Klasična binarna pretraga

Binarna pretraga koristi sortiran niz i u svakom koraku prepolovljava preostali interval [5]. Za niz od \(N\) elemenata vremenska složenost iznosi

$$
O(\log N).
$$

Sa stanovišta broja poređenja ovaj algoritam je veoma efikasan. Međutim, obrazac pristupa memoriji nije sekvencijalan.

Pretraga najpre pristupa sredini niza, zatim sredini leve ili desne polovine, zatim odgovarajućoj četvrtini i tako dalje. Razmak između uzastopnih pristupa zbog toga je u početnim koracima veoma veliki.

Kod malih nizova to ne predstavlja veliki problem jer značajan deo strukture može ostati u kešu. Kada niz postane veliki, pristupi na različitim nivoima pretrage mogu zahtevati podatke iz međusobno udaljenih keš linija.

Pored memorijskih pristupa, u svakom koraku postoji i odluka o tome u kojoj polovini niza treba nastaviti pretragu. Za nasumično raspoređene upite ishod ove odluke može biti teško predvidiv, što povećava mogućnost pogrešne predikcije grananja.

Binarna pretraga je zato dobar primer algoritma koji ima veoma povoljnu teorijsku složenost, ali čije stvarne performanse zavise od karakteristika savremenog procesora.

### 5.3.2. Binarno stablo pretrage

Druga implementacija koristi binarno stablo pretrage, kod koga svaki čvor sadrži vrednost i veze ka odgovarajućim potomcima [5].

Uravnoteženo stablo takođe omogućava pretragu u

$$
O(\log N)
$$

koraka.

Za razliku od sortiranog niza, elementi stabla nisu nužno smešteni uzastopno u memoriji. Čvorovi povezani pokazivačima mogu se nalaziti na udaljenim memorijskim adresama, pa obilazak od korena prema listu može zahtevati pristup većem broju različitih keš linija.

Ovakva reprezentacija uvodi i zavisnost između memorijskih pristupa: adresa sledećeg čvora postaje poznata tek nakon što je pročitan trenutni čvor. To otežava preklapanje više memorijskih zahteva i unapredno učitavanje podataka.

Sa druge strane, tok grananja može imati drugačije ponašanje od klasične binarne pretrage. Zbog toga se ne može unapred pretpostaviti da će stablo sa pokazivačima nužno biti sporije ili brže samo na osnovu jednog svojstva.

Završni rezultati upravo pokazuju da se njegovo relativno ponašanje značajno razlikuje između dve testirane procesorske platforme.

### 5.3.3. Eytzinger raspored

Eytzinger raspored predstavlja način smeštanja elemenata binarnog stabla u kompaktan niz [4].

Elementi se raspoređuju po nivoima stabla. Ako se koristi indeksiranje od 1, za čvor na poziciji \(i\) levi i desni potomak nalaze se na pozicijama

$$
2i
$$

i

$$
2i+1.
$$

Na ovaj način nije potrebno čuvati eksplicitne pokazivače ka potomcima. Struktura je smeštena u jednom kompaktnom memorijskom području, dok se sledeća pozicija može izračunati jednostavnom aritmetikom.

Primer Eytzinger rasporeda prikazan je na slici 9.

[Ovde umetnuti odgovarajući grafički prikaz.]

**Slika 9. Primer sortiranog skupa elemenata i odgovarajućeg Eytzinger rasporeda**

Ovakav raspored ima nekoliko potencijalnih prednosti.

Gornji nivoi stabla, koji se posećuju pri praktično svakoj pretrazi, smešteni su blizu jedan drugog i imaju dobru vremensku lokalnost. Istovremeno, izbegavaju se pokazivači i odvojene memorijske alokacije karakteristične za klasično stablo.

Još važnije, način obilaska je veoma pravilan. Indeks narednog čvora može se izračunati direktno, što omogućava optimizovan tok izvršavanja i olakšava eksplicitno unapredno učitavanje budućih podataka.

Eytzinger raspored zbog toga ne smanjuje asimptotski broj koraka pretrage, ali menja način na koji ti koraci koriste memorijsku hijerarhiju.

### 5.3.4. Eytzinger raspored sa unaprednim učitavanjem

Poslednja analizirana varijanta dodaje eksplicitni softversko unapredno učitavanje Eytzinger pretrazi.

Tokom obilaska stabla moguće je unapred proceniti memorijsku oblast koja će verovatno biti potrebna u narednim koracima. Instrukcijom za unapredno učitavanje može se pokrenuti njeno učitavanje pre nego što procesor dođe do trenutka kada je podatak neophodan.

Cilj nije da se smanji ukupan broj memorijskih pristupa, već da se deo njihove latencije sakrije preklapanjem memorijskog transfera sa trenutnim računanjem.

Efekat ove optimizacije trebalo bi da bude izraženiji kada struktura više ne staje u brze nivoe keša. Kod malih struktura veliki deo potrebnih podataka već može biti prisutan u kešu, pa dodatni zahtevi za unapredno učitavanje donose malo ili nimalo koristi.

Za veće strukture, gde pristupi češće zahtevaju podatke iz udaljenijih nivoa memorijske hijerarhije, ranije pokretanje zahteva može dati merljivo poboljšanje.

### 5.3.5. Rezultati na primarnoj platformi

Za \(N=1\,048\,576\) i pet miliona upita, prosečna vremena pet ponovljenih merenja na procesoru AMD Ryzen 5 3500U bila su:

Rezultati pretrage na primarnoj platformi prikazani su u tabeli 12.
**Tabela 12. Performanse algoritama pretrage na AMD Ryzen 5 3500U**

| Implementacija       | Vreme [s] | ns/upitu |
| -------------------- | --------: | -------: |
| Binarna pretraga               |     1,590 |   318,05 |
| BST                  |     1,944 |   388,73 |
| Eytzinger            |     0,776 |   155,16 |
| Eytzinger + unapredno učitavanje |     0,623 |   124,57 |

Poređenje vremena algoritama pretrage na obe platforme prikazano je na slici 10.

[Ovde umetnuti odgovarajući grafički prikaz.]

**Slika 10. Poređenje vremena algoritama pretrage na obe platforme**

Na ovoj platformi klasično binarno stablo pretrage je čak sporije od binarne pretrage nad nizom. To je u skladu sa činjenicom da BST zahteva praćenje pokazivača između fizički odvojenih čvorova, dok binarna pretraga radi nad jednim kompaktnim nizom.

Najznačajnija promena pojavljuje se kod Eytzinger rasporeda. Prosečno vreme pretrage smanjuje se sa približno 318 ns kod klasične binarne pretrage na oko 155 ns, odnosno približno dva puta.

Dodavanjem unaprednog učitavanja vreme se dalje smanjuje na oko 125 ns po upitu.

U odnosu na običan Eytzinger raspored, unapredno učitavanje smanjuje vreme za približno 20%. U odnosu na klasičnu binarnu pretragu, završna Eytzinger varijanta je približno 2,55 puta brža.

Ovakav rezultat je značajan zato što se ne menja osnovna složenost problema. Sve implementacije i dalje izvršavaju približno logaritamski broj koraka, ali promena rasporeda elemenata u memoriji dovodi do višestruke razlike u stvarnom vremenu izvršavanja.

### 5.3.6. Rezultati na drugoj platformi

Isti reprezentativni eksperiment ponovljen je na procesoru Intel Core i7-13700H.

Dobijeni proseci bili su:

Rezultati pretrage na drugoj platformi prikazani su u tabeli 13.
**Tabela 13. Performanse algoritama pretrage na Intel Core i7-13700H**

| Implementacija       | Vreme [s] | ns/upitu |
| -------------------- | --------: | -------: |
| Binarna pretraga               |     0,916 |   183,15 |
| BST                  |     0,650 |   130,07 |
| Eytzinger            |     0,305 |    60,90 |
| Eytzinger + unapredno učitavanje |     0,279 |    55,79 |

Na novijoj platformi sve implementacije izvršavaju se brže, ali je važnije posmatrati njihov relativni odnos.

Za razliku od AMD platforme, BST je ovde brži od klasične binarne pretrage. Ovo pokazuje da se relativno ponašanje dve tradicionalne strukture ne može generalizovati na osnovu samo jedne arhitekture.

Eytzinger raspored, međutim, ponovo daje veoma veliko poboljšanje. U odnosu na klasičnu binarnu pretragu vreme se smanjuje sa približno 183 na 61 ns po upitu, odnosno Eytzinger je približno tri puta brži.

Varijanta sa unaprednim učitavanjem dodatno smanjuje vreme na približno 56 ns po upitu. Poboljšanje u odnosu na običan Eytzinger iznosi oko 8%, što je manje nego na Ryzen platformi, ali se javlja stabilno i u istom smeru.

Završna Eytzinger varijanta sa unaprednim učitavanjem približno je 3,3 puta brža od binarne pretrage na ovoj platformi.

Za razliku od odnosa binary–BST, koji se menja između procesora, Eytzinger raspored daje jasno poboljšanje na obe testirane arhitekture. To ukazuje da prednost kompaktnog i pravilno organizovanog memorijskog rasporeda nije ograničena na jednu konkretnu mikroarhitekturu.

### 5.3.7. Uticaj veličine strukture i unaprednog učitavanja

Dodatna merenja Eytzinger implementacije sprovedena su za više veličina strukture.

Na primarnoj platformi unapredno učitavanje je pokazivao malo, ali merljivo poboljšanje već za srednje veličine strukture, dok je efekat postajao značajniji kada je skup podataka rastao.

Na primer, za ranije testirane veličine dobijeno je približno:

Uticaj veličine strukture na Eytzinger pretragu prikazan je u tabeli 14.
**Tabela 14. Uticaj veličine strukture na Eytzinger pretragu sa unaprednim učitavanjem**

|      \(N\) | Eytzinger [s] | Eytzinger + unapredno učitavanje [s] |
| ---------: | ------------: | -----------------------: |
|    131 072 |         0,397 |                    0,384 |
|  1 048 576 |         0,575 |                    0,555 |
|  4 194 304 |         0,766 |                    0,743 |
| 16 777 216 |         1,086 |                    1,066 |

Ova merenja predstavljaju zaseban sweep iz ranije faze eksperimenta i zato njihove apsolutne vrednosti ne treba direktno mešati sa završnim petostrukim merenjima. Njihova svrha je da pokažu trend.

U svim navedenim slučajevima eksplicitni unapredno učitavanje daje nešto kraće vreme izvršavanja. Razlika nije dramatična, ali je konzistentna.

To je očekivano jer Eytzinger raspored već sam po sebi ima povoljnije memorijsko ponašanje. unapredno učitavanje zato predstavlja dodatnu optimizaciju postojećeg dobrog rasporeda, a ne promenu istog reda veličine kao prelazak sa klasične binarne pretrage na Eytzinger strukturu.

### 5.3.8. Analiza hardverskih brojača

Hardverski brojači dodatno pomažu u objašnjavanju razlika između implementacija.

Na AMD platformi klasična binarna pretraga za \(N=1\,048\,576\) ostvaruje IPC od približno 0,42, uz stopu pogrešne predikcije grananja od oko 15–16%.

Takva vrednost pokazuje da procesor tokom izvršavanja često ne može efikasno da popuni izvršni tok korisnim instrukcijama. Dva važna potencijalna uzroka su nepredvidiva grananja i čekanje na podatke sa udaljenih memorijskih lokacija.

Kod BST implementacije IPC je još niži, približno 0,37. Iako je stopa pogrešne predikcije grananja znatno manja, oko 0,9%, obilazak pokazivača i nepovoljna memorijska lokalnost dovode do dužeg ukupnog vremena izvršavanja.

Kod Eytzinger rasporeda IPC raste na približno 0,67, dok se stopa pogrešne predikcije grananja kreće oko 1%. To odgovara znatno kraćem vremenu izvršavanja.

Najizraženija promena pojavljuje se kod Eytzinger varijante sa unaprednim učitavanjem. IPC raste na približno

$$
1,35,
$$

dok stopa pogrešne predikcije grananja ostaje ispod 1%.

IPC vrednosti algoritama pretrage na primarnoj platformi prikazane su na slici 11.

[Ovde umetnuti odgovarajući grafički prikaz.]

**Slika 11. IPC algoritama pretrage na primarnoj platformi**

Stope pogrešne predikcije grananja prikazane su na slici 12.

[Ovde umetnuti odgovarajući grafički prikaz.]

**Slika 12. Stopa pogrešne predikcije grananja kod algoritama pretrage na primarnoj platformi**

Ovaj rezultat je posebno interesantan jer Eytzinger sa unaprednim učitavanjem zapravo može izvršavati dodatne instrukcije potrebne za unapredno učitavanje podataka, ali uprkos tome završava ranije. Broj instrukcija zato sam po sebi nije dovoljan za procenu performansi. Ako dodatne instrukcije omogućavaju da se smanji vreme tokom kog procesor čeka podatke, ukupno vreme može biti značajno kraće.

Brojači generičkog događaja `cache-misses` na AMD platformi takođe pokazuju povoljnije ponašanje Eytzinger varijanti od klasične binarne pretrage. Ipak, kao i kod matričnih eksperimenata, ove vrednosti treba tumačiti zajedno sa ostalim pokazateljima, a ne kao potpun opis memorijske hijerarhije.

Na Intel platformi ponovo se pojavljuje veoma niska IPC vrednost kod klasične binarne pretrage, približno 0,41, uz stopu pogrešne predikcije grananja od oko 15,3%.

BST ostvaruje IPC od približno 0,65, Eytzinger oko 0,98, dok Eytzinger sa unaprednim učitavanjem dostiže približno 1,62.

Trend IPC-a zato vrlo dobro prati izmerena vremena izvršavanja:

$$
\text{binary} < \text{BST} < \text{Eytzinger} < \text{Eytzinger + unapredno učitavanje}.
$$

Ovo ne znači da IPC sam određuje performanse, ali pokazuje da optimizovane varijante omogućavaju procesoru efikasnije izvršavanje postojećeg toka instrukcija i bolje skrivanje memorijske latencije.

### 5.3.9. Diskusija rezultata

Eksperiment sa pretragom pokazuje da fizički raspored podataka može promeniti performanse i kada asimptotska složenost ostaje ista. Odnos između klasične binarne pretrage i BST-a razlikuje se između dve platforme, dok Eytzinger raspored na obe daje jasno kraće vreme izvršavanja.

Unapredno učitavanje donosi dodatno, ali manje poboljšanje od same promene rasporeda. Kombinacija kompaktne organizacije podataka, pravilnijeg obilaska i ranijeg pokretanja memorijskih zahteva zato može značajno smanjiti cenu pretrage [4].

## 5.4. Heš tabele

Heš tabele predstavljaju drugačiji tip problema u odnosu na prethodne eksperimente. Njihova prosečna vremenska složenost pretrage često se smatra konstantnom, odnosno \(O(1)\), ali stvarna cena operacije zavisi od načina rešavanja kolizija, faktora popunjenosti i rasporeda elemenata u memoriji [5].

U radu su poređene tri implementacije: ulančavanje (*separate chaining*), linearno ispitivanje (*linear probing*) i Robin Hood heširanje. Za sve varijante korišćeni su isti skup ključeva i ista heš funkcija, kako bi poređenje prvenstveno odražavalo razlike u organizaciji tabele i rešavanju kolizija. Glavni eksperimenti izvedeni su za \(N=1\,048\,576\) elemenata i \(Q=5\,000\,000\) upita.

### 5.4.1. Ulančavanje

Kod ulančavanja svaka pozicija osnovne heš tabele povezana je sa kolekcijom elemenata koji imaju isti indeks. Ako više ključeva daje isti indeks, pretraga najpre određuje odgovarajuću poziciju, a zatim prolazi kroz pripadajuće elemente. Prednost ovog pristupa je relativno stabilno ponašanje sa rastom broja kolizija, dok dodatna indirekcija i odvojene memorijske strukture mogu umanjiti prostornu lokalnost.

### 5.4.2. Linearno ispitivanje

Linearno ispitivanje koristi otvoreno adresiranje, pa su svi elementi smešteni direktno u jednom nizu. Ako je pozicija određena heš funkcijom zauzeta, proveravaju se naredne pozicije:

$$
h(k), h(k)+1, h(k)+2, \ldots
$$

Ovaj pristup ima dobru prostornu lokalnost jer se posećuju susedne memorijske lokacije. Problem nastaje pri visokom faktoru popunjenosti, kada se formiraju dugi klasteri i broj pregledanih pozicija može značajno da poraste.

### 5.4.3. Robin Hood heširanje

Robin Hood heširanje takođe koristi otvoreno adresiranje, ali prilikom umetanja uzima u obzir udaljenost elementa od njegove idealne pozicije [6]. Cilj je da se dužine sekvenci ispitivanja ujednače, a informacija o udaljenosti omogućava i raniji prekid nekih neuspešnih pretraga. Ovaj pristup uvodi dodatnu logiku, pa njegov praktični dobitak zavisi od faktora popunjenosti i tipa upita.

### 5.4.4. Faktor popunjenosti

Faktor popunjenosti definiše odnos broja smeštenih elemenata \(N\) i kapaciteta tabele \(M\):

$$
\alpha = \frac{N}{M}.
$$

Za završno poređenje posmatrane su vrednosti \(\alpha=0,70\) i \(\alpha=0,95\). Na primarnoj platformi prosečna vremena pet ponovljenih merenja uspešnih pretraga bila su:

Uticaj faktora popunjenosti na uspešne pretrage prikazan je u tabeli 15.
**Tabela 15. Uticaj faktora popunjenosti na uspešne pretrage u heš tabelama**

| Implementacija | Faktor popunjenosti 0,70 [s] | Faktor popunjenosti 0,95 [s] |
| --- | ---: | ---: |
| Ulančavanje | 0,314 | 0,359 |
| Linearno ispitivanje | 0,281 | 0,448 |
| Robin Hood | 0,350 | 0,764 |

Uticaj faktora popunjenosti na uspešnu pretragu prikazan je na slici 13.

[Ovde umetnuti odgovarajući grafički prikaz.]

**Slika 13. Uticaj faktora popunjenosti na vreme uspešne pretrage kod heš tabela**

Pri faktoru popunjenosti 0,70 linearno ispitivanje ostvaruje najbolje vreme. Povećanjem faktora na 0,95 vreme ulančavanja raste relativno malo, dok se kod metoda otvorenog adresiranja povećava cena dužih sekvenci ispitivanja. U testiranoj Robin Hood implementaciji dodatna logika dovodi do većeg troška kod uspešnih pretraga, pa ovaj rezultat treba tumačiti kao ponašanje konkretne implementacije, a ne kao opštu osobinu Robin Hood heširanja.

### 5.4.5. Uspešna i neuspešna pretraga

Pri faktoru popunjenosti \(\alpha=0,95\) posebno su poređene uspešne (*hit*) i neuspešne (*miss*) pretrage:

Razlika između uspešnih i neuspešnih pretraga prikazana je u tabeli 16.
**Tabela 16. Uspešni i neuspešni upiti pri faktoru popunjenosti 0,95 na primarnoj platformi**

| Implementacija | Uspeh [s] | Neuspeh [s] | Uspeh [ns/upitu] | Neuspeh [ns/upitu] |
| --- | ---: | ---: | ---: | ---: |
| Ulančavanje | 0,359 | 0,371 | 71,8 | 74,3 |
| Linearno ispitivanje | 0,448 | 2,318 | 89,6 | 463,5 |
| Robin Hood | 0,764 | 0,763 | 152,7 | 152,6 |

Poređenje uspešnih i neuspešnih pretraga prikazano je na slici 14.

[Ovde umetnuti odgovarajući grafički prikaz.]

**Slika 14. Uspešne i neuspešne pretrage pri faktoru popunjenosti 0,95 na primarnoj platformi**

Kod ulančavanja razlika između uspešne i neuspešne pretrage je mala. Kod linearnog ispitivanja neuspešna pretraga mora da nastavi kroz klaster sve do prazne pozicije, pa pri popunjenosti od 95% vreme raste na više od 460 ns po upitu. To je posledica primarnog grupisanja (*primary clustering*). Robin Hood heširanje pokazuje stabilnije ponašanje između dva tipa upita jer se neuspešna pretraga u određenim slučajevima može prekinuti ranije.

### 5.4.6. Ponašanje na drugoj platformi

Reprezentativni eksperiment neuspešnih pretraga pri faktoru popunjenosti 0,95 ponovljen je na procesoru Intel Core i7-13700H. Prosečna vremena tri merenja bila su:

Rezultati neuspešnih upita na Intel platformi prikazani su u tabeli 17.
**Tabela 17. Neuspešni upiti pri faktoru popunjenosti 0,95 na Intel Core i7-13700H**

| Implementacija | Vreme [s] | ns/upitu |
| --- | ---: | ---: |
| Ulančavanje | 0,150 | 30,0 |
| Linearno ispitivanje | 0,969 | 193,7 |
| Robin Hood | 0,229 | 45,8 |

Redosled je isti kao na primarnoj platformi: ulančavanje je najbrže, Robin Hood je na drugom mestu, a linearno ispitivanje je znatno sporije. Time se potvrđuje da dugačke sekvence neuspešnog ispitivanja pri visokom faktoru popunjenosti predstavljaju značajan trošak na obe arhitekture.

### 5.4.7. Analiza hardverskih brojača

Za neuspešne pretrage pri faktoru popunjenosti 0,95 na primarnoj platformi dobijene su sledeće reprezentativne vrednosti:

Hardverski pokazatelji neuspešnih upita prikazani su u tabeli 18.
**Tabela 18. Hardverski pokazatelji neuspešnih upita u heš tabelama na primarnoj platformi**

| Implementacija | IPC | Instrukcije | Promašaji među generičkim keš događajima [%] | Pogrešna predikcija grananja [%] |
| --- | ---: | ---: | ---: | ---: |
| Ulančavanje | 0,45–0,46 | ≈ 0,72 milijarde | ≈ 44–45 | ≈ 4,0 |
| Linearno ispitivanje | ≈ 1,84 | ≈ 9,55 milijardi | ≈ 12,2 | ≈ 0,29 |
| Robin Hood | ≈ 0,57 | ≈ 1,24 milijarde | ≈ 33,7 | ≈ 3,45 |

Linearno ispitivanje ima najveći IPC i najnižu prijavljenu stopu generičkih keš promašaja, ali je ipak najsporije. Razlog je ukupan broj instrukcija: izvršava približno 9,5 milijardi instrukcija, više od deset puta više od ulančavanja. Sekvencijalni pristupi jesu povoljni za keš, ali algoritam pri neuspešnoj pretrazi obrađuje previše pozicija.

Sličan obrazac pojavljuje se i na Intel platformi. Linearno ispitivanje ostvaruje visok IPC, ali i približno 9,6 milijardi instrukcija, zbog čega ostaje višestruko sporije od druge dve implementacije.

### 5.4.8. Diskusija rezultata

Rezultati heš tabela pokazuju da dobra prostorna lokalnost nije dovoljna ako se istovremeno značajno poveća količina rada. Linearno ispitivanje je veoma efikasno pri umerenom faktoru popunjenosti, ali kod neuspešnih pretraga pri \(\alpha=0,95\) dugački klasteri postaju dominantan trošak.

Ovaj eksperiment zato jasno razdvaja cenu pojedinačnog memorijskog pristupa od ukupnog broja pristupa koje algoritam mora da izvrši. *Cache-aware* organizacija podataka može smanjiti cenu pristupa, ali ne može uvek da nadoknadi nepovoljno algoritamsko ponašanje.

## 5.5. Grafovi i BFS

Grafovi predstavljaju izazovan slučaj za memorijsku hijerarhiju jer njihov obrazac pristupa često nije sekvencijalan. Za razliku od matrica, kod kojih je raspored elemenata pravilan, obilazak grafa zavisi od njegove strukture i redosleda kojim se posećuju čvorovi.

U ovom radu posmatran je algoritam **pretrage u širinu** (*Breadth-First Search*, BFS) nad slučajno generisanim usmerenim grafovima sa konstantnim izlaznim stepenom

$$
d = 8.
$$

Analizirane su tri reprezentacije:

* klasična lista susedstva;
* CSR (*Compressed Sparse Row*);
* CSR sa promenjenim redosledom čvorova.

Cilj eksperimenta je da se ispita u kojoj meri kompaktniji raspored grana i reorganizacija numeracije čvorova mogu poboljšati lokalnost tokom BFS obilaska.

### 5.5.1. Lista susedstva

Kod klasične liste susedstva za svaki čvor čuva se posebna kolekcija njegovih suseda.

Ovakva reprezentacija je jednostavna i fleksibilna, ali podaci koji pripadaju različitim čvorovima ne moraju biti smešteni u jednom neprekidnom memorijskom području. Tokom BFS-a zato prelazak sa susedstva jednog čvora na susedstvo drugog može zahtevati pristup udaljenoj memorijskoj lokaciji.

Za male grafove ovaj trošak može biti relativno mali, ali kod velikih grafova struktura podataka višestruko premašuje kapacitet keša i lokalnost postaje značajnija.

### 5.5.2. CSR reprezentacija

CSR predstavlja graf pomoću nekoliko kompaktnih nizova [7].

Jedan niz sadrži sve susede uzastopno, dok drugi niz određuje početak susedstva svakog čvora. Svi susedi jednog čvora zbog toga zauzimaju neprekidan deo memorije.

U odnosu na klasičnu listu susedstva, ovakva organizacija ima manji broj zasebnih memorijskih objekata i omogućava sekvencijalni prolazak kroz susede jednog čvora.

To predstavlja dobru prostornu lokalnost prilikom obrade pojedinačne liste suseda.

Međutim, samo kompaktno čuvanje grana ne rešava sve probleme BFS-a. Redosled kojim BFS posećuje čvorove zavisi od strukture grafa, pa pristupi pomoćnim strukturama, kao što su niz posećenih čvorova i red za obradu, i dalje mogu biti raspoređeni kroz veliki memorijski prostor.

### 5.5.3. Reorganizacija redosleda čvorova

Treća varijanta koristi CSR reprezentaciju nakon promene numeracije čvorova.

Ideja je da se čvorovi koji se tokom BFS obilaska pojavljuju blizu jedan drugom dobiju i numerički bliske identifikatore. Nakon određivanja novog redosleda, graf se ponovo gradi u CSR formatu prema toj numeraciji.

Time se ne menja logička struktura grafa. Menja se samo fizički raspored podataka u memoriji.

Takva reorganizacija može poboljšati lokalnost na dva načina. Prvo, susedstva čvorova koji se obrađuju u sličnom periodu mogu biti smeštena bliže jedno drugom. Drugo, pristupi pomoćnim nizovima indeksiranim identifikatorom čvora, kao što je niz `visited`, postaju lokalniji.

Ovo je posebno važna razlika u odnosu na običan CSR. CSR garantuje da su **susedi jednog čvora** kompaktno smešteni, dok reorganizacija redosleda pokušava da poboljša i lokalnost između **različitih čvorova koji se obrađuju jedan za drugim**.

### 5.5.4. Rezultati

Za reprezentativni eksperiment korišćen je graf sa

$$
N = 1\,048\,576
$$

čvorova i izlaznim stepenom 8. Tokom jednog BFS prolaza pregledano je približno 8,39 miliona grana.

Prosečna vremena na primarnoj platformi bila su približno:

Rezultati BFS-a na primarnoj platformi prikazani su u tabeli 19.
**Tabela 19. Performanse BFS-a na primarnoj platformi**

| Reprezentacija  | Vreme po BFS prolazu [s] | ns/grani |
| --------------- | -----------------------: | -------: |
| Lista susedstva |                    0,186 |   ≈ 22,2 |
| CSR             |                    0,188 |   ≈ 22,4 |
| reorganizovani CSR   |                    0,042 |    ≈ 5,0 |

Na ovoj platformi običan CSR ne donosi značajno poboljšanje u odnosu na listu susedstva. Vremena su praktično ista, uz određenu varijabilnost između pojedinačnih izvršavanja.

Nasuprot tome, reorganizovani CSR smanjuje vreme jednog BFS prolaza na približno 0,042 s, što predstavlja ubrzanje od oko

$$
4,5\times
$$

u odnosu na običan CSR.

Poređenje BFS reprezentacija na obe platforme prikazano je na slici 15.

[Ovde umetnuti odgovarajući grafički prikaz.]

**Slika 15. Poređenje vremena BFS-a za listu susedstva, CSR i reorganizovani CSR na obe platforme**

Na drugoj platformi dobijen je sličan odnos, ali običan CSR već pokazuje jasnu prednost nad listom susedstva:

Rezultati BFS-a na Intel platformi prikazani su u tabeli 20.
**Tabela 20. Performanse BFS-a na Intel Core i7-13700H**

| Reprezentacija  | Vreme po BFS prolazu [s] | ns/grani |
| --------------- | -----------------------: | -------: |
| Lista susedstva |                    0,093 |   ≈ 11,1 |
| CSR             |                    0,066 |    ≈ 7,9 |
| reorganizovani CSR   |                   0,0178 |   ≈ 2,13 |

Lista susedstva zahteva približno 0,093 s po obilasku.

CSR smanjuje vreme na približno 0,066 s, dok reorganizovani CSR dodatno spušta vreme na oko 0,018 s.
U odnosu na običan CSR, reorganizacija na ovoj platformi daje ubrzanje od približno

$$
3,7\times.
$$

U odnosu na listu susedstva, razlika je nešto veća od pet puta.

Rezultat na obe platforme zato pokazuje isti osnovni trend: **sama kompaktna reprezentacija grafa može pomoći, ali je promena rasporeda čvorova imala znatno veći uticaj na posmatrani BFS test**.

### 5.5.5. Analiza hardverskih brojača

`perf` rezultati pružaju dodatnu potvrdu da se kod reorganizovanog grafa menja način na koji procesor izvršava BFS.

Na primarnoj platformi običan CSR pri reprezentativnom izvršavanju ostvaruje IPC od približno

$$
0,72,
$$

dok reorganizovani CSR dostiže vrednosti od približno 1,15 do 1,26. Istovremeno, prijavljena stopa generičkih promašaja u kešu smanjuje se sa približno 45% na oko 40%.
Povećanje IPC-a je posebno značajno. Ono ukazuje da procesor tokom reorganizovanog obilaska provodi manje vremena u situacijama u kojima nije moguće napredovati zbog čekanja na podatke ili drugih zavisnosti.

Na Intel platformi javlja se isti osnovni obrazac. Običan CSR ostvaruje IPC od približno 1,16–1,23, dok reorganizovani CSR dostiže približno 1,8–1,9.
Promena stopa generičkih promašaja u kešu na ovoj platformi nije toliko dramatična koliko promena ukupnog vremena i IPC-a. To je još jedan primer zbog čega jednu `perf` metriku ne treba posmatrati izolovano. Reorganizacija menja veći broj aspekata izvršavanja: redosled memorijskih zahteva, mogućnost ponovne upotrebe keš linija, ponašanje pomoćnih nizova i količinu vremena tokom kog procesor čeka podatke.

### 5.5.6. Značaj reorganizacije

Dobijeno ubrzanje je znatno veće od razlike između liste susedstva i običnog CSR-a. To pokazuje da kod BFS-a nije dovoljno posmatrati samo način čuvanja grana.

CSR rešava lokalnost prilikom čitanja suseda jednog čvora, ali BFS istovremeno pristupa i podacima vezanim za veliki broj različitih čvorova. Kod slučajno numerisanog grafa identifikatori susednih ili vremenski blisko obrađenih čvorova mogu biti potpuno udaljeni.

Reorganizacija redosleda menja upravo tu osobinu. Čvorovi koji se tokom obilaska pojavljuju u sličnom delu pretrage dobijaju bliske indekse, pa pristupi strukturama indeksiranim po čvorovima postaju povoljniji za keš.

Dodatni eksperimenti sa različitim početnim čvorovima pokazali su da poboljšanje nije nestajalo kada se BFS pokrene iz čvora različitog od onog korišćenog pri formiranju novog redosleda. Zbog toga veliki dobitak ne treba posmatrati isključivo kao specijalan slučaj jednog početnog čvora, iako konkretna korist reorganizacije svakako zavisi od strukture grafa.

Važno ograničenje jeste da reorganizacija redosleda zahteva dodatnu fazu pripreme grafa. Prednost ima najviše smisla u situacijama u kojima će se nad istim grafom izvršavati više obilazaka ili drugih algoritama, tako da se jednokratni trošak reorganizacije može raspodeliti na više operacija.

Ako je potrebno izvršiti samo jedan BFS nad grafom koji prethodno nije reorganizovan, ukupan trošak pripreme takođe mora biti uzet u obzir.

### 5.5.7. Diskusija rezultata

Kod grafova se najveće poboljšanje ne dobija samo kompaktnijim čuvanjem grana, već promenom numeracije čvorova tako da redosled u memoriji bolje prati redosled obrade. CSR sa reorganizovanim čvorovima višestruko je brži od običnog CSR-a na obe platforme, iako BFS pregleda isti broj grana.

Rezultat pokazuje da lokalnost složenih struktura zavisi i od međusobnog rasporeda različitih delova podataka, a ne samo od kompaktnosti pojedinačnog niza. Pri praktičnoj primeni treba, međutim, uračunati i jednokratni trošak reorganizacije grafa.

# 6. Diskusija rezultata

Eksperimenti obuhvataju različite obrasce rada sa memorijom, ali ukazuju na nekoliko zajedničkih principa. Najvažniji je da stvarne performanse nisu određene samo asimptotskom složenošću. Redosled pristupa, veličina aktivnog radnog skupa i fizički raspored podataka mogu promeniti vreme izvršavanja od nekoliko desetina procenata do nekoliko puta.

## 6.1. Lokalnost i aktivni radni skup

Mikrotestovi pokazuju da povećanje koraka pristupa smanjuje iskorišćenost učitane keš linije, dok prekoračenje kapaciteta L1 keša povećava cenu ponovljenih pristupa. Konfliktni test dodatno pokazuje da ni mali radni skup nije dovoljan ako se veliki broj aktivnih linija nepovoljno mapira u isti skup.

Isti principi pojavljuju se kod množenja matrica. Promena redosleda petlji poboljšava sekvencijalnost pristupa, dok blokiranje i rekurzivna dekompozicija ograničavaju količinu podataka koja je aktivna u jednom delu računanja. Rezultati pokazuju da je veličina aktivnog radnog skupa često važnija od ukupne veličine problema.

## 6.2. Fizički raspored podataka

Pretraga i grafovi posebno jasno pokazuju značaj fizičkog rasporeda. Eytzinger raspored zadržava logaritamski broj koraka, ali menja redosled memorijskih pristupa i na obe platforme daje znatno bolje rezultate od klasične binarne pretrage. Kod BFS-a običan CSR obezbeđuje kompaktno čuvanje suseda jednog čvora, dok dodatna reorganizacija numeracije poboljšava i lokalnost između čvorova koji se obrađuju u bliskom vremenskom intervalu.

Ovi primeri pokazuju da optimizacija strukture podataka može biti jednako važna kao optimizacija samog toka algoritma.

## 6.3. Lokalnost i količina algoritamskog rada

Heš tabele predstavljaju važnu granicu prethodnog principa. Linearno ispitivanje ima povoljnu prostornu lokalnost, ali pri visokom faktoru popunjenosti neuspešna pretraga može zahtevati prolazak kroz veoma duge klastere. U tim slučajevima nizak procenat prijavljenih promašaja u kešu ne vodi do kratkog vremena izvršavanja, jer algoritam izvršava višestruko veći broj pristupa i instrukcija.

Lokalnost zato treba posmatrati zajedno sa količinom algoritamskog rada. Povoljan raspored smanjuje cenu potrebnih pristupa, ali ne može uvek da nadoknadi strategiju koja proizvodi mnogo više operacija.

## 6.4. Veličina problema i asimptotska složenost

Prednost *cache-aware* optimizacija nije konstantna za sve veličine problema. Kada se podaci već uklapaju u brže nivoe keša, složenija organizacija može doneti malo poboljšanje ili dodatni trošak. Sa rastom radnog skupa memorijsko ponašanje postaje izraženije, ali dobitak ne mora rasti monotono.

Ovo istovremeno pokazuje ograničenje same asimptotske \(O\)-notacije. Množenja matrica ostaju \(O(N^3)\), analizirane pretrage zadržavaju logaritamski broj koraka, a BFS ostaje \(O(V+E)\), ali implementacije unutar istih klasa složenosti ostvaruju veoma različita vremena. Asimptotska analiza zato ostaje neophodna za procenu rasta količine rada, ali ne opisuje cenu memorijskih pristupa na konkretnom hardveru.

## 6.5. Razlike između platformi i hardverski brojači

Poređenje dve platforme pokazuje da se opšti princip lokalnosti prenosi između arhitektura, ali da optimalna implementacija ili parametar ne mora biti isti. Rekurzivno množenje matrica ima najbolji rezultat na AMD platformi za veće probleme, dok na Intel platformi pri \(N=3072\) malu prednost ima blokirana varijanta. Odnos BST-a i binarne pretrage takođe se menja između procesora, dok su prednosti Eytzinger rasporeda i reorganizovanog CSR-a prisutne na obe platforme.

Hardverski brojači pomažu u tumačenju ovih razlika, ali nijedna pojedinačna metrika nije dovoljna. Procenat generičkih `cache-misses` događaja ne predstavlja direktno ukupan kvalitet korišćenja keša, a visok IPC ne garantuje kratko vreme ako je broj izvršenih instrukcija veoma veliki. Zato su brojači korišćeni zajedno sa vremenom izvršavanja i prvenstveno za poređenje implementacija unutar iste platforme.

## 6.6. Cena optimizacija i ograničenja eksperimenta

Blokiranje zahteva izbor veličine bloka, rekurzija uvodi dodatni kontrolni trošak, a Eytzinger i reorganizovani CSR zahtevaju prethodnu pripremu podataka. Takvi jednokratni troškovi imaju najviše smisla kada se optimizovana struktura kasnije koristi za veliki broj operacija.

Rezultati su dobijeni na dve procesorske platforme i sekvencijalnim implementacijama vezanim za jedno logičko jezgro. Grafovi su sintetički i imaju fiksni izlazni stepen, a rezultati heš tabela zavise od konkretne heš funkcije, distribucije ključeva i implementacionih detalja. Zbog toga rezultati ne predstavljaju univerzalne vrednosti za sve sisteme, već eksperimentalnu potvrdu posmatranih principa na dve različite arhitekture.

## 6.7. Zajednički zaključci eksperimenata

Na osnovu eksperimenata mogu se izdvojiti sledeći zaključci:

1. prostorna i vremenska lokalnost mogu značajno smanjiti vreme izvršavanja;
2. kontrola aktivnog radnog skupa može biti važnija od ukupne veličine podataka;
3. fizički raspored podataka može značajno promeniti performanse bez promene asimptotske složenosti;
4. dobra lokalnost ne može uvek da nadoknadi veliki porast količine algoritamskog rada;
5. unapredno učitavanje može dodatno sakriti memorijsku latenciju kada je budući obrazac pristupa predvidiv;
6. konkretni optimalni parametri zavise od mikroarhitekture i veličine problema;
7. hardverske brojače treba tumačiti zajedno sa vremenom izvršavanja i brojem instrukcija.

Zajednička poruka jeste da efikasna implementacija treba da vodi računa i o količini rada i o načinu na koji se podaci kreću kroz memorijsku hijerarhiju.

# 7. Zaključak

Cilj ovog rada bio je da se analizira uticaj memorijske hijerarhije i *cache-aware* tehnika na praktične performanse algoritama. Teorijska analiza i eksperimenti pokazali su da asimptotska složenost, iako neophodna za razumevanje rasta količine rada, nije dovoljna za procenu vremena izvršavanja na savremenim procesorima.

Mikrotestovi su izolovano pokazali značaj prostorne lokalnosti, veličine radnog skupa i skupovne asocijativnosti. Složeniji eksperimenti zatim su pokazali kako se isti principi ispoljavaju u realnijim algoritmima. Kod množenja matrica promena redosleda petlji, blokiranje i rekurzivna dekompozicija menjaju način ponovne upotrebe podataka. Kod pretrage Eytzinger raspored i unapredno učitavanje smanjuju cenu nepravilnih memorijskih pristupa. Kod grafova reorganizacija čvorova značajno poboljšava lokalnost BFS obilaska.

Heš tabele pokazale su i ograničenje jednostavnog posmatranja lokalnosti. Linearno ispitivanje može imati veoma povoljan sekvencijalni obrazac pristupa, ali pri visokom faktoru popunjenosti neuspešna pretraga izvršava toliko dodatnog rada da dobra lokalnost nije dovoljna za dobre ukupne performanse. *Cache-aware* optimizacija zato treba da dopunjuje, a ne da zamenjuje algoritamsku analizu.

Poređenje dve procesorske platforme potvrdilo je da se osnovni principi lokalnosti ponavljaju na različitim arhitekturama, ali i da najbolja konkretna implementacija ne mora biti ista. Parametri poput veličine bloka ili granične vrednosti rekurzije zato treba da se biraju uzimajući u obzir ciljnu platformu i veličinu problema.

Hardverski brojači performansi dopunili su vremenska merenja, ali su istovremeno pokazali da jednu metriku ne treba tumačiti izolovano. Broj instrukcija, IPC, događaji vezani za keš i predikcija grananja imaju smisla tek kada se posmatraju zajedno sa ukupnim vremenom izvršavanja.

Na osnovu sprovedenih eksperimenata može se zaključiti da redosled pristupa, veličina aktivnog radnog skupa i fizički raspored podataka mogu imati veliki uticaj na praktične performanse. Razumevanje memorijske hijerarhije zato predstavlja važan deo projektovanja efikasnog softvera za probleme koji obrađuju velike količine podataka.

Dalji rad mogao bi obuhvatiti detaljnije mikroarhitekturne brojače, dodatne procesorske arhitekture i paralelne implementacije. Zanimljivo proširenje predstavljalo bi i sistematsko poređenje *cache-aware* i *cache-oblivious* pristupa, kao i automatsko podešavanje parametara za konkretnu platformu.

# Literatura

[1] R. E. Bryant, D. R. O'Hallaron, *Computer Systems: A Programmer's Perspective*, 3. izdanje, Pearson, 2016. Dostupno na: https://csapp.cs.cmu.edu/3e/about.html, pristupljeno 20. 9. 2026.

[2] J. L. Hennessy, D. A. Patterson, *Computer Architecture: A Quantitative Approach*, 6. izdanje, Morgan Kaufmann/Elsevier, 2017. Dostupno na: https://shop.elsevier.com/books/computer-architecture/hennessy/978-0-12-811905-1, pristupljeno 20. 9. 2026.

[3] M. Frigo, C. E. Leiserson, H. Prokop, S. Ramachandran, „Cache-Oblivious Algorithms“, *ACM Transactions on Algorithms*, vol. 8, br. 1, 2012, str. 1–22. DOI: 10.1145/2071379.2071383. Dostupno na: https://doi.org/10.1145/2071379.2071383, pristupljeno 20. 9. 2026.

[4] P.-V. Khuong, P. Morin, „Array Layouts for Comparison-Based Searching“, *ACM Journal of Experimental Algorithmics*, vol. 22, br. 1, članak 1.3, 2017. DOI: 10.1145/3053370. Dostupno na: https://doi.org/10.1145/3053370, pristupljeno 20. 9. 2026.

[5] T. H. Cormen, C. E. Leiserson, R. L. Rivest, C. Stein, *Introduction to Algorithms*, 4. izdanje, The MIT Press, 2022. Dostupno na: https://mitpress.mit.edu/9780262046305/introduction-to-algorithms/, pristupljeno 20. 9. 2026.

[6] P. Celis, P.-Å. Larson, J. I. Munro, „Robin Hood Hashing (Preliminary Report)“, *26th Annual Symposium on Foundations of Computer Science*, 1985, str. 281–288. DOI: 10.1109/SFCS.1985.48. Dostupno na: https://doi.org/10.1109/SFCS.1985.48, pristupljeno 20. 9. 2026.

[7] Y. Saad, *Iterative Methods for Sparse Linear Systems*, 2. izdanje, Society for Industrial and Applied Mathematics, 2003. DOI: 10.1137/1.9780898718003. Dostupno na: https://doi.org/10.1137/1.9780898718003, pristupljeno 20. 9. 2026.

[8] GNU Project, *Using the GNU Compiler Collection (GCC) 11.4.0: Optimize Options and x86 Options*, 2023. Dostupno na: https://gcc.gnu.org/onlinedocs/gcc-11.4.0/gcc/Optimize-Options.html i https://gcc.gnu.org/onlinedocs/gcc-11.4.0/gcc/x86-Options.html, pristupljeno 20. 9. 2026.

[9] Linux kernel project, *perf-stat(1), perf manual*, 2026. Dostupno na: https://man7.org/linux/man-pages/man1/perf-stat.1.html, pristupljeno 20. 9. 2026.
