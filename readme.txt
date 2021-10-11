Protocoale de comunicatii - Tema 2
Aplicatie client-server TCP si UDP pentru gestionarea mesajelor

Student: Hodoboc-Velescu Tudor
Grupa: 325CC

    Structuri folosite:
        - unordered_map in care cheia este id-ul unui client si valoarea
    socket-ului de unde isi primeste date; daca aceasta este -1, atunci clientul
    nu este conectat
        - unordered_map in care retin un vector de id-uri ale clientilor atasati
    unei chei ce reprezinta topic-ul la care acestia sunt abonati
        - unordered_map in care cheia reprezinta socket-ul pe care este conectat
    un client, iar valoarea reprezinta id-ul clientului
        - am o structura folosita pentru stocarea mesajelor de tip udp(topicul,
    tip-ul si datele)
        - o structura ce modeleaza mesajele trimise de la server catre
    subscriber si vece-versa; aceasta structura exista pentru a evita trunchierea
    mesajelor si face parte din implementarea protocolului la nivel de aplicatie
        - structura "mesaj_conectare" se foloseste pentru trimiterea id-ului de
    la subscriberi la server
        - "mesaj_subscribe" se foloseste pentru a trimite mesaje ce tin de
    functiile de subscribe si unsubscribe ale clientului

    Logica program subscriber:
        - se dezactiveaza buffering-ul la STDOUT
        - se deschide un socket
        - se conecteaza la server
        - se dezactiveaza algoritmul lui Nagle
        - se trimite un mesaj de conectare avand id-ul clientului
        - la nivelul aplicatiei se trimite un mesaj_tcp de tip 0 ce reprezinta un
    mesaj de conectare
        - se adauga STDIN-ul si socket-ul intr-un file descriptor set
        - se intra intr-un loop infinit si se asteapta mesaje de la STDIN sau
    de la socket-ul de server
        - in momentul in care se primeste un mesaj de la tastatura, exista 3
    posibilitati: de exit, de subscribe sau de unsubscribe
        - pentru input de tip "exit" se iese din loop si se inchide socket-ul
    alocat server-ului
        - pentru input de tip subscribe se verifica argumentele: sa fie mai mult
    de doua, primul sa aiba maxim 50 de caracter si al doilea sa fie cifra
        - pentru input de tip unsubscribe se verifica ca argumentul al doilea sa
    exista si sa fie mai mic de 50 de caractere
        - atat pentru subscribe, cat si pentru unsubscribe se asteapta un mesaj
    de un octet de la server ce reprezinta confirmarea; dupa aceasta se afiseaza
    un mesaj corespunzator
        - pentru mesajele de la server, acestea se citesc intr-o structura de
    tip mesaj_tcp
        - daca se primesc 0 octeti, atunci s-a pierdut conexiunea cu server-ul
    si se opreste aplicatia
        - mesajele primite sunt convertite intr-un mesaj de tip mesaj_udp descris
    mai sus si apoi se copiaza si datele referitoare la ip-ul clientului UDP si
    portul de intrare pe server
        - in functie de tipul de date primite(mesaj_udp.tip) se face conversia si
    se afiseaza mesajul la STDOUT

    Logica server:
        - similar ca in subscriber, se dezactiveaza buffering-ul la STDOUT
        - se porneste un socket pentru clientii de tip UDP si un socket pentru
    cei de tip TCP pentru a primi cereri de conectare
        - se dezactiveaza algoritmul lui Nagle pentru socket-ul de listen TCP
        - sunt setate socket-urile intr-un file descriptor set
        - se incepe functionarea normala a server-ului
        - se selecteaza socket-ul pe care exista date
        - daca este de tip udp, se primesc datele si se creaza un mesaj de tip
    mesaj_tcp ce va fi transmis mai departe
        - selectez topic-ul mesajului si cu ajutorul unui map se prea lista de
    clienti ce sunt abonati la acest topic
        - daca clientul este logat, ii este transmis mesajul
        - daca se primeste o conexiune noua de tip TCP, se face acceptul, se
    dezactiveaza algoritmul lui Nagle si apoi se asteapta id-ul
        - daca id-ul este deja conectat, se inchide socket-ul si se afiseaza un
    mesaj corespunzator
        - id-ul este adaugat in doua map-uri in functie daca acesta a mai
    fost conectat sau nu inainte si se afiseaza un mesaj corespunzator
        - socket-ul este si acesta adaugat in setul de file descriptors
        - daca mesaul este de la STDIN, se verifica daca este exit, caz in care
    se inchid toate socket-urile si se iese din loop, apoi se inchid si ultimele
    doua socket-uri pentru UDP si TCP 
        - daca mesajul este de la alt socket, atunci s-a primit un mesaj de la
    un subscriber
        - daca nu s-a primit niciun byte, atunci subscriber-ul s-a deconectat
    si si este eliminat din liste de socketi activi si din setul de file descriptors
        - altfel, s-a primit un mesaj legat de subscriptii
        - daca se doreste un subscribe la un topic, se verifica daca exista vreo
    intrare pentru topicul respectiv; in caz afirmativ, se adauga in lista respectiva
    de clienti stocata in map-ul de topic-clienti; in caz negativ, se creaza o
    intrare pentru respectivul topic si se adauga clientul
        -  daca se doreste unsubscribe, se verifica daca exita o intrare pentu
    topic; in caz negativ, nu se face nimic; in caz afirmativ, se ia lista de
    clienti si, daca clientul care a facut cererea exista in acea lista, este
    eliminat
        - la final, atat pentru subscribe, cat si pentru unsubscribe, se trimite
    un mesaj de verificare de un byte

    Alte elemente:
        - s-au folosit unorder_map-uri pentru a se optimiza cautarile in O(1).
        - pentru partea de SF s-a incercat o implementare, insa aceasta nu a
    dorit sa fie implementata si nu am reusit sa o implementez
        - pentru partea de asigurare a transmiterii mesajelor de tip TCP s-a
    incercat sa se retrimita mesajul partial pentru a se asigura ca nu se pierd
    niciun byte din mesaj; de asemenea, acest lucru nu s-a reusit sa fie
    implementat, asa ca exista o posibilitate ca testele de quickflow sa mai pice
        - imi pare sincer rau daca exista greseli gramaticale sau de exprimare
    in readme sau daca exista lucruri in cod care nu au sens sau pentru alte
    probleme existente
