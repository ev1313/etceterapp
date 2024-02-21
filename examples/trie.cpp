#include<iostream>
#include <algorithm>
#include <vector>
#include <map>
#include <spdlog/spdlog.h>
#include <cpptrace/cpptrace.hpp>

class Trie {
  const std::vector<char> characters = {'\\', '-', '.', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '_', ' ', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};

  class TrieNode {

  public:
    std::map<char, TrieNode> children;
    // A flag that marks if the word ends on this particular node.
    bool end_of_word = false;
    // Character stored in this node
    char letter = '\0';
  };

public:

  TrieNode root;

  // Insert the word in the trie.
  // Check each character in the string
  // If none of the children of the current node contains the character,
  // create a new child of the current node for storing the character.
  void insert (const std::string& str) {
    auto& current = root;
    bool first = true;
    for(char letter : str) {
      if(first) {
        first = false;
        current.letter = letter;
        continue;
      }
      if(std::find(characters.begin(), characters.end(), letter) == characters.end()) {
        spdlog::error("Invalid character in string: {}", letter);
        throw std::runtime_error("Invalid character in string " + str);
      }

      if(current.children.find(letter) == current.children.end()) {
        current.children[letter] = TrieNode();
        current.children[letter].letter = letter;
        current = current.children[letter];
      }
    }
    current.end_of_word = true;
  }

  size_t get_size() {
    size_t ret = 0;
    for(size_t i = 0; i < characters.size(); i++) {
      if(root.children[i] != nullptr) {
        ret += 1;
      }
    }
    return ret;
  }
};
