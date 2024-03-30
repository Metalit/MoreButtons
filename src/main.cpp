#include "main.hpp"

#include <map>

#include "GlobalNamespace/UIKeyboardManager.hpp"
#include "HMUI/UIKeyboard.hpp"
#include "HMUI/UIKeyboardKey.hpp"
#include "System/Action_1.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/UI/HorizontalLayoutGroup.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "scotland2/shared/modloader.h"

static constexpr int numberOffset = 256;
static constexpr int stateSwitchStart = -10;
static std::map<HMUI::UIKeyboard*, int> keyboardState = {};

static std::vector<std::vector<char>> const stateNumpads = {
    {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'},
    {'@', '&', '"', '\'', '!', ':', ';', '?', '.', ','},
    {'=', '+', '{', '}', '-', '[', ']', '*', '(', ')'},
    {'$', '%', '~', '`', '#', '<', '>', '_', '/', '\\'},
};
static std::vector<std::vector<int>> const stateSwitches = {
    {1, 2},
    {0, 3},
    {0, 3},
    {1, 2},
};
static std::vector<std::string> const stateIcons = {
    "123",
    "?!",
    "+=",
    "/_",
};

void UpdateKeyboardState(HMUI::UIKeyboard* keyboard) {
    auto numpad = keyboard->transform->Find("Numpad");

    for (auto key : numpad->GetComponentsInChildren<HMUI::UIKeyboardKey*>(true)) {
        int castCode = (int) key->_keyCode;
        if (castCode > stateSwitchStart)
            key->_overrideText = std::string_view(&stateNumpads[keyboardState[keyboard]][-castCode], 1);
        else
            key->_overrideText = stateIcons[stateSwitches[keyboardState[keyboard]][stateSwitchStart - castCode]];
        key->Awake();
    }
}

void SwitchKeyboardState(HMUI::UIKeyboard* keyboard, int switchIdx) {
    int const state = keyboardState[keyboard];

    keyboardState[keyboard] = stateSwitches[state][switchIdx];

    UpdateKeyboardState(keyboard);
}

MAKE_HOOK_MATCH(UIKeyboard_Awake, &HMUI::UIKeyboard::Awake, void, HMUI::UIKeyboard* self) {

    auto numpad = self->transform->Find("Numpad");

    for (auto key : numpad->GetComponentsInChildren<HMUI::UIKeyboardKey*>(true)) {
        if ((int) key->_keyCode >= numberOffset && (int) key->_keyCode < numberOffset + 10)
            key->_keyCode = (UnityEngine::KeyCode) - ((int) key->_keyCode - numberOffset);
    }

    auto baseKey = numpad->Find("Row/0");
    auto clone1 = UnityEngine::Object::Instantiate(baseKey);
    auto clone2 = UnityEngine::Object::Instantiate(baseKey);

    auto bottomRow = baseKey->parent;

    clone1->SetParent(bottomRow, false);
    clone2->SetParent(bottomRow, false);
    clone1->GetComponent<HMUI::UIKeyboardKey*>()->_keyCode = stateSwitchStart;
    clone2->GetComponent<HMUI::UIKeyboardKey*>()->_keyCode = stateSwitchStart - 1;

    auto layout = bottomRow->GetComponent<UnityEngine::UI::HorizontalLayoutGroup*>();
    layout->CalculateLayoutInputHorizontal();
    layout->SetLayoutHorizontal();

    keyboardState[self] = 0;
    UpdateKeyboardState(self);

    UIKeyboard_Awake(self);
}

MAKE_HOOK_MATCH(UIKeyboardManager_OpenKeyboardFor,
    &GlobalNamespace::UIKeyboardManager::OpenKeyboardFor,
    void,
    GlobalNamespace::UIKeyboardManager* self,
    HMUI::InputFieldView* input) {

    UIKeyboardManager_OpenKeyboardFor(self, input);

    keyboardState[self->_uiKeyboard] = 0;
    UpdateKeyboardState(self->_uiKeyboard);
}

MAKE_HOOK_MATCH(UIKeyboard_HandleKeyPress, &HMUI::UIKeyboard::HandleKeyPress, void, HMUI::UIKeyboard* self, UnityEngine::KeyCode keyCode) {

    UIKeyboard_HandleKeyPress(self, keyCode);

    int castCode = (int) keyCode;
    if (castCode > 0 || !keyboardState.contains(self))
        return;

    if (castCode > stateSwitchStart)
        self->keyWasPressedEvent->Invoke(stateNumpads[keyboardState[self]][-castCode]);
    else
        SwitchKeyboardState(self, stateSwitchStart - castCode);
}

static modloader::ModInfo modInfo = {MOD_ID, VERSION, 0};

extern "C" void setup(CModInfo* info) {
    info->id = MOD_ID;
    info->version = VERSION;
    modInfo.assign(*info);

    logger.info("Completed setup!");
}

extern "C" void late_load() {
    il2cpp_functions::Init();

    logger.info("Installing hooks...");
    INSTALL_HOOK(logger, UIKeyboard_Awake);
    INSTALL_HOOK(logger, UIKeyboardManager_OpenKeyboardFor);
    INSTALL_HOOK(logger, UIKeyboard_HandleKeyPress);
    logger.info("Installed all hooks!");
}
