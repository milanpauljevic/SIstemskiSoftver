#ifndef _EMULATOR_HPP_
#define _EMULATOR_HPP_
#include <stdlib.h>
#include <unordered_map>
#include <vector>
#include <map>
using namespace std;

#define MEMORY_NOT_DEFINED {printMemory();exitWithError("Memory not defined!",-1);}
#define sp regs[14]
#define pc regs[15]
#define status csr[0]
#define handler csr[1]
#define cause csr[2]
#define CODE_ERROR {cout<<"error "<<hex<<pc<<endl;exit(-5);}

class Emulator{

public:

  void loadFile(string);
  void printMemory();
  void printState();
  void start();
  int executeLine();
  void write32(uint32_t, uint32_t);
  uint32_t read32(uint32_t);

  map<uint32_t,uint8_t> memory;
  uint32_t regs[16];
  uint32_t csr[3];
};

#endif