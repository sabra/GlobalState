/* 
 * File:   Main.cpp
 * Author: sabra
 * 
 */


#include "BranchBank.h"


/**
 * Hlavní metoda.
 */
int main(int argc, char *argv[]) {
	// TODO validace vstupnich parametru
	BranchBank *branchBank = new BranchBank(atoi(argv[1]), atoi(argv[2]), argv[3]/*, atoi(argv[4]), argv[5]*/);
	branchBank->start();

	return 0;
}
