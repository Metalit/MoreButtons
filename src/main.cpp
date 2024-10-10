#include "main.hpp"

#include <map>

#include "GlobalNamespace/UIKeyboardManager.hpp"
#include "HMUI/ButtonSpriteSwapToggle.hpp"
#include "HMUI/NoTransitionsButton.hpp"
#include "HMUI/UIKeyboard.hpp"
#include "HMUI/UIKeyboardKey.hpp"
#include "System/Action_1.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/UI/HorizontalLayoutGroup.hpp"
#include "UnityEngine/UI/Image.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "scotland2/shared/modloader.h"

static constexpr int numberOffset = 256;
static constexpr int stateSwitchStart = -10;
static std::map<HMUI::UIKeyboard*, std::pair<int, bool>> keyboardState = {};

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

static void UpdateKeyboardState(HMUI::UIKeyboard* keyboard) {
    auto numpad = keyboard->transform->Find("Numpad");

    for (auto key : numpad->GetComponentsInChildren<HMUI::UIKeyboardKey*>(true)) {
        int castCode = (int) key->_keyCode;
        if (castCode > stateSwitchStart)
            key->_overrideText = std::string_view(&stateNumpads[keyboardState[keyboard].first][-castCode], 1);
        else
            key->_overrideText = stateIcons[stateSwitches[keyboardState[keyboard].first][stateSwitchStart - castCode]];
        key->Awake();
    }
}

static void SwitchKeyboardState(HMUI::UIKeyboard* keyboard, int switchIdx) {
    int const state = keyboardState[keyboard].first;

    keyboardState[keyboard].first = stateSwitches[state][switchIdx];

    UpdateKeyboardState(keyboard);
}

static void UpdateShiftKey(HMUI::UIKeyboard* keyboard) {
    bool const shifted = keyboardState[keyboard].second;

    auto shift = keyboard->transform->Find("Letters/Row/ShiftKey");
    if (!shift)
        return;
    auto sprites = shift->GetComponent<HMUI::ButtonSpriteSwapToggle*>();
    auto sprite = shifted ? sprites->_pressedStateSprite : (keyboard->_shouldCapitalize ? GetCapsLockSprite() : sprites->_normalStateSprite);
    for (auto& image : sprites->_images)
        image->sprite = sprite;
}

static void DisableKeyboardShift(HMUI::UIKeyboard* keyboard) {
    bool const shifted = keyboardState[keyboard].second;
    if (!shifted)
        return;

    keyboardState[keyboard].second = false;
    keyboard->HandleCapsLockPressed();

    UpdateShiftKey(keyboard);
}

MAKE_HOOK_MATCH(UIKeyboard_Awake, &HMUI::UIKeyboard::Awake, void, HMUI::UIKeyboard* self) {
    auto numpad = self->transform->Find("Numpad");
    auto shift = self->transform->Find("Letters/Row/ShiftKey");
    if (!numpad || !shift)
        return;

    for (auto key : numpad->GetComponentsInChildren<HMUI::UIKeyboardKey*>(true)) {
        if ((int) key->_keyCode >= numberOffset && (int) key->_keyCode < numberOffset + 10)
            key->_keyCode = (UnityEngine::KeyCode) - ((int) key->_keyCode - numberOffset);
    }

    auto baseKey = numpad->Find("Row/0");
    auto clone1 = UnityEngine::Object::Instantiate(baseKey);
    clone1->name = "Switch1";
    auto clone2 = UnityEngine::Object::Instantiate(baseKey);
    clone2->name = "Switch2";

    auto bottomRow = baseKey->parent;

    clone1->SetParent(bottomRow, false);
    clone2->SetParent(bottomRow, false);
    clone1->GetComponent<HMUI::UIKeyboardKey*>()->_keyCode = stateSwitchStart;
    clone2->GetComponent<HMUI::UIKeyboardKey*>()->_keyCode = stateSwitchStart - 1;

    auto layout = bottomRow->GetComponent<UnityEngine::UI::HorizontalLayoutGroup*>();
    layout->CalculateLayoutInputHorizontal();
    layout->SetLayoutHorizontal();

    shift->GetComponent<HMUI::ButtonSpriteSwapToggle*>()->enabled = false;

    keyboardState[self] = {0, false};
    UpdateKeyboardState(self);

    UIKeyboard_Awake(self);
}

MAKE_HOOK_MATCH(
    UIKeyboardManager_OpenKeyboardFor,
    &GlobalNamespace::UIKeyboardManager::OpenKeyboardFor,
    void,
    GlobalNamespace::UIKeyboardManager* self,
    HMUI::InputFieldView* input
) {
    UIKeyboardManager_OpenKeyboardFor(self, input);

    if (!keyboardState.contains(self->_uiKeyboard))
        return;

    keyboardState[self->_uiKeyboard] = {0, false};
    UpdateKeyboardState(self->_uiKeyboard);
}

MAKE_HOOK_MATCH(UIKeyboard_HandleKeyPress, &HMUI::UIKeyboard::HandleKeyPress, void, HMUI::UIKeyboard* self, UnityEngine::KeyCode keyCode) {
    UIKeyboard_HandleKeyPress(self, keyCode);

    if (!keyboardState.contains(self))
        return;

    int const castCode = (int) keyCode;
    if (castCode > 0)
        DisableKeyboardShift(self);
    else if (castCode > stateSwitchStart)
        self->keyWasPressedEvent->Invoke(stateNumpads[keyboardState[self].first][-castCode]);
    else
        SwitchKeyboardState(self, stateSwitchStart - castCode);
}

MAKE_HOOK_MATCH(UIKeyboard_HandleCapsLockPressed, &HMUI::UIKeyboard::HandleCapsLockPressed, void, HMUI::UIKeyboard* self) {
    if (!keyboardState.contains(self)) {
        UIKeyboard_HandleCapsLockPressed(self);
        return;
    }

    bool const shifted = keyboardState[self].second;

    if (!shifted && !self->_shouldCapitalize) {
        keyboardState[self].second = true;
        UIKeyboard_HandleCapsLockPressed(self);
    }
    else if (shifted)
        keyboardState[self].second = false;
    else
        UIKeyboard_HandleCapsLockPressed(self);

    UpdateShiftKey(self);
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
    INSTALL_HOOK(logger, UIKeyboard_HandleCapsLockPressed);
    logger.info("Installed all hooks!");
}
