#ifndef AI4C_OPTION_YAML_UTILS_H
#define AI4C_OPTION_YAML_UTILS_H

#include <yaml-cpp/yaml.h>

#include <functional>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace ai4c {

struct IntOptionValues {
  int min_val;
  int max_val;
};

struct IntOptDynConfigStrategy {
  virtual ~IntOptDynConfigStrategy() = default;
  virtual void execute(IntOptionValues& int_opt_vals, int default_val,
                       std::vector<int>& dyn_config,
                       int n_elements = 6) const = 0;
};

struct UnifDynConfigStrategy : IntOptDynConfigStrategy {
  virtual ~UnifDynConfigStrategy() = default;
  virtual void execute(IntOptionValues& int_opt_vals, int default_val,
                       std::vector<int>& dyn_config,
                       int n_elements = 6) const override {
    int min_val = int_opt_vals.min_val;
    int max_val = int_opt_vals.max_val;
    int intvl = (max_val - min_val) / n_elements;
    intvl = intvl > 0 ? intvl : 1;

    int remain_size = n_elements - 1;
    dyn_config.push_back(default_val);
    int incr_val = default_val;
    int decr_val = default_val;
    while ((decr_val - intvl >= min_val || incr_val + intvl <= max_val) &&
           remain_size > 0) {
      if (decr_val - intvl >= min_val) {
        decr_val -= intvl;
        dyn_config.push_back(decr_val);
        remain_size--;

        if (remain_size == 0) break;
      }
      if (incr_val + intvl <= max_val) {
        incr_val += intvl;
        dyn_config.push_back(incr_val);
        remain_size--;
      }
    }
  }
};

struct Option {
  std::string name;
  std::string value_type;
  std::string option_category;
  std::string repr;
  /* Additional Args: Pass, EnableBy, Dep */
  std::vector<std::map<std::string, std::string>> args;

  virtual ~Option() = default;
  virtual std::string base_config_serialize() const = 0;
  virtual std::string dyn_config_serialize() const = 0;
  virtual void deserialize(const YAML::Node& node,
                           const std::string& category) {
    const std::set<std::string> other_options = {"Pass", "EnableBy", "Dep"};
    if (node.IsMap()) {
      this->option_category = category;
      for (const auto& opt_attr : node) {
        std::string attr = opt_attr.first.as<std::string>();
        if (attr == "Name") {
          this->name = opt_attr.second.as<std::string>();
        } else if (attr == "Type") {
          this->value_type = opt_attr.second.as<std::string>();
        } else if (attr == "Repr") {
          this->repr = opt_attr.second.as<std::string>();
        } else if (other_options.count(attr)) {
          std::map<std::string, std::string> arg_pairs =
              std::map<std::string, std::string>();
          arg_pairs[attr] = opt_attr.second.as<std::string>();
          this->args.push_back(arg_pairs);
        }
      }
      if (this->repr.empty()) this->repr = this->name;
    }
  }
};

struct EnumOption : public Option {
  std::string default_val;
  std::vector<std::string> values;

  std::string base_config_serialize() const override {
    return "  " + this->repr + ": " + this->default_val + "\n";
  }

  std::string dyn_config_serialize() const override {
    std::string config = "  " + this->repr + ": \n";
    for (const std::string& value : this->values) {
      config += "    - " + value + "\n";
    }
    return config;
  }

  void deserialize(const YAML::Node& node,
                   const std::string& category) override {
    Option::deserialize(node, category);
    if (node.IsMap()) {
      for (const auto& opt_attr : node) {
        std::string attr = opt_attr.first.as<std::string>();
        if (attr == "Value") {
          deserialize_value(opt_attr.second);
        }
      }
    }
  }

 private:
  void deserialize_value(const YAML::Node& node) {
    if (node.IsSequence()) {
      for (size_t i = 0; i < node.size(); ++i) {
        if (node[i].IsScalar()) {
          this->values.push_back(node[i].as<std::string>());
        }
      }
      if (node.size() > 1) {
        this->default_val = node[0].as<std::string>();
      }
    }
  }
};

struct BoolOption : public Option {
  bool default_val;

  std::string base_config_serialize() const override {
    std::string value = this->default_val ? "1" : "0";
    return "  " + this->repr + ": " + value + "\n";
  }

  std::string dyn_config_serialize() const override {
    std::string config = "  " + this->repr + ": \n";
    return config;
  }

  void deserialize(const YAML::Node& node,
                   const std::string& category) override {
    Option::deserialize(node, category);
    if (node.IsMap()) {
      for (const auto& opt_attr : node) {
        std::string attr = opt_attr.first.as<std::string>();
        if (attr == "Default") {
          deserialize_value(opt_attr.second);
        }
      }
    }
  }

 private:
  void deserialize_value(const YAML::Node& node) {
    if (node.IsScalar()) {
      this->default_val = node.as<int>();
    }
  }
};

struct IntOption : public Option {
  int default_val;
  std::vector<int> values;
  IntOptionValues int_opt_vals;
  enum class Values { DEFAULT, MIN, MAX };
  std::unique_ptr<IntOptDynConfigStrategy> strategy;

  std::string base_config_serialize() const override {
    std::string value = std::to_string(this->default_val);
    return "  " + this->repr + ": " + value + "\n";
  }

  std::string dyn_config_serialize() const override {
    std::string config = "  " + this->repr + ": \n";
    for (int value : this->values) {
      config += "    - " + std::to_string(value) + "\n";
    }
    return config;
  }

