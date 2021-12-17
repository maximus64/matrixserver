#include <vector>
#include <iostream>
#include <future>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <sys/time.h>
#include <random>
#include <chrono>
#include <unistd.h>
#include <alsa/asoundlib.h>
#include <alsa/pcm.h>
#include "Server.h"

namespace {
    static std::string defaultApp("/usr/bin/MainMenu");
    static bool defaultAppStarted = false;
}

App *Server::getAppByID(int searchID) {
    for (unsigned int i = 0; i < apps.size(); i++) {
        if (apps[i].getAppId() == searchID) {
            return &apps[i];
        }
    }
    return nullptr;
}

Server::Server(std::shared_ptr<IRenderer> setRenderer, matrixserver::ServerConfig &setServerConfig) :
        ioContext(),
        serverConfig(setServerConfig),
        tcpServer(ioContext, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), std::stoi(setServerConfig.serverconnection().serverport()))),
//        unixServer(ioContext, boost::asio::local::stream_protocol::endpoint("/tmp/matrixserver.sock")),
        ipcServer("matrixserver"),
        joystickmngr(8),
        shutdown_server_(false) {
    boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::debug);
    renderers.push_back(setRenderer);
    tcpServer.setAcceptCallback(std::bind(&Server::newConnectionCallback, this, std::placeholders::_1));
//    unixServer.setAcceptCallback(std::bind(&Server::newConnectionCallback, this, std::placeholders::_1));
    ipcServer.setAcceptCallback(std::bind(&Server::newConnectionCallback, this, std::placeholders::_1));
    ioThread = new boost::thread([this]() { this->ioContext.run(); });
    std::random_device rd;
    srand(rd());
    updateConfig();
}

void Server::newConnectionCallback(std::shared_ptr<UniversalConnection> connection) {
    BOOST_LOG_TRIVIAL(debug) << "[matrixserver] NEW SocketConnection CALLBACK!";
    connection->setReceiveCallback(
            std::bind(&Server::handleRequest, this, std::placeholders::_1, std::placeholders::_2));
    connections.push_back(connection);
}

void Server::handleRequest(std::shared_ptr<UniversalConnection> connection, std::shared_ptr<matrixserver::MatrixServerMessage> message) {
    switch (message->messagetype()) {
        case matrixserver::registerApp:
            if (message->appid() == 0) {
                BOOST_LOG_TRIVIAL(debug) << "[matrixserver] register new App request received";
                apps.push_back(App(connection));
                auto response = std::make_shared<matrixserver::MatrixServerMessage>();
                response->set_appid(apps.back().getAppId());
                response->set_messagetype(matrixserver::registerApp);
                response->set_status(matrixserver::success);
                connection->sendMessage(response);
            }
            break;
        case matrixserver::getServerInfo: {
            BOOST_LOG_TRIVIAL(debug) << "[matrixserver] get ServerInfo request received";
            auto response = std::make_shared<matrixserver::MatrixServerMessage>();
            response->set_messagetype(matrixserver::getServerInfo);
            auto *tempServerConfig = new matrixserver::ServerConfig();
            tempServerConfig->CopyFrom(serverConfig);
            response->set_allocated_serverconfig(tempServerConfig);
            connection->sendMessage(response);
            break;
        }
        case matrixserver::requestScreenAccess:
            //TODO App level logic, set App on top, pause all other Apps, which aren't on top any more
            break;
        case matrixserver::setScreenFrame:
            if (message->appid() == apps.back().getAppId()) {
                for (auto renderer : renderers) {
                    for (auto screenInfo : message->screendata()) {
                        renderer->setScreenData(screenInfo.screenid(), (Color *) screenInfo.framedata().data()); //TODO: remove C style cast
                    }
                    auto usStart = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
//                  std::thread([renderer](){renderer->render();}).detach();
                    renderer->render();
                    auto response = std::make_shared<matrixserver::MatrixServerMessage>();
                    response->set_messagetype(matrixserver::setScreenFrame);
                    response->set_status(matrixserver::success);
                    connection->sendMessage(response);
                    auto usTotal = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()) - usStart;
                    //BOOST_LOG_TRIVIAL(debug) << "[Server] rendertime: " << usTotal.count() << " us"; // ~ 15ms
                }
            } else {
                //send app to pause
                BOOST_LOG_TRIVIAL(debug) << "[Server] send app " << message->appid() << " to pause";
                auto msg = std::make_shared<matrixserver::MatrixServerMessage>();
                msg->set_messagetype(matrixserver::appKill);
                connection->sendMessage(msg);
            }
            break;
        case matrixserver::appAlive:
        case matrixserver::appPause:
        case matrixserver::appResume:
        case matrixserver::appKill:
            BOOST_LOG_TRIVIAL(debug) << "[Server] appkill " << message->appid() << " successfull";
            apps.erase(std::remove_if(apps.begin(), apps.end(), [message](App a) {
                if (a.getAppId() == message->appid()) {
                    BOOST_LOG_TRIVIAL(debug) << "[Server] App " << message->appid() << " deleted";
                    return true;
                } else {
                    return false;
                }
            }), apps.end());
            break;
        case matrixserver::shutDown:
            BOOST_LOG_TRIVIAL(debug) << "[Server] shutdown request by: " << message->appid();
            shutdown_server_ = true;
            break;
        default:
            break;
    }

    if (message->has_serverconfig()) {
        serverConfig.CopyFrom(message->serverconfig());
        updateConfig();
    }
}

