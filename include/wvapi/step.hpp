#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <fstream>
#include <tuple>
#include <unordered_map>
#include <cstdint>
#include <string>
#include <sstream>
#include <chrono>

#include <boost/sml.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

namespace sml = boost::sml;

const std::string given_tab{"    "};

int glob_argc{};
char **glob_argv{};

using test_case_dict_um = std::vector<std::unordered_map<std::string, std::string>>;

using match_words_v = std::vector<std::string>;
using step_fp = bool(*)(match_words_v);
bool empty_step_fp([[maybe_unused]]match_words_v){
    return true;
}

std::unordered_map<std::string, void*> __state_object__{};
std::unordered_map<std::string, step_fp> __step_map__{}; 

#define MST0(s) uniq_##s
#define MST1(s) MST0(s)
#define MST(EXPR) static int MST1(__COUNTER__) = [](){EXPR; return 0;}();

#define STEP_MAKE_STATE(KEY, TYPE) __state_object__[KEY] = new TYPE
#define STEP_SET_STATE(KEY, TYPE) __state_object__[KEY] = TYPE

#define STEP_GET_STATE(KEY, TYPE) \
    (*static_cast<TYPE*>(__state_object__[KEY]));

#define STEP_CLEAN_STATE(KEY, TYPE) \
    (delete static_cast<TYPE*>(__state_object__[KEY]));

#define STEP_VALUES input_values

#define EXP_NME(s) uniq_##s
#define EXP_NM(s) EXP_NME(s)
#define MAKE_STATIC_EXP(FA, FB, TYPE, SENTENCE)\
bool FA(match_words_v);\
static int FB = [](){__step_map__[std::string(TYPE) + boost::to_lower_copy(std::string(SENTENCE))] = FA; return 0;}();\
bool FA(match_words_v input_values)

#define GIVEN(SENTENCE) MAKE_STATIC_EXP(EXP_NM(__COUNTER__),EXP_NM(__COUNTER__),"given ",SENTENCE)
#define THEN(SENTENCE) MAKE_STATIC_EXP(EXP_NM(__COUNTER__),EXP_NM(__COUNTER__),"then ",SENTENCE)
#define WHEN(SENTENCE) MAKE_STATIC_EXP(EXP_NM(__COUNTER__),EXP_NM(__COUNTER__),"when ",SENTENCE)

#define THROW_ERROR(MSG) \
std::cout << "\033[1;31m===========================\n" << std::string(MSG) << "\n"; return false;
#define EXPND(A) A
#define EXPECT_EQ(A, B) \
if (A != B) { std::cout << "\033[1;31m===========================\n" << "       Expected " << EXPND(A) <<" to equal " << EXPND(B)<< ", Failed" << "\n"; return false;  }

auto serialize_v = [](auto v){
    std::stringstream ss;
    ss << "{ ";
    for (auto _ : v) {
        ss << std::to_string(_) << ", ";
    }
    ss << " }";
    return ss.str();
};

#define EXPECT_EQ_V(A, B) \
if (A != B) { std::cout << "\033[1;31m===========================\n" << "       Expected " << serialize_v(A) <<" to equal " << serialize_v(B) << ", Failed" << "\n"; return false;  }

static constexpr auto help = R"(
==================================
==================================
OSDP Test Module

Input the feature file you want test like so:

./osdp.step --file=/path/to/my/hello.feature

Output will be log:

./hello.feature.log
==================================
==================================
)";

enum parser_status_e : std::uint16_t {
    success = 0,
    lookup_error = 1,
    syntax_error = 2,
    missing_file = 3,
    test_fail = 4,
    no_feature_file_found = 5
};

const boost::regex example_row_r("^\\|(.+)\\|$", boost::regex::perl | boost::regex::icase);
const boost::regex step_parser_r("^(when|then|given|and).*", boost::regex::perl|boost::regex::icase);
const boost::regex brace_parser_r("\\{(.+?)\\}", boost::regex::perl|boost::regex::icase);
const auto w_end = boost::sregex_iterator();

auto extract_words = [](const std::string& input, const auto& re){
    match_words_v matches{};
    auto w_begin = boost::sregex_iterator(input.begin(), input.end(), re);
    auto w_end = boost::sregex_iterator();
    for (auto it = w_begin; it != w_end; ++it) {
        auto s = (*it).str();
        matches.push_back(s.substr(1, s.length() - 2));
    }
    return matches;
};

auto extract_table_row = [](const std::string& input){
    std::vector<std::string> matches{};
    auto nstring = boost::trim_copy(input);
    boost::split(matches, nstring.substr(1, nstring.length() - 2), boost::is_any_of("|"));
    for(auto& _ : matches) boost::trim(_);
    return matches;
};

