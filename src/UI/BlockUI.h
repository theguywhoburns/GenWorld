#ifndef BLOCKUI_H
#define BLOCKUI_H

#include "GeneratorUI.h"
#include "../Core/Engine/Application.h"
#include "../Controllers/BlockController.h"
#include "../Core/BlockData.h"


class BlockUI : GeneratorUI {
public:
    BlockUI(BlockController* controller);
    ~BlockUI();
    void DisplayUI() override;

    void SetParameters(BlockUtilities::BlockData params) {
        parameters = params;
    }

    BlockUtilities::BlockData GetParameters() {
        return parameters;
    }

private:
    BlockUtilities::BlockData parameters;
    BlockController* controller;

};

#endif