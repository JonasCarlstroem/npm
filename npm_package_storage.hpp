#pragma once
#include <string>
#include <fstream>
#include <filesystem>
//#include <nlohmann/json>

using json = nlohmann::json;
using string = std::string;
using namespace std::filesystem;

namespace npm {

template <class... T> path combine_path(const T &...t) {
    path p = (path(t), ...);
    return p;
}

class package_storage {
  public:
    static constexpr const char *base_path = "registry_data";

    template <class... T> static path get_path(const T &...t) {
        return (path(base_path) / (t, ...));
    }

    static string get_tarball_filename(string name, string version) {
        return name + "-" + version + ".tgz";
    }

    static path metadata_path(const string &package) {
        return (path)base_path / package / "metadata.json";
        // return std::string(base_path) + "/" + package + "/metadata.json"
    }

    static bool package_exists(const string &package) {
        return ::exists(metadata_path(package));
    }

    static json load_metadata(const string &package) {
        std::ifstream in(metadata_path(package));
        if (!in)
            return json();

        json j;
        in >> j;
        return j;
    }

    static void save_metadata(const string &package, const json &j) {
        ::create_directories(combine_path(base_path, package));
        std::ofstream out(metadata_path(package));
        out << j.dump(2);
    }
};

} // namespace npm