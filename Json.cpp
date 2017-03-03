#include "Json.h"

Json::Json()
{
    root = cJSON_CreateObject();
}

Json::~Json()
{
    cJSON_Delete(root);
}

void Json::add(string key, string value)
{
    cJSON_AddStringToObject(root, key.c_str(), value.c_str());
}

string Json::print()
{
    char* p = cJSON_Print(root);
    string ret = p;
    free(p);
    return ret;
}

void Json::parse(string buf)
{
    if(root)
        cJSON_Delete(root);

    root = cJSON_Parse(buf.c_str());
}

string Json::value(string key)
{
    cJSON* obj = cJSON_GetObjectItem(root, key.c_str());
    return obj->valuestring;
}
