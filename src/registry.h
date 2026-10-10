#pragma once
#include "mylib.h"

class REGISTRY {
public:
    STRING path;

    REGISTRY(const STRING& path);
    ~REGISTRY();

    const STRING& Path() const
    {
        return path;
    }

    STRING GetString(const STRING& valueName,const STRING& defaultValue);
    int GetInt(const STRING& valueName,int defaultValue);

    void SetString(const STRING& valueName,const STRING& value);
    void SetInt(const STRING& valueName,int value);

    void DeleteKey(const STRING& keyName);
    void Delete(const STRING& valueName);
    void SaveToFile(const STRING& keyName,const STRING& fileName) const;
    void RestoreFromFile(const STRING& keyName,const STRING& fileName);
};

extern REGISTRY* Registry;

