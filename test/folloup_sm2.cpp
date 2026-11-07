#include "../src/folloup/Sm2.h"
#include <cassert>
#include <iostream>
int main() {
  using namespace folloup;
  const uint64_t now=1800000000;
  Review r;
  r=grade(r,4,now);
  assert(r.intervalDays==1&&r.repetitions==1&&r.due==now+86400&&r.ease==2500);
  r=grade(r,4,r.due);
  assert(r.intervalDays==6&&r.repetitions==2);
  r=grade(r,4,r.due);
  assert(r.intervalDays==15&&r.correct==3);
  r=grade(r,1,now);
  assert(r.repetitions==0&&r.incorrect==1&&r.due==now+60);
  for(int i=0;i<100;++i)r=grade(r,0,now);
  assert(r.ease==1300);
  for(int i=0;i<100;++i)r=grade(r,5,now);
  assert(r.intervalDays<=36500);
  Review jp,en;
  jp=grade(jp,4,now);
  assert(en.correct==0&&en.due==0&&jp.correct==1);
  assert(checksum("hello",5)==0x4f9f2cab);
  assert(checksum("hello",5)!=checksum("jello",5));
  std::cout<<"PASS: SM-2 intervals, lapse, ease floor, cap, direction isolation, checksum\n";
}
