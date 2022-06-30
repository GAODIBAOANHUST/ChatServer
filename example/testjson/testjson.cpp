#include <nlohmann/json.hpp>
#include <iostream>
#include <vector>
#include <map>
#include <string>

using json = nlohmann::json;
using namespace std;

// json序列化示例1
void func1(){
    json js;
    js["msg_type"] = 2;
    js["from"] = "zhang san";
    js["to"] = "li si";
    js["msg"] = "hello, what are you doing now?"; 

    string sendBuf = js.dump();
    cout << sendBuf.c_str() << endl;
}

// json序列化示例2
void func2(){
    json js;
    // 添加数组
    js["id"] = {1,2,3,4,5};
    // 添加key-value
    js["name"] = "zhang san";
    // 添加对象
    js["msg"]["zhang san"] = "hello world";
    js["msg"]["liu shuo"] = "hello China";
    // 上面等同于下面这句一次性添加数组对象
    js["msg"] = {{"zhang san", "hello world"}, {"liu shuo", "hello China"}};
    cout << js << endl;
}

// json序列化示例3
void func3(){
    json js;

    // 直接序列化一个vector容器
    vector<int> vec{1,2,5};
    js["list"] = vec;

    // 直接序列化一个map容器
    map<int, string> m{{1, "黄山"}, {2, "华山"}, {3, "泰山"}};
    js["path"] = m;

    string sendBuf = js.dump();
    cout << sendBuf << endl;
}

// json反序列化示例1
string re_func1(){
    json js;
    js["msg_type"] = 2;
    js["from"] = "zhang san";
    js["to"] = "li si";
    js["msg"] = "hello, what are you doing now?"; 

    string sendBuf = js.dump();
    return sendBuf;
}

int main(){
    string recvBuf1 = re_func1();
    // 数据的反序列化
    json jsbuf = json::parse(recvBuf1);
    cout << jsbuf["msg_type"] << endl;
    cout << jsbuf["from"] << endl;
    cout << jsbuf["to"] << endl;
    cout << jsbuf["msg"] << endl;
    return 0;
}