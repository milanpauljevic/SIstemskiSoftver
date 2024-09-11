#ifndef _ASEMBLER_HPP_ 
#define _ASEMBLER_HPP_ 

#include <stdio.h>
#include "./TES.hpp"
#include <unordered_map>
#include <map>
#include <fstream>

#define CHECK_SECTION if(currSec==nullptr)exitWithError("Section not defined yet!", -1);
#define SECTION_TOO_BIG if(currSec==nullptr)exitWithError("Section ntoo big!", -1);
#define SYMBOL_NOT_DEFINED(a) exitWithError("Symbol "+a+" not defined", -1);
#define LITERAL_TOO_BIG(lit) exitWithError("Literal " + to_string(lit) + " can't be represented as a signed value with 12 bits!", -1);

class Asembler{

public:

  void add_symbol(string);
  void write_32bit(uint32_t);
  void instruction(uint8_t, uint8_t, uint8_t, uint8_t);
  void backpatching(Section* s);
  void empty_the_pool(Section *s);

  void dir_word(vector<WordNode>);
  void dir_skip(uint32_t);
  void dir_end();
  void dir_global(vector<string>&);
  void dir_extern(vector<string>&);
  void dir_section(string);
  void addGlobExtSymbol(string, int);

  void csr_instruction(uint8_t, uint8_t, uint8_t);
  void ari_log_sh_instruction(uint8_t, uint8_t, uint8_t);
  void xchg_instruction(uint8_t, uint8_t);

  void push_instruction(uint8_t);
  void pop_instruction(uint8_t);

  void halt_int_instruction(uint8_t);
  void iret_instruction();

  void jump_instructions(uint8_t, uint8_t, uint8_t, int32_t, string);
  void call_instruction(int32_t, string);

  void load_instruction(Operand op, uint8_t reg);
  void store_instruction(Operand op, uint8_t reg);

  void print_section(string);
  void print_symbol_table();
  void binary_print();



  int symbolInd=0;
  fstream textOut;
  unordered_map<string, SymTableEntry> symTable;
  unordered_map<string, Section*> sections;
  Section* currSec=nullptr;
  string fileOut="asembler_output.txt";

};

#endif