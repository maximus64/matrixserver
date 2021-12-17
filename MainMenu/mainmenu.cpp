#include "mainmenu.h"

#include <stdio.h>
#include <algorithm>
#include <iterator>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <unistd.h>

size_t hostnameLen = 20;
char hostname[20];

enum MenuState {
    applist, settings, settingsUpdate
};

MenuState menuState = applist;


static const Bitmap1bpp battery_indicator = {
    Vector2i(0,0),
    Vector2i(0,1),
    Vector2i(0,2),
    Vector2i(0,3),
    Vector2i(0,4),
    Vector2i(0,5),

    Vector2i(1,0),
    Vector2i(1,5),
    Vector2i(2,0),
    Vector2i(2,5),
    Vector2i(3,0),
    Vector2i(3,5),
    Vector2i(4,0),
    Vector2i(4,5),
    Vector2i(5,0),
    Vector2i(5,5),
    Vector2i(6,0),
    Vector2i(6,5),
    Vector2i(7,0),
    Vector2i(7,5),
    Vector2i(8,0),
    Vector2i(8,5),
    Vector2i(9,0),
    Vector2i(9,5),

    Vector2i(10,0),
    Vector2i(10,1),
    Vector2i(10,2),
    Vector2i(10,3),
    Vector2i(10,4),
    Vector2i(10,5),

    Vector2i(11,1),
    Vector2i(11,2),
    Vector2i(11,3),
    Vector2i(11,4),
};

MainMenu::MainMenu() : CubeApplication(40), joystickmngr(8), soundfx() {
    searchDirectory = "/opt/ledcube/apps";
    for (const auto &p : std::experimental::filesystem::directory_iterator(searchDirectory)) {
        //if(p.path().extension() == "cube"){
        appList.push_back(AppListItem(std::string(p.path().filename()), std::string(p.path())));
        //}
    }

    appList.push_back(AppListItem("settings", "settings", true, Color::blue()));
    settingsList.push_back(AppListItem("brightness", "brightness", true));
    settingsList.push_back(AppListItem("volume", "volume", true));
    settingsList.push_back(AppListItem("update", "update", true));
    settingsList.push_back(AppListItem("shutdown", "shutdown", true));
    settingsList.push_back(AppListItem("return", "return", true, Color::blue()));

    gethostname(hostname, hostnameLen);
    menuState = applist;
}


