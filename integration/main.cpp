#include <cpprest/http_client.h>
#include <cpprest/http_listener.h>
#include <fusekit/daemon.h>
#include <fusekit/stream_callback_file.h>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <stdexcept>
#include <string>

using namespace web::http;
using namespace web::http::client;
using namespace web::http::oauth2::experimental;
using web::http::experimental::listener::http_listener;

DEFINE_string(access_code, "", "OAuth2 access code");
DEFINE_string(oauth_client_secret, "", "Client secret for OAuth authentication");
DEFINE_string(oauth_client_id, "", "Client ID for OAuth authentication");

fusekit::daemon<>& fusekit_daemon = fusekit::daemon<>::instance();

int gdrive(std::string url,
           web::http::client::http_client_config config, std::ostream& os) {
  web::http::client::http_client client(url, config);
  client.request(web::http::methods::GET, U(""))
      .then([](pplx::task<web::http::http_response> task) {
        auto result = task.get();
        if (result.status_code() != 200) {
          LOG(ERROR) << result.reason_phrase();
          throw std::runtime_error("");
        } else {
          return result.extract_vector();
        }
      })
      .then([&](pplx::task<std::vector<unsigned char>> task) {
        auto result = task.get();
        for (const auto& x : result)
          os << x;
        os << std::flush;
      }).wait();
  return 0;
}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  utility::string_t redirect_uri(U("urn:ietf:wg:oauth:2.0:oob"));
  oauth2_config m_oauth2_config(
      U(FLAGS_oauth_client_id),                        // Client ID
      U(FLAGS_oauth_client_secret),                    // Client Secret
      U("https://accounts.google.com/o/oauth2/auth"),  // OAuth2.0 endpoint
      U("https://accounts.google.com/o/oauth2/token"), // Token endpoint
      redirect_uri);
  m_oauth2_config.set_http_basic_auth(false);
  m_oauth2_config.set_scope(
      U("https://www.googleapis.com/auth/drive.readonly"));
  m_oauth2_config.token_from_code(FLAGS_access_code).wait();
  web::http::client::http_client_config client_config;
  client_config.set_oauth2(m_oauth2_config);
  web::http::client::http_client client(U("https://www.googleapis.com"), client_config);

  std::string request_str = std::string("drive/v2/files");

  client.request(web::http::methods::GET, request_str)
      .then([&](pplx::task<web::http::http_response> task) {
        web::http::http_response response = task.get();
        if (response.status_code() != 200) {
          LOG(ERROR) << "HTTP status code " << response.status_code();
          throw std::runtime_error("");
        } else {
          return response.extract_json();
        }
      })
      .then([=](web::json::value value) {
        if (!value.has_field("items")) {
          LOG(ERROR) << "No items returned!";
        } else {
          auto items = value.at("items").as_array();
          for (const auto& item : items) {
            if (!item.has_field("title")) continue;
            if (!item.has_field("downloadUrl")) continue;
            std::string acc;
            for (const auto& x : item.at("title").as_string()) {
              if (x == '/') {
                acc += "%2F";
              } else {
                acc += x;
              }
            }
            std::function<int(std::ostream&)> fn =
                std::bind(gdrive, item.at("downloadUrl").as_string(),
                          client_config, std::placeholders::_1);
            auto q = fusekit_daemon.root().add_file(
                acc.c_str(), fusekit::make_ostream_callback_file(fn));
          }
        }
      })
      .wait();

  std::vector<char*> argv_vec(argv, argv+argc);
  argv_vec.push_back(const_cast<char*>("-d"));

  return fusekit_daemon.run(argv_vec.size(), &argv_vec[0], false);
}
