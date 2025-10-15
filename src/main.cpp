#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <zlib.h>

using namespace std;

string decompressZlib(const string &compressed) {
  z_stream zs{};
  inflateInit(&zs);

  zs.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(compressed.data()));
  zs.avail_in = compressed.size();

  string out;
  char buffer[32768];

  int ret;
  do {
    zs.next_out = reinterpret_cast<Bytef *>(buffer);
    zs.avail_out = sizeof(buffer);

    ret = inflate(&zs, Z_NO_FLUSH);
    if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR)
      throw runtime_error("Decompression failed");

    out.append(buffer, sizeof(buffer) - zs.avail_out);
  } while (ret != Z_STREAM_END);

  inflateEnd(&zs);
  return out;
}

int main(int argc, char *argv[]) {
  // Flush after every std::cout / std::cerr
  cout << std::unitbuf;
  cerr << std::unitbuf;

  // You can use print statements as follows for debugging, they'll be visible
  // when running tests.
  cerr << "Logs from your program will appear here!\n";

  if (argc < 2) {
    cerr << "No command provided.\n";
    return EXIT_FAILURE;
  }

  string command = argv[1];

  if (command == "init") {
    try {
      filesystem::create_directory(".git");
      filesystem::create_directory(".git/objects");
      filesystem::create_directory(".git/refs");

      ofstream headFile(".git/HEAD");
      if (headFile.is_open()) {
        headFile << "ref: refs/heads/main\n";
        headFile.close();
      } else {
        cerr << "Failed to create .git/HEAD file.\n";
        return EXIT_FAILURE;
      }

      cout << "Initialized git directory\n";
    } catch (const filesystem::filesystem_error &e) {
      cerr << e.what() << '\n';
      return EXIT_FAILURE;
    }
  } else if (command == "cat-file") {
    int argp = 2;
    string hash;
    while (hash.empty()) {
      argp++;
      if (argc < argp) {
        cerr << "No object hash provided.\n";
        return EXIT_FAILURE;
      }
      if (argv[argp][0] != '-') {
        hash = argv[argp];
      }
    }

    string path = string(".git/objects/") + hash.substr(0, 2).data() + "/" +
                  hash.substr(2).data();
    ifstream file(path);
    if (!file.is_open()) {
      cerr << "Couldn't open file for the hash.\n";
      return EXIT_FAILURE;
    }

    stringstream buffer;
    buffer << file.rdbuf();
    string compressed = buffer.str();

    string decompressed = decompressZlib(compressed);
    cout << decompressed.substr(decompressed.find('\0') + 1);
  } else {
    cerr << "Unknown command " << command << '\n';
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
