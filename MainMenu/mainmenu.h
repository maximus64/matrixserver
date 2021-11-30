#ifndef SNAKE_PIXELFLOW_H
#define SNAKE_PIXELFLOW_H

#include <CubeApplication.h>

#include "Joystick.h"
#include "BattSensor.h"

#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

class MainMenu : public CubeApplication {
public:
    MainMenu();

    bool loop();

private:
    class AppListItem;

    JoystickManager joystickmngr;
    std::vector<AppListItem> appList;
    std::vector<AppListItem> settingsList;
    std::string searchDirectory;
    BattSensor adcBattery;
};

class MainMenu::AppListItem {
public:
    AppListItem(std::string setName, std::string setExecPath, bool setInternal = false, Color setColor = Color::white());

    std::string name;
    std::string execPath;
    bool isInternal = false;
    Color color = Color::white();
};

#endif //SNAKE_PIXELFLOW_H
