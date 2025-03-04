#include "dcpl/args.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string_view>

#include "dcpl/assert.h"

namespace dcpl {

std::vector<args::flag> args::g_global_flags;

args::parsed args::parse(int argc, char** argv) const {
  std::map<std::string, std::any> args;
  std::vector<std::string> unargs;

  std::map<std::string_view, const flag*> all_args = collect_args();
  int in_args = -1;

  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);

    if (arg == "--") {
      in_args = 0;
    } else if (in_args == 0) {
      unargs.push_back(std::move(arg));
    } else if (arg == "--help") {
      show_help(argv[0]);
      std::exit(1);
    } else if (arg.substr(0, 2) == "--") {
      in_args = 1;

      std::string name = arg.substr(2);

      if (name.substr(0, 3) == "no-") {
        std::string bname = name.substr(3);
        auto ait = all_args.find(bname);

        if (ait != all_args.end() && ait->second->type == args_detail::type_spec<bool_t>()) {
          args.emplace(std::move(bname), false);
          continue;
        }
      }

      auto ait = all_args.find(name);

      DCPL_ASSERT(ait != all_args.end()) << "Unknown flag: " << name;

      const flag* fvalue = ait->second;
      int eoa = i + 1;

      for (; eoa < argc && std::strncmp(argv[eoa], "--", 2) != 0; ++eoa) { }

      args.emplace(std::move(name), parse_range(argv, i + 1, eoa - i - 1, *fvalue));

      i = eoa - 1;
    } else if (in_args == -1) {
      unargs.push_back(std::move(arg));
    }
  }

  for (auto& it : all_args) {
    auto ait = args.find(std::string(it.first));

    if (ait == args.end()) {
      const flag* fvalue = it.second;

      if (fvalue->defval.has_value()) {
        args.emplace(std::string(fvalue->name), fvalue->defval);
      } else {
        std::cerr << "Missing required flag: " << it.first << "\n\n";

        show_help(argv[0]);
        std::exit(1);
      }
    }
  }

  return parsed(std::move(args), std::move(unargs));
}

std::map<std::string_view, const args::flag*> args::collect_args() const {
  std::map<std::string_view, const flag*> all_args;

  for (auto& fvalue : g_global_flags) {
    all_args.emplace(fvalue.name, &fvalue);
  }
  for (auto& fvalue : flags_) {
    all_args.emplace(fvalue.name, &fvalue);
  }

  return std::move(all_args);
}

void args::show_help(const char* prog_name) const {
  std::cerr << "Usage: " << prog_name << "\n\n";

  auto show_arg = [](const auto& fvalue) {
    const char* argn = fvalue.type.starts_with("vector<") ? "ARG..." : "ARG";

    std::cerr << "  --" << fvalue.name << "\t" << argn << "\t\t"
              << fvalue.help << " (" << fvalue.type << ")";

    if (fvalue.defval.has_value()) {
      if (fvalue.type == args_detail::type_spec<bool_t>()) {
        std::cerr << "  [default = " << std::any_cast<bool_t>(fvalue.defval) << "]";
      } else if (fvalue.type == args_detail::type_spec<boolv_t>()) {
        std::cerr << "  [default = " << std::any_cast<boolv_t>(fvalue.defval) << "]";
      } else if (fvalue.type == args_detail::type_spec<int_t>()) {
        std::cerr << "  [default = " << std::any_cast<int_t>(fvalue.defval) << "]";
      } else if (fvalue.type == args_detail::type_spec<intv_t>()) {
        std::cerr << "  [default = " << std::any_cast<intv_t>(fvalue.defval) << "]";
      } else if (fvalue.type == args_detail::type_spec<uint_t>()) {
        std::cerr << "  [default = " << std::any_cast<uint_t>(fvalue.defval) << "]";
      } else if (fvalue.type == args_detail::type_spec<uintv_t>()) {
        std::cerr << "  [default = " << std::any_cast<uintv_t>(fvalue.defval) << "]";
      } else if (fvalue.type == args_detail::type_spec<float_t>()) {
        std::cerr << "  [default = " << std::any_cast<float_t>(fvalue.defval) << "]";
      } else if (fvalue.type == args_detail::type_spec<floatv_t>()) {
        std::cerr << "  [default = " << std::any_cast<floatv_t>(fvalue.defval) << "]";
      } else if (fvalue.type == args_detail::type_spec<string_t>()) {
        std::cerr << "  [default = " << std::any_cast<string_t>(fvalue.defval) << "]";
      } else if (fvalue.type == args_detail::type_spec<stringv_t>()) {
        std::cerr << "  [default = " << std::any_cast<stringv_t>(fvalue.defval) << "]";
      }
    }

    std::cerr << "\n";
  };

  for (const auto& fvalue : g_global_flags) {
    show_arg(fvalue);
  }
  for (const auto& fvalue : flags_) {
    show_arg(fvalue);
  }
}

