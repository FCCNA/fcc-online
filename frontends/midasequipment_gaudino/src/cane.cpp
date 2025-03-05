#include "cane.h"
#include <stdexcept>
#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <strings.h> // bzero()
#include <unistd.h>  // read(), write(), close()
#include <iostream>
#include <sstream>
#include <thread> //std::this_thread::yield()
cane::cane() {}

void cane::WriteCmd(const std::string &cmd)
{
}

std::string cane::ReadCmd(const std::string &cmd)
{
    return "1";
}

void cane::QueryState()
{
    for (int i = 0; i < DOG_NCHANNEL; i++)
    {
        // std::string enabled = ReadCmd("DIS:GLO:CH" + std::to_string(i + 1) + ":STATE?\n");
        if (true)
        {
            fChannelEnabled[i] = true;
        }
        fChannelPosition[i] = std::stof(ReadCmd("temp"));
        fChannelOffset[i] = std::stof(ReadCmd("temp"));
        fChannelScale[i] = std::stof(ReadCmd("temp"));
        fChannelBandwidth[i] = std::stof(ReadCmd("temp"));
        fChannelInfo[i] = ReadCmd("temp");
        fChannelStatus[i] = ReadCmd("temp");
    }
    fHorizontalPosition = std::stof(ReadCmd("temp"));
    fHorizontalScale = std::stof(ReadCmd("temp"));
    fHorizontalSampleRate = std::stof(ReadCmd("temp"));
    fAcquisitionMode = ReadCmd("temp");
}

void cane::Connect(const std::string &ip, int port)
{
}

bool cane::ReadData()
{
    bool receiving_data = true;
    return receiving_data;
}

bool cane::HasEvent()
{
    return true;
}