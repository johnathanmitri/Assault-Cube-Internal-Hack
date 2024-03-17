// dllmain.cpp : Defines the entry point for the DLL application.
#include <Windows.h>
#include <TlHelp32.h>
#include "mem.h"

struct vec3
{
    float x, y, z;
};

struct player
{
    char pad_0000[4]; //0x0000
    vec3 posVector;
    char pad_0010[48]; //0x0010
    float horizAim; //0x0040
    float vertAim; //0x0044
    char pad_0048[176]; //0x0048
    int32_t Health; //0x00F8
    char pad_00FC[296]; //0x00FC
    int8_t N000000DD; //0x0224
    char name[15]; //0x0225
    char pad_0234[248]; //0x0234
    int32_t team; //0x032C
    char pad_0330[68]; //0x0330
    class weapon* currentWeaponPtr; //0x0374
    char pad_0378[720]; //0x0378
}; //Size: 0x0648
//static_assert(sizeof(player) == 0x648);

class weapon
{
public:
    char pad_0000[68]; //0x0000
}; //Size: 0x0044     
//static_assert(sizeof(weapon) == 0x44);

bool exitFlag = false;

bool isTeamChecking = false;

player* localPlayer;

player* aimbotTargetPlayer;

uintptr_t aimbotTargetPlayerOffsetInList = 0x0;

uintptr_t moduleBase;

uintptr_t localPlayerAddr;

int* playerCountPtr;

bool locking = false; //aimlock on or off

void snapToTarget()
{
    vec3 localPlayerCoords = localPlayer->posVector;
    float xDisp = localPlayerCoords.x - aimbotTargetPlayer->posVector.x;
    float yDisp = localPlayerCoords.z - aimbotTargetPlayer->posVector.z; //z is y and y is z because i mixed up the order in the CODE. the Player object is the right order. Z is vertical, x,y is the plane
    float zDisp = localPlayerCoords.y - aimbotTargetPlayer->posVector.y;

    if (isnan(xDisp) || isnan(zDisp))
    {
        return;
    }

    float lateralHypotenuse = sqrt(xDisp * xDisp + zDisp * zDisp);

    float angleFromEast = acos(xDisp / lateralHypotenuse) * 180 / (float)3.141592;

    if (zDisp < 0)
        angleFromEast = -angleFromEast;

    float angleFromNorth = angleFromEast + 270;

    float nonNeg;
    if (angleFromNorth >= 360)
        nonNeg = angleFromNorth - 360;
    else
        nonNeg = angleFromNorth;

    float verticalHypotenuse = sqrt(lateralHypotenuse * lateralHypotenuse + yDisp * yDisp);

    float vertAngle = -acos(lateralHypotenuse / verticalHypotenuse) * 180 / (float)3.141592;

    if (yDisp < 0)
        vertAngle = -vertAngle;

    if (isnan(nonNeg) || isnan(vertAngle))
    {
        return;
    }
    localPlayer->horizAim = nonNeg;
    localPlayer->vertAim = vertAngle;
}

void keyBinds()
{
    if (GetAsyncKeyState(0x51) & 1)  //key q
    {
        locking = !locking;

        if (locking)
        {
            aimbotTargetPlayer = (player*)mem::FindDMAAddy(moduleBase + 0x10F4F8, { aimbotTargetPlayerOffsetInList, 0x0 });
            while (aimbotTargetPlayer == nullptr || !(aimbotTargetPlayer->Health > 0 && aimbotTargetPlayer->Health <= 100) || (localPlayer->team == aimbotTargetPlayer->team && isTeamChecking))
            {
                aimbotTargetPlayerOffsetInList += 0x4;
                if (aimbotTargetPlayerOffsetInList >= (*playerCountPtr) * 4)
                    aimbotTargetPlayerOffsetInList = 0x0;

                aimbotTargetPlayer = (player*)mem::FindDMAAddy(moduleBase + 0x10F4F8, { aimbotTargetPlayerOffsetInList, 0x0 });
                Sleep(5);
            }

            std::cout << "Now tracking " << aimbotTargetPlayer->name << ", Offset: " << std::hex << aimbotTargetPlayerOffsetInList << std::endl;
        }

        else
        {
            std::cout << "Stopped tracking " << std::endl;

        }

    }

    if (GetAsyncKeyState(0x46) & 1)  //key f
    {
        aimbotTargetPlayerOffsetInList += 0x4;
        if (aimbotTargetPlayerOffsetInList >= (*playerCountPtr) * 4)
            aimbotTargetPlayerOffsetInList = 0x0;

        aimbotTargetPlayer = (player*)mem::FindDMAAddy(moduleBase + 0x10F4F8, { aimbotTargetPlayerOffsetInList, 0x0 });
        while (aimbotTargetPlayer == nullptr || !(aimbotTargetPlayer->Health > 0 && aimbotTargetPlayer->Health <= 100) || (localPlayer->team == aimbotTargetPlayer->team && isTeamChecking))
        {
            aimbotTargetPlayerOffsetInList += 0x4;
            if (aimbotTargetPlayerOffsetInList >= (*playerCountPtr) * 4)
                aimbotTargetPlayerOffsetInList = 0x0;

            aimbotTargetPlayer = (player*)mem::FindDMAAddy(moduleBase + 0x10F4F8, { aimbotTargetPlayerOffsetInList, 0x0 });
            Sleep(5);
        }

        std::cout << "Now targetting " << aimbotTargetPlayer->name << ", Offset: " << std::hex << aimbotTargetPlayerOffsetInList << "Team bit: " << aimbotTargetPlayer->team << std::endl;
    }

    if (GetAsyncKeyState(0x6b) & 1)  //numpad plus
    {
        isTeamChecking = !isTeamChecking;

        std::cout << "Checking team: " << std::boolalpha << isTeamChecking << std::endl;
    }

    if (GetAsyncKeyState(VK_END) & 1)
    {
        exitFlag = true;
    }

    if (GetAsyncKeyState(VK_NUMPAD1) & 1) // ammo hack ? 
    {
        std::vector<unsigned int> ammoOffsets = { 0x374, 0x14, 0x0 };

        uintptr_t ammoAddr = mem::FindDMAAddy(moduleBase + 0x10f4f4, ammoOffsets);

        *(int*)ammoAddr = 1337;
    }

    if (GetAsyncKeyState(VK_NUMPAD2) & 1)  //AIMBOT
    {
        locking = !locking;
        if (locking)
            aimbotTargetPlayer = (player*)mem::FindDMAAddy(moduleBase + 0x10F4F8, { 0x4, 0x0 });
        Sleep(1);
    }

    if (GetAsyncKeyState(VK_NUMPAD3) & 1)
    {

    }

}

void mainLoop()
{
    keyBinds();


    if (locking)
    {
        snapToTarget();
    }
}

DWORD WINAPI HackThread(HMODULE hModule)
{
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);

    std::cout << "Injected";

    moduleBase = (uintptr_t)GetModuleHandle(L"ac_client.exe");

    localPlayerAddr = *(uintptr_t*)(moduleBase + 0x10f4f4);

    playerCountPtr = (int*)(moduleBase + 0x10F500);

    localPlayer = (player*)localPlayerAddr;



    while (true)
    {
        if (exitFlag)
            break;

        mainLoop();
        Sleep(10);
    }

    fclose(f);
    FreeConsole();
    FreeLibraryAndExitThread(hModule, 0);
    return 0;
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)HackThread, hModule, 0, nullptr));
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH: 
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

