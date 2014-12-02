#include <cpprest/http_client.h>
#include <cpprest/http_listener.h>
#include <gflags/gflags.h>
#include <iostream>
#include <memory>
#include <signal.h>
#include <unistd.h>

using web::http::oauth2::experimental::oauth2_config;

DEFINE_string(oauth_client_id, "", "Client ID for Oauth Authentication");
DEFINE_string(oauth_client_secret, "", "Client secret for OAuth Authentication");

int main(int argc, char **argv) {
  utility::string_t redirect_uri(U("urn:ietf:wg:oauth:2.0:oob"));
  oauth2_config m_oauth2_config(
      U(FLAGS_oauth_client_id),                         // Client ID
      U(FLAGS_oauth_client_secret),                     // Client Secret
      U("https://accounts.google.com/o/oauth2/auth"),   // OAuth2.0 endpoint
      U("https://accounts.google.com/o/oauth2/token"),  // Token endpoint
      redirect_uri);
  m_oauth2_config.set_http_basic_auth(false);
  m_oauth2_config.set_scope(
      U("https://www.googleapis.com/auth/drive.readonly"));
  auto auth_uri = m_oauth2_config.build_authorization_uri(false);
  std::cout << auth_uri << std::endl;
  std::cout << "Please paste the code from the login page:";
  std::string code;
  std::getline(std::cin, code);

  std::vector<char *> argv_vec(argv, argv + argc);
  argv_vec.push_back(
      const_cast<char *>((std::string("-access_code=") + code).c_str()));
  std::unique_ptr<char *> argv_new(new char *[argv_vec.size() + 1]);
  std::copy(argv_vec.begin(), argv_vec.end(), argv_new.get());
  argv_new.get()[argv_vec.size()] = nullptr;

  daemon(1, 0);
  execve("./main", argv_new.get(), nullptr);
}
