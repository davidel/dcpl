#include "dcpl/args.h"

#include <cstring>
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
    const char* arg = argv[i];

    if (std::strcmp(arg, "--") == 0) {
      in_args = 0;
    } else if (in_args == 0) {
      unargs.push_back(std::string(arg));
    } else if (std::strncmp(arg, "--", 2) == 0) {
      in_args = 1;

      const char* name = arg + 2;
      auto ait = all_args.find(std::string_view(name));

      DCPL_ASSERT(ait != all_args.end()) << "Unknown flag: " << name;

      const flag* fvalue = ait->second;
      int eoa = i + 1;

      for (; eoa < argc && std::strncmp(argv[eoa], "--", 2) != 0; ++eoa) { }

      args.emplace(std::string(name), parse_range(argv, i + 1, eoa - i - 1, *fvalue));

      i = eoa - 1;
    } else if (in_args == -1) {
      unargs.push_back(std::string(arg));
    }
  }

  for (auto& it : all_args) {
    auto ait = args.find(std::string(it.first));

    if (ait == args.end()) {
      const flag* fvalue = it.second;

      if (fvalue->defval.has_value()) {
        args.emplace(std::string(fvalue->name), fvalue->defval);
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

std::any args::parse_range(char** argv, int pos, int count, const flag& fvalue) {
  if (fvalue.type == dcpl::type_name<bool_t>()) {
    if (std::strcmp(argv[pos], "true") == 0 ||
        std::strcmp(argv[pos], "1") == 0) {
      return true;
    } else if (std::strcmp(argv[pos], "false") == 0 ||
               std::strcmp(argv[pos], "0") == 0) {
      return false;
    } else {
      DCPL_THROW() << "Invalid value for " << fvalue.name << " flag : " << argv[pos];
    }
  } else if (fvalue.type == dcpl::type_name<boolv_t>()) {
    boolv_t values;

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

  } else if (fvalue.type == dcpl::type_name<int_t>()) {
    return dcpl::from_chars<int_t>(std::string_view(argv[pos]));
  } else if (fvalue.type == dcpl::type_name<intv_t>()) {
    intv_t values;

    for (int i = pos; i < pos + count; ++i) {
      values.push_back(dcpl::from_chars<int_t>(std::string_view(argv[i])));
    }
    return std::any(std::move(values));
  } else if (fvalue.type == dcpl::type_name<uint_t>()) {
    return dcpl::from_chars<uint_t>(std::string_view(argv[pos]));
  } else if (fvalue.type == dcpl::type_name<uintv_t>()) {
    uintv_t values;

    for (int i = pos; i < pos + count; ++i) {
      values.push_back(dcpl::from_chars<uint_t>(std::string_view(argv[i])));
    }
    return std::any(std::move(values));
  } else if (fvalue.type == dcpl::type_name<float_t>()) {
    return dcpl::from_chars<float_t>(std::string_view(argv[pos]));
  } else if (fvalue.type == dcpl::type_name<floatv_t>()) {
    floatv_t values;

    for (int i = pos; i < pos + count; ++i) {
      values.push_back(dcpl::from_chars<float_t>(std::string_view(argv[i])));
    }
    return std::any(std::move(values));
  } else if (fvalue.type == dcpl::type_name<double_t>()) {
    return dcpl::from_chars<double_t>(std::string_view(argv[pos]));
  } else if (fvalue.type == dcpl::type_name<doublev_t>()) {
    doublev_t values;

    for (int i = pos; i < pos + count; ++i) {
      values.push_back(dcpl::from_chars<double_t>(std::string_view(argv[i])));
    }
    return std::any(std::move(values));
  } else if (fvalue.type == dcpl::type_name<string_t>()) {
    return std::string(argv[pos]);
  } else if (fvalue.type == dcpl::type_name<stringv_t>()) {
    stringv_t values;

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

