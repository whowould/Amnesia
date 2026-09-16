#include "offsets.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winhttp.h>

#include <cctype>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#pragma comment(lib, "winhttp.lib")

namespace Offsets
{
    namespace
    {
        std::string error_text;
        std::string json_body;

        auto set_error(std::string msg) -> void
        {
            error_text = std::move(msg);
        }

        auto skip_ws(std::string_view s, std::size_t& i) -> void
        {
            while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])))
                ++i;
        }

        auto match_key(std::string_view s, std::size_t i, std::string_view key) -> bool
        {
            if (i >= s.size() || s[i] != '"')
                return false;
            ++i;
            if (i + key.size() >= s.size())
                return false;
            if (s.compare(i, key.size(), key) != 0)
                return false;
            i += key.size();
            return i < s.size() && s[i] == '"';
        }

        auto skip_string(std::string_view s, std::size_t& i) -> bool
        {
            if (i >= s.size() || s[i] != '"')
                return false;
            ++i;
            while (i < s.size())
            {
                if (s[i] == '\\')
                {
                    i += 2;
                    continue;
                }
                if (s[i] == '"')
                {
                    ++i;
                    return true;
                }
                ++i;
            }
            return false;
        }

        auto skip_value(std::string_view s, std::size_t& i) -> bool
        {
            skip_ws(s, i);
            if (i >= s.size())
                return false;

            if (s[i] == '"')
                return skip_string(s, i);

            if (s[i] == '{' || s[i] == '[')
            {
                const auto open = s[i];
                const auto close = (open == '{') ? '}' : ']';
                int depth = 0;
                bool in_string = false;
                while (i < s.size())
                {
                    const auto c = s[i];
                    if (in_string)
                    {
                        if (c == '\\')
                        {
                            i += 2;
                            continue;
                        }
                        if (c == '"')
                            in_string = false;
                        ++i;
                        continue;
                    }
                    if (c == '"')
                    {
                        in_string = true;
                        ++i;
                        continue;
                    }
                    if (c == open)
                        ++depth;
                    else if (c == close)
                    {
                        --depth;
                        ++i;
                        if (depth == 0)
                            return true;
                        continue;
                    }
                    ++i;
                }
                return false;
            }

            while (i < s.size() && s[i] != ',' && s[i] != '}' && s[i] != ']')
                ++i;
            return true;
        }

        auto find_field(std::string_view obj, std::string_view key, std::size_t& value_at) -> bool
        {
            std::size_t i = 0;
            skip_ws(obj, i);
            if (i >= obj.size() || obj[i] != '{')
                return false;
            ++i;

            while (i < obj.size())
            {
                skip_ws(obj, i);
                if (i >= obj.size())
                    return false;
                if (obj[i] == '}')
                    return false;
                if (obj[i] != '"')
                    return false;

                if (match_key(obj, i, key))
                {
                    skip_string(obj, i);
                    skip_ws(obj, i);
                    if (i >= obj.size() || obj[i] != ':')
                        return false;
                    ++i;
                    skip_ws(obj, i);
                    value_at = i;
                    return true;
                }

                if (!skip_string(obj, i))
                    return false;
                skip_ws(obj, i);
                if (i >= obj.size() || obj[i] != ':')
                    return false;
                ++i;
                if (!skip_value(obj, i))
                    return false;
                skip_ws(obj, i);
                if (i < obj.size() && obj[i] == ',')
                    ++i;
            }
            return false;
        }

        auto object_field(std::string_view obj, std::string_view key) -> std::string_view
        {
            std::size_t at = 0;
            if (!find_field(obj, key, at))
                return {};
            if (at >= obj.size() || obj[at] != '{')
                return {};

            std::size_t end = at;
            if (!skip_value(obj, end))
                return {};
            return obj.substr(at, end - at);
        }

        auto parse_number(std::string_view raw) -> std::uintptr_t
        {
            while (!raw.empty() && std::isspace(static_cast<unsigned char>(raw.front())))
                raw.remove_prefix(1);
            while (!raw.empty() && std::isspace(static_cast<unsigned char>(raw.back())))
                raw.remove_suffix(1);
            if (raw.empty())
                return 0;

            if (raw.front() == '"')
            {
                raw.remove_prefix(1);
                if (!raw.empty() && raw.back() == '"')
                    raw.remove_suffix(1);
            }
            if (raw.empty())
                return 0;

            char* end = nullptr;
            if (raw.size() > 2 && raw[0] == '0' && (raw[1] == 'x' || raw[1] == 'X'))
                return static_cast<std::uintptr_t>(std::strtoull(raw.data(), &end, 16));
            return static_cast<std::uintptr_t>(std::strtoull(raw.data(), &end, 10));
        }

        auto string_field(std::string_view obj, std::string_view key) -> std::string
        {
            std::size_t at = 0;
            if (!find_field(obj, key, at))
                return {};
            if (at >= obj.size() || obj[at] != '"')
                return {};

            std::size_t end = at;
            if (!skip_string(obj, end))
                return {};
            if (end - at < 2)
                return {};
            return std::string(obj.substr(at + 1, end - at - 2));
        }

        auto number_field(std::string_view obj, std::string_view key) -> std::uintptr_t
        {
            std::size_t at = 0;
            if (!find_field(obj, key, at))
                return 0;

            std::size_t end = at;
            if (!skip_value(obj, end))
                return 0;
            return parse_number(obj.substr(at, end - at));
        }

        auto http_get(std::wstring_view host, std::wstring_view path, std::string& body) -> bool
        {
            body.clear();

            const auto session = ::WinHttpOpen(
                L"Amnesia/1.0",
                WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                WINHTTP_NO_PROXY_NAME,
                WINHTTP_NO_PROXY_BYPASS,
                0);
            if (!session)
            {
                set_error("WinHttpOpen failed");
                return false;
            }

            ::WinHttpSetTimeouts(session, 8000, 8000, 8000, 8000);

            const auto connect = ::WinHttpConnect(session, host.data(), INTERNET_DEFAULT_HTTPS_PORT, 0);
            if (!connect)
            {
                ::WinHttpCloseHandle(session);
                set_error("WinHttpConnect failed");
                return false;
            }

            const auto request = ::WinHttpOpenRequest(
                connect,
                L"GET",
                path.data(),
                nullptr,
                WINHTTP_NO_REFERER,
                WINHTTP_DEFAULT_ACCEPT_TYPES,
                WINHTTP_FLAG_SECURE);
            if (!request)
            {
                ::WinHttpCloseHandle(connect);
                ::WinHttpCloseHandle(session);
                set_error("WinHttpOpenRequest failed");
                return false;
            }

            auto ok = ::WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) != FALSE;
            ok = ok && ::WinHttpReceiveResponse(request, nullptr) != FALSE;
            if (!ok)
            {
                ::WinHttpCloseHandle(request);
                ::WinHttpCloseHandle(connect);
                ::WinHttpCloseHandle(session);
                set_error("http request failed");
                return false;
            }

            DWORD status = 0;
            DWORD status_size = sizeof(status);
            ::WinHttpQueryHeaders(
                request,
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX,
                &status,
                &status_size,
                WINHTTP_NO_HEADER_INDEX);

            if (status != 200)
            {
                ::WinHttpCloseHandle(request);
                ::WinHttpCloseHandle(connect);
                ::WinHttpCloseHandle(session);
                set_error("http status " + std::to_string(status));
                return false;
            }

            while (true)
            {
                DWORD available = 0;
                if (!::WinHttpQueryDataAvailable(request, &available))
                    break;
                if (!available)
                    break;

                std::vector<char> chunk(available);
                DWORD read = 0;
                if (!::WinHttpReadData(request, chunk.data(), available, &read) || !read)
                    break;
                body.append(chunk.data(), read);
            }

            ::WinHttpCloseHandle(request);
            ::WinHttpCloseHandle(connect);
            ::WinHttpCloseHandle(session);

            if (body.empty())
            {
                set_error("empty response");
                return false;
            }
            return true;
        }

        auto apply(std::string_view json) -> bool
        {
            const auto root = json;
            auto offsets = object_field(root, "Offsets");
            if (offsets.empty())
            {
                set_error("missing Offsets object");
                return false;
            }

            ClientVersion = string_field(root, "Roblox Version");
            DumperVersion = string_field(root, "Dumper Version");
            DumpedAt = string_field(root, "Dumped At");

            auto load = [&](std::string_view cls, auto assign) -> void
            {
                const auto obj = object_field(offsets, cls);
                if (!obj.empty())
                    assign(obj);
            };

            load("AirProperties", [](std::string_view o)
            {
                AirProperties::AirDensity = number_field(o, "AirDensity");
                AirProperties::GlobalWind = number_field(o, "GlobalWind");
            });
            load("AnimationTrack", [](std::string_view o)
            {
                AnimationTrack::Animation = number_field(o, "Animation");
                AnimationTrack::Animator = number_field(o, "Animator");
                AnimationTrack::IsPlaying = number_field(o, "IsPlaying");
                AnimationTrack::Looped = number_field(o, "Looped");
                AnimationTrack::Speed = number_field(o, "Speed");
                AnimationTrack::TimePosition = number_field(o, "TimePosition");
            });
            load("Animator", [](std::string_view o)
            {
                Animator::ActiveAnimations = number_field(o, "ActiveAnimations");
            });
            load("Atmosphere", [](std::string_view o)
            {
                Atmosphere::Color = number_field(o, "Color");
                Atmosphere::Decay = number_field(o, "Decay");
                Atmosphere::Density = number_field(o, "Density");
                Atmosphere::Glare = number_field(o, "Glare");
                Atmosphere::Haze = number_field(o, "Haze");
                Atmosphere::Offset = number_field(o, "Offset");
            });
            load("Attachment", [](std::string_view o)
            {
                Attachment::Position = number_field(o, "Position");
            });
            load("BasePart", [](std::string_view o)
            {
                BasePart::CastShadow = number_field(o, "CastShadow");
                BasePart::Color3 = number_field(o, "Color3");
                BasePart::Locked = number_field(o, "Locked");
                BasePart::Massless = number_field(o, "Massless");
                BasePart::Primitive = number_field(o, "Primitive");
                BasePart::Reflectance = number_field(o, "Reflectance");
                BasePart::Shape = number_field(o, "Shape");
                BasePart::Transparency = number_field(o, "Transparency");
            });
            load("Beam", [](std::string_view o)
            {
                Beam::Attachment0 = number_field(o, "Attachment0");
                Beam::Attachment1 = number_field(o, "Attachment1");
                Beam::Brightness = number_field(o, "Brightness");
                Beam::CurveSize0 = number_field(o, "CurveSize0");
                Beam::CurveSize1 = number_field(o, "CurveSize1");
                Beam::LightEmission = number_field(o, "LightEmission");
                Beam::LightInfluence = number_field(o, "LightInfluence");
                Beam::Texture = number_field(o, "Texture");
                Beam::TextureLength = number_field(o, "TextureLength");
                Beam::TextureSpeed = number_field(o, "TextureSpeed");
                Beam::Width0 = number_field(o, "Width0");
                Beam::Width1 = number_field(o, "Width1");
                Beam::ZOffset = number_field(o, "ZOffset");
            });
            load("BloomEffect", [](std::string_view o)
            {
                BloomEffect::Enabled = number_field(o, "Enabled");
                BloomEffect::Intensity = number_field(o, "Intensity");
                BloomEffect::Size = number_field(o, "Size");
                BloomEffect::Threshold = number_field(o, "Threshold");
            });
            load("BlurEffect", [](std::string_view o)
            {
                BlurEffect::Enabled = number_field(o, "Enabled");
                BlurEffect::Size = number_field(o, "Size");
            });
            load("ByteCode", [](std::string_view o)
            {
                ByteCode::Pointer = number_field(o, "Pointer");
                ByteCode::Size = number_field(o, "Size");
            });
            load("Camera", [](std::string_view o)
            {
                Camera::CameraSubject = number_field(o, "CameraSubject");
                Camera::CameraType = number_field(o, "CameraType");
                Camera::FieldOfView = number_field(o, "FieldOfView");
                Camera::ImagePlaneDepth = number_field(o, "ImagePlaneDepth");
                Camera::Position = number_field(o, "Position");
                Camera::Rotation = number_field(o, "Rotation");
                Camera::Viewport = number_field(o, "Viewport");
                Camera::ViewportSize = number_field(o, "ViewportSize");
            });
            load("CharacterMesh", [](std::string_view o)
            {
                CharacterMesh::BaseTextureId = number_field(o, "BaseTextureId");
                CharacterMesh::BodyPart = number_field(o, "BodyPart");
                CharacterMesh::MeshId = number_field(o, "MeshId");
                CharacterMesh::OverlayTextureId = number_field(o, "OverlayTextureId");
            });
            load("ClickDetector", [](std::string_view o)
            {
                ClickDetector::MaxActivationDistance = number_field(o, "MaxActivationDistance");
                ClickDetector::MouseIcon = number_field(o, "MouseIcon");
            });
            load("Clothing", [](std::string_view o)
            {
                Clothing::Color3 = number_field(o, "Color3");
                Clothing::Template = number_field(o, "Template");
            });
            load("ColorCorrectionEffect", [](std::string_view o)
            {
                ColorCorrectionEffect::Brightness = number_field(o, "Brightness");
                ColorCorrectionEffect::Contrast = number_field(o, "Contrast");
                ColorCorrectionEffect::Enabled = number_field(o, "Enabled");
                ColorCorrectionEffect::TintColor = number_field(o, "TintColor");
            });
            load("ColorGradingEffect", [](std::string_view o)
            {
                ColorGradingEffect::Enabled = number_field(o, "Enabled");
                ColorGradingEffect::TonemapperPreset = number_field(o, "TonemapperPreset");
            });
            load("DataModel", [](std::string_view o)
            {
                DataModel::CreatorId = number_field(o, "CreatorId");
                DataModel::GameId = number_field(o, "GameId");
                DataModel::GameLoaded = number_field(o, "GameLoaded");
                DataModel::JobId = number_field(o, "JobId");
                DataModel::PlaceId = number_field(o, "PlaceId");
                DataModel::PlaceVersion = number_field(o, "PlaceVersion");
                DataModel::PrimitiveCount = number_field(o, "PrimitiveCount");
                DataModel::ScriptContext = number_field(o, "ScriptContext");
                DataModel::ServerIP = number_field(o, "ServerIP");
                DataModel::ToRenderView1 = number_field(o, "ToRenderView1");
                DataModel::ToRenderView2 = number_field(o, "ToRenderView2");
                DataModel::ToRenderView3 = number_field(o, "ToRenderView3");
                DataModel::Workspace = number_field(o, "Workspace");
            });
            load("DepthOfFieldEffect", [](std::string_view o)
            {
                DepthOfFieldEffect::Enabled = number_field(o, "Enabled");
                DepthOfFieldEffect::FarIntensity = number_field(o, "FarIntensity");
                DepthOfFieldEffect::FocusDistance = number_field(o, "FocusDistance");
                DepthOfFieldEffect::InFocusRadius = number_field(o, "InFocusRadius");
                DepthOfFieldEffect::NearIntensity = number_field(o, "NearIntensity");
            });
            load("DragDetector", [](std::string_view o)
            {
                DragDetector::ActivatedCursorIcon = number_field(o, "ActivatedCursorIcon");
                DragDetector::CursorIcon = number_field(o, "CursorIcon");
                DragDetector::MaxActivationDistance = number_field(o, "MaxActivationDistance");
                DragDetector::MaxDragAngle = number_field(o, "MaxDragAngle");
                DragDetector::MaxDragTranslation = number_field(o, "MaxDragTranslation");
                DragDetector::MaxForce = number_field(o, "MaxForce");
                DragDetector::MaxTorque = number_field(o, "MaxTorque");
                DragDetector::MinDragAngle = number_field(o, "MinDragAngle");
                DragDetector::MinDragTranslation = number_field(o, "MinDragTranslation");
                DragDetector::ReferenceInstance = number_field(o, "ReferenceInstance");
                DragDetector::Responsiveness = number_field(o, "Responsiveness");
            });
            load("FakeDataModel", [](std::string_view o)
            {
                FakeDataModel::Pointer = number_field(o, "Pointer");
                FakeDataModel::RealDataModel = number_field(o, "RealDataModel");
            });
            load("GuiBase2D", [](std::string_view o)
            {
                GuiBase2D::AbsolutePosition = number_field(o, "AbsolutePosition");
                GuiBase2D::AbsoluteRotation = number_field(o, "AbsoluteRotation");
                GuiBase2D::AbsoluteSize = number_field(o, "AbsoluteSize");
            });
            load("GuiObject", [](std::string_view o)
            {
                GuiObject::BackgroundColor3 = number_field(o, "BackgroundColor3");
                GuiObject::BackgroundTransparency = number_field(o, "BackgroundTransparency");
                GuiObject::BorderColor3 = number_field(o, "BorderColor3");
                GuiObject::Image = number_field(o, "Image");
                GuiObject::LayoutOrder = number_field(o, "LayoutOrder");
                GuiObject::Position = number_field(o, "Position");
                GuiObject::RichText = number_field(o, "RichText");
                GuiObject::Rotation = number_field(o, "Rotation");
                GuiObject::ScreenGui_Enabled = number_field(o, "ScreenGui_Enabled");
                GuiObject::Size = number_field(o, "Size");
                GuiObject::Text = number_field(o, "Text");
                GuiObject::TextColor3 = number_field(o, "TextColor3");
                GuiObject::Visible = number_field(o, "Visible");
                GuiObject::ZIndex = number_field(o, "ZIndex");
            });
            load("Humanoid", [](std::string_view o)
            {
                Humanoid::AutoJumpEnabled = number_field(o, "AutoJumpEnabled");
                Humanoid::AutoRotate = number_field(o, "AutoRotate");
                Humanoid::AutomaticScalingEnabled = number_field(o, "AutomaticScalingEnabled");
                Humanoid::BreakJointsOnDeath = number_field(o, "BreakJointsOnDeath");
                Humanoid::CameraOffset = number_field(o, "CameraOffset");
                Humanoid::DisplayDistanceType = number_field(o, "DisplayDistanceType");
                Humanoid::DisplayName = number_field(o, "DisplayName");
                Humanoid::EvaluateStateMachine = number_field(o, "EvaluateStateMachine");
                Humanoid::FloorMaterial = number_field(o, "FloorMaterial");
                Humanoid::Health = number_field(o, "Health");
                Humanoid::HealthDisplayDistance = number_field(o, "HealthDisplayDistance");
                Humanoid::HealthDisplayType = number_field(o, "HealthDisplayType");
                Humanoid::HipHeight = number_field(o, "HipHeight");
                Humanoid::HumanoidRootPart = number_field(o, "HumanoidRootPart");
                Humanoid::HumanoidState = number_field(o, "HumanoidState");
                Humanoid::HumanoidStateID = number_field(o, "HumanoidStateID");
                Humanoid::IsWalking = number_field(o, "IsWalking");
                Humanoid::Jump = number_field(o, "Jump");
                Humanoid::JumpHeight = number_field(o, "JumpHeight");
                Humanoid::JumpPower = number_field(o, "JumpPower");
                Humanoid::MaxHealth = number_field(o, "MaxHealth");
                Humanoid::MaxSlopeAngle = number_field(o, "MaxSlopeAngle");
                Humanoid::MoveDirection = number_field(o, "MoveDirection");
                Humanoid::MoveToPart = number_field(o, "MoveToPart");
                Humanoid::MoveToPoint = number_field(o, "MoveToPoint");
                Humanoid::NameDisplayDistance = number_field(o, "NameDisplayDistance");
                Humanoid::NameOcclusion = number_field(o, "NameOcclusion");
                Humanoid::PlatformStand = number_field(o, "PlatformStand");
                Humanoid::PlatformStatePointer = number_field(o, "PlatformStatePointer");
                Humanoid::RequiresNeck = number_field(o, "RequiresNeck");
                Humanoid::RigType = number_field(o, "RigType");
                Humanoid::SeatPart = number_field(o, "SeatPart");
                Humanoid::Sit = number_field(o, "Sit");
                Humanoid::TargetPoint = number_field(o, "TargetPoint");
                Humanoid::UseJumpPower = number_field(o, "UseJumpPower");
                Humanoid::WalkTimer = number_field(o, "WalkTimer");
                Humanoid::Walkspeed = number_field(o, "Walkspeed");
                Humanoid::WalkspeedCheck = number_field(o, "WalkspeedCheck");
            });
            load("Instance", [](std::string_view o)
            {
                Instance::ChildrenEnd = number_field(o, "ChildrenEnd");
                Instance::ChildrenStart = number_field(o, "ChildrenStart");
                Instance::ClassBase = number_field(o, "ClassBase");
                Instance::ClassDescriptor = number_field(o, "ClassDescriptor");
                Instance::ClassName = number_field(o, "ClassName");
                Instance::Name = number_field(o, "Name");
                Instance::NameContainer = number_field(o, "NameContainer");
                Instance::Parent = number_field(o, "Parent");
                Instance::This = number_field(o, "This");
            });
            load("Lighting", [](std::string_view o)
            {
                Lighting::Ambient = number_field(o, "Ambient");
                Lighting::Brightness = number_field(o, "Brightness");
                Lighting::ClockTime = number_field(o, "ClockTime");
                Lighting::ColorShift_Bottom = number_field(o, "ColorShift_Bottom");
                Lighting::ColorShift_Top = number_field(o, "ColorShift_Top");
                Lighting::EnvironmentDiffuseScale = number_field(o, "EnvironmentDiffuseScale");
                Lighting::EnvironmentSpecularScale = number_field(o, "EnvironmentSpecularScale");
                Lighting::ExposureCompensation = number_field(o, "ExposureCompensation");
                Lighting::FogColor = number_field(o, "FogColor");
                Lighting::FogEnd = number_field(o, "FogEnd");
                Lighting::FogStart = number_field(o, "FogStart");
                Lighting::GeographicLatitude = number_field(o, "GeographicLatitude");
                Lighting::GlobalShadows = number_field(o, "GlobalShadows");
                Lighting::GradientBottom = number_field(o, "GradientBottom");
                Lighting::GradientTop = number_field(o, "GradientTop");
                Lighting::LightColor = number_field(o, "LightColor");
                Lighting::LightDirection = number_field(o, "LightDirection");
                Lighting::MoonPosition = number_field(o, "MoonPosition");
                Lighting::OutdoorAmbient = number_field(o, "OutdoorAmbient");
                Lighting::Sky = number_field(o, "Sky");
                Lighting::Source = number_field(o, "Source");
                Lighting::SunPosition = number_field(o, "SunPosition");
            });
            load("LocalScript", [](std::string_view o)
            {
                LocalScript::ByteCode = number_field(o, "ByteCode");
                LocalScript::GUID = number_field(o, "GUID");
                LocalScript::Hash = number_field(o, "Hash");
            });
            load("MaterialColors", [](std::string_view o)
            {
                MaterialColors::Asphalt = number_field(o, "Asphalt");
                MaterialColors::Basalt = number_field(o, "Basalt");
                MaterialColors::Brick = number_field(o, "Brick");
                MaterialColors::Cobblestone = number_field(o, "Cobblestone");
                MaterialColors::Concrete = number_field(o, "Concrete");
                MaterialColors::CrackedLava = number_field(o, "CrackedLava");
                MaterialColors::Glacier = number_field(o, "Glacier");
                MaterialColors::Grass = number_field(o, "Grass");
                MaterialColors::Ground = number_field(o, "Ground");
                MaterialColors::Ice = number_field(o, "Ice");
                MaterialColors::LeafyGrass = number_field(o, "LeafyGrass");
                MaterialColors::Limestone = number_field(o, "Limestone");
                MaterialColors::Mud = number_field(o, "Mud");
                MaterialColors::Pavement = number_field(o, "Pavement");
                MaterialColors::Rock = number_field(o, "Rock");
                MaterialColors::Salt = number_field(o, "Salt");
                MaterialColors::Sand = number_field(o, "Sand");
                MaterialColors::Sandstone = number_field(o, "Sandstone");
                MaterialColors::Slate = number_field(o, "Slate");
                MaterialColors::Snow = number_field(o, "Snow");
                MaterialColors::WoodPlanks = number_field(o, "WoodPlanks");
            });
            load("MeshPart", [](std::string_view o)
            {
                MeshPart::MeshId = number_field(o, "MeshId");
                MeshPart::Texture = number_field(o, "Texture");
            });
            load("Misc", [](std::string_view o)
            {
                Misc::Adornee = number_field(o, "Adornee");
                Misc::AnimationId = number_field(o, "AnimationId");
                Misc::StringLength = number_field(o, "StringLength");
                Misc::Value = number_field(o, "Value");
            });
            load("Model", [](std::string_view o)
            {
                Model::PrimaryPart = number_field(o, "PrimaryPart");
                Model::Scale = number_field(o, "Scale");
            });
            load("ModuleScript", [](std::string_view o)
            {
                ModuleScript::ByteCode = number_field(o, "ByteCode");
                ModuleScript::GUID = number_field(o, "GUID");
                ModuleScript::Hash = number_field(o, "Hash");
                ModuleScript::IsCoreScript = number_field(o, "IsCoreScript");
            });
            load("MouseService", [](std::string_view o)
            {
                MouseService::InputObject = number_field(o, "InputObject");
                MouseService::InputObject2 = number_field(o, "InputObject2");
                MouseService::MousePosition = number_field(o, "MousePosition");
                MouseService::SensitivityPointer = number_field(o, "SensitivityPointer");
            });
            load("ParticleEmitter", [](std::string_view o)
            {
                ParticleEmitter::Acceleration = number_field(o, "Acceleration");
                ParticleEmitter::Brightness = number_field(o, "Brightness");
                ParticleEmitter::Drag = number_field(o, "Drag");
                ParticleEmitter::Lifetime = number_field(o, "Lifetime");
                ParticleEmitter::LightEmission = number_field(o, "LightEmission");
                ParticleEmitter::LightInfluence = number_field(o, "LightInfluence");
                ParticleEmitter::Rate = number_field(o, "Rate");
                ParticleEmitter::RotSpeed = number_field(o, "RotSpeed");
                ParticleEmitter::Rotation = number_field(o, "Rotation");
                ParticleEmitter::Speed = number_field(o, "Speed");
                ParticleEmitter::SpreadAngle = number_field(o, "SpreadAngle");
                ParticleEmitter::Texture = number_field(o, "Texture");
                ParticleEmitter::TimeScale = number_field(o, "TimeScale");
                ParticleEmitter::VelocityInheritance = number_field(o, "VelocityInheritance");
                ParticleEmitter::ZOffset = number_field(o, "ZOffset");
            });
            load("Player", [](std::string_view o)
            {
                Player::AccountAge = number_field(o, "AccountAge");
                Player::CameraMode = number_field(o, "CameraMode");
                Player::DisplayName = number_field(o, "DisplayName");
                Player::HealthDisplayDistance = number_field(o, "HealthDisplayDistance");
                Player::LocalPlayer = number_field(o, "LocalPlayer");
                Player::LocaleId = number_field(o, "LocaleId");
                Player::MaxZoomDistance = number_field(o, "MaxZoomDistance");
                Player::MinZoomDistance = number_field(o, "MinZoomDistance");
                Player::ModelInstance = number_field(o, "ModelInstance");
                Player::Mouse = number_field(o, "Mouse");
                Player::NameDisplayDistance = number_field(o, "NameDisplayDistance");
                Player::Team = number_field(o, "Team");
                Player::TeamColor = number_field(o, "TeamColor");
                Player::UserId = number_field(o, "UserId");
            });
            load("PlayerMouse", [](std::string_view o)
            {
                PlayerMouse::Icon = number_field(o, "Icon");
                PlayerMouse::Workspace = number_field(o, "Workspace");
            });
            load("Primitive", [](std::string_view o)
            {
                Primitive::AssemblyAngularVelocity = number_field(o, "AssemblyAngularVelocity");
                Primitive::AssemblyLinearVelocity = number_field(o, "AssemblyLinearVelocity");
                Primitive::Flags = number_field(o, "Flags");
                Primitive::Material = number_field(o, "Material");
                Primitive::Owner = number_field(o, "Owner");
                Primitive::Position = number_field(o, "Position");
                Primitive::Rotation = number_field(o, "Rotation");
                Primitive::Size = number_field(o, "Size");
                Primitive::Validate = number_field(o, "Validate");
            });
            load("PrimitiveFlags", [](std::string_view o)
            {
                PrimitiveFlags::Anchored = number_field(o, "Anchored");
                PrimitiveFlags::CanCollide = number_field(o, "CanCollide");
                PrimitiveFlags::CanQuery = number_field(o, "CanQuery");
                PrimitiveFlags::CanTouch = number_field(o, "CanTouch");
            });
            load("ProximityPrompt", [](std::string_view o)
            {
                ProximityPrompt::ActionText = number_field(o, "ActionText");
                ProximityPrompt::Enabled = number_field(o, "Enabled");
                ProximityPrompt::GamepadKeyCode = number_field(o, "GamepadKeyCode");
                ProximityPrompt::HoldDuration = number_field(o, "HoldDuration");
                ProximityPrompt::KeyCode = number_field(o, "KeyCode");
                ProximityPrompt::MaxActivationDistance = number_field(o, "MaxActivationDistance");
                ProximityPrompt::ObjectText = number_field(o, "ObjectText");
                ProximityPrompt::RequiresLineOfSight = number_field(o, "RequiresLineOfSight");
            });
            load("RenderJob", [](std::string_view o)
            {
                RenderJob::FakeDataModel = number_field(o, "FakeDataModel");
                RenderJob::RealDataModel = number_field(o, "RealDataModel");
                RenderJob::RenderView = number_field(o, "RenderView");
            });
            load("RenderView", [](std::string_view o)
            {
                RenderView::DeviceD3D11 = number_field(o, "DeviceD3D11");
                RenderView::LightingValid = number_field(o, "LightingValid");
                RenderView::SkyValid = number_field(o, "SkyValid");
                RenderView::VisualEngine = number_field(o, "VisualEngine");
            });
            load("RunService", [](std::string_view o)
            {
                RunService::HeartbeatFPS = number_field(o, "HeartbeatFPS");
                RunService::HeartbeatTask = number_field(o, "HeartbeatTask");
            });
            load("Script", [](std::string_view o)
            {
                Script::ByteCode = number_field(o, "ByteCode");
                Script::GUID = number_field(o, "GUID");
                Script::Hash = number_field(o, "Hash");
            });
            load("Seat", [](std::string_view o)
            {
                Seat::Occupant = number_field(o, "Occupant");
            });
            load("Sky", [](std::string_view o)
            {
                Sky::MoonAngularSize = number_field(o, "MoonAngularSize");
                Sky::MoonTextureId = number_field(o, "MoonTextureId");
                Sky::SkyboxBk = number_field(o, "SkyboxBk");
                Sky::SkyboxDn = number_field(o, "SkyboxDn");
                Sky::SkyboxFt = number_field(o, "SkyboxFt");
                Sky::SkyboxLf = number_field(o, "SkyboxLf");
                Sky::SkyboxOrientation = number_field(o, "SkyboxOrientation");
                Sky::SkyboxRt = number_field(o, "SkyboxRt");
                Sky::SkyboxUp = number_field(o, "SkyboxUp");
                Sky::StarCount = number_field(o, "StarCount");
                Sky::SunAngularSize = number_field(o, "SunAngularSize");
                Sky::SunTextureId = number_field(o, "SunTextureId");
            });
            load("Sound", [](std::string_view o)
            {
                Sound::IsPlaying = number_field(o, "IsPlaying");
                Sound::Looped = number_field(o, "Looped");
                Sound::PlaybackSpeed = number_field(o, "PlaybackSpeed");
                Sound::RollOffMaxDistance = number_field(o, "RollOffMaxDistance");
                Sound::RollOffMinDistance = number_field(o, "RollOffMinDistance");
                Sound::SoundGroup = number_field(o, "SoundGroup");
                Sound::SoundId = number_field(o, "SoundId");
                Sound::Volume = number_field(o, "Volume");
            });
            load("SpawnLocation", [](std::string_view o)
            {
                SpawnLocation::AllowTeamChangeOnTouch = number_field(o, "AllowTeamChangeOnTouch");
                SpawnLocation::Enabled = number_field(o, "Enabled");
                SpawnLocation::ForcefieldDuration = number_field(o, "ForcefieldDuration");
                SpawnLocation::Neutral = number_field(o, "Neutral");
                SpawnLocation::TeamColor = number_field(o, "TeamColor");
            });
            load("SpecialMesh", [](std::string_view o)
            {
                SpecialMesh::MeshId = number_field(o, "MeshId");
                SpecialMesh::Scale = number_field(o, "Scale");
            });
            load("StatsItem", [](std::string_view o)
            {
                StatsItem::Value = number_field(o, "Value");
            });
            load("SunRaysEffect", [](std::string_view o)
            {
                SunRaysEffect::Enabled = number_field(o, "Enabled");
                SunRaysEffect::Intensity = number_field(o, "Intensity");
                SunRaysEffect::Spread = number_field(o, "Spread");
            });
            load("SurfaceAppearance", [](std::string_view o)
            {
                SurfaceAppearance::AlphaMode = number_field(o, "AlphaMode");
                SurfaceAppearance::Color = number_field(o, "Color");
                SurfaceAppearance::ColorMap = number_field(o, "ColorMap");
                SurfaceAppearance::EmissiveMaskContent = number_field(o, "EmissiveMaskContent");
                SurfaceAppearance::EmissiveStrength = number_field(o, "EmissiveStrength");
                SurfaceAppearance::EmissiveTint = number_field(o, "EmissiveTint");
                SurfaceAppearance::MetalnessMap = number_field(o, "MetalnessMap");
                SurfaceAppearance::NormalMap = number_field(o, "NormalMap");
                SurfaceAppearance::RoughnessMap = number_field(o, "RoughnessMap");
            });
            load("TaskScheduler", [](std::string_view o)
            {
                TaskScheduler::JobEnd = number_field(o, "JobEnd");
                TaskScheduler::JobName = number_field(o, "JobName");
                TaskScheduler::JobStart = number_field(o, "JobStart");
                TaskScheduler::MaxFPS = number_field(o, "MaxFPS");
                TaskScheduler::Pointer = number_field(o, "Pointer");
            });
            load("Team", [](std::string_view o)
            {
                Team::BrickColor = number_field(o, "BrickColor");
            });
            load("Terrain", [](std::string_view o)
            {
                Terrain::GrassLength = number_field(o, "GrassLength");
                Terrain::MaterialColors = number_field(o, "MaterialColors");
                Terrain::WaterColor = number_field(o, "WaterColor");
                Terrain::WaterReflectance = number_field(o, "WaterReflectance");
                Terrain::WaterTransparency = number_field(o, "WaterTransparency");
                Terrain::WaterWaveSize = number_field(o, "WaterWaveSize");
                Terrain::WaterWaveSpeed = number_field(o, "WaterWaveSpeed");
            });
            load("Textures", [](std::string_view o)
            {
                Textures::Decal_Texture = number_field(o, "Decal_Texture");
                Textures::Texture_Texture = number_field(o, "Texture_Texture");
            });
            load("Tool", [](std::string_view o)
            {
                Tool::CanBeDropped = number_field(o, "CanBeDropped");
                Tool::Enabled = number_field(o, "Enabled");
                Tool::Grip = number_field(o, "Grip");
                Tool::ManualActivationOnly = number_field(o, "ManualActivationOnly");
                Tool::RequiresHandle = number_field(o, "RequiresHandle");
                Tool::TextureId = number_field(o, "TextureId");
                Tool::Tooltip = number_field(o, "Tooltip");
            });
            load("UnionOperation", [](std::string_view o)
            {
                UnionOperation::AssetId = number_field(o, "AssetId");
            });
            load("UserInputService", [](std::string_view o)
            {
                UserInputService::WindowInputState = number_field(o, "WindowInputState");
            });
            load("VehicleSeat", [](std::string_view o)
            {
                VehicleSeat::MaxSpeed = number_field(o, "MaxSpeed");
                VehicleSeat::SteerFloat = number_field(o, "SteerFloat");
                VehicleSeat::ThrottleFloat = number_field(o, "ThrottleFloat");
                VehicleSeat::Torque = number_field(o, "Torque");
                VehicleSeat::TurnSpeed = number_field(o, "TurnSpeed");
            });
            load("VisualEngine", [](std::string_view o)
            {
                VisualEngine::Dimensions = number_field(o, "Dimensions");
                VisualEngine::FakeDataModel = number_field(o, "FakeDataModel");
                VisualEngine::Pointer = number_field(o, "Pointer");
                VisualEngine::RenderView = number_field(o, "RenderView");
                VisualEngine::ViewMatrix = number_field(o, "ViewMatrix");
            });
            load("Weld", [](std::string_view o)
            {
                Weld::Part0 = number_field(o, "Part0");
                Weld::Part1 = number_field(o, "Part1");
            });
            load("WeldConstraint", [](std::string_view o)
            {
                WeldConstraint::Part0 = number_field(o, "Part0");
                WeldConstraint::Part1 = number_field(o, "Part1");
            });
            load("WindowInputState", [](std::string_view o)
            {
                WindowInputState::CapsLock = number_field(o, "CapsLock");
                WindowInputState::CurrentTextBox = number_field(o, "CurrentTextBox");
            });
            load("Workspace", [](std::string_view o)
            {
                Workspace::CurrentCamera = number_field(o, "CurrentCamera");
                Workspace::DistributedGameTime = number_field(o, "DistributedGameTime");
                Workspace::ReadOnlyGravity = number_field(o, "ReadOnlyGravity");
                Workspace::World = number_field(o, "World");
            });
            load("World", [](std::string_view o)
            {
                World::AirProperties = number_field(o, "AirProperties");
                World::FallenPartsDestroyHeight = number_field(o, "FallenPartsDestroyHeight");
                World::Gravity = number_field(o, "Gravity");
                World::Primitives = number_field(o, "Primitives");
                World::worldStepsPerSec = number_field(o, "worldStepsPerSec");
            });

            if (!FakeDataModel::Pointer || !VisualEngine::Pointer || !Instance::ChildrenStart)
            {
                set_error("parsed json but required offsets are zero");
                return false;
            }
            return true;
        }

        auto parse_live_version(std::string_view json) -> std::string
        {
            return string_field(json, "clientVersionUpload");
        }
    }

    auto last_error() -> const std::string&
    {
        return error_text;
    }

    auto fetch() -> bool
    {
        error_text.clear();
        json_body.clear();
        LiveVersion.clear();

        std::string dump;
        if (!http_get(L"offsets.imtheo.lol", L"/offsetshex.json", dump))
            return false;

        if (!apply(dump))
            return false;

        json_body = std::move(dump);

        std::string live;
        if (!http_get(L"clientsettings.roblox.com", L"/v1/client-version/WindowsPlayer", live))
            return false;

        LiveVersion = parse_live_version(live);
        if (LiveVersion.empty())
        {
            set_error("missing clientVersionUpload in roblox client-version json");
            return false;
        }

        if (LiveVersion != ClientVersion)
        {
            set_error("version mismatch");
            return false;
        }

        return true;
    }
}
