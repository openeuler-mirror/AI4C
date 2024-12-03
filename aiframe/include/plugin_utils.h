#ifndef AI4C_GCC_PLUGIN_UTILS_H
#define AI4C_GCC_PLUGIN_UTILS_H
#include <option_yaml_utils.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace ai4c {
enum class TuneMode { Unknown, Generate, Tuning };
static std::unordered_map<std::string, TuneMode> TuneOption = {
    {"generate", TuneMode::Generate}, {"autotune", TuneMode::Tuning}};

class CodeRegionHash {
  const size_t offset = 0x9e3779b9;
  std::hash<std::string> stringHash;
  std::hash<int> intHash;

  size_t hashCombine(size_t seed, std::string s) const {
    return seed ^ (stringHash(s) + offset + (seed << 6) + (seed >> 2));
  }

  size_t hashCombine(size_t seed, int i) const {
    return seed ^ (intHash(i) + offset + (seed << 6) + (seed >> 2));
  }

 public:
  size_t operator()(const location_t& loc, std::string func) const {
    size_t seed = 0;
    std::string file = std::string(LOCATION_FILE(loc));

    seed = this->hashCombine(seed, file);
    seed = this->hashCombine(seed, func);
    seed = this->hashCombine(seed, LOCATION_LINE(loc));

    return seed;
  }

  size_t operator()(const location_t& loc, std::string func,
                    int discriminator) const {
    size_t seed = this->operator()(loc, func);
    seed = this->hashCombine(seed, discriminator);

    return seed;
  }
};
static CodeRegionHash codeRegionHash;

struct DebugLoc {
  std::string file;
  int line{0};
  int column{0};

  std::string dump() {
    std::string dl = "DebugLoc: \n";
    dl += "  File: " + file + "\n";
    dl += "  Line: " + std::to_string(line) + "\n";
    dl += "  Column: " + std::to_string(column) + "\n";
    return dl;
  }
};

enum class FeatureType { IntType, FloatType };
struct ExtraFeature {
  ExtraFeature(std::string name, int64_t value) : name(name), int_v(value) {
    this->ft = FeatureType::IntType;
  }

  ExtraFeature(std::string name, float value) : name(name), float_v(value) {
    this->ft = FeatureType::FloatType;
  }

  FeatureType ft{FeatureType::IntType};
  std::string name;
  int64_t int_v;
  float float_v;

  std::string dump() {
    std::string feature;
    switch (ft) {
      case FeatureType::IntType:
        feature = name + ":" + std::to_string(int_v) + ";";
        break;
      case FeatureType::FloatType:
        feature = name + ":" + std::to_string(float_v) + ";";
        break;
      default:
        feature = {};
    }
    return feature;
  }
};

struct ExtraFeatures {
  std::vector<ExtraFeature> fs;
  std::string dump() {
    std::string features{};
    if (fs.size() > 0) {
      features += "(";
      for (ExtraFeature& f : fs) {
        features += f.dump();
      }
      features += ")";
    }
    return features;
  }
};

struct AutoTuning {
  std::string pass;
  std::string name;
  DebugLoc debug_loc;
  size_t code_region_hash;
  std::string code_region_type;
  std::string function;
  int invocation{0};
  int order{0};
  std::map<std::string, int> args;
  ExtraFeatures extra_features;
};

/* Dump YAML FILE */
static void dump_autotuning(AutoTuning& auto_tuning, std::ofstream& out,
                            OptionManager& opt_mgr) {
  out << "Pass: " << auto_tuning.pass << "\n";
  out << "Name: " << auto_tuning.name << auto_tuning.extra_features.dump()
      << "\n";
  out << auto_tuning.debug_loc.dump();
  out << "CodeRegionType: " << auto_tuning.code_region_type << "\n";
  if (auto_tuning.code_region_type != "function") {
    out << "Function: " << auto_tuning.function << "\n";
  }
  out << "CodeRegionHash: " << auto_tuning.code_region_hash << "\n";
  out << opt_mgr.dump_baseline_config();
  out << opt_mgr.dump_dynamic_config();
  out << "Invocation: " << auto_tuning.invocation << "\n";
  out << "Order: " << auto_tuning.order << "\n";
}

