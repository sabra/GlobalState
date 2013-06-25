/* 
 * File:   Bank.cpp
 * Author: sabra
 * 
 */

#include "BranchBank.h"

/**
 * Konstruktor pro nastavení parametrů pobočky.
 */
BranchBank::BranchBank(unsigned short localPort, unsigned short id, char *color) {
    this->localPort = localPort;
    this->bankIp = "127.0.0.1";
    this->bankPort = 5000;
    this->identifier = id;
    this->color = color;


    // číslo posledního snapshotu
    snapshotHistory.push_back(0);

    // řídí běh vláken pro odesílání
    this->sendThreadExist = false;

    // základní částka pobočky
    this->money = DEFAULT_AMOUNT;

    // inicializace generátoru náhodných čísel
    srand((unsigned) getTime().tv_usec);

    // vytvoření souborů pro logování
    string trafficFileName = "traffic" + intToString(identifier) + ".txt";
    trafficFile.open(trafficFileName.c_str());

    string marksFileName = "marks" + intToString(identifier) + ".txt";
    marksFile.open(marksFileName.c_str());
}

/**
 * Ukončí všechna spojení a zavře soubory.
 */
BranchBank::~BranchBank() {
    // ukončení spojení s bankou
    close(bankSocket);

    // ukončení spojení s pobočkami
    for (unsigned int i = 0; i < clients.size(); ++i) {
        close(clients[i].clientSocket);
    }

    // uzavření logovacích souborů
    trafficFile.close();
    marksFile.close();
}

/**
 * Spustí hlavní činnost pobočky.
 */
void BranchBank::start() {
    // spojení s bankou
    connectToBank();

    // příjem spojení s pobočkami
    acceptBranchConnection();
}

/**
 * Logování přijatých zpráv.
 */
void BranchBank::logMessage(Message &message, int money) {
    trafficFile << "- color: '#000000'" << endl;
    trafficFile << "  data: '" << message.value << "'" << endl;
    trafficFile << "  dst: {p: SUB" << identifier << ", ts: " << timevalToString(
            getTime()) << "}" << endl;
    trafficFile << "  src: {p: SUB" << message.sender << ", ts: "
            << timevalToString(message.timestamp) << "}" << endl;

    marksFile << "- {color: '#888888', p: SUB" << identifier << ", text: '"
            << money << "', ts: " << timevalToString(getTime()) << "}" << endl;
}

/**
 * Logování markerů.
 */
void BranchBank::logMarker(Message &message) {
    trafficFile << "- color: '#" << message.color << "'" << endl;
    trafficFile << "  data: 'M" << message.marker[0] << "'" << endl;
    trafficFile << "  dst: {p: SUB" << identifier << ", ts: " << timevalToString(
            getTime()) << "}" << endl;
    trafficFile << "  src: {p: SUB" << message.sender << ", ts: "
            << timevalToString(message.timestamp) << "}" << endl;
}

/**
 * Logování stavu uzlu a příchozích kanálů.
 */
void BranchBank::logSnapshot(Snapshot snapshot, char *color) {
    marksFile << "- {color: '#" << color << "', p: SUB" << identifier
            << ", text: '" << snapshot.clientState << "|" << snapshot.chanelState
            << "', ts: " << timevalToString(getTime()) << "}" << endl;
}

/**
 * Vrátí aktuální čas s přesností na mikrosekundy.
 */
timeval BranchBank::getTime() {
    timeval time;
    gettimeofday(&time, NULL);

    return time;
}

/**
 * Převede časovou značku na řetězec (v sekundách).
 */
string BranchBank::timevalToString(timeval time) {
    stringstream temp;
    temp << time.tv_sec << "." << time.tv_usec;

    return temp.str();
}

/**
 * Převed celé číslo na řetězec.
 */
string BranchBank::intToString(int value) {
    ostringstream temp;
    temp << value;

    return temp.str();
}

