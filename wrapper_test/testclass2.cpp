#include "delegator.h"

#include <regex>
#include <unordered_map>

class TestFuseClient : public FuseClient {
 public:
  int getattr(const char* path, struct stat* stbuf) override {
    int res = 0;
    memset(stbuf, 0, sizeof(*stbuf));
    if (strcmp(path, "/") == 0) {
      stbuf->st_mode = S_IFDIR | 0755;
      stbuf->st_nlink = 2;
    } else if (strcmp(path, "/sup") == 0) {
      stbuf->st_mode = S_IFREG | 0444;
      stbuf->st_nlink = 1;
      stbuf->st_size = strlen("The quick brown fox\n");
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

    char text[] = "The quick brown fox\n";
    size_t len = strlen(text);
    if (offset < len) {
      if (offset + size > len) size = len - offset;
      memcpy(buf, text + offset, size);
    } else
      size = 0;

    return size;
  }

  struct Entity {
    std::string name;
    std::unordered_map<std::string, std::unique_ptr<Entity>> children;
    std::string download_url;
  };

  Entity root;

  TestFuseClient() {
    root.children["foo"].reset(new Entity{.name = "foo"});
    root.children["bar"].reset(new Entity{.name = "bar"});
    root.children["baz"].reset(new Entity{.name = "baz"});
  }
};

int main(int argc, char** argv) {
  TestFuseClient tfc;
  tfc.wrapper.RegisterFuseClient("/", &tfc);
  tfc.wrapper.FuseMain(argc, argv);
}
