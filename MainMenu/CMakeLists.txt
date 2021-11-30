project(MainMenu)

set(MAINSRC
        main.cpp
        mainmenu.cpp)

set(MAINLIBS
        matrixapplication
        common
        stdc++fs
        )

add_executable(MainMenu ${MAINSRC})
target_link_libraries(MainMenu ${MAINLIBS})

install(TARGETS MainMenu DESTINATION bin)
