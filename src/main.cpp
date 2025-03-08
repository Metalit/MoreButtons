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

// in the UnityEngine::KeyCode enum, numpad keys start at 256
static constexpr int numberOffset = 256;
// since we have 10 numpad keys, they go from 0 to -9, anything past that is a state switch button
static constexpr int stateSwitchStart = -10;
// in case of multiple keyboards, keep track of their custom numpad and one-time shift state
static std::map<HMUI::UIKeyboard*, std::pair<int, bool>> keyboardState = {};

// bottom to top contents of each state/numpad
static std::vector<std::vector<char>> const stateNumpads = {
    {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'},
    {'@', '&', '"', '\'', '!', ':', ';', '?', '.', ','},
    {'=', '+', '{', '}', '-', '[', ']', '*', '(', ')'},
    {'$', '%', '~', '`', '#', '<', '>', '_', '/', '\\'},
};
// first and second switch destinations for each state (two buttons on the bottom right)
static std::vector<std::vector<int>> const stateSwitches = {
    {1, 2},
    {0, 3},
    {0, 3},
    {1, 2},
};
// what is displayed for the destinations of the switch buttons
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
    // use default "on" sprite as one-time shift, and custom icon for caps lock
    auto sprite = shifted ? sprites->_pressedStateSprite : (keyboard->_shouldCapitalize ? GetCapsLockSprite() : sprites->_normalStateSprite);
    for (auto& image : sprites->_images)
        image->sprite = sprite;
}

static void DisableKeyboardShift(HMUI::UIKeyboard* keyboard) {
    bool const shifted = keyboardState[keyboard].second;
    // do nothing if already disabled
    if (!shifted)
        return;

    keyboardState[keyboard].second = false;
    keyboard->HandleCapsLockPressed();

    UpdateShiftKey(keyboard);
}

MAKE_HOOK_MATCH(UIKeyboard_Awake, &HMUI::UIKeyboard::Awake, void, HMUI::UIKeyboard* self) {
    auto numpad = self->transform->Find("Numpad");
    auto shift = self->transform->Find("Letters/Row/ShiftKey");
    if (!numpad || !shift || keyboardState.contains(self)) {
        UIKeyboard_Awake(self);
        return;
    }

    // find the numpad keys and offset them so they go from 0 to -9
    for (auto key : numpad->GetComponentsInChildren<HMUI::UIKeyboardKey*>(true)) {
        if ((int) key->_keyCode >= numberOffset && (int) key->_keyCode < numberOffset + 10)
            key->_keyCode = (UnityEngine::KeyCode) - ((int) key->_keyCode - numberOffset);
    }

    // add buttons for the state switch
    // Awake should only be called once per keyboard
    auto baseKey = numpad->Find("Row/0");
    auto clone1 = UnityEngine::Object::Instantiate(baseKey);
    clone1->name = "Switch1";
    auto clone2 = UnityEngine::Object::Instantiate(baseKey);
    clone2->name = "Switch2";

    auto bottomRow = baseKey->parent;

    clone1->SetParent(bottomRow, false);
    clone2->SetParent(bottomRow, false);
    // set key codes to signify state switches
    clone1->GetComponent<HMUI::UIKeyboardKey*>()->_keyCode = stateSwitchStart;
    clone2->GetComponent<HMUI::UIKeyboardKey*>()->_keyCode = stateSwitchStart - 1;

    // update layout manually because unity ui is bad
    auto layout = bottomRow->GetComponent<UnityEngine::UI::HorizontalLayoutGroup*>();
    layout->CalculateLayoutInputHorizontal();
    layout->SetLayoutHorizontal();

    // disable sprite switcher since we do it manually
    shift->GetComponent<HMUI::ButtonSpriteSwapToggle*>()->enabled = false;

    // add self to map and make sure button displays are correct
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

    // reset back to normal numpad on open
    keyboardState[self->_uiKeyboard] = {0, false};
    UpdateKeyboardState(self->_uiKeyboard);
    DisableKeyboardShift(self->_uiKeyboard);
}

MAKE_HOOK_MATCH(UIKeyboard_HandleKeyPress, &HMUI::UIKeyboard::HandleKeyPress, void, HMUI::UIKeyboard* self, UnityEngine::KeyCode keyCode) {
    if ((int) keyCode > 0)
        UIKeyboard_HandleKeyPress(self, keyCode);

    if (!keyboardState.contains(self))
        return;

    // all our custom keys were set to have keycodes <= 0
    int const castCode = (int) keyCode;
    if (castCode > 0)
        DisableKeyboardShift(self);
    // pressed a number key, needs translation to actual character
    else if (castCode > stateSwitchStart)
        self->keyWasPressedEvent->Invoke(stateNumpads[keyboardState[self].first][-castCode]);
    // pressed a state switch
    else
        SwitchKeyboardState(self, stateSwitchStart - castCode);
}

MAKE_HOOK_MATCH(UIKeyboard_HandleCapsLockPressed, &HMUI::UIKeyboard::HandleCapsLockPressed, void, HMUI::UIKeyboard* self) {
    if (!keyboardState.contains(self)) {
        UIKeyboard_HandleCapsLockPressed(self);
        return;
    }

    // nothing -> one-time shift -> caps lock -> nothing
    // with the one-time shift, _shouldCapitalize is still true
    bool const shifted = keyboardState[self].second;

    // nothing
    if (!shifted && !self->_shouldCapitalize) {
        keyboardState[self].second = true;
        UIKeyboard_HandleCapsLockPressed(self);
    }
    // one-time shift
    else if (shifted)
        keyboardState[self].second = false;
    // caps lock
    else
        UIKeyboard_HandleCapsLockPressed(self);

    UpdateShiftKey(self);
}

static modloader::ModInfo modInfo = {MOD_ID, VERSION, 0};

extern "C" void setup(CModInfo* info) {
    *info = modInfo.to_c();
    Paper::Logger::RegisterFileContextId(MOD_ID);

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
