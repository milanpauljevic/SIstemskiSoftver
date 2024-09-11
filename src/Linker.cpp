#include "../inc/Linker.hpp"
#include <fstream>
#include <iomanip>
#include <stdio.h>
#include <iostream>
#include <algorithm>
#include <string.h>
using namespace std;

void Linker::processing(){
    makeGlobalTable();
    organizeSections();
    resolveReloc();
  }

int main(int argc, char** argv){
  Linker l;
  bool oo=false, hex=0;
  string outFile;
  if(argc<3)return -2;
  for(int i=1;i<argc;++i){
    string s=argv[i];
    if(strcmp("-o", argv[i])==0){
      if(oo)exitWithError("Output file name already defined!", -2);
      oo=true;
      outFile=argv[++i];
    }
    else if(strcmp("-hex", argv[i])==0){
      hex=true;
    }
    else if(s.substr(0, 7).compare("-place=")==0){
      int x=s.find("@0x");
      l.reqs.push_back(new Request(stol(s.substr(x+3),0,16), s.substr(7, x-7)));
    }
    else{
      l.loadFile(argv[i]);
    }
  }
  if(!hex)return -7;
  l.processing();
  l.printBinary(outFile);
}

void Linker::loadFile(string fileName){
  fstream fileIn;
  ObjectFile *o=new ObjectFile(fileName, fileId++);
  fileIn.open(fileName,ios::in | ios::binary );
  if(!fileIn)FILE_NOT_FOUND(fileName);
  int n, cntSym=0, cntSec, int1, int2, int3;
  uint32_t un32; 
  uint8_t un8; 
  bool flag; 
  string s="";
  char c;
  SymTableEntry sym=SymTableEntry();

  //symbol table
  fileIn.read((char*)&cntSym, sizeof(int));
  for(int i=0;i<cntSym;++i){
    s="";
    fileIn.read((char*)&sym.id, sizeof(int));
    fileIn.read((char*)&sym.value, sizeof(uint32_t));
    fileIn.read((char*)&sym.isSection, sizeof(bool));
    sym.bind=(sym.isSection?LOC:GLOB);
    fileIn.read((char*)&sym.ind, sizeof(int));
    
    fileIn.read((char*)&n, sizeof(int));
    for(int j=0;j<n;++j){
      fileIn>>c;
      s+=c;
    }
    if(!sym.isSection)o->syms[s]=sym;
    else {
      o->sections[sym.id]=new Section(s);
      o->secs.push_back(sym.id);
    }
  }

  //sections
  int secId;
  Section* sec;
  fileIn.read((char*)&cntSec, sizeof(int));
  for(int i=0;i<cntSec;++i){
    uint8_t memByte;
    fileIn.read((char*)&secId, sizeof(int));
    sec=o->sections[secId];
    sec->fileId=o->id;
    fileIn.read((char*)&n, sizeof(int));
    for(int j=0;j<n;++j){
      fileIn.read((char*)&memByte, sizeof(uint8_t));
      sec->mem.push_back(memByte);
    }
    
    sec->locCnt=sec->mem.size();
    
    //loading reloc table
    fileIn.read((char*)&cntSym, sizeof(int));
    for(int j=0;j<cntSym;++j){
      RelocTableEntry r=RelocTableEntry(0, ABS, 0,0);
      fileIn.read((char*)&r.off, sizeof(uint32_t));
      fileIn.read((char*)&r.symbol, sizeof(int));
      fileIn.read((char*)&r.addend, sizeof(uint32_t));
      sec->reloc.push_back(RelocTableEntry(r.off, ABS, r.symbol, r.addend));
    }
    o->sections[secId]=sec;
    
    for(auto par:o->syms){
      if(par.second.isSection && secId==par.second.id)
        o->sections[secId]->name=string(par.first);
    }
  }
  files[fileName]=o;
  fileIn.close();
  std::sort(o->secs.begin(), o->secs.end());
  fileNames.push_back(fileName);
}

void Linker::printFile(string name){
  if(files.find(name)==files.end())exitWithError("Ne pstoji fajl", -2);
  ObjectFile* o=files[name];
  Section* s;
  cout<<name<<" "<<o->id<<endl;

  cout<<"SYMBOL TABLE"<<endl;
  cout<<"ID"<<'\t'<<"value"<<'\t'<< " Type"<<"\t"<<"  BIND"<<'\t'<<"IND"<<'\t'<<"name"<<endl;
  for(auto par:o->syms){
    cout<<par.second.id<<'\t'<<hex<<setw(8)<<setfill('0')<<par.second.value<<
      (par.second.isSection?" SCTN ":" NOTYP")<<'\t'<<(par.second.bind==GLOB?"GLOB":"LOC ")<<'\t'<<
      dec<<(int)(par.second.ind)<<'\t'<<par.first<<endl;
  }
  cout<<endl;

  for(auto par:o->sections){
    s=o->sections[par.first];
    cout<<'#'<<s->name;
    for(int i=0;i<s->mem.size();++i){
      if(i%8==0){
        cout<<endl<<hex<<i<<": ";
      }
      cout<<hex<<setw(2)<<setfill('0')<<uint32_t(s->mem[i])<<" ";
    }
    
    cout<<endl<<endl<<"Reloc Table";
    if(s->reloc.size()==0){cout<<" is empty"<<endl<<endl;return;}
    cout<<"\nOffset\t Type\tSymbol\t Addend"<<endl;

    for(auto rel:s->reloc){
      cout<<hex<<setw(8)<<setfill('0')<<rel.off<<(rel.typ==ABS?" ABS":" UND")<<'\t'<<
        setw(6)<<setfill(' ')<<rel.symbol<<'\t'<<hex<<setw(8)<<setfill('0')<<rel.addend<<endl;
      
    }
    cout<<endl;
  } 
}

