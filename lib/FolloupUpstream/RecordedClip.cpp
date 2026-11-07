// Adapted from Folloup-Sticky recording_service.cpp (GPL-3.0).
#include "recording_service.h"
#include <HalStorage.h>
#include <array>
#include <utility>
namespace recording_service {
RecordedClip::RecordedClip(uint32_t rate,size_t count,ChunkList chunks)
 : sample_rate_hz_(rate),sample_count_(count),chunks_(std::move(chunks)){}
RecordedClip::RecordedClip(uint32_t rate,size_t count,std::string path)
 : sample_rate_hz_(rate),sample_count_(count),path_(std::move(path)){}
uint32_t RecordedClip::duration_ms() const{return sample_rate_hz_?uint64_t(sample_count_)*1000/sample_rate_hz_:0;}
void RecordedClip::ForEachChunk(const std::function<void(const int16_t*,size_t)>& visitor) const {
  if(!visitor)return;
  if(path_.empty()){for(const auto& c:chunks_)if(!c.empty())visitor(c.data(),c.size());return;}
  auto file=Storage.open(path_.c_str());
  if(!file||!file.seekSet(44)){read_failed_=true;return;}
  std::array<int16_t,1024> buffer{};
  size_t remaining=sample_count_;
  while(remaining){
    const size_t count=std::min(remaining,buffer.size());
    if(file.read(buffer.data(),count*2)!=int(count*2)){read_failed_=true;return;}
    visitor(buffer.data(),count);remaining-=count;
  }
}
}
