#pragma once

#include <fstream>
#include <string>

namespace io {

// https://insanecoding.blogspot.com/2011/11/how-to-read-in-file-in-c.html
[[nodiscard]] std::string read_file(auto &&path) {
  std::ifstream in(std::forward<decltype(path)>(path),
                   std::ios::in | std::ios::binary);
  if (in) {
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();
    return (contents);
  }
#ifdef __cpp_exceptions
  throw(errno);
#endif
  return {};
}

} // namespace io