/* 
 * File:   BranchBank.cpp
 * Author: sabra
 * 
 */

#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <cstdlib> // exit()
#include <cstring> // memset()
#include <sys/time.h>
#include <sys/socket.h> // socket(), connect(), sendto(), recvfrom()
#include <arpa/inet.h> // sockaddr_in, inet_addr()
#include <unistd.h> // close()
#include <errno.h>
#include <signal.h>

using namespace std;

#ifndef BRANCHBANK_H_
#define BRANCHBANK_H_

/**
 * Maximální počet markerů, které je možné v jednu chvíli zpracovat.
 */
#define MARKER_HISTORY_LIMIT 20

/**
 * Počet markerů v síti, po kterém je program vypnut.
 */
#define MARKER_LIMIT 5

/**
 * Doba čekání před vypnutí (kvůli socket_retarderu).
 */
#define WAITING 8

/**
 * Struktura pro přenos adres pobočkám.
 */
struct InitMessage {
    struct sockaddr_in clientAddress;
};

/**
 * Struktura pro přenos peněz a markerů.
 */
struct Message {
    // markery zprávy
    short marker[MARKER_HISTORY_LIMIT];

    // částka
    int value;

    // časová značka
    timeval timestamp;

    // barva (RGB v hexadecimálně)
    char color[7]; // 6 B + '\0'

    // odesilatel
    unsigned short sender;
};

/**
 * Struktura pro přenos a zpracování stavu pobočky.
 */
struct Snapshot {
    // číslo markeru
    short marker;

    // odesílatel
    short sender;

    // stav uzlu
    short clientState;

    // stav příchozích kanálů
    short chanelState;

    // čítač
    short count;
};

/**
 * Struktura pro identifikaci pobočky a obslužného vlákna.
 */
struct ClientIdentifier {
    // socket pobočky
    int clientSocket;

    // adresa pobočky
    struct sockaddr_in clientAddress;

    // vlákno pro obsluhu pobočky
    pthread_t thread;
    
    short int id;
};

/**
 * Třída s implementovanou činností pobočky. Vytvoří za pomocí banky spojení
 * s ostatníma pobočkama. V náhodných intervalech realizuje převod peněz a
 * zjišťování globálního stavu.
 */
class BranchBank {
public:
    /**
     * Konstruktor pro nastavení parametrů pobočky.
     */
    BranchBank(unsigned short port, unsigned short id, char *color);

    /**
     * Ukončí všechna spojení a zavře soubory.
     */
    virtual ~BranchBank();

    /**
     * Spustí hlavní činnost pobočky.
     */
    void start();
private:
    /** Délka fronty příchozích spojení. */
    static const unsigned int QUEUE_LENGTH = 5;

    /** Základní částka pobočky. Na bance musí být nastavena shodná. */
    static const int DEFAULT_AMOUNT = 100;

    /** Číslo lokálního portu. */
    unsigned short localPort;

    /** IP adresa banky. */
    string bankIp;

    /** Číslo portu banky. */
    unsigned short bankPort;

    /** Socket banky. */
    int bankSocket;

    /** Seznam poboček. */
    vector<ClientIdentifier> clients;

    /** Seznam aktuálních zjišťování globálního stavu. */
    vector<Snapshot> snapshots;

    /** Indikuje existenci vlákna pro odesílání. */
    bool sendThreadExist;

    /** Aktuální částka na pobočce. */
    int money;

    /** ID pobočky. */
    unsigned short identifier;

    /** Poslední dokončený snapshot. */
    vector<short> snapshotHistory;

    /** Barva pro logování. */
    char *color;

    /** Stream pro logování přijatých zpráv do souboru. */
    ofstream trafficFile;

    /** Stream pro logování výpočtu do souboru na základě přijatých zpráv. */
    ofstream marksFile;

    /** Mutex. */
    pthread_mutex_t mutex;

    /**
     * Zpracuje zprávu s penězi.
     */
    void handleMoney(Message message);

    /**
     * Zpracuje příchozí marker.
     */
    void handleMarker(Message & message);

    /**
     * Odešle informace o stavu pobočky a příchozích kanálů bance.
     */
    void sendSnapshot(Snapshot snapshot, Message & message);

    /**
     * Logování přijatých zpráv.
     */
    void logMessage(Message &message, int money);

    /**
     * Logování stavu uzlu a příchozích kanálů.
     */
    void logSnapshot(Snapshot snapshot, char *color);

    /**
     * Převede časovou značku na řetězec (v sekundách).
     */
    string timevalToString(timeval time);

    /**
     * Převed celé číslo na řetězec.
     */
    string intToString(int value);

    /**
     * Logování markerů.
     */
    void logMarker(Message &message);

    /**
     * Vrátí aktuální čas s přesností na mikrosekundy.
     */
    timeval getTime();

    /**
     * Spustí vlákno pro odesílání peněz pobočkám.
     */
    static void* startSend(void *object);

    /**
     * Tělo vlákna pro odesílání peněz pobočkám. Částka i intervaly odesílání
     * jsou náhodné.
     */
    void sendHandler();

    /**
     * Získání pořadového čísla markeru od banky.
     */
    short getMarkerId();

    /**
     * Spustí vlákno pro odesílání markerů.
     */
    static void* startMarkerSender(void *object);

    /**
     * Tělo vlákna pro odesílání markerů. Odesílá se v náhodných intervalech.
     */
    void markerSender();

    /**
     * Vrátí index pobočky, kterou vlákno obsluhuje.
     */
    int getClientIndex();

    /**
     * Vrátí index snapshotu, pokud již existuje.
     */
    int getSnapshotIndex(short marker);

    /**
     * Vytvoření vlákna pro příjem a odesílání.
     */
    void createBranchHandler(int clientSocket, struct sockaddr_in clientAddress);

    /**
     * Naváže spojení s pobočkou.
     */
    void connectToBranchBank(struct sockaddr_in clientAddress);

    /**
     * Spustí vlákno pro příjem.
     */
    static void* startReceive(void *object);

    /**
     * Tělo vlákna pro příjem.
     */
    void receiveHandler();

    /**
     * Naváže spojení s bankou.
     */
    void connectToBank();

    /**
     * Přijímá spojení s pobočkou.
     */
    void acceptBranchConnection();
};

#endif /* BRANCHBANK_H_ */