struct step_s {
  std::string description{};
  step_fp step{empty_step_fp};
  match_words_v values{};
};
struct scenario_s {
  std::uint32_t index{};
  std::string description{};
  std::vector<step_s> steps{};
  test_case_dict_um test_cases{};
  match_words_v test_var_to_key{};
};
struct feature_s {
  std::uint32_t index{};
  std::string description{};
  std::vector<scenario_s> scenarios{};
};
struct feature_machine_s {
  std::uint32_t line_count{};
  std::vector<feature_s> features{};
};


using lookup_step_t = std::tuple<parser_status_e, match_words_v, step_fp, std::string>;
lookup_step_t lookup_step(const std::string& entry) {
    const std::string empty{"{}"};
    // Essentially we have some string but we need to map the string to a provided function
    // we will save the lower case version of the strings to some map
    auto entry_f = boost::to_lower_copy(entry);
    auto key = boost::regex_replace(entry_f, brace_parser_r, empty);
    if (auto search = __step_map__.find(key); search != __step_map__.end()) {
        return std::make_tuple<>(
            parser_status_e::success,
            extract_words(entry,brace_parser_r),
            search->second,
            entry
        );
    }

    std::cout << "Could not find the step!" << std::endl;
    std::cout << "Your step post pre-process:\n" << key << std::endl;
    std::cout << "Available steps:\n";
    for (auto it = __step_map__.begin(); it != __step_map__.end(); ++it) {
        std::cout << it->first << std::endl;
    }

    return std::make_tuple<>(
        parser_status_e::lookup_error,
        match_words_v{},
        empty_step_fp,
        "Error: step not implemented"
    );
}

struct found_feature_e {
    std::string value;
};
struct found_scenario_e {
    std::string value;
};
struct found_step_e {
    match_words_v values;
    step_fp fxn;
    std::string description;
};
struct found_line_e {
    std::string value;
};
struct found_examples_e {};
struct found_example_line_e {
    match_words_v values;
};
struct found_eof_e {};

std::string prepend_line_description(feature_machine_s& feature_machine, const std::string& value){
    return std::to_string(++feature_machine.line_count) + std::string(": ") + value + std::string("\n");
}

auto append_description = [](const found_line_e& event, feature_machine_s& feature_machine){

};
auto add_feature = [](const auto& event, feature_machine_s& feature_machine){
    feature_machine.features.push_back({.description = prepend_line_description(feature_machine, "Feature: " + event.value)});
};
auto append_description_feature = [](const found_line_e& event, feature_machine_s& feature_machine){
    feature_machine.features.back().description += prepend_line_description(feature_machine, event.value);
};
auto add_scenario = [](const auto& event, feature_machine_s& feature_machine){
    feature_machine.
        features.back().
        scenarios.push_back({
            .description = prepend_line_description(feature_machine, "  Scenario: " + event.value)
        });
};
auto append_description_scenario = [](const auto& event, feature_machine_s& feature_machine){
    feature_machine.features.back().description += prepend_line_description(feature_machine, event.value);
};
auto add_step = [](const auto& event, feature_machine_s& feature_machine){
    feature_machine.
        features.back().
        scenarios.back().
        steps.push_back({
            .description = prepend_line_description(feature_machine, given_tab + event.description),
            .step = event.fxn,
            .values = event.values
        });
};
auto add_header = [](const auto& event, feature_machine_s& feature_machine) {
    feature_machine.
        features.back().
        scenarios.back().
        test_var_to_key = event.values;
};
auto add_test_case = [](const auto& event, feature_machine_s& feature_machine) {
    auto& scenario = feature_machine.
        features.back().
        scenarios.back();

    std::unordered_map<std::string, std::string> map{};
    for (auto idx = 0; idx < event.values.size(); ++idx) {
        // Match up column to variable name
        map[scenario.test_var_to_key[idx]] = event.values[idx];
    }
    
    scenario.test_cases.push_back(map);
};
auto commit_feature = [](const auto& event, feature_machine_s& feature_machine){
};
auto commit_scenario = [](const auto& event, feature_machine_s& feature_machine){};
auto commit_step = [](const auto& event, feature_machine_s& feature_machine){};
auto error = [](const auto& event, feature_machine_s& feature_machine){};

