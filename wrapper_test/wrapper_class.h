#ifndef _FUSE_WRAPPER_H_
#define _FUSE_WRAPPER_H_

#define FUSE_USE_VERSION 26

#include <functional>
#include <fuse.h>
#include <mutex>
#include <unistd.h>
#include <unordered_map>

#include <cstring>

class FuseClient {
 public:
  virtual int getattr(const char*, struct stat*) = 0;
  virtual int readdir(const char*, void*, fuse_fill_dir_t, off_t,
                      struct fuse_file_info*) = 0;
  virtual int open(const char*, fuse_file_info*) = 0;
  virtual int read(const char*, char*, size_t, off_t, fuse_file_info*) = 0;
};

class FuseWrapper {
 public:
  FuseWrapper()
      : oper_{.getattr = getattr_fn,
              .readdir = readdir_fn,
              .open = open_fn,
              .read = read_fn} {}

  // Does not take ownership of client
  static void RegisterFuseClient(std::string path_prefix, FuseClient* client);

  int FuseMain(int argc, char** argv) {
    return fuse_main(argc, argv, &oper_, nullptr);
  }

 private:
  fuse_operations oper_ = {};

  // Requires list_mutex to be held
  static FuseClient* lookup_path(std::string path);

  static int getattr_fn(const char* path, struct stat* stbuf);
  static int readdir_fn(const char* path, void* buf, fuse_fill_dir_t filler,
                        off_t offset, fuse_file_info* fi);
  static int open_fn(const char* path, fuse_file_info* fi);
  static int read_fn(const char* path, char* buf, size_t size, off_t offset,
                     fuse_file_info* fi);

  static std::unordered_map<std::string, FuseClient*> delegation_map;
  static std::mutex list_mutex;
};

#endif //  _FUSE_WRAPPER_H_
