cmake_minimum_required(VERSION 3.0.2)

include(ExternalProject)

ExternalProject_Add(spatialhash
    DOWNLOAD_COMMAND ""
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/spatialhash
    INSTALL_COMMAND cmake -E echo "Skipping install step."
)

ExternalProject_Add(spritebatch
    DOWNLOAD_COMMAND ""
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/spritebatch
    INSTALL_COMMAND cmake -E echo "Skipping install step."
)

ExternalProject_Add(ecs
        DOWNLOAD_COMMAND ""
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/ecs
        INSTALL_COMMAND cmake -E echo "Skipping install step."
)