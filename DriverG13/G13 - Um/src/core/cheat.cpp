#include "cheat.h"
#include <algorithm>
#include "bone..hpp" // Corrected filename

#ifdef max
#undef max
#endif

std::vector<PlayerInfo> playerData;
std::mutex dataMutex;


float centerw = windowWidth / 2;
float centerh = windowHeight / 2;

DWORD lastTick = GetTickCount();

void cheatCore::cheat() {
   
    while (var::ModBase == 0) {
        var::ModBase = mem.GetModuleBaseAddress(L"client.dll");
        Sleep(500);
    }

    DWORD procId = mem.GetProcessId();
    std::cout << "ModBase = " << std::hex << var::ModBase << std::endl;


    while (true) {
        localPlayerPawn = mem.ReadMemory<uintptr_t>(var::ModBase + offsets::dwLocalPlayerPawn);
        while (!localPlayerPawn) {
            localPlayerPawn = mem.ReadMemory<uintptr_t>(var::ModBase + offsets::dwLocalPlayerPawn);

            Sleep(3000);
        }

        uintptr_t entityList = mem.ReadMemory<uintptr_t>(var::ModBase + offsets::dwEntityList);
        if (!entityList) return;

        var::localTeam = mem.ReadMemory<BYTE>(localPlayerPawn + offsets::m_iTeamNum);
        std::vector<PlayerInfo> tempPlayerData;
        for (int i = 1; i < 32; i++) {
            uintptr_t entityList = mem.ReadMemory<uintptr_t>(var::ModBase + offsets::dwEntityList);
            uintptr_t listEntry1 = mem.ReadMemory<uintptr_t>(entityList + ((8 * (i & 0x7FFF) >> 9) + 16));
            if (!listEntry1) continue;

            uintptr_t playerController = mem.ReadMemory<uintptr_t>(listEntry1 + 120 * (i & 0x1FF));
            if (!playerController) continue;

            uint32_t PlayerPawn1 = mem.ReadMemory<uint32_t>(playerController + offsets::m_hPlayerPawn);
            if (!PlayerPawn1) continue;

            uintptr_t listEntry2 = mem.ReadMemory<uintptr_t>(entityList + 0x8 * ((PlayerPawn1 & 0x1FFF) >> 9) + 16);
            if (!listEntry2) continue;

            uintptr_t entityPawn = mem.ReadMemory<uintptr_t>(listEntry2 + 120 * (PlayerPawn1 & 0x1FF));
            if (entityPawn == localPlayerPawn) continue;

            int playerTeam = mem.ReadMemory<int>(entityPawn + offsets::m_iTeamNum);
            if (!var::drawEspTeam && var::localTeam == playerTeam) continue;

            int playerHealth = mem.ReadMemory<int>(entityPawn + offsets::m_iHealth);
            if (playerHealth <= 0) continue;
            Vector3 playerPos = mem.ReadMemory<Vector3>(entityPawn + offsets::m_vOldOrigin);
            uintptr_t gameScene = mem.ReadMemory<uintptr_t>(entityPawn + offsets::m_pGameSceneNode);
            uintptr_t boneArray = mem.ReadMemory<uintptr_t>(gameScene + offsets::m_modelState + 0x80);
            Vector3 Head = mem.ReadMemory<Vector3>(boneArray + 6 * 32);
            PlayerInfo info = { playerHealth, playerTeam, playerPos, playerController, entityPawn,boneArray,Head };
            tempPlayerData.push_back(info);
           
        }

        // Lock the mutex and update the shared playerData
        {
            std::lock_guard<std::mutex> guard(dataMutex);
            playerData = std::move(tempPlayerData);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void cheat::esp() {

    DWORD interval = 5000; // 5000 milliseconds (5 seconds)
    std::lock_guard<std::mutex> guard(dataMutex);
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    view_matrix_t view_matrix = mem.ReadMemory<view_matrix_t>(var::ModBase + offsets::dwViewMatrix);
    ImU32 enemybox1 = var::BoxRainbow ? GetRainbowColor(1.0f) : ColorConvertFloat3ToU32(var::enemybox);
    ImU32 Teambox1 = var::BoxRainbowTeam ? GetRainbowColor(1.0f) : ColorConvertFloat3ToU32(var::Teambox);
    ImU32 enemybone1 = var::BoxRainbow ? GetRainbowColor(1.0f) : ColorConvertFloat3ToU32(var::boneColorEn);
    ImU32 Teambone1 = var::BoxRainbowTeam ? GetRainbowColor(1.0f) : ColorConvertFloat3ToU32(var::boneColorTeam);

    for (const auto& player : playerData) {
     
        Vector3 localPlayerPos = mem.ReadMemory<Vector3>(localPlayerPawn + offsets::m_vOldOrigin);
        Vector3 topPos = { player.position.x, player.position.y, player.Head.z +8.0f };
        Vector3 bottomPos = { player.position.x, player.position.y, player.position.z };
        auto distance = localPlayerPos.Distance(player.position);
        Vector3 screenTop = worldToScreen(view_matrix, topPos);
        Vector3 screenBottom = worldToScreen(view_matrix, bottomPos);

        bool entitySpotted = mem.ReadMemory<bool>(player.entityPawn + offsets::m_entitySpottedState + offsets::m_bSpottedByMask);
        if (var::snapline && screenTop.z >= -0.001f && screenBottom.z >= -0.001f) {
            auto color = (entitySpotted && var::visibility) ? ColorConvertFloat3ToU32(var::visibilitycolor) : enemybox1;
             color = (var::localTeam == player.team) ? Teambox1 : color;
             switch (var::snaplineMethod)
             {
             case 1:
                     drawList->AddLine(ImVec2{ centerw , windowHeight }, ImVec2{ screenBottom.x, screenBottom.y }, color);
                 break; 
             case 2:
                     drawList->AddLine(ImVec2{ centerw , centerh }, ImVec2{ screenBottom.x, screenBottom.y }, color);
                 break;
             case 3:
                     drawList->AddLine(ImVec2{ centerw, 0.0f   }, ImVec2{ screenBottom.x, screenBottom.y }, color);
                 break;
             }
         
        }

        if (screenTop.z > 0.1f && screenBottom.z > 0.1f) {
            float boxHeight = fabs(screenBottom.y - screenTop.y);
            float boxWidth = boxHeight / 2.0f;
            ImVec2 boxMin = ImVec2(screenBottom.x - boxWidth / 2, screenTop.y);
            ImVec2 boxMax = ImVec2(screenBottom.x + boxWidth / 2, screenBottom.y);
            float cornerLength = 10.0f / (distance / 500.0f);
            cornerLength = std::max(cornerLength, 3.0f);
            auto boxColor = (entitySpotted && var::visibility) ? ColorConvertFloat3ToU32( var::visibilitycolor) : enemybox1;
             boxColor = (var::localTeam == player.team) ? Teambox1 : boxColor;
            if (var::corneresp) {

                drawList->AddLine(boxMin, ImVec2(boxMin.x + cornerLength, boxMin.y), boxColor, var::boxthick);
                drawList->AddLine(boxMin, ImVec2(boxMin.x, boxMin.y + cornerLength), boxColor, var::boxthick);
                drawList->AddLine(ImVec2(boxMax.x, boxMin.y), ImVec2(boxMax.x - cornerLength, boxMin.y), boxColor, var::boxthick);
                drawList->AddLine(ImVec2(boxMax.x, boxMin.y), ImVec2(boxMax.x, boxMin.y + cornerLength), boxColor, var::boxthick);
                drawList->AddLine(ImVec2(boxMin.x, boxMax.y), ImVec2(boxMin.x + cornerLength, boxMax.y), boxColor, var::boxthick);
                drawList->AddLine(ImVec2(boxMin.x, boxMax.y), ImVec2(boxMin.x, boxMax.y - cornerLength), boxColor, var::boxthick);
                drawList->AddLine(ImVec2(boxMax.x, boxMax.y), ImVec2(boxMax.x - cornerLength, boxMax.y), boxColor, var::boxthick);
                drawList->AddLine(ImVec2(boxMax.x, boxMax.y), ImVec2(boxMax.x, boxMax.y - cornerLength), boxColor, var::boxthick);
            }
            if (var::enemyboxCheck && !var::corneresp) {
                drawList->AddRect(boxMin, boxMax, boxColor, 0.0f, 15, var::boxthick);
            }

            if (var::boneEsp) {
                auto colorBone = (entitySpotted && var::visibility) ? ColorConvertFloat3ToU32(var::visibilitycolor) : enemybone1;
                 colorBone = (var::localTeam == player.team) ? Teambone1 : colorBone;
               
              
                    for (int i = 0; i < sizeof(boneConnections) / sizeof(boneConnections[0]); i++) {
                        int bone1 = boneConnections[i].bone1;
                        int bone2 = boneConnections[i].bone2;
                        Vector3 VectorBone1 = mem.ReadMemory<Vector3>(player.boneArray + bone1 * 32);
                        Vector3 VectorBone2 = mem.ReadMemory<Vector3>(player.boneArray + bone2 * 32);
                        Vector3 b1 = worldToScreen(view_matrix, VectorBone1);
                        Vector3 b2 = worldToScreen(view_matrix, VectorBone2);
                       
                        if (b2.z > 0.1f && b1.z > 0.1f) {
                            if (bone1 == 6) {
                                drawList->AddCircleFilled(ImVec2(b1.x, b1.y), 2.f, colorBone);
                            }
                            drawList->AddLine(ImVec2(b1.x, b1.y), ImVec2(b2.x, b2.y), colorBone, var::bonethickness);
                        }
                        
                    }
                
            }

            if (var::HealthEsp) {
                float maxHealthBarHeight = boxHeight;
                float healthBarHeight = maxHealthBarHeight * (player.health / 100.0f);
                ImVec2 healthBarMin = ImVec2(boxMin.x - 8.0f, boxMax.y - healthBarHeight);
                ImVec2 healthBarMax = ImVec2(boxMin.x - 3.0f, boxMax.y);

                ImU32 healthColor = (player.health > 70) ? IM_COL32(0, 255, 0, 255) :
                    (player.health > 40) ? IM_COL32(255, 255, 0, 255) :
                    IM_COL32(255, 0, 0, 255);

                drawList->AddRectFilled(ImVec2(boxMin.x - 8.0f, boxMin.y), ImVec2(boxMin.x - 3.0f, boxMax.y), IM_COL32(0, 0, 0, 255), 1.f);
                drawList->AddRectFilled(healthBarMin, healthBarMax, healthColor, 1.f);
            }

            if (var::EspPlayerName) {
                std::string playerName = mem.ReadString(player.PlayerCont + offsets::m_iszPlayerName);
                ImVec2 textPos = ImVec2((boxMin.x + boxMax.x) * 0.5f, boxMin.y - 15.0f);
                ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(var::textSize, FLT_MAX, 0.0f, playerName.c_str());
                drawList->AddText(ImVec2(textPos.x - textSize.x * 0.5f, textPos.y), ColorConvertFloat3ToU32(var::PlayerNameColor), playerName.c_str());
            }

            if (var::DistanceEsp) {
                std::string formattedDis = formatNumberWithCommas(distance / 10) + " M";
                char distanceText[128];
                snprintf(distanceText, sizeof(distanceText), "%s", formattedDis.c_str());
                ImVec2 boxMiddle((boxMin.x + boxMax.x) * 0.5f, boxMax.y + 15.f);
                ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(var::textSize, FLT_MAX, 0.0f, distanceText);
                ImVec2 textPos(boxMiddle.x - textSize.x * 0.5f, boxMiddle.y);
                drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), distanceText);
            }
          
            if (var::cashEsp) {
                auto inGameMoneyServices = mem.ReadMemory<uintptr_t>(player.PlayerCont + offsets::m_pInGameMoneyServices);
                if (!inGameMoneyServices) continue;

                auto accountBalance = mem.ReadMemory<int>(inGameMoneyServices + offsets::m_iAccount);
                std::string formattedBalance = formatNumberWithCommas(accountBalance) + " $";
                ImVec2 boxMiddle((boxMin.x + boxMax.x) * 0.5f, boxMax.y);
                ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(var::textSize, FLT_MAX, 0.0f, formattedBalance.c_str());
                ImVec2 textPos(boxMiddle.x - textSize.x * 0.5f, boxMiddle.y);
                drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), formattedBalance.c_str());
            }
        }
      
    }
    DWORD currentTick = GetTickCount64();
    if (currentTick - lastTick >= interval) {
        playerData.clear();
        lastTick = currentTick;
    }
}