static void SetAlsaMasterVolume(int volume) {
    long min, max, range;
    snd_mixer_t *handle;
    snd_mixer_selem_id_t *sid;
    const char *card = "default";
    const char *selem_name = "Master";
    int x, mute_state;
    long i;
    int ret;
    double vol;

    static bool once;

    /*
     * HACK HACK HACK: Dirty trick to make alsa create the softvol
     * volume control interface. Without this hack, the volume control
     * isn't available upon start up. Need to investigate why?
     */
    if (!once) {
        snd_pcm_t *pcm_h;
        int err;
        if ((err = snd_pcm_open(&pcm_h, card, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
            printf("Playback open error: %s\n", snd_strerror(err));
            return;
        }
        snd_pcm_close(pcm_h);
        once = true;
    }

    snd_mixer_open(&handle, 0);
    snd_mixer_attach(handle, card);
    snd_mixer_selem_register(handle, NULL, NULL);
    snd_mixer_load(handle);
    
    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, selem_name);
    snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

    if (!elem) {
        BOOST_LOG_TRIVIAL(error) << "Cannot find Master volume ctrl";
        goto out;
    }

    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);

    vol = volume / 100.0;
    range = max - min;
    vol = (vol * range) + min;

    snd_mixer_selem_set_playback_volume_all(elem, (long)vol);

out:
    snd_mixer_close(handle);
}

void Server::updateConfig(void) {
    for (auto renderer : renderers) {
        renderer->setGlobalBrightness(serverConfig.globalscreenbrightness());
    }
    SetAlsaMasterVolume(serverConfig.globalvolume());
}


matrixserver::ServerConfig &Server::getServerConfig(void) {
    return serverConfig;
}

bool Server::tick() {
    if (joystickmngr.getButtonPress(11)) {
        if (apps.size() > 0) {
            BOOST_LOG_TRIVIAL(debug) << "kill current app" << std::endl;
            auto msg = std::make_shared<matrixserver::MatrixServerMessage>();
            msg->set_messagetype(matrixserver::appKill);
            apps.back().sendMsg(msg);
        }
    }
    joystickmngr.clearAllButtonPresses();

    if (apps.size() == 0 && !defaultAppStarted) {
        BOOST_LOG_TRIVIAL(debug) << "starting default app" << std::endl;
        std::vector<char*> args;
        args.push_back(&defaultApp[0]);
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
            defaultAppStarted = true;
        }
    }
    if (apps.size() > 0) {
        defaultAppStarted = false;
    }

    apps.erase(std::remove_if(apps.begin(), apps.end(), [](App a) {
        bool returnVal = a.getConnection()->isDead();
        if (returnVal)
            BOOST_LOG_TRIVIAL(debug) << "[matrixserver] App " << a.getAppId() << " deleted";
        return returnVal;
    }), apps.end());

    connections.erase(std::remove_if(connections.begin(), connections.end(), [](std::shared_ptr<UniversalConnection> con) {
        bool returnVal = con->isDead();
        if (returnVal) {
            BOOST_LOG_TRIVIAL(debug) << "[matrixserver] Connection deleted";
        }
        return returnVal;
    }), connections.end());

    if (shutdown_server_) {
        ioContext.stop();
        ioThread->join();
        return false;
    }

    return true;
}

void Server::addRenderer(std::shared_ptr<IRenderer> newRenderer) {
    renderers.push_back(newRenderer);
    updateConfig();
}
