#include "delegator.h"

#include <iostream>

std::unordered_map<std::string, FuseClient*> FuseWrapper::delegation_map{};
std::mutex FuseWrapper::list_mutex{};

void FuseWrapper::RegisterFuseClient(std::string path_prefix,
                                     FuseClient* client) {
  std::unique_lock<std::mutex> lk(list_mutex);
  delegation_map[path_prefix] = client;
}

FuseClient* FuseWrapper::lookup_path(std::string path) {
  std::string acc;
  FuseClient* retval = nullptr;
  std::cout << "path: " << path << std::endl;
  for (const auto& x : path) {
    acc += x;
    auto map_itr = delegation_map.find(acc);
    if (map_itr == delegation_map.end())
      continue;
    else {
      retval = map_itr->second;
      break;
    }
  }
  return retval;
}

int FuseWrapper::getattr_fn(const char* path, struct stat* stbuf) {
  std::unique_lock<std::mutex> lk(list_mutex);
  FuseClient* client = lookup_path(path);
  if (client == nullptr)
    return -1;
  else
    return client->getattr(path, stbuf);
}

int FuseWrapper::readdir_fn(const char* path, void* buf, fuse_fill_dir_t filler,
                            off_t offset, fuse_file_info* fi) {
  std::unique_lock<std::mutex> lk(list_mutex);
  FuseClient* client = lookup_path(path);
  if (client == nullptr)
    return -1;
  else
    return client->readdir(path, buf, filler, offset, fi);
}

int FuseWrapper::open_fn(const char* path, fuse_file_info* fi) {
  std::unique_lock<std::mutex> lk(list_mutex);
  FuseClient* client = lookup_path(path);
  if (client == nullptr)
    return -1;
  else
    return client->open(path, fi);
}

int FuseWrapper::read_fn(const char* path, char* buf, size_t size, off_t offset,
            fuse_file_info* fi) {
  std::unique_lock<std::mutex> lk(list_mutex);
  FuseClient* client = lookup_path(path);
  if (client == nullptr)
    return -1;
  else
    return client->read(path, buf, size, offset, fi);
}


