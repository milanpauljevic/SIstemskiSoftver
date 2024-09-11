#include "../inc/Emulator.hpp"
#include "../inc/TES.hpp"
#include <fstream>
#include <iostream>
#include <iomanip>
using namespace std;

void Emulator::loadFile(string fileName){
  fstream input;
  input.open(fileName, ios::binary | ios::in);
  if(!input)exitWithError("File "+fileName+" not found",-1);
  int i=0, n;
  input.read((char*)&n, sizeof(int));
  uint32_t adr;
  uint8_t val;
  while(i++<n){
    input.read((char*)&adr, sizeof(uint32_t));
    input.read((char*)&val, sizeof(uint8_t));
    if(memory.find(adr)!=memory.end())exitWithError("Memory error!",-2);
    memory[adr]=val;
  }
  input.close();
}

void Emulator::printMemory(){
  fstream izlaz;
  izlaz.open("emulator_output.emu", ios::out);
  int i=0;
  izlaz<<"Memory: ";
  for(auto par:memory){
    if(i%4==0)izlaz<<endl<<hex<<par.first<<':';
    i++;
    izlaz<<hex<<setw(2)<<setfill('0')<<(int)par.second<<' ';
  }
  izlaz.close();
}

void Emulator::printState(){
  cout<<"Emulated processor state";

  for(int i=0;i<16;++i){
    if(i%4==0)cout<<endl;
    if(i<10)cout<<" r"<<dec<<i;
    else cout<<"r"<<dec<<i;
    cout<<"=0x"<<hex<<setw(8)<<setfill('0')<<regs[i]<<' ';
  }
  cout<<endl;
  cout<<"status=0x"<<hex<<status<<endl;
  cout<<"handler=0x"<<hex<<handler<<endl;
  cout<<"cause=0x"<<hex<<cause<<endl;
  cout<<"------------------------------------------------------------"<<endl;
}

void Emulator::start(){
  for(int i=0;i<15;++i)regs[i]=0;
  csr[0]=csr[1]=csr[2]=0;
  regs[15]=0x40000000;
}

void Emulator::write32(uint32_t adr, uint32_t val){
  memory[adr]=val & 0xFF;
  memory[adr+1]=(val>>8) & 0xFF;
  memory[adr+2]=(val>>16) & 0xFF;
  memory[adr+3]=(val>>24) & 0xFF;
}

uint32_t Emulator::read32(uint32_t adr){
  if(memory.find(adr)==memory.end())MEMORY_NOT_DEFINED
  return memory[adr]+(memory[adr+1]<<8)+(memory[adr+2]<<16)+(memory[adr+3]<<24);
}