/**
 * Spustí vlákno pro odesílání peněz pobočkám.
 */
void* BranchBank::startSend(void *object) {
    reinterpret_cast<BranchBank *> (object)->sendHandler();

    return NULL;
}

/**
 * Tělo vlákna pro odesílání peněz pobočkám. Částka i intervaly odesílání
 * jsou náhodné.
 */
void BranchBank::sendHandler() {
    int clientSocket;
    ClientIdentifier *cid;

    while (sendThreadExist) {
        // čekání
        sleep(rand() % 10 + 1);

        Message message;
        memset(&message, 0, sizeof (Message));
        message.sender = identifier;

        unsigned int index = rand() % clients.size();
        cid = &clients[index];
        clientSocket = cid->clientSocket;

        // indikuje, že byl přijat marker
        unsigned int i;
        for (i = 0; i < snapshotHistory.size(); ++i) {
            message.marker[i] = snapshotHistory[i];
        }

        for (unsigned int j = 0; j < snapshots.size() && (i + 1 + j) < MARKER_HISTORY_LIMIT; ++j) {
            message.marker[i + j] = snapshots[j].marker;
        }

        // převáděné peníze
        message.value = rand() % 10 + 1;
        message.timestamp = getTime();

        if (write(clientSocket, (void*) &message, sizeof (Message))
                != sizeof (Message)) {
            cerr << "send() sent a different number of bytes than expected" << endl;
            cerr << "errno: " << strerror(errno) << endl;
        }

        // výpočet aktuální částky
        money = money - message.value;

        // logování
        marksFile << "- {color: '#888888', p: SUB" << identifier << ", text: '"
                << money << "', ts: " << timevalToString(message.timestamp) << "}"
                << endl;

        cout << "[SUB" << identifier << "] Sent money to SUB" << getClientIndex() << ": " << message.value << " => "
                << money;

        // čtení markeru zprávy
        for (unsigned int i = 0; i < MARKER_HISTORY_LIMIT; ++i) {
            if (message.marker[i] == 0) {
                break;
            }

            cout << " M" << message.marker[i] << ";";
        }

        cout << endl;
    }

    // čekání před vypnutím (kvůli socket_retarderu).
    sleep(WAITING);

    cout << "[SUB" << identifier << "] Exit" << endl;
    exit(0);
}

/**
 * Spustí vlákno pro odesílání markerů.
 */
void* BranchBank::startMarkerSender(void *object) {
    reinterpret_cast<BranchBank *> (object)->markerSender();

    return NULL;
}

/**
 * Získání pořadového čísla markeru od banky.
 */
short BranchBank::getMarkerId() {
    Snapshot snapshot;
    memset(&snapshot, 0, sizeof (Snapshot));
    snapshot.sender = identifier;

    if (write(bankSocket, (void*) &snapshot, sizeof (Snapshot))
            != sizeof (Snapshot)) {
        cerr << "send() sent a different number of bytes than expected" << endl;
        cerr << "errno: " << strerror(errno) << endl;
    }

    if (read(bankSocket, (void*) ((&snapshot)), sizeof (Snapshot))
            != sizeof (Snapshot)) {
        cerr << "recv() failed or connection closed prematurely" << endl;
        cerr << "errno: " << strerror(errno) << endl;
    }

    return snapshot.marker;
}

/**
 * Tělo vlákna pro odesílání markerů. Odesílá se v náhodných intervalech.
 */
