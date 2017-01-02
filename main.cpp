#include "jsoncpp/src/lib_json/json_reader.cpp"
#include "jsoncpp/src/lib_json/json_writer.cpp"
#include "jsoncpp/src/lib_json/json_value.cpp"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/error/error.h"
#include <cstdlib>
#include <ctime>
#include <iostream>

#define PERFORM_TYPE_CHECKING 1
#define PRINT_PARSED_JSON 0

void print_int_array(std::vector<int> vec) {
  std::cout << "[";
  for (int i = 0; i < vec.size(); i++) {
    std::cout << vec[i];
    if (i < vec.size() - 1) {
      std::cout << ", ";
    }
  }
  std::cout << "]\n";
}

void print_pair_array(std::vector<std::pair<std::string, int>> vec) {
  std::cout << "[";
  for (int i = 0; i < vec.size(); i++) {
    std::cout << "(" << vec[i].first << ", " << vec[i].second << ")";
    if (i < vec.size() - 1) {
      std::cout << ", ";
    }
  }
  std::cout << "]\n";
}

std::string generate_json_string(
    std::string name, int num_models, int input_len) {
  srand(1);
  std::stringstream ss;
  ss << "{";
  ss << "\"name\": \"" << name << "\", ";
  ss << "\"input\": ";
  ss << "[";
  for (int i = 0; i < input_len; i++) {
    ss << rand() % 256;
    if (i < input_len - 1) {
      ss << ", ";
    }
  }
  ss << "], ";

  ss << "\"models\": ";
  ss << "[";
  for (int i = 0; i < num_models; i++) {
    ss << "{";
    ss << "\"" << ((char) ('a' + rand() % 26)) << "\"";
    ss << ": " << rand() % 10;
    ss << "}";
    if (i < num_models - 1) {
      ss << ", ";
    }
  }
  ss << "]";
  ss << "}";
  return ss.str();
}

std::pair<double, double> run_rapidjson_trial(
    std::string json, int num_trials) {
  const char* json_str = json.c_str();
  double parse_time = 0;
  double deserialize_time = 0;
  for (int i = 0; i < num_trials; i++) {
    std::clock_t start = std::clock();
    rapidjson::Document d;
    d.Parse(json_str);
    std::clock_t end = std::clock();
    parse_time += (end - start) / (double) CLOCKS_PER_SEC;

    std::vector<int> input_vals;
    std::vector<std::pair<std::string, int>> models;
    input_vals.reserve(d["input"].Capacity());
    models.reserve(d["models"].Capacity());
    start = std::clock();
    std::string name = d["name"].GetString();
    for (rapidjson::Value& elem : d["input"].GetArray()) {
#if PERFORM_TYPE_CHECKING
      if (elem.IsInt()) {
        input_vals.push_back(elem.GetInt());
      }
#else
      input_vals.push_back(elem.GetInt());
#endif
    }

    for (rapidjson::Value& entry : d["models"].GetArray()) {
#if PERFORM_TYPE_CHECKING
      if (entry.IsObject()) {
        for (auto &m : entry.GetObject()) {
          if (m.name.IsString() && m.value.IsInt()) {
            models.push_back(std::make_pair(m.name.GetString(), m.value.GetInt())); 
          }
        }
      }
#else
      for (auto &m : entry.GetObject()) {
        models.push_back(std::make_pair(m.name.GetString(), m.value.GetInt())); 
      }
#endif
    }
    end = std::clock();
    deserialize_time += (end - start) / (double) CLOCKS_PER_SEC;
#if PRINT_PARSED_JSON
    print_int_array(input_vals);
    print_pair_array(models);
#endif
  }
  parse_time /= (double) num_trials;
  deserialize_time /= (double) num_trials;

  return std::make_pair(parse_time, deserialize_time);
}

std::pair<double, double> run_jsoncpp_trial(
    std::string json, int num_trials) {
  const char* json_str = json.c_str();
  double parse_time = 0;
  double deserialize_time = 0;
  for (int i = 0; i < num_trials; i++) {
    std::clock_t start = std::clock();
    Json::Value root;
    Json::Reader reader;
    reader.parse(json_str, root);
    std::clock_t end = std::clock();
    parse_time += (end - start) / (double) CLOCKS_PER_SEC;

    std::vector<int> input_vals;
    std::vector<std::pair<std::string, int>> models;
    input_vals.reserve(root["input"].size());
    models.reserve(root["models"].size());
    start = std::clock();
    root["name"].asString();
    for (const Json::Value& elem : root["input"]) {
#if PERFORM_TYPE_CHECKING
      if (elem.isInt()) {
        input_vals.push_back(elem.asInt());
      }
#else
      input_vals.push_back(elem.asInt());
#endif
    }
    for (const Json::Value& entry: root["models"]) {
#if PERFORM_TYPE_CHECKING
      if (entry.isObject()) {
        for (auto itr = entry.begin(); itr != entry.end(); itr++) {
          if (itr.key().isString() && (*itr).isInt()) {
            std::string key = std::string(itr.key().asCString());
            int value = (*itr).asInt();
            models.push_back(std::make_pair(key, value));
          }
        }
      }
#else
      for (auto itr = entry.begin(); itr != entry.end(); itr++) {
        std::string key = std::string(itr.key().asCString());
        int value = (*itr).asInt();
        models.push_back(std::make_pair(key, value));
      }
#endif
    }
    end = std::clock();
    deserialize_time += (end - start) / (double) CLOCKS_PER_SEC;
#if PRINT_PARSED_JSON
    print_int_array(input_vals);
    print_pair_array(models);
#endif
  }
  parse_time /= (double) num_trials;
  deserialize_time /= (double) num_trials;

  return std::make_pair(parse_time, deserialize_time);
}

void run_benchmark() {
  int NUM_TRIALS = 10000;
  int NUM_MODELS = 2000;
  int INPUT_LEN = 2000;
  std::string json = generate_json_string("digits", NUM_MODELS, INPUT_LEN);

  std::pair<double, double> rapidjson_result =
    run_rapidjson_trial(json, NUM_TRIALS);
  std::cout << "Rapidjson parse time: " << rapidjson_result.first << "\n";
  std::cout << "Rapidjson deserialize time: " << rapidjson_result.second<< "\n";

  std::pair<double, double> jsoncpp_result =
    run_jsoncpp_trial(json, NUM_TRIALS);
  std::cout << "JsonCpp parse time: " << jsoncpp_result.first << "\n";
  std::cout << "JsonCpp deserialize time: " << jsoncpp_result.second<< "\n";
}

void run_basic() {
  std::string json = "{\"name\": \"digits\","
                      "\"input\": [1, 2, 3, 4, 5, 6, 7, 8, 9],"
                      "\"models\": [{\"m\": 1}, {\"m\": 2}, {\"n\": 1}]}";
  const char* json_str = json.c_str();

  /* jsoncpp */
  Json::Value root;
  Json::Reader reader;
  bool parse_success = reader.parse(json_str, root);
  if (!parse_success) {
    std::cout << "Failed to parse JSON\n"
              << reader.getFormattedErrorMessages();
  }
  std::cout << "name: " << root["name"].asString() << "\n";

  /* rapidjson */
  rapidjson::Document d;
  rapidjson::ParseResult ok = d.Parse(json.c_str());
  if (!ok) {
    std::cout << "JSON parse error: " << rapidjson::GetParseError_En(ok.Code())
              << " (offset " << ok.Offset() << ")\n";
  }
  rapidjson::Value& n = d["name"];
  std::cout << "name: " << n.GetString() << "\n";
}

int main() {
  run_basic();
  run_benchmark();

  return 0;
}
