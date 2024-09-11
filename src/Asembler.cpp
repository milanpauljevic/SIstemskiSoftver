#include "../inc/Asembler.hpp"
#include <iostream>
#include <iomanip>

void Asembler::write_32bit(uint32_t value){
  instruction(value & 255, (value>>8)& 255, (value>>16)& 255, (value>>24)& 255);
}

void Asembler::backpatching(Section *s){
  if(s==nullptr)exitWithError("Iregular situation", -1);
  uint32_t val;
  string symbol;
  for(auto par: s->backPatch){
    symbol=par.first;
    if(symTable[symbol].ind==symTable[s->name].id){
      val=symTable[symbol].value;
      for(addr a:par.second){
        s->mem[a]=uint8_t(val & 255);
        s->mem[a+1]=uint8_t((val>>8) & 255);
        s->mem[a+2]=uint8_t((val>>16) & 255);
        s->mem[a+3]=uint8_t((val>>24) & 255);
      }
    }
    else{
      uint32_t addend;
      RelocType rt;
      int id;
      for(addr adr:par.second){
        if(symTable[symbol].bind==GLOB){
          addend=0;id=symTable[symbol].id;rt=ABS;
        }
        else{
          addend=symTable[symbol].value;id=symTable[symbol].ind;rt=ABS;
        }
        s->reloc.push_back(RelocTableEntry(adr, rt, id, addend));
      }
    }
  }

}

void Asembler::empty_the_pool(Section *s){
  int16_t val;
  for(PoolEntry p:s->pool){
    currSec=s;
    val=s->locCnt-p.adr-4;
    if(val>2047) SECTION_TOO_BIG;
    if(p.sym==""){
      s->mem[p.adr+2]|=(val>>8) & 0xF;
      s->mem[p.adr+3]=(uint8_t)(val & 0xFF);
      write_32bit(p.val);
    }
    else if(symTable.find(p.sym)==symTable.end()){
      SYMBOL_NOT_DEFINED(p.sym);
    }
    else if(symTable[p.sym].ind==symTable[s->name].id){
      if(p.pt==JUMP){
        if(s->mem[p.adr]==0x21)//call
          s->mem[p.adr]-=1;
        else 
          s->mem[p.adr]-=8;
        
        val=symTable[p.sym].value-p.adr-4;
        s->mem[p.adr+2]+=(val>>8) & 0xF;
        s->mem[p.adr+3]=(uint8_t)(val & 0xFF);
      }
      else{
        s->mem[p.adr+2]+=(val>>8) & 0xF;
        s->mem[p.adr+3]=(uint8_t)(val & 0xFF);
        write_32bit(symTable[p.sym].value);
      }
    }
    else{
      s->mem[p.adr+2]+=(val>>8) & 0xF;
      s->mem[p.adr+3]=(uint8_t)(val & 0xFF);
      write_32bit(0);
      if(symTable[p.sym].bind==GLOB){
        s->reloc.push_back(RelocTableEntry(s->locCnt-4, ABS,symTable[p.sym].id, 0));
      }
      else{
        s->reloc.push_back(RelocTableEntry(s->locCnt-4, ABS, symTable[p.sym].ind, symTable[p.sym].value));
      }
    }
  }
}

void Asembler::add_symbol(string name)
{

  CHECK_SECTION

  if(symTable.find(name)==symTable.end()){
    symTable[name]=SymTableEntry(symbolInd++,currSec->locCnt, false,
      symTable.find(name)==symTable.end()?LOC:GLOB, symTable[currSec->name].id);
  }
  else if(symTable[name].bind==GLOB && symTable[name].ind<0){
    symTable[name].ind=symTable[currSec->name].id;
    symTable[name].value=currSec->locCnt;
  }
;
}

/*************************/
    //DIRECTIVES// 
/*************************/

