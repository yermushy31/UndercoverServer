//
// Created by yermushy on 19/09/2023.
//

#ifndef UNDERCOVERCLIENT_FEATURES_H
#define UNDERCOVERCLIENT_FEATURES_H

#include <iostream>
#include <fstream>
#include <windows.h>
#include <mmsystem.h>
#include <iostream>
#include <string>
#pragma comment(lib, "winmm.lib")





class features {

public:
    void PrintAvailableAudioDevices();
    void RecordMicrophone();

};

#endif //UNDERCOVERCLIENT_FEATURES_H
