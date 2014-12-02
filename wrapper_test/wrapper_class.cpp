#include "wrapper_class.h"

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

class TestFuseClient : public FuseClient {
  int getattr(const char* path, struct stat* stbuf) override {
    int res = 0;
    memset(stbuf, 0, sizeof(*stbuf));
    if (strcmp(path, "/") == 0) {
      stbuf->st_mode = S_IFDIR | 0755;
      stbuf->st_nlink = 2;
    } else if (strcmp(path, "/sup") == 0) {
      stbuf->st_mode = S_IFREG | 0444;
      stbuf->st_nlink = 1;
      stbuf->st_size = strlen("The quick brown fox");
    } else
      res = -ENOENT;
    return res;
  }
  int readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset,
              fuse_file_info* fi) override {
    if (strcmp(path, "/") != 0) return -ENOENT;

    filler(buf, ".", nullptr, 0);
    filler(buf, "..", nullptr, 0);
    filler(buf, "sup", nullptr, 0);
    return 0;
  }
  int open(const char* path, fuse_file_info* fi) override {
    if (strcmp(path, "/sup") != 0)
      return -ENOENT;

    return 0;
  }
  int read(const char* path, char* buf, size_t size, off_t offset,
           fuse_file_info* fi) override {
    if (strcmp(path, "/sup") != 0) return -ENOENT;

    char text[] = "The quick brown fox";
    size_t len = strlen(text);
    if (offset < len) {
      if (offset + size > len) size = len - offset;
      memcpy(buf, text + offset, size);
    } else
      size = 0;

    return size;
  }
};

int main(int argc, char** argv){
  FuseWrapper fw;
  TestFuseClient tfc;
  fw.RegisterFuseClient("/", &tfc);
  fw.FuseMain(argc, argv);
}