void Linker::printFiles(){
  cout<<"GLOBAL TABLE"<<endl;
  for(auto par: globTable){
    cout<< par.first<<' '<<par.second.fileId<<' '<<par.second.secId<<' '<<hex<<par.second.value<<
    ' '<<par.second.oldId<<' '<<dec<<par.second.newId<<endl;
  }
  cout<<endl;
  Section *s;
  for(auto fajl:files){
    cout<<fajl.first<<endl;
    printFile(fajl.first);
    cout<<"Kraj fajla "<< fajl.second->id<<endl;
  }
}

void Linker::makeGlobalTable()
{
  for(auto par1:files){
    for(auto par2:par1.second->syms){
      if(par2.second.bind==GLOB && par2.second.ind>=0){
        if(globTable.find(par2.first)!=globTable.end())
          exitWithError("Multiple definitions of global symbol "+par2.first, -1);
        globTable[par2.first]=globalSym(par1.second->id, par2.second.ind, par2.second.value, par2.second.id, globTableId--);
      }
    }
  }

  //resolve extern symbols
  for(auto file: files){
    for(auto& par: file.second->syms){
      if(globTable.find(par.first)==globTable.end())
        exitWithError("Global symbol "+par.first+" not found!", -1);
      par.second.ind=globTable[par.first].newId;
    }
  }

  //fix relocTables
  string sym="";
  for(auto file: files){//file
    for(auto par: file.second->sections){//section
      for(RelocTableEntry& r: par.second->reloc){//relocEntry
        sym="";
        for(auto sim:file.second->syms){//globSymTable
          if(sim.second.id==r.symbol){
            sym=sim.first;
            break;
          }
        }
        if(sym=="")continue;
        r.symbol=file.second->syms[sym].ind;
      }
    }
  }
}

void Linker::organizeSections(){
  for(int i=0;i<fileId;++i){
    ObjectFile* o=files[fileNames[i]];
    for(int k:o->secs){
      int indeks=findIndex(globalSections, o->sections[k]->name);
      if(indeks==-1)globalSections.push_back(o->sections[k]);
      else{
        globalSections.insert(this->globalSections.begin()+indeks, o->sections[k]);
      }
    }
  }
  for(Request* r:reqs){
    if(r->address<lastAddr)lastAddr=r->address;
  }
  if(reqs.size()==0)lastAddr=0x40000000;

  for(Request *r:reqs){
    for(Section *s:globalSections){
      if(r->sectionName.compare(s->name)==0){
        s->startAddress=r->address;
        s->addrSet=true;
        r->address+=s->locCnt;
        lastAddr=r->address>lastAddr?r->address:lastAddr;
      }
    }
  }
  for(Section *s:globalSections){
    if(!s->addrSet){
      s->startAddress=lastAddr;
      lastAddr+=s->locCnt;
    }
  }

  //overlapping sections
  Section* sec1, *sec2;
  for(int i=0;i<globalSections.size()-i;++i){
    sec1=globalSections[i];
    for(int j=i+1;j<globalSections.size();++j){
      sec2=globalSections[j];
      if(sec1->startAddress<sec2->startAddress+sec2->locCnt && sec1->startAddress>sec2->startAddress ||
        sec2->startAddress<sec1->startAddress+sec1->locCnt && sec2->startAddress>sec1->startAddress){
          exitWithError("Sections "+sec1->name+" and "+sec2->name+" are overlapping!", -1);
      }
    }
  }
}

int Linker::findIndex(vector<Section*> sections, string name){
  for(int i=sections.size()-2;i>=0;--i){
    if(sections[i]->name==name)return i+1;
  }
  return -1;
}

void Linker::resolveReloc(){
  for(auto& g:globTable){
    g.second.value+=files[fileNames[g.second.fileId]]->sections[g.second.secId]->startAddress;
  }

  uint32_t value;
  for(Section *s:globalSections){
    for(RelocTableEntry& r:s->reloc){
      if(r.symbol>=0){
        value=files[fileNames[s->fileId]]->sections[r.symbol]->startAddress+r.addend; 
      }
      else{
        for(auto par:globTable){
          if(par.second.newId==r.symbol){
            value=par.second.value;
            break;
          }
        }
      }
      s->mem[r.off]=value & 0xFF;
      s->mem[r.off+1]=(value>>8) & 0xFF;
      s->mem[r.off+2]=(value>>16) & 0xFF;
      s->mem[r.off+3]=(value>>24) & 0xFF;
    }
  }
}

void Linker::printBinary(string name){
  fstream output;
  uint32_t cnt;
  output.open(name, ios::binary | ios::out);
  int totalLocations=0;
  for(Section *s:globalSections)
    totalLocations+=s->locCnt;
  output.write((const char*)&totalLocations, sizeof(int));
  for(Section *s:globalSections){
    cnt=s->startAddress;
    while(cnt<s->locCnt+s->startAddress){
      output.write((const char*)&cnt, sizeof(uint32_t));
      output.write((const char*)&s->mem[cnt-s->startAddress], sizeof(uint8_t));
      cnt++;
    }
  }
  output.close();
}