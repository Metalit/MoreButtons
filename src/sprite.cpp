#include "System/Convert.hpp"
#include "UnityEngine/ImageConversion.hpp"
#include "UnityEngine/Rect.hpp"
#include "UnityEngine/SpriteMeshType.hpp"
#include "UnityEngine/Texture2D.hpp"
#include "UnityEngine/TextureFormat.hpp"
#include "UnityEngine/Vector2.hpp"
#include "UnityEngine/Vector4.hpp"
#include "main.hpp"

extern char const* const arrowBase64;

static UnityEngine::Sprite* MakeCapsLockSprite() {
    auto data = System::Convert::FromBase64String(arrowBase64);

    auto texture = UnityEngine::Texture2D::New_ctor(0, 0, UnityEngine::TextureFormat::RGBA32, false, false);
    UnityEngine::ImageConversion::LoadImage(texture, data, false);

    auto ret = UnityEngine::Sprite::Create(
        texture,
        UnityEngine::Rect(0.0f, 0.0f, texture->width, texture->height),
        {0.5, 0.5},
        100,
        1,
        UnityEngine::SpriteMeshType::FullRect,
        {0, 0, 0, 0},
        false
    );

    UnityEngine::Object::DontDestroyOnLoad(ret);

    return ret;
}

static UnityW<UnityEngine::Sprite> capsLockSprite;

UnityW<UnityEngine::Sprite> GetCapsLockSprite() {
    if (!capsLockSprite)
        capsLockSprite = MakeCapsLockSprite();
    return capsLockSprite;
}

char const* const arrowBase64 =
    "iVBORw0KGgoAAAANSUhEUgAAAGUAAAB8CAYAAABwpX6cAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAADsIAAA7CARUoSoAAAAUVSURBVHhe7Zw7rFRFHMZ3MTFgYwyhApXCxhiBxABK1MRHSMAHCaESqMACoqWJjXTQYaPRQi2IPBowgIKJiVhgBHlUFDY0BDpjjI0Fibl+3xwel3uX3XPOnpn55tzvl3y5Z8K9u3vmx+7M/M+eGc7MzAyMFovu/OwDH9xJ8fTlnbIZOV0dDrYgZ6rDMumDlOeR88jjoTUY/IO8glwLrQIpXcoy5BKyMrTucwNZi/wZWoVR8piyGPkemSuEPI3w3/g7xVGqlCHyNbI+tEbDf+Pv8HeLolQpnyDbq8Ox8Hf2VYflUOKY8h5yGKn7DuAJ7kSOhFYBlCaFH0nnkMdCqz7/Iq8jv4eWOCVJeRK5inDG1QbOxF5AboaWMKWMKVyDcEHYVgjh3/Ix7q5nZClByiPItwgXidPCx+Bj8TFlKUHKQeSd6rAT+FifVoeaqI8pe5AvqsPO2Yt8WR1qoSyFs6WfkFgfNf8hGxHO5qRQlfIscgGJPSizePkS8kdoiaA4pnCWxDJ8ilkSn4PPNc2srnPUpDyKHEeeCa008Ln4nHxuCdSksID4anWYFD7nN9VhfpSkfIywRpWLHQhfQ3ZUBvqtCD9CcpfZ2RnbkO9CKxMKUtYhvyBNi4yxYPHyNYRXNLOQW8oKhFNf/lTiFsKpMn8mJ+eYwncGL9mqCSF8TT8gWd69uaRwlX4MWRNamqxG+BqTFy9zSdmPvFsdSsPXyNealBxjyi6E65GS2I0kW8eklsIi44+IzOq5JreRTUiS4mVKKSxnXESWhlZ5/IW8iFwPrYikkvIEwnl/yppWDCiE66q/QysSKQb6HEXGWCQpXqaQ8jnCsaQv8Fx4TtGILeUj5P3qsFfwnHhuUYg5pnCOz8Ke9DdHpoCXk1lIvXtfTGfEkrIKYU1LpcgYCxYvOSPr9F6YGB9fyxF+6a3vQgjP8SzCc+6MrqXwRZ5AFIuMseC58pw7+0/YpRReoDqEjLtnpK/wnHnunVyk61LKAYRX7RYqPHf2wdR0NdDz5hzeM7LQYWdOfS9MF1JeRn5GSisyxoLFyzeQX0OrBdNKYdnhN0Tqy2wC8F6YDUir4uU0Ywq/XXgSsZD5sE/YN62+5dlWClfpLMw9F1pmFOwb9lHjikZbKZ8hb1aHZgzsI/ZVI9pI+RDhfSOmHuyrRhv5NB3o30JOIX0tMsaCxcvaG/k0kTJ3YxrTjNob+dSVwtnEZYR7npj21NrIp86YsgThNxktZHpqbeQzSQoLbF8hC7HIGIuJG/lMksLNZupsTGOawT7lpj8jGTem8A+5EUDue0b6Cjt+ZPHyYVLabkxjmjFyI59RUp5CriCuaaVh3kY+c8cUrkF4X4aFpIN9/cBGPrOlcJXOC1VdbExjmvHARj6zpXATmberQ5OBexv5NCmzdAFnGtxusESOIkmWB5PWKSYDliKIpQhiKYJYiiCWIoilCGIpgliKIJYiiKUIYimCWIogliKIpQhiKYJYiiCWIoilCGIpgliKIJYiiKUIYimCWIogliKIpQhiKYJYiiCWIoilCGIpgliKIJYiiKUIYimC5LgRtWSS3IiaTMpw2J8tXmL3WZKPrz4JIbHPJ8o7pW8SxhGj/zzQC2IpgliKIJYiiKUIYimCJFk89m2K3IvFo2mGpQhiKYJYiiCWIoilCJJ0SoznKvoiF84jXOSK3Wc5rjx6C9wJ+ONLEEsRxFIEsRRBLEUQSxHEUgSxFEFSLx7NRAaD/wGF8ALPUolbiQAAAABJRU5ErkJggg==";
