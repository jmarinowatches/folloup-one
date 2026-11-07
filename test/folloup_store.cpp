#include "../src/folloup/Store.h"
#include <cassert>
#include <iostream>
int main(){
  using namespace folloup;
  JsonDocument doc,loaded;uint32_t gen=0,found=0;
  assert(!loadSnapshot("/p",loaded,found));
  doc["correct"]=1;
  assert(saveSnapshot("/p",doc,gen)&&gen==1);
  assert(loadSnapshot("/p",loaded,found)&&found==1&&loaded["correct"]==1);
  // Inject a power loss at each byte of the next write. The old snapshot must
  // remain loadable, and a failed save must not advance the generation.
  for(size_t cut=0;cut<80;++cut){
    fakeStorage.files.erase("/p.b.json");fakeStorage.budget=cut;
    uint32_t trial=gen;doc["correct"]=2;
    bool saved=saveSnapshot("/p",doc,trial);
    assert(loadSnapshot("/p",loaded,found));
    if(!saved){assert(trial==1&&found==1&&loaded["correct"]==1);}
    else assert(trial==2&&found==2&&loaded["correct"]==2);
  }
  fakeStorage.budget=SIZE_MAX;
  assert(saveSnapshot("/p",doc,gen)&&gen==2);
  auto& raw=fakeStorage.files["/p.b.json"];
  auto at=raw.find("correct");assert(at!=std::string::npos);raw[at]='x';
  assert(loadSnapshot("/p",loaded,found)&&found==1&&loaded["correct"]==1);
  std::cout<<"PASS: dual snapshots, 80 interrupted writes, generation guard, checksum fallback\n";
}
