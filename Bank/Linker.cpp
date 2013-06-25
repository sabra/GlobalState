/* 
 * File:   Linker.cpp
 * Author: sabra
 * 
 */

#include <iostream>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include "Linker.h"

using namespace std;

Linker::Linker() {
}

Linker::~Linker() {
}

/**
 * Čte obsah adresáře.
 */
int Linker::getDirContent(string dir, vector<string> &files) {
    DIR *dp;
    struct dirent *dirp;
    if ((dp = opendir(dir.c_str())) == NULL) {
        cout << "Error(" << errno << ") opening " << dir << endl;
        return errno;
    }

    while ((dirp = readdir(dp)) != NULL) {
        files.push_back(string(dirp->d_name));
    }
    closedir(dp);

    return 0;
}

/**
 * Extrahuje číslice z řetězce.
 */
string Linker::extractNumber(string s) {
    string temp = "";

    for (unsigned int i = 0; i < s.size(); ++i) {
        if (isdigit(s[i])) {
            temp += s[i];
        }
    }

    return temp;
}

/**
 * Hlavní metoda.
 */
void Linker::start() {
  string dir = string(".");
  vector<string> files = vector<string>();

  getDirContent(dir, files);
	if (files.size() > 0) {
		ofstream writeFile("net_flow.txt");

		writeFile << "processes: [";

		// seznam poboček
		bool isFirst = true;
		for (unsigned int i = 0; i < files.size(); i++) {
			if (files[i].find("traffic") != string::npos) {
				string number = extractNumber(files[i]);

				if (!number.empty()) {
					if (isFirst) {
						isFirst = false;
					} else {
						writeFile << ", ";
					}
					writeFile << "SUB" << number;
				}
			}
		}

		writeFile << "]\ntraffic:" << endl;

		// spojení dat o komunikaci
		for (unsigned int i = 0; i < files.size(); i++) {
			if (files[i].find("traffic") != string::npos) {
				ifstream readFile(files[i].c_str());

				string line;
				if (readFile.is_open()) {
					while (readFile.good()) {
						getline(readFile, line);
						if (!line.empty()) {
							writeFile << line << endl;
						}
					}

					readFile.close();
                                        remove(files[i].c_str());
				}
			}
		}

		writeFile << "marks:" << endl;

		// spojení dat o výpočtu
		for (unsigned int i = 0;i < files.size();i++) {
			if (files[i].find("marks") != string::npos) {
				ifstream readFile(files[i].c_str());

				string line;
				if (readFile.is_open()) {
					while (readFile.good()) {
						getline(readFile, line);
						if (!line.empty()) {
							writeFile << line << endl;
						}
					}

					readFile.close();
                                        remove(files[i].c_str());
				}
			}
		}

		writeFile.close();
	}
}