  void deserialize(const YAML::Node& node,
                   const std::string& category) override {
    Option::deserialize(node, category);
    if (node.IsMap()) {
      for (const auto& opt_attr : node) {
        std::string attr = opt_attr.first.as<std::string>();
        if (attr == "Default") {
          deserialize_value(opt_attr.second, Values::DEFAULT);
        } else if (attr == "Min") {
          deserialize_value(opt_attr.second, Values::MIN);
        } else if (attr == "Max") {
          deserialize_value(opt_attr.second, Values::MAX);
        }
      }
      strategy = std::make_unique<UnifDynConfigStrategy>();
      strategy->execute(this->int_opt_vals, this->default_val, this->values);
    }
  }

  void set_option_attribute(const std::string& type,
                            const std::string& category,
                            const std::string& name,
                            const std::vector<int>& dynamic_values,
                            int default_val) {
    this->option_category = category;
    this->name = name;
    this->repr = name;
    this->default_val = default_val;
    this->values = dynamic_values;
  }

 private:
  void deserialize_value(const YAML::Node& node, Values values) {
    if (node.IsScalar()) {
      int value = node.as<int>();
      switch (values) {
        case Values::DEFAULT:
          this->default_val = value;
          break;
        case Values::MIN:
          this->int_opt_vals.min_val = value;
          break;
        case Values::MAX:
          this->int_opt_vals.max_val = value;
          break;
      }
    }
  }
};

template <typename T>
std::string base_config_serialize(const T& obj) {
  return obj.base_config_serialize();
}

template <typename T>
std::string dyn_config_serialize(const T& obj) {
  return obj.dyn_config_serialize();
}

template <typename T>
void deserialize(T& obj, const YAML::Node& node, const std::string& category) {
  obj.deserialize(node, category);
}

class OptionManager {
  std::vector<std::shared_ptr<Option>> options;
  std::map<std::string, std::function<std::shared_ptr<Option>()>> creators;

 public:
  OptionManager() {
    creators["Enum"] = []() { return std::make_shared<EnumOption>(); };
    creators["Uint"] = []() { return std::make_shared<IntOption>(); };
    creators["Bool"] = []() { return std::make_shared<BoolOption>(); };
  }

  void add_option(const std::string& type, const YAML::Node& node,
                  const std::string& category) {
    auto creator = creators[type];
    auto option = creator();
    deserialize(*option, node, category);
    this->options.push_back(option);
  }

  void add_fine_grained_option(const std::string& type,
                               const std::string& category,
                               const std::string& name,
                               const std::vector<int>& dynamic_values,
                               int default_val) {
    if (type != "Uint") {
      std::cerr << "Type " << type << " is not currently supported "
                << "for adding fine-grained options." << std::endl;
      return;
    }
    auto creator = creators[type];
    auto option = creator();

    std::shared_ptr<IntOption> int_opt_ptr =
        std::static_pointer_cast<IntOption>(option);
    int_opt_ptr->set_option_attribute(type, category, name, dynamic_values,
                                      default_val);
    this->options.push_back(option);
  }

  std::string dump_baseline_config() const {
    std::string config = "BaselineConfig: \n";
    for (const auto& opt_ptr : this->options) {
      config += opt_ptr->base_config_serialize();
    }
    return config;
  }

  std::string dump_dynamic_config() const {
    std::string config = "DynamicConfigs: \n";
    for (const auto& opt_ptr : this->options) {
      config += opt_ptr->dyn_config_serialize();
    }
    return config;
  }

  void dump() const {
    std::cout << "size: " << this->options.size() << std::endl;
    for (const auto& opt_ptr : this->options) {
      std::cout << "name: " << opt_ptr->name << std::endl;
      std::cout << "value_type: " << opt_ptr->value_type << std::endl;
      if (opt_ptr->value_type == "Enum") {
        std::shared_ptr<EnumOption> enum_opt_ptr =
            std::static_pointer_cast<EnumOption>(opt_ptr);
        for (const auto& value : enum_opt_ptr->values) {
          std::cout << "value: " << value << std::endl;
        }
      } else if (opt_ptr->value_type == "Uint") {
        std::shared_ptr<IntOption> int_opt_ptr =
            std::static_pointer_cast<IntOption>(opt_ptr);
        std::cout << "min value: " << int_opt_ptr->int_opt_vals.min_val
                  << std::endl;
        std::cout << "max value: " << int_opt_ptr->int_opt_vals.max_val
                  << std::endl;
      }
      std::cout << std::endl;
    }
  }
};

void read_option(const YAML::Node& opt_node, const std::string& category,
                 OptionManager& opt_mgr) {
  if (opt_node.IsMap()) {
    for (const auto& opt_kvpair : opt_node) {
      std::string opt_attr = opt_kvpair.first.as<std::string>();
      if (opt_attr == "Type") {
        std::string opt_type = opt_kvpair.second.as<std::string>();
        opt_mgr.add_option(opt_type, opt_node, category);
      }
    }
  }
}

void read_option_category(const YAML::Node& node, const std::string& category,
                          OptionManager& opt_mgr) {
  if (node.IsSequence()) {
    for (size_t i = 0; i < node.size(); ++i) {
      read_option(node[i], category, opt_mgr);
    }
  }
}

void load_coarse_option_yaml(const std::string& filename,
                             OptionManager& opt_mgr) {
  // Load the YAML file
  const std::set<std::string> core_categories = {"GIMPLE", "RTL", "PARAM"};

  YAML::Node doc;
  try {
    doc = YAML::LoadFile(filename);
  } catch (YAML::ParserException& e) {
    std::cerr << "Error parsing coarse option YAML: " << e.what() << std::endl;
  }

  if (doc.IsMap()) {
    for (const auto& option_type : doc) {
      const YAML::Node& node = option_type.second;

      std::string category = option_type.first.as<std::string>();
      if (core_categories.count(category)) {
        read_option_category(node, category, opt_mgr);
      }
    }
  }
}

}  // namespace ai4c

#endif  // AI4C_OPTION_YAML_UTILS_H