#pragma once
#include <string>
#include <map>
#include <cstring>
#include <algorithm>
using String=std::string;
constexpr int O_WRONLY=1,O_CREAT=2,O_TRUNC=4;
struct FakeFile {
  std::string* contents;
  size_t* budget;
  explicit operator bool() const{return contents;}
  size_t write(const uint8_t* data,size_t count){
    const size_t n=std::min(count,*budget);
    contents->append(reinterpret_cast<const char*>(data),n);
    *budget-=n;return n;
  }
  size_t write(uint8_t c){return write(&c,1);}
  void flush(){}
  void close(){}
};
struct FakeStorage {
  std::map<std::string,std::string> files;
  size_t budget=SIZE_MAX;
  String readFile(const char* path){return files[path];}
  FakeFile open(const char* path,int flags){if(flags&O_TRUNC)files[path].clear();return {&files[path],&budget};}
};
inline FakeStorage fakeStorage;
#define Storage fakeStorage
