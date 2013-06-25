/* 
 * File:   Linker.h
 * Author: sabra
 *
 * Created on 24. červen 2013, 19:30
 */

using namespace std;

#ifndef LINKER_H
#define	LINKER_H

class Linker {
public:
    Linker();
    virtual ~Linker();
    void start();
private:
    int getDirContent(string dir, vector<string> &files);
    string extractNumber(string s);
};

#endif	/* LINKER_H */