void BranchBank::markerSender() {
    while (sendThreadExist) {
        // čekání
        sleep((rand() % 20 + 1));

        Message message;
        memset(&message, 0, sizeof (Message));
        message.marker[0] = getMarkerId();

        // ukončení vlákna
        if (message.marker[0] > MARKER_LIMIT) {
            sendThreadExist = false;
            break;
        }

        message.timestamp = getTime();
        message.sender = identifier;
        strcpy(message.color, color);

        // uložení stavu uzlu
        Snapshot snapshot;
        memset(&snapshot, 0, sizeof (Snapshot));
        snapshot.sender = identifier;
        snapshot.marker = message.marker[0];
        snapshot.count = clients.size();
        snapshot.clientState = money;

        snapshots.push_back(snapshot);

        cout << "[SUB" << identifier << "] Sent marker M" << message.marker[0]
                << endl;

        // odeslání markeru všem pobočkám
        for (unsigned int i = 0; i < clients.size(); ++i) {
            if (write(clients[i].clientSocket, (void*) &message, sizeof (Message))
                    != sizeof (Message)) {
                cerr << "send() sent a different number of bytes than expected" << endl;
                cerr << "errno: " << strerror(errno) << endl;
            }
        }

        sleep((rand() % 10 + 1));
    }
}

/**
 * Spustí vlákno pro příjem.
 */
void* BranchBank::startReceive(void *object) {
    reinterpret_cast<BranchBank *> (object)->receiveHandler();

    return NULL;
}

/**
 * Vrátí index pobočky, kterou vlákno obsluhuje.
 */
int BranchBank::getClientIndex() {
    for (unsigned int i = 0; i < clients.size(); ++i) {
        if (pthread_self() == clients.at(i).thread) {
            return i;
        }
    }

    return 0;
}

/**
 * Vrátí index snapshotu, pokud již existuje.
 */
int BranchBank::getSnapshotIndex(short marker) {
    for (unsigned int i = 0; i < snapshots.size(); ++i) {
        if (snapshots[i].marker == marker) {
            return i;
        }
    }

    return -1;
}

/**
 * Odešle informace o stavu pobočky a příchozích kanálů bance.
 */
void BranchBank::sendSnapshot(Snapshot snapshot, Message & message) {
    if (write(bankSocket, (void*) &snapshot, sizeof (Snapshot))
            != sizeof (Snapshot)) {
        cerr << "send() sent a different number of bytes than expected" << endl;
        cerr << "errno: " << strerror(errno) << endl;
    }

    snapshotHistory.push_back(snapshot.marker);

    // logování
    logSnapshot(snapshot, message.color);

    cout << "[SUB" << identifier << "] Sent snapshot M" << snapshot.marker
            << " to BANK: " << snapshot.clientState << "|" << snapshot.chanelState
            << endl;

    // ukonční vlákna pro odesílání po dosažení limitu
    if (snapshotHistory.size() == MARKER_LIMIT + 1) {
        sendThreadExist = false;
    }
}

/**
 * Zpracuje příchozí marker.
 */
void BranchBank::handleMarker(Message & message) {
    cout << "[SUB" << identifier << "] Received marker M"
            << message.marker[0] << " from SUB" << message.sender << endl;

    // logování
    logMarker(message);

    int index = getSnapshotIndex(message.marker[0]);
    if (index >= 0) { // snapshot již byl zahájen
        snapshots[index].count--;

        // pokud je hodnota čítače nula, pak se jedná o konec zjišťování glob. stavu
        if (snapshots[index].count == 0) {
            // odešle stav pobočky a příchozích kanálů bance
            sendSnapshot(snapshots[index], message);

            // odstraní informace o snapshotu
            snapshots.erase(snapshots.begin() + getSnapshotIndex(message.marker[0]));
        }
    } else { // první marker
        // uložení stavu pobočky
        Snapshot snapshot;
        memset(&snapshot, 0, sizeof (Snapshot));
        snapshot.sender = identifier;
        snapshot.marker = message.marker[0];
        snapshot.count = clients.size() - 1;
        snapshot.clientState = money;

        if (snapshot.count > 0) {
            snapshots.push_back(snapshot);
        } else {
            // odešle stav pobočky a příchozích kanálů bance
            sendSnapshot(snapshot, message);
        }

        // přepošle marker všem pobočkám
        message.sender = identifier;
        message.timestamp = getTime();
        for (unsigned int i = 0; i < clients.size(); ++i) {
            if (write(clients[i].clientSocket, (void*) &message, sizeof (Message))
                    != sizeof (Message)) {
                cerr << "send() sent a different number of bytes than expected" << endl;
                cerr << "errno: " << strerror(errno) << endl;
            }
        }
    }
}

