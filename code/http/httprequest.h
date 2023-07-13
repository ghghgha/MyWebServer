/*
 * @Author: Wang
 * @Date: 2022-04-06 21:21:58
 * @Description: httprequest类的头文件
 * @LastEditTime: 2022-04-06 22:38:00
 * @FilePath: \WebServer\http\httprequest.h
 */

 #ifndef WJ_HTTP_REQUEST_H
#define WJ_HTTP_REQUEST_H
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex> // 正则表达式
#include <errno.h>     
#include <mysql/mysql.h>  //mysql

#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../pool/sqlconnpool.h"
#include "../pool/sqlconnRAII.h"

class HttpRequest {
public:
    enum PARSE_STATE {
        REQUEST_LINE,//请求行
        HEADERS,//请求头部字段
        BODY,//请求消息体
        FINISH,  //请求结束      
    };

    enum HTTP_CODE {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURSE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };
    
    HttpRequest() { Init(); } //构造函数
    ~HttpRequest() = default;//析沟函数

    void Init();//初始化
    bool parse(Buffer& buff);//解析

    std::string path() const;
    std::string& path();
    std::string method() const;
    std::string version() const;
    std::string GetPost(const std::string& key) const;
    std::string GetPost(const char* key) const;

    bool IsKeepAlive() const;

    /* 
    todo 
    void HttpConn::ParseFormData() {}
    void HttpConn::ParseJson() {}
    */

private:
    bool ParseRequestLine_(const std::string& line);
    void ParseHeader_(const std::string& line);
    void ParseBody_(const std::string& line);

    void ParsePath_();
    void ParsePost_();
    void ParseFromUrlencoded_();

    static bool UserVerify(const std::string& name, const std::string& pwd, bool isLogin);

    PARSE_STATE state_;// 解析状态
    std::string method_, path_, version_, body_;// 请求方法，目标资源的URL，版本号
    std::unordered_map<std::string, std::string> header_; //HTTP请求的头部字段
    std::unordered_map<std::string, std::string> post_;// 解析POST类型的HTTP请求的消息体<key,value>

    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
    static int ConverHex(char ch);
};


#endif //MRL_HTTP_REQUEST_H