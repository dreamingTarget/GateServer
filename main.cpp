//#include <iostream>
//using namespace std;
//#include <boost/lexical_cast.hpp>
//
//int main() {
//	cout << "hello world" << endl;
//
//    cout << "Enter your weight: ";
//    float weight;
//    cin >> weight;
//    string gain = "A 10% increase raises ";
//    string wt = boost::lexical_cast<string> (weight);
//    gain = gain + wt + " to ";      // string operator()
//    weight = 1.1 * weight;
//    gain = gain + boost::lexical_cast<string>(weight) + ".";
//    cout << gain << endl;
//    system("pause");
//
//	return 0;
//}

#include <iostream>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>

int main()
{
    Json::Value root;
    root["id"] = 1001;
    root["data"] = "hello world";
    std::string request = root.toStyledString();
    std::cout << "request is " << request << std::endl;

    Json::Value root2;
    Json::Reader reader;
    reader.parse(request, root2);
    std::cout << "msg id is " << root2["id"] << " msg is " << root2["data"] << std::endl;
}