void Asembler::dir_word(vector<WordNode> lista){

  CHECK_SECTION

  uint32_t value;
  for(WordNode wn:lista){
    
    if(wn.lit){
      value=wn.val;
    }
    else{
      if(symTable.find(wn.sim)==symTable.end() || symTable[wn.sim].ind<0){
        //dodaj u backpathing tabelu
        currSec->backPatch[wn.sim].push_back(currSec->locCnt);
        value=0;
      }
      else{
        value=symTable[wn.sim].value;
      }
    }
    write_32bit(value);
  }
}

void Asembler::dir_skip(uint32_t val){

  CHECK_SECTION

  for(int i=0;i<val;++i){
    currSec->mem.push_back(0x00);
    currSec->locCnt++;
  }
}

void Asembler::dir_end(){
  for(auto par:sections){
    empty_the_pool(par.second);
    backpatching(par.second);
  }
  binary_print();
  textOut.open("text_output_"+fileOut, ios::out);
  if(!textOut)exitWithError("File error!", -2);
  print_symbol_table();
  for(auto par:sections){
    print_section(par.first);
    delete par.second;
  }
  textOut.close();
}

void Asembler::dir_global(vector<string>& syms){
  for(string s:syms){
    addGlobExtSymbol(s, -1);
  }
}

void Asembler::dir_extern(vector<string>& syms){
  for(string s:syms){
    addGlobExtSymbol(s, -2);
  }
}

void Asembler::addGlobExtSymbol(string s, int n){
  if(symTable.find(s)==symTable.end()){
      symTable[s]=SymTableEntry(symbolInd++, 0, false, GLOB, n);
    }
  else{
    symTable[s].bind=GLOB;
    symTable[s].ind=symTable[currSec->name].id;
  }
}

void Asembler::dir_section(string name){
  if(sections.find(name)!=sections.end()){
    currSec=sections[name];
    return;
  }
  else{
    currSec=new Section(name);
    sections[name]=currSec;
    symTable[name]=SymTableEntry(symbolInd, 0, true, LOC, symbolInd);
    symbolInd++;
  }
}

/*************************/
    //INSTRCUTIONS// 
/*************************/

void Asembler::instruction(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4){
  
  CHECK_SECTION

  currSec->mem.push_back(b1);
  currSec->mem.push_back(b2);
  currSec->mem.push_back(b3);
  currSec->mem.push_back(b4);
  currSec->locCnt+=4;
}

void Asembler::csr_instruction(uint8_t code, uint8_t to, uint8_t from)
{
  instruction(code, (to<<4)+from, 0, 0);
}

void Asembler::ari_log_sh_instruction(uint8_t code , uint8_t src, uint8_t dest){
  instruction(code, (dest<<4)+dest, src<<4, 0);
}

void Asembler::xchg_instruction(uint8_t r1, uint8_t r2){
  instruction(0x40, r1, r2<<4, 0);
}

//sp=15, FFC is -4 on 12 bits
void Asembler::push_instruction(uint8_t reg){
  instruction(0x81, 14<<4, (reg<<4)+0xF, 0xFC);
}

void Asembler::pop_instruction(uint8_t reg){
  instruction(0x93, (reg<<4)+14, 0, 4);
}

void Asembler::halt_int_instruction(uint8_t code){
  instruction(code, 0, 0, 0);
}

void Asembler::iret_instruction(){
  instruction(0x97, 14, 0, 4);
  pop_instruction(15);//pop
}

void Asembler::jump_instructions(uint8_t mode, uint8_t r1, uint8_t r2, int32_t literal, string symbol=""){
  uint8_t code=0x30;//mode needed
  if(symbol==""){
    if(literal >>12==0){
      instruction(code+mode, r1, (r2<<4)+((literal>>8)&0xF), (uint8_t)(literal & 0xFF));
    }
    else{
      currSec->pool.push_back(PoolEntry(literal, currSec->locCnt, JUMP, ""));
      instruction(code+8+mode, 0xF0+r1, (r2<<4)+((literal>>8)&0xF), (uint8_t)(literal & 0xFF));
      
    }
  }
  else{
    if(symTable.find(symbol)!=symTable.end() && symTable[symbol].ind==symTable[currSec->name].id){
      literal=symTable[symbol].value - currSec->locCnt - 4;
      if(literal<-2048 && literal>2047)
        SECTION_TOO_BIG;
      instruction(code+mode, 0xF0+r1, (r2<<4)+((literal>>8)&0xF), (uint8_t)(literal & 0xFF));
    }
    else{
      currSec->pool.push_back(PoolEntry(0, currSec->locCnt, JUMP, symbol));
      instruction(code+8+mode, 0xF0+r1, r2<<4, 0);
    }
  }
}