void Misc::watermark() {
    ImGui::GetBackgroundDrawList()->AddText(ImVec2{ centerw  - 20.f , windowHeight - 20.f }, GetRainbowColor(0.2f), "@g13_haze_1");
}

void Misc::Crosshair() {
    ImU32 crosshairColor = var::crossrainbowcolor ? GetRainbowColor(0.5f) : ColorConvertFloat3ToU32(var::crosshairColor);
    if (var::AimBot) {
        ImGui::GetForegroundDrawList()->AddCircle(ImVec2(centerw, centerh), CalculateFOVRadius(var::fov,windowWidth), IM_COL32(255, 255, 255, 255));
    }
    if (var::crosshair2) {
        ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2{ centerw, centerh }, var::crosshairSize, crosshairColor);
    }
    if (var::crosshair3) {
        ImGui::GetBackgroundDrawList()->AddLine(ImVec2{ centerw - var::crosshairSize1, centerh }, ImVec2{ centerw + var::crosshairSize1, centerh }, crosshairColor, var::crosshairThick);
        ImGui::GetBackgroundDrawList()->AddLine(ImVec2{ centerw, centerh - var::crosshairSize1 }, ImVec2{ centerw, centerh + var::crosshairSize1 }, crosshairColor, var::crosshairThick);
    }
}
void cheatCore::Trigger_bot() {
    const auto playerTeam = mem.ReadMemory<int>(localPlayerPawn + offsets::m_iTeamNum);
  
    while (true) {
        if (GetAsyncKeyState(VK_SHIFT)  &&0x8000 && var::trigger_bot) {


            const auto entityId = mem.ReadMemory<int>(localPlayerPawn + offsets::m_iIDEntIndex);

            if (localPlayerPawn) {
                auto health = mem.ReadMemory<LONGLONG>(localPlayerPawn + offsets::m_iHealth);
            
                if (health && entityId > 0) {
                    auto entList = mem.ReadMemory<LONGLONG>(var::ModBase + offsets::dwEntityList);
                    auto entEntry = mem.ReadMemory<LONGLONG>(entList + 0x8 * (entityId >> 9) + 0x10);
                    auto entity = mem.ReadMemory<LONGLONG>(entEntry + 120 * (entityId & 0x1FF));
                    auto entityTeam = mem.ReadMemory<int>(entity + offsets::m_iTeamNum);

                    if (entityTeam == playerTeam) continue;

                    auto entityHp = mem.ReadMemory<int>(entity + offsets::m_iHealth);
                    if (entityHp > 0) {

                        std::this_thread::sleep_for(std::chrono::microseconds(var::trigger_delay));

                        SimulateHumanMouseClick();

                    }

                }
            }
        }
        if (var::AimBot && (GetAsyncKeyState(VK_RBUTTON) & 0x8000)) {
            std::lock_guard<std::mutex> guard(dataMutex);

            // Variables for targeting
            Vector3 smoothedTargetPos;
            float closestDistance = FLT_MAX;
            uintptr_t closestEntityPawn = 0;

            // Read local player position and view angles
            Vector3 localPlayerPos = mem.ReadMemory<Vector3>(localPlayerPawn + offsets::m_vOldOrigin);
            Vector3 viewAngles = mem.ReadMemory<Vector3>(var::ModBase + offsets::dwViewAngles);
            view_matrix_t matrix = mem.ReadMemory<view_matrix_t>(var::ModBase + offsets::dwViewMatrix);

            // Loop through each player in playerData
            for (const auto& player : playerData) {
                if (player.team == playerTeam) continue;

                // Check if enemy player is spotted
                bool spotted = mem.ReadMemory<INT32>(player.entityPawn + offsets::m_entitySpottedState + offsets::m_bSpottedByMask);
                if (spotted) {
                    // Calculate distance and angle to the enemy
                    float distance = sqrt(pow(localPlayerPos.x - player.position.x, 2) +
                        pow(localPlayerPos.y - player.position.y, 2) +
                        pow(localPlayerPos.z - player.position.z, 2));
                    Vector3 angleToEnemy = CalcAngle(localPlayerPos, player.position);
                    // Check if enemy is within FOV and closest so far
                    float fov = var::fov;
                    float angleDifference = fabs(angleToEnemy.y - viewAngles.y);

                    // Log angle difference and distance for debugging
                    std::cout << "Angle difference: " << angleDifference << ", Distance: " << distance << std::endl;

                    if (angleDifference < fov && distance < closestDistance) {
                        closestDistance = distance;
                        closestEntityPawn = player.entityPawn;

                        // Predict enemy head position based on velocity
                        Vector3 enemyVelocity = mem.ReadMemory<Vector3>(player.entityPawn + offsets::m_vecAbsVelocity);
                        Vector3 myVelocity = mem.ReadMemory<Vector3>(localPlayerPawn + offsets::m_vecAbsVelocity);
                        Vector3 relativeVelocity = enemyVelocity - myVelocity;
                        float predictionTime = 0.1f;  // Example prediction time in seconds
                        Vector3 predictedHeadPos = player.Head + (relativeVelocity * predictionTime);

                        // Smooth target position
                        smoothedTargetPos = worldToScreen(matrix, predictedHeadPos);
                    }
                }
            }

            // If a valid target is found and still spotted, adjust mouse position
            if (closestEntityPawn != 0 && closestDistance != FLT_MAX) {
                POINT currentPos;
                GetCursorPos(&currentPos);

                // Calculate delta between current cursor position and smoothed target position
                int deltaX = static_cast<int>(smoothedTargetPos.x - currentPos.x);
                int deltaY = static_cast<int>(smoothedTargetPos.y - currentPos.y);

                // Log delta values for debugging
                std::cout << "Delta X: " << deltaX << ", Delta Y: " << deltaY << std::endl;

                // Adjust mouse position using mouse_event or SendInput
                mouse_event(MOUSEEVENTF_MOVE, deltaX, deltaY, 0, 0);
            }
        }

    }
}