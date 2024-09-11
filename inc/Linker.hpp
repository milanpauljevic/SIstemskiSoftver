#ifndef _LINKER_HPP_
#define _LINKER_HPP_

#include "TES.hpp"
#include <vector>
#include <map>
#define FILE_NOT_FOUND(fileName) exitWithError("File "+fileName+" not found!", -1);

struct ObjectFile{
  unordered_map<int, Section*> sections;
  string name;
  int id;
  unordered_map<string, SymTableEntry> syms;
  vector<int> secs;

  ObjectFile(string n, int i):name(n), id(i){} 
};

struct globalSym{
  int fileId;
  int secId;
  int oldId;
  int newId;
  uint32_t value;

  globalSym(int f, int s, uint32_t v, int o, int n):fileId(f), secId(s), value(v), oldId(o), newId(n){}
  globalSym():fileId(-1), secId(-1), value(0), oldId(0), newId(1){}
};

struct Request{
  uint32_t address;
  string sectionName;

  Request(uint32_t adr, string name): address(adr), sectionName(name){}
};

class Linker{
public:

  void loadFile(string);
  void makeGlobalTable();
  void organizeSections();
  void resolveReloc();
  void processing();

  void printFile(string);
  void printFiles();
  void printBinary(string);
  static int findIndex(vector<Section*>, string);

  int globTableId=-1;
  unordered_map<string, globalSym> globTable;
  vector<Section*> globalSections;
  unordered_map<string, ObjectFile*> files;
  vector<string> fileNames;
  int fileId=0;
  vector<Request*> reqs;
  map<uint32_t,uint8_t> memory;
  uint32_t lastAddr=0;
};





#endif

