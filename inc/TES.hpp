#ifndef _TYPES_ENUMS_STRUCTS_HPP_
#define _TYPES_ENUMS_STRUCTS_HPP_

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
using namespace std;

typedef uint32_t addr;

struct WordNode{
    bool lit;
    long val;
    char* sim;

    WordNode(bool b, long v, char* s):lit(b), val(v), sim(s){}
};

enum RelocType{ ABS=0, PCREL=1};

struct RelocTableEntry{

  addr off;
  RelocType typ;
  int symbol;
  uint32_t addend;
  RelocTableEntry(addr offset, RelocType t, int sym, uint32_t add):
    off(offset),typ(t), symbol(sym), addend(add){}

};

enum BIND{ LOC=1, GLOB=2 };

enum SYM_TABLE_IND{ EXT=-2, UND=-1};

struct SymTableEntry{
  int id;
  uint32_t value;
  bool isSection;
  BIND bind;
  int ind;

  SymTableEntry(const int& n, const uint32_t& val, const bool& sec,
     const BIND& b, const int& id):
     id(n), value(val), isSection(sec), bind(b), ind(id){}
    
  SymTableEntry():id(0), value(0), isSection(false), bind(LOC), ind(-1){}
};

enum PoolType{JUMP=0, LOAD=1, STORE=2};

struct PoolEntry{
  uint32_t val;
  addr adr;
  PoolType pt;
  string sym;

  PoolEntry(uint32_t v, addr a, PoolType p, string s=""): val(v), adr(a), pt(p), sym(s){}
};

struct Section{
  int startAddress;
  bool addrSet=false;
  string name;
  vector<uint8_t> mem;
  int locCnt;
  unordered_map<string, vector<addr>> backPatch;
  vector<RelocTableEntry> reloc;
  vector<PoolEntry> pool;
  int fileId;
  Section(string n):name(n), locCnt(0), startAddress(-1), fileId(0){}

};

void exitWithError(string, int);

enum OpType{
  IMMED_LIT, IMMED_SYM, IMMED_REG, 
  DIR_LIT, DIR_SYM, DIR_REG, 
  DIR_REG_LIT
};

struct Operand{
  OpType tip;
  uint8_t reg;
  uint32_t lit;
  string sym;

  Operand(OpType t, uint8_t r, int32_t l, string s):\
    tip(t), reg(r), lit(l), sym(s){}
};

#endif