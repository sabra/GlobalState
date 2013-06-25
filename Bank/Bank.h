/* 
 * File:   Bank.h
 * Author: sabra
 * 
 */

#include <iostream>
#include <vector>
#include <map>
#include <cstdlib> // exit()
#include <cstring> // memset()
#include <sys/socket.h> // socket(), connect(), sendto(), recvfrom()
#include <arpa/inet.h> // sockaddr_in, inet_addr()
#include <unistd.h> // close()
#include <pthread.h>
#include <errno.h>

using namespace std;

#ifndef BANK_H_
#define BANK_H_

/**
 * Struktura pro přenos adres pobočkám.
 */
struct InitMessage {
	struct sockaddr_in clientAddress;
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

	// lokální port pobočky
	unsigned short clientPort;
};

/**
 * Třída s implementovanou činností banky. Pomáhá pobočkám realizovat spojení
 * mezi sebou. Uchovává adresy poboček, rozesílá nově připojeným pobočkám
 * adresy již připojených, aby jim umožňila vytvořit mezi sebou spojení. Kromě
 * této činnosti zachytává glob. stavy poboček.
 */
class Bank {
public:

	/**
	 * Konstruktor nastaví číslo portu banky.
	 */
	Bank(unsigned short localPort);

	/**
	 * Destruktor.
	 */
	virtual ~Bank();

	/**
	 * Zahájí činnost banky.
	 */
	void start();

private:

	/**	Délka fronty příchozích spojení. */
	static const unsigned QUEUE_LENGTH = 5;

	/**	Základní částka každé pobočky (musí být na pobočkách nastaveno stejně. */
	static const int DEFAULT_AMOUNT = 100;

	/** Číslo portu banky. */
	unsigned short localPort;

	/** Seznam poboček. */
	vector<ClientIdentifier> clients;

	/**	Mapa pro uchovávání a kontrolu glob. stabu. */
	map<short, Snapshot> snapshots;

	/**	Pořadové číslo markeru. */
	short markerSequence;

	/** Mutex. */
	pthread_mutex_t mutex;

	/**
	 * Odešle adresy všech již připojených poboček nové pobočce.
	 */
	void sendBranchAddress(unsigned int & index);

	/**
	 * Odešle pořadové číslo markeru.
	 */
	void sendMarkerSeq(Snapshot snapshot, unsigned int & index);

	/**
	 * Zpracování přijatého stavu pobočky a kontrola globálního stavu (po přijetí
	 * dat od všech poboček).
	 */
	void controlSnapshot(Snapshot & snapshot);

	/**
	 * Vrácí index pobočky, kterou obsluhuje vlákno.
	 */
	int getClientIndex();

	/**
	 * Tělo vlákna pro obsluhu poboček. Přijme a uloží od pobočky číslo
	 * komunikačního portu a poté přijímá od poboček hodnoty globálních stavů.
	 */
	void handler();

	/**
	 * Spustí vlákno pro zpracování spojení s pobočkou.
	 */
	static void* startHandler(void *object);

	/**
	 * Vytvoří vlákno pro zpracování spojení s pobočkou a uloží informace o spojení
	 * do seznamu poboček.
	 */
	void createClientHandler(int clientSocket, struct sockaddr_in clientAddress);

	/**
	 * Přijímá spojení s pobočkama a vytváří pro každé spojení samostatné obslužné
	 * vlákno.
	 */
	void acceptConnection();
};

#endif /* BANK_H_ */
