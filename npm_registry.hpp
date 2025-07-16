#pragma once
#include "npm_package_storage.hpp"
#include <filesystem>
#include <net/server/http>
#include <utils/net>

using namespace std::filesystem;
using namespace net;

namespace npm {

class registry {
    friend class registry_service;

    http::server server;

    static path tarball_full_path(const string& name, const string& version) {
        return path(name + "-" + version + ".tgz");
    }

    static string
    tarball_url(const string& host, const string& name, const string& file) {
        return (path("http:/") / host / name / "-" / file).string();
    }

  public:
    registry(const string& ip = "127.0.0.1", int port = 8080)
        : server(ip, port) {
        server.get("/:name", get_package_metadata);
        server.get("/:name/-/:file", get_package_tarball);
        server.get("/-/v1/search", get_search);
        server.get("/-/all", get_all);
        server.post("/-/v1/login", post_login);
        server.put("/:name", put_package);

        server.non_blocking() = true;
    }

    void start_and_wait() {
        server.start();
        server.wait();
    }

    void start() { server.start(); }

    void stop() { server.stop(); }

    static http::response get_package_metadata(const http::request& req) {
        string name = net::url_decode(req.params.at("name"));

        if (!package_storage::package_exists(name))
            return http::response::not_found();

        try {
            auto meta   = package_storage::load_metadata(name);

            string host = req.headers.count("host") ? req.headers.at("host")
                                                    : "localhost:8080";
            for (auto& [version, vmeta] : meta["versions"].items()) {
                string file = tarball_full_path(name, version).string();
                vmeta["dist"]["tarball"] = tarball_url(host, name, file);
            }

            return http::response::json(meta);
        } catch (const std::exception& ex) {
            return http::response::error(ex.what());
        }
    }

    static http::response get_package_tarball(const http::request& req) {
        string name = req.params.at("name");
        string file = req.params.at("file");

        path p      = package_storage::get_path(name, file);
        if (!::exists(p)) {
            return http::response::not_found("Tarball not found");
        }

        std::ifstream in(p, std::ios::binary);
        std::ostringstream buf;
        buf << in.rdbuf();

        http::response res = http::response::ok();
        res.set_binary(list<char>(buf.str().begin(), buf.str().end()));

        return res;
    }

    static http::response get_packages(const http::request& req) {
        json list = json::array();

        for (const auto& entry :
             ::directory_iterator(package_storage::base_path)) {
            if (entry.is_directory()) {
                list.push_back(entry.path().filename().string());
            }
        }
    }

    static http::response get_search(const http::request& req) {
        string text;

        if (req.query_params.count("text")) {
            text = req.query_params.at("text");
        }

        json results;
        results["objects"] = json::array();

        for (const auto& entry :
             std::filesystem::directory_iterator(package_storage::base_path)) {
            if (!entry.is_directory())
                continue;

            string name = entry.path().filename().string();
            if (!text.empty() && name.find(text) == string::npos)
                continue;

            try {
                auto meta         = package_storage::load_metadata(name);
                string latest     = meta["dist-tags"]["latest"];
                auto version_info = meta["versions"][latest];

                json obj;
                json package;
                package["name"]        = name;
                package["version"]     = latest;
                package["description"] = version_info.value("description", "");
                package["keywords"] =
                    version_info.value("keywords", json::array());

                obj["package"]        = package;
                obj["score"]["final"] = 1.0;
                results["objects"].push_back(obj);
            } catch (...) {
            }
        }

        results["total"] = results["objects"].size();
        return http::response::json(results);
    }

    static http::response get_all(const http::request& req) {
        json result = json::object();

        for (auto& entry : directory_iterator(package_storage::base_path)) {
            if (!entry.is_directory())
                continue;
            string name = entry.path().filename().string();

            try {
                result[name] = package_storage::load_metadata(name);
            } catch (...) {
                result[name] = "Error loading metadata";
            }
        }

        return http::response::json(result);
    }

    static http::response post_login(const http::request& req) {
        auto body       = json(req.get_body_as_string());
        string username = body["name"];
        string password = body["password"];

        json token_data = {{"token", "gkdfgldkf"}};

        return http::response::json(token_data);
    }

    static http::response put_package(const http::request& req) {
        string name = net::url_decode(req.params.at("name"));

        try {
            auto js                  = json::parse(req.body);

            string version           = js["versions"].begin().key();
            const auto& version_meta = js["versions"].begin().value();

            path file_name           = tarball_full_path(name, version);
            auto attachments         = js["_attachments"];
            auto tarball_info        = attachments.at(file_name.string());
            string b64               = tarball_info.at("data");

            string decoded_tarball   = base64_decode(b64);

            path dir                 = package_storage::get_path(name);
            ::create_directories(dir);

            std::ofstream tarout(
                path(dir) / file_name.filename(), std::ios::binary
            );
            tarout.write(decoded_tarball.data(), decoded_tarball.size());

            json meta;
            path meta_path = package_storage::metadata_path(name);

            if (::exists(meta_path)) {
                std::ifstream in(meta_path);
                in >> meta;
            } else {
                meta["name"]     = name;
                meta["versions"] = json::object();
            }

            meta["versions"][version]   = version_meta;
            meta["dist-tags"]["latest"] = version;

            package_storage::save_metadata(name, meta);

            return http::response::ok("Package published");
        } catch (const std::exception& ex) {
            return http::response::error(ex.what());
        }
    }

    static http::response put_package_tarball(const http::request& req) {
        string name = req.params.at("name");
        string file = req.params.at("file");

        path dir    = package_storage::get_path(name);
        ::create_directories(dir);

        std::ofstream out(dir / file, std::ios::binary);
        out.write(req.body.data(), req.body.size());

        return http::response::ok("Tarball uploaded");
    }
};

} // namespace npm