struct parse_features {
    auto operator()() const {
        using namespace sml;
        return make_transition_table(
            *"init"_s + event<found_feature_e> / add_feature = "parsing_feature"_s,
            "parsing_feature"_s + event<found_scenario_e> / add_scenario = "parsing_scenario"_s,
            "parsing_feature"_s + event<found_line_e> / append_description_feature = "parsing_scenario"_s,
            "parsing_feature"_s + event<found_examples_e> / error = X,
            "parsing_feature"_s + event<found_step_e> / error = X,
            "parsing_feature"_s + event<found_eof_e> = X,

            "parsing_scenario"_s + event<found_feature_e> / add_feature = "parsing_feature"_s,
            "parsing_scenario"_s + event<found_scenario_e> / add_scenario = "parsing_scenario"_s,
            "parsing_scenario"_s + event<found_line_e> / append_description = "parsing_scenario"_s,
            "parsing_scenario"_s + event<found_step_e> / add_step = "parsing_scenario"_s,
            "parsing_scenario"_s + event<found_examples_e> = "parsing_examples_header"_s,
            "parsing_scenario"_s + event<found_eof_e> = X,

            "parsing_examples_header"_s + event<found_example_line_e> / add_header = "parsing_examples_body"_s,
            "parsing_examples_header"_s + event<found_line_e> = "parsing_examples_header"_s,
            "parsing_examples_header"_s + event<found_scenario_e> = "parsing_scenario"_s,
            "parsing_examples_header"_s + event<found_feature_e> = "parsing_feature"_s,
            "parsing_examples_header"_s + event<found_eof_e> = X,

            "parsing_examples_body"_s + event<found_example_line_e> / add_test_case = "parsing_examples_body"_s,
            "parsing_examples_body"_s + event<found_line_e> = "parsing_examples_body"_s,
            "parsing_examples_body"_s + event<found_feature_e> / add_feature = "parsing_features"_s,
            "parsing_examples_body"_s + event<found_scenario_e> / add_scenario = "parsing_scenario"_s,
            "parsing_examples_body"_s + event<found_eof_e> = X
        );
    }
};

parser_status_e parse(auto& sm, const std::string& line) {
    using namespace sml;
    boost::regex feature_scenario_r ("^(\\s|\\t)*(feature|scenario)\\s*:(.*)", boost::regex::perl | boost::regex::icase);
    
    std::vector<std::string> tokens_co{};
    std::vector<std::string> tokens{};
    auto entry = boost::trim_copy(line);
    entry = boost::split(tokens_co, entry, boost::is_any_of("#"))[0];
    entry = boost::trim_copy(entry);
    if (boost::regex_match(entry, example_row_r)) {
        if (sm.is("parsing_examples_header"_s) || sm.is("parsing_examples_body"_s)) {
            sm.process_event(found_example_line_e{.values = extract_table_row(entry)});
            return parser_status_e::success;
        }
    }

    boost::regex_split(std::back_inserter(tokens), entry, feature_scenario_r);
    if (tokens.size() == 0) {
        if (boost::regex_match(entry, step_parser_r) && sm.is("parsing_scenario"_s)) {
            auto [status, values, fxn, description] = lookup_step(entry);
            if (parser_status_e::lookup_error == status) {
                return parser_status_e::lookup_error;
            }
            // We found a step which means we must map the expression to a step
            // function and extract the variables
            sm.process_event(found_step_e{.values = values, .fxn = fxn, .description = description});
            return parser_status_e::success;
        }
        sm.process_event(found_line_e{.value = entry});
        return parser_status_e::success;
    }

    auto first = tokens[1];
    auto second = tokens[2];
    boost::trim(first);
    boost::to_lower(first);
    boost::trim(second);

    if (first == "feature") {
        sm.process_event(
            found_feature_e {
                .value = second
            }
        );
        return parser_status_e::success;
    }
    if (first == "scenario") {
        sm.process_event(
            found_scenario_e {
                .value = second
            }
        );
        return parser_status_e::success;
    }
    
    if (first == "examples") {
        sm.process_event(found_examples_e{});
        return parser_status_e::success;
    }
    
    return parser_status_e::success;
}

bool run_step(const auto& step, const auto& description, const auto& values) {
    bool status = step.step(values);
    if (status) {
        std::cout << "\033[0;32m" << description << "\033[0;37m";
    } else {
        std::cout << "\033[0;31m" << description << "===========================\n\033[0;37m";
    }
    return status;
} 

