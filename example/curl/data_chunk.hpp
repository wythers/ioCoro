#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <stdio.h>
#include <sys/stat.h>

using std::string;
using std::vector;
using std::string_view;
using namespace std::string_view_literals;

struct DataChunk
{
        string Host{};
        string host{};
        string resrc_name{};
        string file_name{};

        vector<char> resrc{};
        char headers[4096]{};

        DataChunk(char const* url)
        {
                string_view checker{url};
                auto pos = checker.find("://");
                if (pos != string_view::npos)
                        checker.remove_prefix(pos + 3);
                pos = checker.find("/");
                if (pos == string_view::npos)
                {
                        Host = {checker.data()};
                        host = Host + ":80";
                        resrc_name = "/";
                        file_name = "null.html";
                } else {
                        Host = {checker.data(), pos};
                        host = Host + ":80";
                        checker.remove_prefix(pos);
                        resrc_name = {checker.data()};
                        pos = checker.find_last_of("/");
                        checker.remove_prefix(pos + 1);
                        file_name = {checker.data()};
                }
        }
};

bool inline CHECKINGERROR(DataChunk& chunk)
{
        string_view checker{chunk.headers};

        auto pos = checker.find(" ");
        string_view version{checker.substr(0, pos)};
        checker.remove_prefix(pos + 1);
        pos = checker.find(" ");
        string_view status_code{checker.substr(0, pos)};
        checker.remove_prefix(pos + 1);
        pos = checker.find("\r\n");
        string_view status_message{checker.substr(0, pos)};
        
        if 
        (
                version != "HTTP/1.1"sv ||
                status_code != "200"sv
        )
        {
                printf("the Server response failed, ERROR CODE:%s, ERROR MESSAGE:%s.\n",
                        string{status_code}.data(),
                        string{status_message}.data());
                
                return true;
        }
        

        pos = checker.find("Content-Length: ");
        checker.remove_prefix(pos + 16);

        chunk.resrc.resize(atoi(checker.data()));
        
        return false;
}