void Asembler::call_instruction(int32_t literal, string symbol){
  uint8_t code=0x20;
  if(symbol==""){
    if(literal>>12==0){
      instruction(code, 0, (literal>>8) & 0xF, (uint8_t)(literal & 0xFF));
    }
    else{
      currSec->pool.push_back(PoolEntry(literal, currSec->locCnt, JUMP, ""));
      instruction(code+1, 0xF0, 0, 0);
    }
  }
  else{
    if(symTable.find(symbol)!=symTable.end() && symTable[symbol].ind==symTable[currSec->name].id){
      literal=(int32_t)(symTable[symbol].value) - currSec->locCnt;
      if(literal | 0xFFFFF000)
        SECTION_TOO_BIG;
      instruction(code, 0xF0, (literal>>8)&0xF, (uint8_t)(literal & 0xFF));
    }
    else{
      currSec->pool.push_back(PoolEntry(0, currSec->locCnt, JUMP, symbol));
      instruction(code+1, 0xF0, 0, 0);
    }
  }
}

void Asembler::load_instruction(Operand op, uint8_t reg){
  uint8_t code=0x90;
  PoolType pt= LOAD;
  switch(op.tip){

    case IMMED_LIT:
      currSec->pool.push_back(PoolEntry(op.lit, currSec->locCnt, pt));
      instruction(code+2, (reg<<4)+0xF, 0, 0);
      break;
    
    case IMMED_REG:
      instruction(code+1, (reg<<4) + op.reg, 0, 0);
      break;

    case IMMED_SYM: 
      currSec->pool.push_back(PoolEntry(0, currSec->locCnt, pt, op.sym));
      instruction(code+2, (reg<<4)+0xF, 0, 0);
      break;

    case DIR_LIT:
      currSec->pool.push_back(PoolEntry(op.lit, currSec->locCnt, pt));
      instruction(code+2, (reg<<4)+0xF, 0, 0);//reg=literal
      instruction(code+2, (reg<<4)+reg, 0, 0);//reg=[reg]=[literal]
      break;

    case DIR_SYM:
      currSec->pool.push_back(PoolEntry(op.lit, currSec->locCnt, pt, op.sym));
      instruction(code+2, (reg<<4)+0xF, 0, 0);//reg=simbol
      instruction(code+2, (reg<<4)+reg, 0, 0);//reg=[reg]=[simbol]
      break;

    case DIR_REG:
      instruction(code+2, (reg<<4)+op.reg, 0, 0);
      break;

    case DIR_REG_LIT:
      if(op.lit >> 12){
        LITERAL_TOO_BIG(op.lit)
      }
      instruction(code+2, (reg<<4) + op.reg,(op.lit>>8)& 0xF, op.lit & 0xFF);
      break;
  }
}

void Asembler::store_instruction(Operand op, uint8_t reg){
  uint8_t code=0x80;
  PoolType pt= STORE;
  switch(op.tip){

    case IMMED_LIT:
    case IMMED_SYM: 
      exitWithError("Store instruction can't accept IMMED addressing!", -1);

    case IMMED_REG:
      //reg u reg
      instruction(0x91, (reg<<4)+op.reg,0,0);
      break;
      
    case DIR_LIT:
      currSec->pool.push_back(PoolEntry(op.lit, currSec->locCnt, pt));
      instruction(code+2, 0xF0, reg<<4, 0);
      break;

    case DIR_SYM:
      currSec->pool.push_back(PoolEntry(0, currSec->locCnt, pt, op.sym));
      instruction(code+2, 0xF0, reg<<4, 0);
      break;

    case DIR_REG:
      instruction(code, op.reg<<4, reg<<4, 0);
      break;

    case DIR_REG_LIT:
      if(op.lit>>12){
        LITERAL_TOO_BIG(op.lit)
      }
      instruction(code, op.reg<<4, (reg<<4)+((op.lit>>8)& 0xF), op.lit & 0xFF);
      break;
  }
}