std::any args::parse_range(char** argv, int pos, int count, const flag& fvalue) {
  if (fvalue.type == args_detail::type_spec<bool_t>()) {
    if (count == 0) {
      return true;
    }

    DCPL_CHECK_EQ(count, 1) << "Wrong number of arguments (" << count
                            << ") for flag: " << fvalue.name;
    if (std::strcmp(argv[pos], "true") == 0 ||
        std::strcmp(argv[pos], "1") == 0) {
      return true;
    } else if (std::strcmp(argv[pos], "false") == 0 ||
               std::strcmp(argv[pos], "0") == 0) {
      return false;
    } else {
      DCPL_THROW() << "Invalid value for " << fvalue.name << " flag : " << argv[pos];
    }
  } else if (fvalue.type == args_detail::type_spec<boolv_t>()) {
    boolv_t values;

    DCPL_CHECK_GE(count, 1) << "Wrong number of arguments (" << count
                            << ") for flag: " << fvalue.name;
    for (int i = pos; i < pos + count; ++i) {
      if (std::strcmp(argv[i], "true") == 0 ||
          std::strcmp(argv[i], "1") == 0) {
        values.push_back(true);
      } else if (std::strcmp(argv[i], "false") == 0 ||
                 std::strcmp(argv[i], "0") == 0) {
        values.push_back(false);
      } else {
        DCPL_THROW() << "Invalid value for " << fvalue.name << " flag : " << argv[i];
      }
    }
    return std::any(std::move(values));

  } else if (fvalue.type == args_detail::type_spec<int_t>()) {
    DCPL_CHECK_EQ(count, 1) << "Wrong number of arguments (" << count
                            << ") for flag: " << fvalue.name;

    return to_number<int_t>(std::string_view(argv[pos]));
  } else if (fvalue.type == args_detail::type_spec<intv_t>()) {
    intv_t values;

    DCPL_CHECK_GE(count, 1) << "Wrong number of arguments (" << count
                            << ") for flag: " << fvalue.name;
    for (int i = pos; i < pos + count; ++i) {
      values.push_back(to_number<int_t>(std::string_view(argv[i])));
    }
    return std::any(std::move(values));
  } else if (fvalue.type == args_detail::type_spec<uint_t>()) {
    DCPL_CHECK_EQ(count, 1) << "Wrong number of arguments (" << count
                            << ") for flag: " << fvalue.name;

    return to_number<uint_t>(std::string_view(argv[pos]));
  } else if (fvalue.type == args_detail::type_spec<uintv_t>()) {
    uintv_t values;

    DCPL_CHECK_GE(count, 1) << "Wrong number of arguments (" << count
                            << ") for flag: " << fvalue.name;
    for (int i = pos; i < pos + count; ++i) {
      values.push_back(to_number<uint_t>(std::string_view(argv[i])));
    }
    return std::any(std::move(values));
  } else if (fvalue.type == args_detail::type_spec<float_t>()) {
    DCPL_CHECK_EQ(count, 1) << "Wrong number of arguments (" << count
                            << ") for flag: " << fvalue.name;

    return to_number<float_t>(std::string_view(argv[pos]));
  } else if (fvalue.type == args_detail::type_spec<floatv_t>()) {
    floatv_t values;

    DCPL_CHECK_GE(count, 1) << "Wrong number of arguments (" << count
                            << ") for flag: " << fvalue.name;
    for (int i = pos; i < pos + count; ++i) {
      values.push_back(to_number<float_t>(std::string_view(argv[i])));
    }
    return std::any(std::move(values));
  } else if (fvalue.type == args_detail::type_spec<string_t>()) {
    DCPL_CHECK_EQ(count, 1) << "Wrong number of arguments (" << count
                            << ") for flag: " << fvalue.name;

    return std::string(argv[pos]);
  } else if (fvalue.type == args_detail::type_spec<stringv_t>()) {
    stringv_t values;

    DCPL_CHECK_GE(count, 1) << "Wrong number of arguments (" << count
                            << ") for flag: " << fvalue.name;
    for (int i = pos; i < pos + count; ++i) {
      values.push_back(std::string(argv[i]));
    }
    return std::any(std::move(values));
  } else {
    DCPL_THROW() << "Invalid type for " << fvalue.name << " flag : "
                 << fvalue.type;
  }

  return {};
}

}

