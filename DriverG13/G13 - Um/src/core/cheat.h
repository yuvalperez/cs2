#pragma once
#include <thread>
#include <iostream>
#include <cstddef> // Ensure this is included for std::ptrdiff_t
#include <mutex>
#include "mem/mem.h"
#include "../../ext/ImGui 1.90/imgui.h"
#include <Windows.h>
#include "math/vector.h"
#include <vector>
#include <locale>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <random>
inline float windowWidth = GetSystemMetrics(SM_CXSCREEN);
inline float  windowHeight = GetSystemMetrics(SM_CYSCREEN);


namespace var {
    inline uintptr_t ModBase = 0;
    inline uintptr_t engine2_dll;
    inline int localTeam;
    inline bool watermark = true;
    inline float textSize = 16.0f;
    inline bool HideWindow = false;
    inline bool closecheat = false;
    inline bool canrun = false;
  

    //esp
    inline bool drawEsp = true;
    inline bool drawEspTeam = true;
    inline bool corneresp = true;;
    inline bool enemyboxCheck = true;;
    inline float enemybox[3] = { 1.0f, 0.0f, 0.0f };
    inline float Teambox[3] = { 0.0f, 1.0f, 0.0f };
    inline float PlayerNameColor[3] = { 1.0f, 1.0f, 1.0f };
    inline float boneColorTeam[3] = { 1.0f, 1.0f, 1.0f };
    inline float boneColorEn[3] = { 1.0f, 1.0f, 1.0f };
    inline float visibilitycolor[3] = { 1.0f, 1.0f, 1.0f };

    inline float boxthick = 1.5f;
    inline bool BoxRainbow = false;
    inline bool BoxRainbowTeam = false;
    inline bool boneColorRainbow = false;
    inline bool HealthEsp = true;
    inline bool EspPlayerName = true;
    inline bool DistanceEsp = true;
    inline bool cashEsp = true;
    inline bool boneEsp = true;
    inline bool visibility = true;
    inline float bonethickness = 1.f;


    ///snap
    inline bool snapline = true;
    inline int snaplineMethod = 1;

    //aimbot
    inline bool AimBot = false;
    inline float smoothing = 1;
    inline float fov = 10;

    //Trigger bot
    inline bool trigger_bot = false;
    inline int trigger_delay = 50;




    //crosshair
    inline bool crosshair1 = true;
    inline bool crosshair2 = false;
    inline bool crosshair3 = true;
    inline bool crossrainbowcolor = false;
    inline float crosshairSize = 3.0f;
    inline float crosshairSize1 = 4.5f;
    inline float crosshairThick = 1.0f;
    inline float crosshairColor[3] = { 1.0f, 1.0f, 1.0f }; // White color



}

namespace offsets {
    //Buttons.hpp

    //offsets.hpp
    constexpr std::ptrdiff_t dwEntityList = 0x19BDD58;
    constexpr std::ptrdiff_t dwLocalPlayerPawn = 0x1823A08;
    constexpr std::ptrdiff_t dwViewMatrix = 0x1A1FCB0;
    constexpr std::ptrdiff_t dwViewAngles = 0x1A2D228;
    //client.hpp
    constexpr std::ptrdiff_t m_hPlayerPawn = 0x7DC; // CHandle<C_CSPlayerPawn>
    constexpr std::ptrdiff_t m_iTeamNum = 0x3C3;
    constexpr std::ptrdiff_t m_iHealth = 0x324;
    constexpr std::ptrdiff_t m_vOldOrigin = 0x1274; // Vector
    constexpr std::ptrdiff_t m_iszPlayerName = 0x630; // char[128]
    constexpr std::ptrdiff_t m_pGameSceneNode = 0x308; // CGameSceneNode*
    constexpr std::ptrdiff_t m_skeletonInstance = 0x50;
    constexpr std::ptrdiff_t m_iIDEntIndex = 0x13A8; // CEntityIndex
    constexpr std::ptrdiff_t m_vecViewOffset = 0xC50; // CNetworkViewOffsetVector
    constexpr std::ptrdiff_t m_iAccount = 0x40; // int32
    constexpr std::ptrdiff_t m_pInGameMoneyServices = 0x6F0; // CCSPlayerController_InGameMoneyServices*
    constexpr std::ptrdiff_t m_modelState = 0x170; // CModelState
    constexpr std::ptrdiff_t m_entitySpottedState = 0x2288; // EntitySpottedState_t
    constexpr std::ptrdiff_t m_bSpottedByMask = 0xC; // uint32[2]
    constexpr std::ptrdiff_t m_vecAbsVelocity = 0x3D0; // Vector
    constexpr std::ptrdiff_t m_bSpotted = 0x8; // bool
     //client.hpp
}

