#include <MarioKartWii/Kart/KartManager.hpp>
#include <UI/CtrlRaceBase/Speedometer.hpp>
#include <Settings/Settings.hpp>

namespace Pulsar {
namespace UI {
u32 CtrlRaceSpeedo::Count() {
    if(Settings::Mgr::Get().GetSettingValue(Settings::SETTINGSTYPE_RACE, SETTINGRACE_RADIO_SOM) == RACESETTING_SOM_DISABLED) return 0;
    const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
    u32 localPlayerCount = scenario.localPlayerCount;
    const SectionId sectionId = SectionMgr::sInstance->curSection->sectionId;
    if(sectionId >= SECTION_WATCH_GHOST_FROM_CHANNEL && sectionId <= SECTION_WATCH_GHOST_FROM_MENU) localPlayerCount += 1;
    if(localPlayerCount == 0 && (scenario.settings.gametype & GAMETYPE_ONLINE_SPECTATOR)) localPlayerCount = 1;
    return localPlayerCount;
}
void CtrlRaceSpeedo::Create(Page& page, u32 index, u32 count) {
    u8 speedoType = (count == 3) ? 4 : count;
    for(int i = 0; i < count; ++i) {
        CtrlRaceSpeedo* som = new(CtrlRaceSpeedo);
        page.AddControl(index + i, *som, 0);
        char variant[0x20];
        int pos = i;
        if(Settings::Mgr::Get().GetSettingValue(Settings::SETTINGSTYPE_RACE, SETTINGRACE_RADIO_SOM) == RACESETTING_SOM_LEFT){
            if(count == 1) pos = 0;           
            snprintf(variant, 0x20, "Speedo_%1d_%1d", speedoType, pos);
        } else if(Settings::Mgr::Get().GetSettingValue(Settings::SETTINGSTYPE_RACE, SETTINGRACE_RADIO_SOM) == RACESETTING_SOM_RIGHT){
            if(count == 1) pos = 1;
            snprintf(variant, 0x20, "SpeedoR_%1d_%1d", speedoType, pos);
        }
        som->Load(variant, i);
    }
}
static CustomCtrlBuilder SOM(CtrlRaceSpeedo::Count, CtrlRaceSpeedo::Create);

void CtrlRaceSpeedo::Load(const char* variant, u8 id) {
    this->hudSlotId = id;
    ControlLoader loader(this);
    const char* anims[] ={
        "Hundreds", "Hundreds", nullptr,
        "Tens", "Tens", nullptr,
        "Units", "Units", nullptr,
        "Dot", "Dot",nullptr,
        "Tenths", "Tenths", nullptr,
        "Hundredths", "Hundredths", nullptr,
        "Thousandths", "Thousandths", nullptr,
        nullptr
    };

    //brlyt subfile loading
    if (Settings::Mgr::Get().GetSettingValue(Settings::SETTINGSTYPE_RACE, SETTINGRACE_RADIO_SOM) == RACESETTING_SOM_LEFT) {
        loader.Load(UI::raceFolder, "PULSpeedo", variant, anims);
    } else if (Settings::Mgr::Get().GetSettingValue(Settings::SETTINGSTYPE_RACE, SETTINGRACE_RADIO_SOM) == RACESETTING_SOM_RIGHT) {
        loader.Load(UI::raceFolder, "PULSpeedo_Right", variant, anims);
    }

    this->Animate();
    return;
}

void CtrlRaceSpeedo::Init() {
    this->HudSlotColorEnable("speed0", true);
    this->HudSlotColorEnable("speed1", true);
    this->HudSlotColorEnable("speed2", true);
    this->HudSlotColorEnable("speed3", true);
    this->HudSlotColorEnable("speed4", true);
    this->HudSlotColorEnable("speed5", true);
    this->HudSlotColorEnable("speed6", true);
    this->HudSlotColorEnable("kmh", true);
    LayoutUIControl::Init();
    return;
}

void CtrlRaceSpeedo::OnUpdate() {
    this->UpdatePausePosition();
    const u8 digits = Settings::Mgr::Get().GetSettingValue(Settings::SETTINGSTYPE_RACE, SETTINGRACE_SCROLL_SOM);
    const Kart::Pointers& pointers = Kart::Manager::sInstance->players[this->GetPlayerId()]->pointers;
    const Kart::Physics* physics = pointers.kartBody->kartPhysicsHolder->physics;

    Vec3 sum;
    MTX::PSVECAdd(&physics->engineSpeed, &physics->speed2, &sum);
    MTX::PSVECAdd(&physics->speed3, &sum, &sum);
    float speed = MTX::PSVECMag(&sum);
    float speedCap = pointers.kartMovement->hardSpeedLimit;
    if(speed > speedCap) speed = speedCap;


    const u32 speedValue = static_cast<u32>(speed * 1000.0f);

    //10 means empty, 11 dot
    const u32 empty = 10;
    const u32 dot = 11;
    const u8 somPosition = Settings::Mgr::Get().GetSettingValue(Settings::SETTINGSTYPE_RACE, SETTINGRACE_RADIO_SOM);

    u32 speedDigits[7] = { empty, empty, empty, empty, empty, empty, empty };
    if(digits <= 3 && (somPosition == RACESETTING_SOM_LEFT || somPosition == RACESETTING_SOM_RIGHT)) {
        const u32 rawHundreds = speedValue / 100000 % 10;
        const u32 rawTens = speedValue / 10000 % 10;
        const u32 rawUnits = speedValue / 1000 % 10;
        const u32 decimals[3] = {
            speedValue / 100 % 10,
            speedValue / 10 % 10,
            speedValue % 10
        };

        u32 integerDigits[3];
        u32 integerDigitCount;
        if(speedValue < 10000) {
            integerDigits[0] = rawUnits;
            integerDigitCount = 1;
        }
        else if(speedValue < 100000) {
            integerDigits[0] = rawTens;
            integerDigits[1] = rawUnits;
            integerDigitCount = 2;
        }
        else {
            integerDigits[0] = rawHundreds;
            integerDigits[1] = rawTens;
            integerDigits[2] = rawUnits;
            integerDigitCount = 3;
        }

        const u32 visibleDigitCount = integerDigitCount + (digits == 0 ? 0 : digits + 1);
        u32 out = somPosition == RACESETTING_SOM_RIGHT ? 7 - visibleDigitCount : 0;
        for(u32 i = 0; i < integerDigitCount; ++i) speedDigits[out++] = integerDigits[i];
        if(digits > 0) {
            speedDigits[out++] = dot;
            for(u32 i = 0; i < digits; ++i) speedDigits[out++] = decimals[i];
        }
    }

    SpeedArg args(speedDigits[0], speedDigits[1], speedDigits[2], speedDigits[3], speedDigits[4], speedDigits[5], speedDigits[6]);
    this->Animate(&args);
    return;
}

void CtrlRaceSpeedo::Animate(const SpeedArg* args) {
    for(int i = 0; i < 7; ++i) {
        AnimationGroup& group = this->animator.GetAnimationGroupById(i);
        float frame = 0.0f;
        if(args != nullptr) frame = static_cast<float>(args->values[i]);
        group.PlayAnimationAtFrameAndDisable(0, frame);
    }
}
}//namespace UI
}//namespace Pulsar