int Emulator::executeLine(){
  uint8_t b1, b2, b3, b4;
  if(memory.find(pc)==memory.end())MEMORY_NOT_DEFINED
  b1=memory[pc++];
  b2=memory[pc++];
  b3=memory[pc++];
  b4=memory[pc++];
  uint8_t code=b1>>4, mode=b1 & 0xF, A=b2>>4, B=b2 & 0xF, C=b3>>4;
  uint32_t DDD=((b3 & 0xF)<<8)+b4;
  int8_t x=0xF & b3;
  if(8 & x){
    x|=0xF0;
  }
  int32_t DDDp= (int32_t)(x<<8) + b4;

  switch(code){
    //halt
    case 0x0:
      if(mode!=0 || A!=0 || B!=0 || C!=0 | DDD!=0)CODE_ERROR
      cout<<"------------------------------------------------------------"<<endl<<
      "Emulated processor executed halt instruction"<<endl;
      printState();
      return 1;

    //int
    case 0x1:
      if(mode!=0 || A!=0 || B!=0 || C!=0 | DDD!=0)CODE_ERROR
      sp-=4;
      write32(sp, pc);
      sp-=4;
      write32(sp, status);
      cause=4;
      status=status&(~0x1);
      pc=handler;
      break;
    
    //call
    case 0x2:
      if(C!=0)CODE_ERROR
      sp-=4;
      write32(sp, pc);
      if(mode==0){
        pc=regs[A]+regs[B]+DDDp;
      }
      else if(mode==1){
        pc=read32(regs[A]+regs[B]+DDDp);
      }
      else{
        CODE_ERROR
      }
      break;
    
    //jump
    case 0x3:
      switch(mode){
        case 0:pc=regs[A]+DDDp;break;
        case 1:if(regs[B]==regs[C])pc=regs[A]+DDDp;break;
        case 2:if(regs[B]!=regs[C])pc=regs[A]+DDDp;break;
        case 3:if((int32_t)(regs[B])>(int32_t)(regs[C]))pc=regs[A]+DDDp;break;
        case 8:pc=read32(regs[A]+DDDp);break;
        case 9:if(regs[B]==regs[C])pc=read32(regs[A]+DDDp);break;
        case 10:if(regs[B]!=regs[C])pc=read32(regs[A]+DDDp);break;
        case 11:if((int32_t)(regs[B])>(int32_t)(regs[C]))pc=read32(regs[A]+DDDp);break;
        default:CODE_ERROR
      }
      break;

    //xchg
    case 0x4:
      if(mode!=0 || A!=0 || DDD!=0)CODE_ERROR
      uint32_t temp;
      temp=regs[B];regs[B]=regs[C];regs[C]=temp;
      regs[0]=0;
      break;
    
    //arithmetic
    case 0x5:
      if(DDD!=0)CODE_ERROR
      if(A==0)break;
      switch(mode){
        case 0: regs[A]=regs[B]+regs[C];break;
        case 1: regs[A]=regs[B]-regs[C];break;
        case 2: regs[A]=regs[B]*regs[C];break;
        case 3: regs[A]=regs[B]/regs[C];break;
        default: CODE_ERROR
      }
      break;
      
    //logical
    case 0x6:
      if(DDD!=0)CODE_ERROR
      if(A==0)break;
      switch(mode){
        case 0: regs[A]=~regs[B];break;
        case 1: regs[A]=regs[B] & regs[C];break;
        case 2: regs[A]=regs[B] | regs[C];break;
        case 3: regs[A]=regs[B] ^ regs[C];break;
        default: CODE_ERROR
      }
      break;
    
    //shifting
    case 0x7:
      if(DDD!=0)CODE_ERROR
      if(A==0)break;
      switch(mode){
        case 0: regs[A]=regs[B] << regs[C];break;
        case 1: regs[A]=regs[B] >> regs[C];break;
        default: CODE_ERROR
      }
      break;
    
    //storing
    case 0x8:
      switch(mode){
        case 0:write32(regs[A]+regs[B]+DDDp, regs[C]);break;
        case 2:write32(read32(regs[A]+regs[B]+DDDp), regs[C]);break;
        case 1:if(A!=0){regs[A]+=DDDp;} write32(regs[A], regs[C]);break;
        default: CODE_ERROR
      }
      break;
      
    //loading
    case 0x9:
      switch(mode){
        case 0:if(A!=0)regs[A]=csr[B];break;
        case 1:if(A!=0)regs[A]=regs[B]+DDDp;break;
        case 2:if(A!=0)regs[A]=read32(regs[B]+regs[C]+DDDp);break;
        case 3:if(A!=0)regs[A]=read32(regs[B]);regs[B]+=DDDp;break;
        case 4:csr[A]=regs[B];break;
        case 5:csr[A]=regs[B] | DDD;break;
        case 6:csr[A]=read32(regs[B]+regs[C]+DDDp);break;
        case 7:csr[A]=read32(regs[B]);regs[B]+=DDDp;break;
        default:CODE_ERROR
      }
      break;

    default:CODE_ERROR
  }
  return 0;
}

int main(int argc, char** argv){
  if(argc!=2)return -2;
  Emulator e;
  e.loadFile(argv[1]);
  e.start();
  while(e.executeLine()==0);
  //e.printMemory();
}