/**
 * Zpracuje zprávu s penězi.
 */
void BranchBank::handleMoney(Message message) {
    // uložení stavu kanálu
    for (unsigned int i = 0; i < snapshots.size(); ++i) {
        bool hasMarker = false;
        for (int j = 0; j < MARKER_HISTORY_LIMIT; ++j) {
            if (message.marker[j] == snapshots[i].marker) {
                //cout << message.marker[j] << " != " << snapshots[i].marker << " /// " << message.value << endl;
                hasMarker = true;
            }

            if (j != MARKER_HISTORY_LIMIT - 1) {
                if (message.marker[j + 1] == 0) {
                    break;
                }
            }
        }

        if (!hasMarker) {
            snapshots[i].chanelState += message.value;
        }
    }

    // výpočet částky
    money = money + message.value;

    // logování
    logMessage(message, money);


    // kontrolní výpis
    cout << "[SUB" << identifier << "] Received from SUB" << message.sender
            << ": " << message.value << " => " << money;

    for (unsigned int i = 0; i < MARKER_HISTORY_LIMIT; ++i) {
        if (message.marker[i + 1] == 0) {
            break;
        }

        if (message.marker[i] != 0) {
            cout << " M" << message.marker[i] << ";";
        }
    }

    cout << endl;
}

/**
 * Tělo vlákna pro příjem.
 */
void BranchBank::receiveHandler() {
    unsigned int index = getClientIndex();
    signal(SIGPIPE, SIG_IGN);
    signal(SIGSEGV, SIG_IGN);

    if (index < 0) {
        cerr << "Error: Thread start failed." << endl;
        return;
    }

    ClientIdentifier *cid;
    cid = &clients[index];

    //struct sockaddr_in clientAddress = cid->clientAddress;
    int clientSocket = cid->clientSocket;

    // příjem zpráv od poboček
    while (true) {
        Message message;

        // příjem zprávy
        if (read(clientSocket, (void*) ((&message)), sizeof (Message))
                != sizeof (Message)) {
            //cerr << "[SUB" << identifier << "] Error: " << strerror(errno) << endl;

            // čekání před vypnutím (kvůli socket_retarderu).
            sleep(WAITING);

            cout << "[SUB" << identifier << "] Exit" << endl;
            exit(0);
        }

        pthread_mutex_lock(&mutex);

        // zpráva je marker, nenese peníze
        if (message.marker[0] > 0 && message.value == 0) {
            handleMarker(message);
        }

        // zpráva nese peníze
        if (message.value) {
            handleMoney(message);
        }

        pthread_mutex_unlock(&mutex);
    }
}

/**
 * Naváže spojení s bankou.
 */
