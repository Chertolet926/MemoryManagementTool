#include "dex/kv_parser.hpp"
#include <iostream>

int main() {
    std::string_view input = "Size:                132 kB\n"
                           "KernelPageSize:        4 kB\n"
                           "MMUPageSize:           4 kB\n"
                           "Rss:                  56 kB\n"
                           "Pss:                  56 kB\n"
                           "Pss_Dirty:            56 kB\n"
                           "Shared_Clean:          0 kB\n"
                           "Shared_Dirty:          0 kB\n"
                           "Private_Clean:         0 kB\n"
                           "Private_Dirty:        0x1F\n"
                           "Referenced:           56 kB\n"
                           "Anonymous:            56 kB\n"
                           "KSM:                   0 kB\n"
                           "LazyFree:              0 kB\n"
                           "AnonHugePages:         0 kB\n"
                           "ShmemPmdMapped:        0 kB\n"
                           "FilePmdMapped:         0xFC\n"
                           "Shared_Hugetlb:        0 kB\n"
                           "Private_Hugetlb:       0 kB\n"
                           "Swap:                  0 kB\n"
                           "SwapPss:               0 kB\n"
                           "Locked:                0kfds34\n"
                           "THPeligible:           0   \n"
                           "key: 123 0x10 string_val\n"
                           "VmFlags: rd wr mr mw me gd ac\n";

  auto res = dex::KVParser::from_string(input);
  if (!res) {
    std::cerr << "Error at " << res.error().offset << ": "
              << res.error().ec.message() << std::endl;
    return 1;
  }

  for (auto const &[name, vals] : res.value()) {
    std::cout << name << ": ";
    for (auto const& v : vals) {
      std::visit([](auto&& arg) { std::cout << "[" << arg << "] "; }, v);
    }
    std::cout << "\n";
  }

  return 0;
}