static void dump_opp_yaml(std::string& yaml_file, AutoTuning& auto_tuning,
                          OptionManager& opt_mgr) {
  std::ofstream out(yaml_file, std::ios::app);
  if (!out.is_open()) {
    throw std::runtime_error("Can not open file: " + yaml_file);
  }
  out << "--- !AutoTuning\n";
  dump_autotuning(auto_tuning, out, opt_mgr);
  out << "...\n";
  // out.close();
}

/* Load YAML FILE */
std::stringstream get_map_value(std::string& input, size_t start, size_t end) {
  if (start == std::string::npos || end == std::string::npos || start >= end) {
    return {};
  }
  return std::stringstream(input.substr(start + 1, end - start - 1));
}

void parse_args(std::string& src, AutoTuning& auto_tuning) {
  std::vector<std::string> blocks;
  size_t start = src.find("{");
  size_t end = 0;
  while (start != std::string::npos) {
    end = src.find("}", start);
    blocks.push_back(src.substr(start + 1, end - start - 1));
    start = src.find("{", end);
  }

  for (std::string& block : blocks) {
    std::stringstream ss(block);
    std::string key, value;
    while (std::getline(ss, key, ':')) {
      key.erase(std::remove_if(key.begin(), key.end(), ::isspace), key.end());
      std::getline(ss, value, ',');
      value.erase(std::remove_if(value.begin(), value.end(), ::isspace),
                  value.end());
      if (value.back() == '\'') value.pop_back();
      if (!value.empty() && value[0] == '\'') value.erase(value.begin());

      try {
        auto_tuning.args[key] = std::stoi(value);
      } catch (const std::invalid_argument& e) {
        auto_tuning.args[key] = -1;
      } catch (const std::out_of_range& e) {
        auto_tuning.args[key] = -2;
      }
    }
  }
}

static int parse_tuning_info(std::string& input, AutoTuning& auto_tuning) {
  std::stringstream ss =
      get_map_value(input, input.find("{"), input.rfind("}"));
  std::string key;
  std::string value;
  while (std::getline(ss, key, ':')) {
    key.erase(std::remove_if(key.begin(), key.end(), ::isspace), key.end());
    if (key == "Args") {
      std::getline(ss, value, '[');
      std::getline(ss, value, ']');
      parse_args(value, auto_tuning);
      std::getline(ss, key, ',');
    } else if (key == "CodeRegionHash") {
      std::getline(ss, value, ',');
      value.erase(std::remove_if(value.begin(), value.end(), ::isspace),
                  value.end());

      try {
        auto_tuning.code_region_hash = std::stoull(value);
      } catch (const std::invalid_argument& e) {
        auto_tuning.code_region_hash = -1;
      } catch (const std::out_of_range& e) {
        auto_tuning.code_region_hash = -2;
      }
    } else {
      std::getline(ss, value, ',');
    }
  }
  return 1;
}

static std::unordered_map<size_t, AutoTuning> AutoTuneOptions;

static int load_config_yaml(std::string& config_file) {
  std::ifstream configs(config_file, std::ios::in);
  if (!configs.is_open()) {
    std::cerr << "Error: " << config_file << std::endl;
    return 1;
  }

  std::string line;
  while (std::getline(configs, line)) {
    if (line.find("CodeRegionType: ") == std::string::npos) {
      continue;  // only do loop unroll / coarse-grained tuning
    }
    AutoTuning auto_tuning;
    if (parse_tuning_info(line, auto_tuning)) {
      AutoTuneOptions[auto_tuning.code_region_hash] = auto_tuning;
    }
  }
  configs.close();
  return 0;
}
}  // namespace ai4c

#endif  // AI4C_GCC_PLUGIN_UTILS_H