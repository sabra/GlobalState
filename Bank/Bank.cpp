/* 
 * File:   Bank.cpp
 * Author: sabra
 * 
 */

#include "Bank.h"
#include "Linker.h"

/**
 * Konstruktor nastaví číslo portu banky.
 */
Bank::Bank(unsigned short localPort) {
	this->localPort = localPort;
	this->markerSequence = 0;

	pthread_mutex_init(&mutex, NULL);
}

/**
 * Destruktor.
 */
Bank::~Bank() {
	// TODO Auto-generated destructor stub
}

/**
 * Zahájí činnost banky.
 */
void Bank::start() {
	cout << "[BANK] Start" << endl;

	// příjem spojení s pobočkami
	acceptConnection();
}

/**
 * Vrácí index pobočky, kterou obsluhuje vlákno.
 */
int Bank::getClientIndex() {
	for (unsigned int i = 0; i < clients.size(); ++i) {
		if (pthread_self() == clients.at(i).thread) {
			return i;
		}
	}

	return -1;
}

/**
 * Zpracování přijatého stavu pobočky a kontrola globálního stavu (po přijetí
 * dat od všech poboček).
 */
void Bank::controlSnapshot(Snapshot & snapshot) {
	cout << "[BANK] Received snapshot M" << snapshot.marker << " from SUB"
			<< snapshot.sender << ": " << snapshot.clientState << "|"
			<< snapshot.chanelState << endl;

	map<short, Snapshot>::iterator it;
	it = snapshots.find(snapshot.marker);

	if (it == snapshots.end()) {
		snapshot.count = clients.size() - 1;
		if (snapshot.count == 0) {
			if ((snapshot.clientState + snapshot.chanelState) % (clients.size()
					* DEFAULT_AMOUNT) == 0) {
				cout << "[BANK] Snapshot M" << snapshot.marker << " OK"
						<< endl;
			} else {
				cout << "[BANK] Snapshot M" << snapshot.marker << " FAILED"
						<< endl;
			}
		} else {
			snapshots.insert(pair<short, Snapshot> (snapshot.marker, snapshot));
		}
	} else {
		it->second.count--;
		it->second.clientState += snapshot.clientState;
		it->second.chanelState += snapshot.chanelState;
		if (it->second.count == 0) {
			if ((it->second.clientState + it->second.chanelState) % (clients.size()
					* DEFAULT_AMOUNT) == 0) {
				cout << "[BANK] SNAPSHOT M" << snapshot.marker << " OK"
						<< endl;
			} else {
				cout << "[BANK] SNAPSHOT M" << snapshot.marker << " FAILED"
						<< endl;
			}
			snapshots.erase(it);
		}
	}
}

/**
 * Odešle pořadové číslo markeru.
 */
void Bank::sendMarkerSeq(Snapshot snapshot, unsigned int & index) {
	markerSequence++;
	snapshot.marker = markerSequence;
	if (write(clients[index].clientSocket, (void*) &snapshot, sizeof(Snapshot))
			!= sizeof(Snapshot)) {
		cerr << "[BANK] Error: " << strerror(errno) << endl;
	}
}

/**
 * Odešle adresy všech již připojených poboček nové pobočce.
 */
void Bank::sendBranchAddress(unsigned int & index) {
	InitMessage initMessage;
	memset(&initMessage, 0, sizeof(InitMessage));

	for (unsigned int j = 0; j < index + 1; ++j) {
		if (clients[j].thread != pthread_self()) {

			// data k odeslání
			initMessage.clientAddress = clients[j].clientAddress;
			initMessage.clientAddress.sin_port = htons(clients[j].clientPort);

			// odeslání IP adresy a č. portu pobočce
			if (write(clients[index].clientSocket, (void*) (&initMessage),
					sizeof(InitMessage)) != sizeof(InitMessage)) {
				cerr << "[BANK] Error: " << strerror(errno) << endl;
			}
                        cout << "Odeslana data pobocek" << endl;
		}
	}

	// odeslání zprávy značící konec
	memset(&initMessage, 0, sizeof(InitMessage));
	if (write(clients[index].clientSocket, (void*) (&initMessage),
			sizeof(InitMessage)) != sizeof(InitMessage)) {
		cerr << "[BANK] Error: " << strerror(errno) << endl;
	}
        
        cout << "Odeslana zprava znacici konec" << endl;
}

/**
 * Tělo vlákna pro obsluhu poboček. Přijme a uloží od pobočky číslo
 * komunikačního portu a poté přijímá od poboček hodnoty globálních stavů.
 */