void BranchBank::connectToBank() {
    struct sockaddr_in bankAddress; /* Echo server address */

    // vytvoření socketu
    if ((bankSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        cerr << "socket() failed" << endl;
    }

    // konstrukce adresy
    memset(&bankAddress, 0, sizeof (bankAddress));
    bankAddress.sin_family = AF_INET;
    bankAddress.sin_addr.s_addr = inet_addr(bankIp.c_str());
    bankAddress.sin_port = htons(bankPort);

    // vytvoření spojení
    if (connect(bankSocket, (struct sockaddr *) &bankAddress, sizeof (bankAddress))
            < 0) {
        cerr << "Spojeni s bankou se nezdarilo" << endl;
    }

    cout << "Spojeni s bankou navazano" << endl;

    // odeslání čísla lokálního portu bance
    if (write(bankSocket, (char *) &localPort, sizeof (unsigned short))
            != sizeof (unsigned short)) {
        cerr << "send() sent a different number of bytes than expected" << endl;
        cerr << "errno: " << strerror(errno) << endl;
    }

    cout << "Odeslano cislo lokalniho portu bance" << endl;

    // příjem adres poboček od banky
    while (true) {
        InitMessage initMessage;

        if (read(bankSocket, (void *) &initMessage,
                sizeof (InitMessage)) != sizeof (InitMessage)) {
            cerr << "recv() failed or connection closed prematurely" << endl;
            cerr << "[SUB" << identifier << "] Error: " << strerror(errno) << endl;
        }

        if (initMessage.clientAddress.sin_port == 0) {
            break;
        }

        connectToBranchBank(initMessage.clientAddress);
    }
}

/**
 * Naváže spojení s pobočkou.
 */
void BranchBank::connectToBranchBank(struct sockaddr_in clientAddress) {
    int clientSocket;

    // vytvoření socketu
    if ((clientSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        cerr << "socket() failed" << endl;
    }

    // vytvoří spojení s pobočkou
    if (connect(clientSocket, (struct sockaddr *) (&clientAddress),
            sizeof (clientAddress)) < 0) {
        cerr << "Spojeni s pobockou se nezdarilo" << endl;
    }

    // vytvoří vlákna pro příjem a odesílání zpráv
    createBranchHandler(clientSocket, clientAddress);

    cout << "[SUB" << identifier << "] Connected to " << inet_ntoa(clientAddress.sin_addr) << ":" << ntohs(clientAddress.sin_port) << endl;
}

/**
 * Vytvoření vlákna pro příjem a odesílání.
 */
void BranchBank::createBranchHandler(int clientSocket,
        struct sockaddr_in clientAddress) {
    pthread_t receiveThread;
    //signal(SIGPIPE, SIG_IGN);
    // uložení adresy klienta a ukazatele na vlákno

    // spuštění vlákna
    pthread_create(&receiveThread, NULL, &BranchBank::startReceive, this);

    ClientIdentifier cid;
    cid.clientSocket = clientSocket;
    cid.clientAddress = clientAddress;
    cid.thread = receiveThread;
    clients.push_back(cid);

    if (!sendThreadExist) {
        sendThreadExist = true;

        // vlákno pro převod peněz
        pthread_t sendThread;
        pthread_create(&sendThread, NULL, &BranchBank::startSend, this);

        // vlákno pro odesílání markeru
        pthread_t markerSendThread;
        pthread_create(&markerSendThread, NULL, &BranchBank::startMarkerSender,
                this);
    }
}

/**
 * Přijímá spojení s pobočkou.
 */
void BranchBank::acceptBranchConnection() {
    int localSocket;
    int clientSocket;
    struct sockaddr_in localAddress;
    struct sockaddr_in remoteAddress;
    unsigned int remoteAddressLenght;

    // vytvoření socketu
    if ((localSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        cerr << "socket() failed" << endl;
    }

    // konstrukce adresy
    memset(&localAddress, 0, sizeof (localAddress));
    localAddress.sin_family = AF_INET;
    localAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddress.sin_port = htons(localPort);

    // přidružení adresy k socketu
    if (bind(localSocket, (struct sockaddr *) &localAddress, sizeof (localAddress))
            < 0) {
        cerr << "bind() failed" << endl;
    }

    // naslouchání na socketu
    if (listen(localSocket, QUEUE_LENGTH) < 0) {
        cerr << "listen() failed" << endl;
    }

    // příjem spojení
    while (true) {
        remoteAddressLenght = sizeof (remoteAddress);

        /* Wait for a client to connect */
        if ((clientSocket = accept(localSocket, (struct sockaddr *) &remoteAddress,
                &remoteAddressLenght)) < 0) {
            cerr << "accept() failed" << endl;
        }

        // vytvoří vlákna pro příjem a odesílání zpráv
        createBranchHandler(clientSocket, remoteAddress);
    }
}
