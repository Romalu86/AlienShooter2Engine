#pragma once
#include <stddef.h>
#include "mylib.h"

enum SAVE_TYPE : int;

class zProfLdr {
public:
    enum { MAX_PROF_NUM = 10 };

    STRING m_aProfs[MAX_PROF_NUM];
    unsigned char m_bLoaded;
    unsigned char m_profileChanged[MAX_PROF_NUM];
    unsigned char m_037;
    int m_currentProfile;

    zProfLdr();
    virtual void* QueryInterface(int interfaceId) const;
    virtual int iRelease();
    virtual ~zProfLdr() noexcept(false);
    static void operator delete(void* memory);
    bool GetLoadState();

    bool ExpFileToReg(bool loadProfileOptions);
    bool ExpRegToFile(bool saveProfileOptions);
    bool DeleteProfile2(int _iProfileNum);
    STRING GetCurSaveName2(SAVE_TYPE saveType, bool survival);
    STRING GetMaxLevSaveName(SAVE_TYPE _eSType);
    int GetCurLevNum(int profile, bool survival);
    void SetMaxLevNum(bool survival);
    int GetMaxLevNum(int _iProfNum, bool survival);
    void SetCurProf(int _iCurProf);

private:
    void ClearObjPrf();
    bool ExpFileToObj();
    bool ExpObjToFile();
    bool ExpObjToReg();
    bool ExpRegToObj();
    STRING GetPlayerNameFName(int profile);
    STRING GetOptFName(int profile, bool createFolder);
    bool SaveOptions(int profile);
    bool LoadOptions(int profile);
    bool DeleteProfileEx(int _iProfileNum);
    STRING GetSaveName2(int profile, SAVE_TYPE _eSType, int _iLevNum);
    STRING GetProfFolderName(int _iProfileNum);
    void CreateFolderIfNeed(int _iProfileNum);
};
