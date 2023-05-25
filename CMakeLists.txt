cmake_minimum_required(VERSION 3.2.2)
project(binocle-c-examples)

include(ExternalProject)

ExternalProject_Add(basic
        DOWNLOAD_COMMAND ""
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/basic
        INSTALL_COMMAND cmake -E echo "Skipping install step."
        )

ExternalProject_Add(ecs
        DOWNLOAD_COMMAND ""
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/ecs
        INSTALL_COMMAND cmake -E echo "Skipping install step."
        )

ExternalProject_Add(lua
        DOWNLOAD_COMMAND ""
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/lua
        INSTALL_COMMAND cmake -E echo "Skipping install step."
        )

ExternalProject_Add(model
        DOWNLOAD_COMMAND ""
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/model
        INSTALL_COMMAND cmake -E echo "Skipping install step."
        )

ExternalProject_Add(pbr
        DOWNLOAD_COMMAND ""
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/pbr
        INSTALL_COMMAND cmake -E echo "Skipping install step."
        )

ExternalProject_Add(physics
        DOWNLOAD_COMMAND ""
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/physics
        INSTALL_COMMAND cmake -E echo "Skipping install step."
        )

ExternalProject_Add(rendertarget
        DOWNLOAD_COMMAND ""
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/rendertarget
        INSTALL_COMMAND cmake -E echo "Skipping install step."
        )

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

