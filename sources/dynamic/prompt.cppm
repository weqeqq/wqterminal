module;

#include <algorithm>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

export module weqeqq.terminal:prompt;

import :console;
import :text;
import :style;

export namespace weqeqq::terminal {

class Prompt {
 public:
  static std::string Ask(Console& console, const std::string& question,
                         const std::string& default_value = "",
                         const std::vector<std::string>& choices = {},
                         bool show_default = true) {
    for (;;) {
      std::string prompt = question;
      if (!choices.empty()) {
        prompt += " \\[";
        for (std::size_t i = 0; i < choices.size(); ++i) {
          prompt += choices[i];
          if (i + 1 < choices.size()) prompt += "/";
        }
        prompt += "]";
      }
      if (show_default && !default_value.empty())
        prompt += " (" + default_value + ")";
      prompt += ": ";

      Text qt = console.RenderStr(prompt);
      console.WriteRaw(console.SegmentsToString(qt.RenderLine()));

      std::string line;
      if (!std::getline(std::cin, line)) return default_value;
      if (line.empty()) return default_value;
      if (choices.empty() ||
          std::find(choices.begin(), choices.end(), line) != choices.end()) {
        return line;
      }
      console.Print("[prompt.choices]Please select one of the choices[/]");
    }
  }
};

class Confirm {
 public:
  static bool Ask(Console& console, const std::string& question,
                  bool default_value = true) {
    std::string def = default_value ? "y" : "n";
    std::string answer =
        Prompt::Ask(console, question, def, {"y", "n", "yes", "no"});
    return answer == "y" || answer == "yes";
  }
};

class IntPrompt {
 public:
  static long long Ask(Console& console, const std::string& question,
                       long long default_value = 0) {
    for (;;) {
      std::string s =
          Prompt::Ask(console, question, std::to_string(default_value));
      try {
        std::size_t pos = 0;
        long long v = std::stoll(s, &pos);
        if (pos == s.size()) return v;
      } catch (...) {
      }
      console.Print("[prompt.choices]Please enter a whole number[/]");
    }
  }
};

class FloatPrompt {
 public:
  static double Ask(Console& console, const std::string& question,
                    double default_value = 0.0) {
    for (;;) {
      std::string s =
          Prompt::Ask(console, question, std::to_string(default_value));
      try {
        std::size_t pos = 0;
        double v = std::stod(s, &pos);
        if (pos == s.size()) return v;
      } catch (...) {
      }
      console.Print("[prompt.choices]Please enter a number[/]");
    }
  }
};

}