void Bank::handler() {
	// index obsluhované pobočky
	unsigned int index = getClientIndex();

	// načtení informací o pobočce
	if (index < 0) {
		cerr << "[BANK] Error: Thread start failed." << endl;
		return;
	}

	// odeslání adres již připojených poboček
	sendBranchAddress(index);

	// příjem a zpracování zpráv s glob. stavem
	while (true) {
		Snapshot snapshot;

		if ((read(clients[index].clientSocket, (void *) &snapshot, sizeof(Snapshot)))
				!= sizeof(Snapshot)) {
//			cerr << "[BANK] Error: " << strerror(errno) << endl;
			break;
		}

		// zpracování a kontrola glob. stavu
                //cout << snapshot.marker << endl;
		if (snapshot.marker > 0) {
			pthread_mutex_lock(&mutex);
			controlSnapshot(snapshot);
			pthread_mutex_unlock(&mutex);
		}

		// odeslání sekvenčního čísla markeru
		if (snapshot.marker == 0) {
			pthread_mutex_lock(&mutex);
			sendMarkerSeq(snapshot, index);
			pthread_mutex_unlock(&mutex);
		}
	}

	// ukončení spojení
	close(clients[index].clientSocket);
        cout << "Ukonceni spojeni" << endl;

	// odstranění pobočky ze seznamu
	clients.erase(clients.begin() + getClientIndex());

	// vypnutí programu po skončení všech obslužných vláken (odpojení poboček)
	if (clients.size() == 0) {
		cout << "[BANK] Exit" << endl;
                
                Linker *link = new Linker();
                link->start();
                
                system("./flow.sh");
		
                exit(0);
	}
}

/**
 * Spustí vlákno pro zpracování spojení s pobočkou.
 */
void* Bank::startHandler(void *object) {
    cout << "Startuju handler" << endl;
	reinterpret_cast<Bank *> (object)->handler();

	return NULL;
}

/**
 * Vytvoří vlákno pro zpracování spojení s pobočkou a uloží informace o spojení
 * do seznamu poboček.
 */
void Bank::createClientHandler(int clientSocket,
		struct sockaddr_in clientAddress) {
	pthread_t thread;

	unsigned short number = 0;

	// příjem čísla portu pobočky
	if (read(clientSocket, (void*) (&number), sizeof(unsigned short))
			!= sizeof(unsigned short)) {
		cerr << "[BANK] Error: " << strerror(errno) << endl;
	}
        
        cout << "Prijato cislo portu pobocky" << endl;

	// spuštění vlákna
	pthread_create(&thread, NULL, &Bank::startHandler, this);

	// uložení adresy klienta a ukazatele na vlákno
	ClientIdentifier cid;

	cid.clientSocket = clientSocket;
	cid.clientAddress = clientAddress;
	cid.thread = thread;
	cid.clientPort = number;

	clients.push_back(cid);
}

/**
 * Přijímá spojení s pobočkama a vytváří pro každé spojení samostatné obslužné
 * vlákno.
 */
void Bank::acceptConnection() {
	// lokální socket
	int localSocket;

	// socket pobočky
	int clientSocket;

	// adresa pobočky
	struct sockaddr_in clientAddress;

	// délka adresy pobočky
	unsigned int clientAddressLength;

	// vytvoření socketu pro příchozí spojení
	if ((localSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		cerr << "[BANK] Error: " << strerror(errno) << endl;
	}

        // lokální adresa
	struct sockaddr_in localAddress;
	// konstrukce lokální adresy
	memset(&localAddress, 0, sizeof(localAddress));
	localAddress.sin_family = AF_INET;
	localAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	localAddress.sin_port = htons(localPort);

	// přiřazení socketu k lokální adrese
	if (bind(localSocket, (struct sockaddr *) &localAddress, sizeof(localAddress))
			< 0) {
		cerr << "[BANK] Error: " << strerror(errno) << endl;
	}

	// naslouchání na socketu
	if (listen(localSocket, QUEUE_LENGTH) < 0) {
		cerr << "[BANK] Error: " << strerror(errno) << endl;
	}

	while (true) {
		// velikost adresy
		clientAddressLength = sizeof(clientAddress);

		// vytvoření nového socketu pro spojení s pobočkou
		if ((clientSocket = accept(localSocket, (struct sockaddr *) &clientAddress,
				&clientAddressLength)) < 0) {
			cerr << "[BANK] Error: " << strerror(errno) << endl;
		}
                cout << "Vytvoren socket pro klienta" << endl;
		// vytvoření obslužného vlákna
		createClientHandler(clientSocket, clientAddress);
	}

	// uzavření socketu
	close(localSocket);
}
