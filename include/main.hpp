#pragma once

#include "beatsaber-hook/shared/utils/logging.hpp"

constexpr auto logger = Paper::ConstLoggerContext(MOD_ID);

#include "UnityEngine/Sprite.hpp"

UnityW<UnityEngine::Sprite> GetCapsLockSprite();
