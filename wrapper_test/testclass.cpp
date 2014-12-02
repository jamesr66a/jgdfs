#include "delegator.h"

#include <regex>
#include <map>

class TestFuseClient : public FuseClient {
 public:
  int getattr(const char* path, struct stat* stbuf) override {
    memset(stbuf, 0, sizeof(*stbuf));
    auto file_itr = root.children.find(path+1);
    if (file_itr == root.children.end())
      return -ENOENT;

    stbuf->st_mode = S_IFREG | 0444;
    stbuf->st_nlink = 1;
    stbuf->st_size = strlen("The quick brown fox\n");

    return 0;
  }
  int readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset,
              fuse_file_info* fi) override {
    if (strcmp(path, "/") != 0) return -ENOENT;

    filler(buf, ".", nullptr, 0);
    filler(buf, "..", nullptr, 0);
    for (const auto& x : root.children) {
      filler(buf, x.first.c_str(), nullptr, 0);
    }
    return 0;
  }
  int open(const char* path, fuse_file_info* fi) override {
    auto file_itr = root.children.find(path+1);
    if (file_itr == root.children.end())
      return -ENOENT;

    return 0;
  }

  int read(const char* path, char* buf, size_t size, off_t offset,
           fuse_file_info* fi) override {
    auto file_itr = root.children.find(path+1);
    if (file_itr == root.children.end())
      return -ENOENT;

    char text[] = "The quick brown fox\n";
    size_t len = strlen(text);
    if (offset < len) {
      if (offset + size > len) size = len - offset;
      memcpy(buf, text+offset, size);
    } else
      size = 0;

    return size;
  }

  struct Entity {
    Entity(std::string name) : name(name) {}
    std::string name;
    std::map<std::string, std::unique_ptr<Entity>> children;
    std::string download_url;
    bool is_folder = false;;
  };

  Entity root{""};

  TestFuseClient() {
    root.children["foo"].reset(new Entity{"foo"});
    root.children["bar"].reset(new Entity{"bar"});
    root.children["baz"].reset(new Entity{"baz"});
  }
};

int main(int argc, char** argv) {
  TestFuseClient tfc;
  tfc.wrapper.RegisterFuseClient("/", &tfc);
  tfc.wrapper.FuseMain(argc, argv);
}