bool MainMenu::loop() {
    static int loopcount = 0;
    static int selectedExec = appList.size() / 2;
    static int lastSelectedExec = selectedExec;
    static int animationOffset = 0;

    static Color colSelectedText = Color::white();
    static Color colVoltageText = Color::blue();

    static float vol, accel_vol;
    static float bright, accel_bright;
    static bool once;
    static bool charging;

    std::time_t t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);

    if (!once) {
        int v = getVolume();
        int b = getBrightness();
        vol = (float)v;
        bright = (float)b;

        settingsList[0].name = "brightness: " + std::to_string(b);
        settingsList[1].name = "volume: " + std::to_string(v);

        charging = adcBattery.isCharging();

        once = true;
    }

    if (joystickmngr.getButtonPress(0)) {
        soundfx.playSound(SoundEffect::Sound::select);
    }

    if (charging != adcBattery.isCharging()) {
        charging = adcBattery.isCharging();
        if (charging)
            soundfx.playSound(SoundEffect::Sound::charging);
    }

    clear();

    colSelectedText.fromHSV((loopcount % 360 / 1.0f), 1.0, 1.0);

    switch (menuState) {
        case applist: {
            selectedExec += (int) joystickmngr.getAxisPress(1);
            if (selectedExec < 0) {
                selectedExec = appList.size() - 1;
            } else {
                selectedExec %= appList.size();
            }
            if(lastSelectedExec != selectedExec){
                animationOffset = (selectedExec-lastSelectedExec)*7;
            }
            lastSelectedExec = selectedExec;
            animationOffset *= 0.85;



            if (joystickmngr.getButtonPress(0)) {
                if (appList.at(selectedExec).execPath == "settings") {
                    selectedExec = 0;
                    lastSelectedExec = selectedExec;
                    animationOffset = 0;
                    menuState = settings;
                } else {
                    std::string temp = appList.at(selectedExec).execPath;
                    std::vector<char*> args;
                    args.push_back(&temp[0]);
                    args.push_back(0);
                    pid_t child_pid = fork();
                    if (child_pid == 0) {
                        /* child */
                        ::execv(args[0], &(args[0]));
                        ::perror("execv");
                        ::exit(-1);
                    }
                    else if (child_pid == -1) {
                        ::perror("fork");
                    }
                    else {
                        stop();
                    }
                }
            }

            for (uint i = 0; i < appList.size(); i++) {
                int yPos = 29 + ((i - selectedExec) * 7) + animationOffset;
                Color textColor = appList.at(i).color;
                if (i == (uint) selectedExec)
                    textColor = colSelectedText;
                for (uint screenCounter = 0; screenCounter < 4; screenCounter++) {
                    if (yPos < CUBEMAXINDEX && yPos > 0) {
                        drawText((ScreenNumber) screenCounter, Vector2i(CharacterBitmaps::centered, yPos), textColor, appList.at(i).name);
                    }
                }
            }

            drawRect2D(top, 10, 10, 53, 53, colSelectedText);
            drawText(top, Vector2i(CharacterBitmaps::centered, 22), colSelectedText, "DOT");
            drawText(top, Vector2i(CharacterBitmaps::centered, 30), colSelectedText, "THE");
            drawText(top, Vector2i(CharacterBitmaps::centered, 38), colSelectedText, "LEDCUBE");
        }
            break;
        case settings: {
            drawText(top, Vector2i(CharacterBitmaps::centered, 30), Color::blue(), "Settings");

            selectedExec += (int) joystickmngr.getAxisPress(1);
            if (selectedExec < 0) {
                selectedExec = settingsList.size() - 1;
            } else {
                selectedExec %= settingsList.size();
            }
            if(lastSelectedExec != selectedExec){
                animationOffset = (selectedExec-lastSelectedExec)*7;
            }
            lastSelectedExec = selectedExec;
            animationOffset *= 0.85;

            if (joystickmngr.getButtonPress(0)) {
                if (settingsList.at(selectedExec).execPath == "return") {
                    menuState = applist;
                    selectedExec = appList.size() - 1;
                    lastSelectedExec = selectedExec;
                    animationOffset = 0;
                } else if (settingsList.at(selectedExec).execPath == "shutdown") {
                    systemShutdown();
                } else if (settingsList.at(selectedExec).execPath == "update") {
                    menuState = settingsUpdate;
                    auto temp = std::string("/usr/local/sbin/Update.sh 1>/dev/null 2>/dev/null &");
                    temp = std::string("nohup ") + temp;
                    system(temp.data());
                }
            }

#define JS_DEADZONE 0.2
            if (menuState == settings && settingsList.at(selectedExec).execPath == "brightness") {
                settingsList.at(selectedExec).name = "brightness: " + std::to_string((int)ceil(bright));
                auto val = joystickmngr.getAxis(0);
                if (abs(val) > JS_DEADZONE) {
                    accel_bright += val * 0.03;
                    bright += accel_bright;
                    if (bright > 100.0) bright = 100.0;
                    if (bright < 10.0) bright = 10.0;
                    setBrightness(ceil(bright));
                }
                else {
                    accel_bright = 0.0;
                }
            }

            if (menuState == settings && settingsList.at(selectedExec).execPath == "volume") {
                settingsList.at(selectedExec).name = "volume: " + std::to_string((int)ceil(vol));
                auto val = joystickmngr.getAxis(0);
                if (abs(val) > JS_DEADZONE) {
                    accel_vol += val * 0.03;
                    vol += accel_vol;
                    if (vol > 100.0) vol = 100.0;
                    if (vol < 0.0) vol = 0.0;
                    setVolume(ceil(vol));
                }
                else {
                    accel_vol = 0.0;
                }
            }

            for (uint i = 0; i < settingsList.size(); i++) {
                int yPos = 29 + ((i - selectedExec) * 7) + animationOffset;
                Color textColor = settingsList.at(i).color;
                if (i == (uint) selectedExec)
                    textColor = colSelectedText;
                for (uint screenCounter = 0; screenCounter < 4; screenCounter++) {
                    if (yPos < CUBEMAXINDEX && yPos > 0) {
                        drawText((ScreenNumber) screenCounter, Vector2i(CharacterBitmaps::centered, yPos), textColor, settingsList.at(i).name);
                    }
                }
            }
        }
            break;
        case settingsUpdate:
            for (uint screenCounter = 0; screenCounter < 5; screenCounter++) {
                drawText((ScreenNumber) screenCounter, Vector2i(CharacterBitmaps::centered, 22), colSelectedText, "Update");
                drawText((ScreenNumber) screenCounter, Vector2i(CharacterBitmaps::centered, 30), colSelectedText, "in");
                drawText((ScreenNumber) screenCounter, Vector2i(CharacterBitmaps::centered, 38), colSelectedText, "progress");
            }
            break;

    }

    //auto battValue = adcBattery.getVoltage();
    auto battPercent = static_cast<float>(adcBattery.getPercentage()) / 100.0f;
    std::stringstream sstime;
    sstime << std::put_time(&tm, "%H:%M  %b %d %y");

    for (uint screenCounter = 0; screenCounter < 4; screenCounter++) {
        drawRect2D((ScreenNumber) screenCounter, 0, 0, CUBEMAXINDEX, 6, Color::black(), true, Color::black());
        drawRect2D((ScreenNumber) screenCounter, 0, CUBEMAXINDEX-6, CUBEMAXINDEX, CUBEMAXINDEX, Color::black(), true, Color::black());

        drawText((ScreenNumber) screenCounter, Vector2i(CharacterBitmaps::left, 58), colVoltageText, sstime.str() );
        //drawText((ScreenNumber) screenCounter, Vector2i(CharacterBitmaps::right, 58), colVoltageText, std::to_string(battValue).substr(0, 5) + " V");
        drawText((ScreenNumber) screenCounter, Vector2i(CharacterBitmaps::left, 1), colVoltageText, hostname);

        /* Draw battery indicator */
        int bat_lvl;
        Color bat_color, bat_outline;

        if (charging) {
            /* display scaning battery animation when charging */
            bat_color = Color::green();
            bat_outline = Color::white();
            bat_lvl = (loopcount >> 5) % 10;
        }
        else {
            bat_lvl = std::nearbyint(battPercent * 9.0f);
            uint8_t bat_color_g = std::nearbyint(battPercent * 255.0f);
            uint8_t bat_color_r = 255 - bat_color_g;
            bat_color = Color(bat_color_r, bat_color_g, 0);

            if (battPercent < 0.05) {
                /* blinking red/white battery outline when low on charge */
                if ((loopcount >> 6) & 1)
                    bat_outline = Color::red();
                else
                    bat_outline = Color::white();
            }
            else {
                bat_outline = Color::white();
            }
        }

        drawBitmap1bpp((ScreenNumber) screenCounter, Vector2i(CUBEMAXINDEX-11, 0), bat_outline, battery_indicator);
        if (bat_lvl)
            drawRect2D((ScreenNumber) screenCounter, CUBEMAXINDEX-10, 1, CUBEMAXINDEX-11+bat_lvl, 4, bat_color, true, bat_color);
        if (charging)
            drawText((ScreenNumber) screenCounter, Vector2i(CUBEMAXINDEX-7, 0), Color::white(), std::string("+") );
    }

    joystickmngr.clearAllButtonPresses();

    render();
    loopcount++;
    return true;
}

MainMenu::AppListItem::AppListItem(std::string setName, std::string setExecPath, bool setInternal, Color setColor) {
    name = setName;
    execPath = setExecPath;
    color = setColor;
    isInternal = setInternal;
};
