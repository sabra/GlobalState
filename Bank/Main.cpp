/* 
 * File:   Main.cpp
 * Author: sabra
 * 
 */

#include "Bank.h"
#include "Linker.h"

/**
 * Hlavní metoda.
 */
int main(int argc, char *argv[]) {
	Bank *bank = new Bank(atoi(argv[1]));
	bank->start();

        Linker *link = new Linker();
        link->start();
        
        system("./flow.sh");
        
	return 0;
}
