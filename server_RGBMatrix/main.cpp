#include <iostream>
#include <fstream>
#include <vector>
#include <boost/log/trivial.hpp>

#include <Server.h>
#include <RGBMatrixRenderer.h>
#include <SimulatorRenderer.h>
#include <TcpServer.h>

#include <matrixserver.pb.h>
#include <google/protobuf/util/json_util.h>

#define DEFAULT_CONFIG_PATH "/etc/matrixserver.conf"

static bool should_stop;

static void createDefaultCubeConfig(matrixserver::ServerConfig &serverConfig) {
    const int screen_map[6] = {2, 3, 0, 1, 4, 5};
    serverConfig.Clear();
    serverConfig.set_globalscreenbrightness(100);
    serverConfig.set_globalvolume(100);
    serverConfig.set_servername("matrixserver");
    matrixserver::Connection *serverConnection = new matrixserver::Connection();
    serverConnection->set_serveraddress("127.0.0.1");
    serverConnection->set_serverport("2017");
    serverConnection->set_connectiontype(matrixserver::Connection_ConnectionType_tcp);
    serverConfig.set_allocated_serverconnection(serverConnection);
    serverConfig.set_assemblytype(matrixserver::ServerConfig_AssemblyType_cube);
    for (int i = 0; i < 6; i++) {
        auto screenInfo = serverConfig.add_screeninfo();
        screenInfo->set_screenid(i);
        screenInfo->set_available(true);
        screenInfo->set_height(64);
        screenInfo->set_width(64);
        screenInfo->set_screenorientation((matrixserver::ScreenInfo_ScreenOrientation) (screen_map[i] + 1));
    }
}

static void saveServerConfig(const std::string &configFileName, matrixserver::ServerConfig &serverConfig) {
    BOOST_LOG_TRIVIAL(debug) << "[Server] saving server config";
    std::string configString;
    google::protobuf::util::JsonOptions jsonOptions;
    jsonOptions.add_whitespace = true;
    jsonOptions.always_print_primitive_fields = true;
    if (!google::protobuf::util::MessageToJsonString(serverConfig, &configString, jsonOptions).ok()) {
        BOOST_LOG_TRIVIAL(debug) << "[Server] fail to convert config to json:";
        return;
    }

    std::ofstream configFileWriteStream(configFileName, std::ios_base::trunc);
    if (!configFileWriteStream.good()) {
        BOOST_LOG_TRIVIAL(debug) << "[Server] cannot save config to: " << configFileName;
        return;
    }
    configFileWriteStream << configString;
    BOOST_LOG_TRIVIAL(debug) << "[Server] config written to " << configFileName;
}

static void signalHandler( int signum ) {
    BOOST_LOG_TRIVIAL(debug) << "Interrupt signal (" << signum << ") received.\n";

    should_stop = true;
}

int main(int argc, char **argv) {
    std::string config_path;
    matrixserver::ServerConfig serverConfig;

    if (argc == 2)
        config_path = std::string(argv[1]);
    else
        config_path = std::string(DEFAULT_CONFIG_PATH);

    BOOST_LOG_TRIVIAL(debug) << "[Server] Trying to read config from: " << config_path;
    std::ifstream configFileReadStream(config_path);
    std::stringstream buffer;
    if (configFileReadStream.good()) {
        buffer << configFileReadStream.rdbuf();
        if (google::protobuf::util::JsonStringToMessage(buffer.str(), &serverConfig).ok()) {
            BOOST_LOG_TRIVIAL(debug) << "[Server] ServerConfig successfully read from: " << config_path;
        } else {
            BOOST_LOG_TRIVIAL(debug) << "[Server] ServerConfig read failed from: " << config_path;
            BOOST_LOG_TRIVIAL(debug) << "[Server] use default config";
            createDefaultCubeConfig(serverConfig);
        }
    }
    else {
        BOOST_LOG_TRIVIAL(debug) << "[Server] use default config";
        createDefaultCubeConfig(serverConfig);
    }

    BOOST_LOG_TRIVIAL(info) << "ServerConfig: " << std::endl << serverConfig.DebugString() << std::endl;

    std::vector<std::shared_ptr<Screen>> screens;
    for (auto screenInfo : serverConfig.screeninfo()){
        auto screen = std::make_shared<Screen>(screenInfo.width(), screenInfo.height(), screenInfo.screenid());
        switch(screenInfo.screenorientation()){
            case matrixserver::ScreenInfo_ScreenOrientation::ScreenInfo_ScreenOrientation_front :
                screen->setOffsetX(1);
                screen->setOffsetY(0);
                screen->setRotation(Rotation::rot270);
                break;
            case matrixserver::ScreenInfo_ScreenOrientation::ScreenInfo_ScreenOrientation_right :
                screen->setOffsetX(2);
                screen->setOffsetY(1);
                screen->setRotation(Rotation::rot180);
                break;
            case matrixserver::ScreenInfo_ScreenOrientation::ScreenInfo_ScreenOrientation_back :
                screen->setOffsetX(1);
                screen->setOffsetY(1);
                screen->setRotation(Rotation::rot180);
                break;
            case matrixserver::ScreenInfo_ScreenOrientation::ScreenInfo_ScreenOrientation_left :
                screen->setOffsetX(0);
                screen->setOffsetY(1);
                screen->setRotation(Rotation::rot180);
                break;
            case matrixserver::ScreenInfo_ScreenOrientation::ScreenInfo_ScreenOrientation_top :
                screen->setOffsetX(0);
                screen->setOffsetY(0);
                screen->setRotation(Rotation::rot90);
                break;
            case matrixserver::ScreenInfo_ScreenOrientation::ScreenInfo_ScreenOrientation_bottom :
                screen->setOffsetX(2);
                screen->setOffsetY(0);
                screen->setRotation(Rotation::rot90);
                break;
            default:
                break;
        }
        screens.push_back(screen);
    }

    auto rendererRGBMatrix = std::make_shared<RGBMatrixRenderer>(screens);
    Server server(rendererRGBMatrix, serverConfig);

    ::signal(SIGINT, signalHandler);

    while (!should_stop && server.tick()) {
        sleep(1);
    };

    saveServerConfig(config_path, serverConfig);

    if (!should_stop) {
        /* Not from interrupt handler, must be powerof request from server */
        /* Send SIGUSR2 signal to init process to initiate system shutdown */
        ::kill(1, SIGUSR2);
    }

    return 0;
}