bool run_feature_machine(const feature_machine_s& feature_machine) {
    std::cout << "Running feature machine" << std::endl;
    for (const auto& feature : feature_machine.features) {
        std::cout << feature.description;
        for (const auto& scenario : feature.scenarios) {
            // Clear out the state object before every scenario
            __state_object__.clear();
            std::cout << scenario.description;
            std::cout << "We've got these test cases " << scenario.test_cases.size() << std::endl;
            if (scenario.test_cases.size() > 0) {
                std::uint32_t count = 0;
                for (auto map : scenario.test_cases) {
                    std::cout << "Running test cases with parameter set: " << (++count) << std::endl;
                    for (const auto& step : scenario.steps) {
                        match_words_v svalues{};
                        std::string description = step.description;
                        for(auto v : step.values) {
                            svalues.push_back(map[v]);
                            description = boost::replace_all_copy(description, v, map[v]);
                        }
                        
                        if(!run_step(step, description, svalues)) return false;
                    }
                }
                // Skip the rest of this scenario
                continue;
            }
            //
            for (const auto& step : scenario.steps) {
                if(!run_step(step, step.description, step.values)) return false;
            }
        }
    }
    return true;
}

std::tuple<parser_status_e, std::string> step_runner() {
  std::vector<std::string> args(glob_argv + 1, glob_argv + glob_argc);
  std::string file{};
  using namespace sml;
  feature_machine_s feature_machine{};
  sm<parse_features> sm(feature_machine);
  for (auto entry : args) {
      if ("--help" == entry) {
          std::cout << help;
          return std::make_tuple<>(parser_status_e::success, "");
      }
      if (entry.find("--file=") != std::string::npos) {
          file = entry.substr(7, std::string::npos);
          std::cout << "Reading file... " << file << std::endl;
      }
  }

  if (file.empty()) {
      return std::make_tuple<>(parser_status_e::missing_file, "Please enter in a file. ./osdp.step --step-help for help.");
  }

  // Open up the file and start reading
  std::ifstream fp(file);
  std::string line{};
  std::uint32_t count{};
  if (!fp.is_open()) {
    return std::make_tuple<>(parser_status_e::no_feature_file_found, std::string("Could not find file: ") + file);;
  }
  while (std::getline(fp, line)) {
    std::cout << line << std::endl;
      // Feature Not Started
      // Feature start, Fetch Feature Description, Scenario Not Started, Fetch Scenario Description Stop, Step Stop
      // Feature stop, Fetch Feature Description Stop, Scenario Started, Fetch Scenario Description, Step Stop
      // Feature stop, Fetch Feature Description Stop, Scenario Started, Fetch Scenario Description Stop, Step Started, Fetch Step Description Start
      // Either EOF, Start of New Step, Start of New Scenario => compile step
      //  -- Append step to current scenario
      //  -- auto&& up = extract_words(line, brace_parser_r);
      std::string msg{};
      switch(parse(sm, line)) {
          case parser_status_e::success:
              break;
          case parser_status_e::lookup_error:
              msg = "Step function not implemented\n"
                                + std::to_string(++count)
                                + std::string(": ")
                                + line;
              return std::make_tuple<>(parser_status_e::lookup_error, msg);
          case parser_status_e::syntax_error:
              msg = "Syntax error\n"
                                + std::to_string(++count)
                                + std::string(": ")
                                + line;
              return std::make_tuple<>(parser_status_e::syntax_error, msg);
      }
  }
  auto run_status = run_feature_machine(feature_machine);
  if (!run_status) {
    return std::make_tuple<>(parser_status_e::test_fail, "Test failed, please checkout your errors. \nMaker sure to return bool status in your step functions if you are making steps.\n");
  }
  return std::make_tuple<>(parser_status_e::success,"");
}

// helper functions
template<class T>
auto extract_hex_array = [](const auto& input){
    std::vector<std::string> array_nos_str{};
    std::vector<T> array_values{};
    boost::split(array_nos_str, input, boost::is_any_of(","));
    for (auto no : array_nos_str) {
        int value;
        std::stringstream ss;
        ss << std::hex << std::string(boost::trim_copy(no));
        ss >> value;

        array_values.push_back((T)value);
    }
    return array_values;
};

template<class T>
auto extract_hex = [](const auto& input){
    std::vector<std::string> array_nos_str{};
    std::vector<T> array_values{};
    int value;
    std::stringstream ss;
    ss << std::hex << std::string(boost::trim_copy(input));
    ss >> value;
    return (T)value;
};

THEN("I wait for {} seconds") {
    auto seconds = atoi(STEP_VALUES[0].c_str());
    
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    while(std::chrono::duration_cast<std::chrono::seconds>(now - start).count() < seconds) {
        now = std::chrono::steady_clock::now();
    }
    return true;
}

int main(int argc, char **argv) {
    glob_argc = argc;
    glob_argv = argv;
    std::cout << "version - 0.0.0" << std::endl;
    auto[status, message] = step_runner();
    if (status == parser_status_e::success) {
        return 0;
    }
    std::cerr << message;
    return 1;
}