//**************************//
      //PRINT//
//**************************//
void Asembler::print_section(string name)
{
  Section* s=sections[name];
  textOut<<'#'<<name;
  for(int i=0;i<s->mem.size();++i){
    if(i%8==0){
      textOut<<endl<<hex<<i<<": ";
    }
    textOut<<hex<<setw(2)<<setfill('0')<<uint32_t(s->mem[i])<<" ";
  }
  
  textOut<<endl<<endl<<"Reloc Table";
  if(s->reloc.size()==0){textOut<<" is empty"<<endl<<endl;return;}
  textOut<<"\nOffset\t Type\tSymbol\t Addend"<<endl;

  for(auto rel:s->reloc){
    textOut<<hex<<setw(8)<<setfill('0')<<rel.off<<(rel.typ==ABS?" ABS":" UND")<<'\t'<<
      setw(6)<<setfill(' ')<<rel.symbol<<'\t'<<hex<<setw(8)<<setfill('0')<<rel.addend<<endl;
    
  }
  textOut<<endl;
}

void Asembler::print_symbol_table(){
  textOut<<"SYMBOL TABLE"<<endl;
  textOut<<"ID"<<'\t'<<"value"<<'\t'<< " Type"<<"\t"<<"  BIND"<<'\t'<<"IND"<<'\t'<<"name"<<endl;
  for(auto par:symTable){
    if(symTable[par.first].ind==-1){
      SYMBOL_NOT_DEFINED(par.first);
    }
    textOut<<par.second.id<<'\t'<<hex<<setw(8)<<setfill('0')<<par.second.value<<
      (par.second.isSection?" SCTN ":" NOTYP")<<'\t'<<(par.second.bind==GLOB?"GLOB":"LOC ")<<'\t'<<
      dec<<(int)(par.second.ind)<<'\t'<<par.first<<endl;
  }
  textOut<<endl;
}

void Asembler::binary_print(){
  int globCnt=0, secCnt=0;
  for(auto par:symTable){
    if(par.second.isSection || par.second.bind==GLOB)globCnt++;
    if(par.second.isSection)secCnt++;
  }
  fstream binout;
  binout.open(fileOut, ios::out | ios::binary);
  if(!binout)exitWithError("File error!", -1);
  binout.write((const char*)(&globCnt), sizeof(int));
  for(auto par:symTable){
    if(par.second.isSection || par.second.bind==GLOB){
      binout.write((const char*)(&par.second.id), sizeof(int));
      binout.write((const char*)(&par.second.value), sizeof(uint32_t));
      binout.write((const char*)(&par.second.isSection), sizeof(bool));
      
      binout.write((const char*)(&par.second.ind), sizeof(int));
      int len=par.first.length();
      binout.write((const char*)(&len), sizeof(int));
      binout<<par.first;
    }
  }
  binout.write((const char*)(&secCnt), sizeof(int));
  for(auto par:sections){
    int id=symTable[par.first].id;
    binout.write((const char*)(&id), sizeof(int));
    id=par.second->locCnt;
    binout.write((const char*)(&id), sizeof(int));
    for(int lok:par.second->mem){
      binout.write((const char*)(&lok), sizeof(uint8_t));
    }
    id=par.second->reloc.size();
    binout.write((const char*)(&id), sizeof(int));
    for(RelocTableEntry r:par.second->reloc){
      binout.write((const char*)(&r.off), sizeof(uint32_t));
      binout.write((const char*)(&r.symbol), sizeof(int));
      binout.write((const char*)(&r.addend), sizeof(uint32_t));
    }
  }
  binout.close();
}
