#include "../inc/TES.hpp"
#include <stdio.h>
#include <iostream>

void exitWithError(string err, int errVal){
  cerr<< "ERROR: "<<err<<endl;
  exit(errVal);
}