#ifndef MATRIXSERVER_MATRIXAPPLICATION_H
#define MATRIXSERVER_MATRIXAPPLICATION_H

#include <boost/thread/thread.hpp>

#include <Screen.h>
#include <TcpClient.h>
#include <UnixSocketClient.h>
#include <IpcConnection.h>
#include <mutex>

#define DEFAULTFPS 40
#define MAXFPS 200
#define MINFPS 1

#define DEFAULTSERVERADRESS "127.0.0.1"
#define DEFAULTSERVERPORT "2017"

enum class AppState {
    starting, running, paused, ended, killed, failure
};

class MatrixApplication {
public:
    MatrixApplication(
            int fps = DEFAULTFPS,
            std::string setServerAddress = DEFAULTSERVERADRESS,
            std::string setServerPort = DEFAULTSERVERPORT);

    ~MatrixApplication() = default;

    void renderToScreens();

    int getFps();

    void setFps(int fps);

    AppState getAppState();

    float getLoad();

    void start();

    bool pause();

    bool resume();

    void stop();

    int getBrightness();

    void setBrightness(int setBrightness);

    virtual bool loop() = 0;

    bool isStopped() {
        return stopped;
    }

protected:
    std::vector<std::shared_ptr<Screen>> screens;

    long micros();

private:
    void internalLoop();

    bool connect(const std::string &serverAddress, const std::string &serverPort);

    void checkConnection();

    void registerAtServer();

    void handleRequest(std::shared_ptr<UniversalConnection>, std::shared_ptr<matrixserver::MatrixServerMessage>);

    bool stopped;

    int appId;
    int fps;
    float load;

    int brightness;
    std::string serverAddress;
    std::string serverPort;
    std::shared_ptr<UniversalConnection> connection;
    boost::thread *mainThread;
    boost::thread *ioThread;
    AppState appState;
    boost::asio::io_service io_context;
    matrixserver::ServerConfig serverConfig;

    std::atomic<bool> renderSync;
};


#endif //MATRIXSERVER_MATRIXAPPLICATION_H