struct PlayerInfo {
    int health;
    int team;
    Vector3 position;
    uintptr_t PlayerCont;
    uintptr_t entityPawn;
    uintptr_t boneArray;
    Vector3 Head;
};

extern std::vector<PlayerInfo> playerData;
extern std::mutex dataMutex;
inline  uintptr_t localPlayerPawn = 0; // Extern declaration for localPlayerPawn
inline AdvancedMemoryTool mem(L"cs2.exe");
class cheatCore {
public:
    void cheat();
    void Trigger_bot();

};
namespace cheat
{
    void esp();

}
namespace Misc {
    void watermark();
    void Crosshair();
}
inline ImU32 ColorConvertFloat3ToU32(const float col[3])
{
    int r = static_cast<int>(col[0] * 255.0f);
    int g = static_cast<int>(col[1] * 255.0f);
    int b = static_cast<int>(col[2] * 255.0f);
    return IM_COL32(r, g, b, 255);

}
inline ImU32 HSVToRGB(float h, float s, float v) {
    int i = int(h * 6);
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    float r, g, b;
    switch (i % 6) {
    case 0: r = v, g = t, b = p; break;
    case 1: r = q, g = v, b = p; break;
    case 2: r = p, g = v, b = t; break;
    case 3: r = p, g = q, b = v; break;
    case 4: r = t, g = p, b = v; break;
    case 5: r = v, g = p, b = q; break;
    }

    return IM_COL32(static_cast<int>(r * 255), static_cast<int>(g * 255), static_cast<int>(b * 255), 255);
}
inline ImU32 GetRainbowColor(float speed) {
    float time = GetTickCount() / 1000.0f * speed;
    float hue = fmod(time, 1.0f);
    return HSVToRGB(hue, 1.0f, 1.0f);
}

inline std::string formatNumberWithCommas(int number) {
    std::stringstream ss;
    ss.imbue(std::locale("en_US.UTF-8")); // Ensure the locale is set to a valid one
    ss << std::fixed << number;
    return ss.str();
}

inline void randomSleep(int minMs, int maxMs) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(minMs, maxMs);
    std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));
}
inline void SimulateMouseMovement(int deltaX, int deltaY) {
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    input.mi.dx = deltaX;
    input.mi.dy = deltaY;

    SendInput(1, &input, sizeof(INPUT));
}
inline void SimulateHumanMouseClick() {
    INPUT input[2] = { 0 };

    // Mouse left button down
    input[0].type = INPUT_MOUSE;
    input[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

    // Mouse left button up
    input[1].type = INPUT_MOUSE;
    input[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

    // Send the button down event
    SendInput(1, &input[0], sizeof(INPUT));

    // Send the button up event
    SendInput(1, &input[1], sizeof(INPUT));
}
inline float CalculateFOVRadius(float fovAngle, float screenWidth) {
    // Convert the FOV angle from degrees to radians
    float fovRadians = fovAngle * (3.14159265358979323846f / 180.0f);


    float radius = tan(fovRadians / 2.0f) * (screenWidth / 2.0f);
    return radius;
}
