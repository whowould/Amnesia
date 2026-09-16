#pragma once

#include <cstdint>
#include <string>

namespace Offsets
{
    inline std::string ClientVersion;
    inline std::string LiveVersion;
    inline std::string DumperVersion;
    inline std::string DumpedAt;

    namespace AirProperties
    {
        inline std::uintptr_t AirDensity;
        inline std::uintptr_t GlobalWind;
    }

    namespace AnimationTrack
    {
        inline std::uintptr_t Animation;
        inline std::uintptr_t Animator;
        inline std::uintptr_t IsPlaying;
        inline std::uintptr_t Looped;
        inline std::uintptr_t Speed;
        inline std::uintptr_t TimePosition;
    }

    namespace Animator
    {
        inline std::uintptr_t ActiveAnimations;
    }

    namespace Atmosphere
    {
        inline std::uintptr_t Color;
        inline std::uintptr_t Decay;
        inline std::uintptr_t Density;
        inline std::uintptr_t Glare;
        inline std::uintptr_t Haze;
        inline std::uintptr_t Offset;
    }

    namespace Attachment
    {
        inline std::uintptr_t Position;
    }

    namespace BasePart
    {
        inline std::uintptr_t CastShadow;
        inline std::uintptr_t Color3;
        inline std::uintptr_t Locked;
        inline std::uintptr_t Massless;
        inline std::uintptr_t Primitive;
        inline std::uintptr_t Reflectance;
        inline std::uintptr_t Shape;
        inline std::uintptr_t Transparency;
    }

    namespace Beam
    {
        inline std::uintptr_t Attachment0;
        inline std::uintptr_t Attachment1;
        inline std::uintptr_t Brightness;
        inline std::uintptr_t CurveSize0;
        inline std::uintptr_t CurveSize1;
        inline std::uintptr_t LightEmission;
        inline std::uintptr_t LightInfluence;
        inline std::uintptr_t Texture;
        inline std::uintptr_t TextureLength;
        inline std::uintptr_t TextureSpeed;
        inline std::uintptr_t Width0;
        inline std::uintptr_t Width1;
        inline std::uintptr_t ZOffset;
    }

    namespace BloomEffect
    {
        inline std::uintptr_t Enabled;
        inline std::uintptr_t Intensity;
        inline std::uintptr_t Size;
        inline std::uintptr_t Threshold;
    }

    namespace BlurEffect
    {
        inline std::uintptr_t Enabled;
        inline std::uintptr_t Size;
    }

    namespace ByteCode
    {
        inline std::uintptr_t Pointer;
        inline std::uintptr_t Size;
    }

    namespace Camera
    {
        inline std::uintptr_t CameraSubject;
        inline std::uintptr_t CameraType;
        inline std::uintptr_t FieldOfView;
        inline std::uintptr_t ImagePlaneDepth;
        inline std::uintptr_t Position;
        inline std::uintptr_t Rotation;
        inline std::uintptr_t Viewport;
        inline std::uintptr_t ViewportSize;
    }

    namespace CharacterMesh
    {
        inline std::uintptr_t BaseTextureId;
        inline std::uintptr_t BodyPart;
        inline std::uintptr_t MeshId;
        inline std::uintptr_t OverlayTextureId;
    }

    namespace ClickDetector
    {
        inline std::uintptr_t MaxActivationDistance;
        inline std::uintptr_t MouseIcon;
    }

    namespace Clothing
    {
        inline std::uintptr_t Color3;
        inline std::uintptr_t Template;
    }

    namespace ColorCorrectionEffect
    {
        inline std::uintptr_t Brightness;
        inline std::uintptr_t Contrast;
        inline std::uintptr_t Enabled;
        inline std::uintptr_t TintColor;
    }

    namespace ColorGradingEffect
    {
        inline std::uintptr_t Enabled;
        inline std::uintptr_t TonemapperPreset;
    }

    namespace DataModel
    {
        inline std::uintptr_t CreatorId;
        inline std::uintptr_t GameId;
        inline std::uintptr_t GameLoaded;
        inline std::uintptr_t JobId;
        inline std::uintptr_t PlaceId;
        inline std::uintptr_t PlaceVersion;
        inline std::uintptr_t PrimitiveCount;
        inline std::uintptr_t ScriptContext;
        inline std::uintptr_t ServerIP;
        inline std::uintptr_t ToRenderView1;
        inline std::uintptr_t ToRenderView2;
        inline std::uintptr_t ToRenderView3;
        inline std::uintptr_t Workspace;
    }

    namespace DepthOfFieldEffect
    {
        inline std::uintptr_t Enabled;
        inline std::uintptr_t FarIntensity;
        inline std::uintptr_t FocusDistance;
        inline std::uintptr_t InFocusRadius;
        inline std::uintptr_t NearIntensity;
    }

    namespace DragDetector
    {
        inline std::uintptr_t ActivatedCursorIcon;
        inline std::uintptr_t CursorIcon;
        inline std::uintptr_t MaxActivationDistance;
        inline std::uintptr_t MaxDragAngle;
        inline std::uintptr_t MaxDragTranslation;
        inline std::uintptr_t MaxForce;
        inline std::uintptr_t MaxTorque;
        inline std::uintptr_t MinDragAngle;
        inline std::uintptr_t MinDragTranslation;
        inline std::uintptr_t ReferenceInstance;
        inline std::uintptr_t Responsiveness;
    }

    namespace FakeDataModel
    {
        inline std::uintptr_t Pointer;
        inline std::uintptr_t RealDataModel;
    }

    namespace GuiBase2D
    {
        inline std::uintptr_t AbsolutePosition;
        inline std::uintptr_t AbsoluteRotation;
        inline std::uintptr_t AbsoluteSize;
    }

    namespace GuiObject
    {
        inline std::uintptr_t BackgroundColor3;
        inline std::uintptr_t BackgroundTransparency;
        inline std::uintptr_t BorderColor3;
        inline std::uintptr_t Image;
        inline std::uintptr_t LayoutOrder;
        inline std::uintptr_t Position;
        inline std::uintptr_t RichText;
        inline std::uintptr_t Rotation;
        inline std::uintptr_t ScreenGui_Enabled;
        inline std::uintptr_t Size;
        inline std::uintptr_t Text;
        inline std::uintptr_t TextColor3;
        inline std::uintptr_t Visible;
        inline std::uintptr_t ZIndex;
    }

    namespace Humanoid
    {
        inline std::uintptr_t AutoJumpEnabled;
        inline std::uintptr_t AutoRotate;
        inline std::uintptr_t AutomaticScalingEnabled;
        inline std::uintptr_t BreakJointsOnDeath;
        inline std::uintptr_t CameraOffset;
        inline std::uintptr_t DisplayDistanceType;
        inline std::uintptr_t DisplayName;
        inline std::uintptr_t EvaluateStateMachine;
        inline std::uintptr_t FloorMaterial;
        inline std::uintptr_t Health;
        inline std::uintptr_t HealthDisplayDistance;
        inline std::uintptr_t HealthDisplayType;
        inline std::uintptr_t HipHeight;
        inline std::uintptr_t HumanoidRootPart;
        inline std::uintptr_t HumanoidState;
        inline std::uintptr_t HumanoidStateID;
        inline std::uintptr_t IsWalking;
        inline std::uintptr_t Jump;
        inline std::uintptr_t JumpHeight;
        inline std::uintptr_t JumpPower;
        inline std::uintptr_t MaxHealth;
        inline std::uintptr_t MaxSlopeAngle;
        inline std::uintptr_t MoveDirection;
        inline std::uintptr_t MoveToPart;
        inline std::uintptr_t MoveToPoint;
        inline std::uintptr_t NameDisplayDistance;
        inline std::uintptr_t NameOcclusion;
        inline std::uintptr_t PlatformStand;
        inline std::uintptr_t PlatformStatePointer;
        inline std::uintptr_t RequiresNeck;
        inline std::uintptr_t RigType;
        inline std::uintptr_t SeatPart;
        inline std::uintptr_t Sit;
        inline std::uintptr_t TargetPoint;
        inline std::uintptr_t UseJumpPower;
        inline std::uintptr_t WalkTimer;
        inline std::uintptr_t Walkspeed;
        inline std::uintptr_t WalkspeedCheck;
    }

    namespace Instance
    {
        inline std::uintptr_t ChildrenEnd;
        inline std::uintptr_t ChildrenStart;
        inline std::uintptr_t ClassBase;
        inline std::uintptr_t ClassDescriptor;
        inline std::uintptr_t ClassName;
        inline std::uintptr_t Name;
        inline std::uintptr_t NameContainer;
        inline std::uintptr_t Parent;
        inline std::uintptr_t This;
    }

    namespace Lighting
    {
        inline std::uintptr_t Ambient;
        inline std::uintptr_t Brightness;
        inline std::uintptr_t ClockTime;
        inline std::uintptr_t ColorShift_Bottom;
        inline std::uintptr_t ColorShift_Top;
        inline std::uintptr_t EnvironmentDiffuseScale;
        inline std::uintptr_t EnvironmentSpecularScale;
        inline std::uintptr_t ExposureCompensation;
        inline std::uintptr_t FogColor;
        inline std::uintptr_t FogEnd;
        inline std::uintptr_t FogStart;
        inline std::uintptr_t GeographicLatitude;
        inline std::uintptr_t GlobalShadows;
        inline std::uintptr_t GradientBottom;
        inline std::uintptr_t GradientTop;
        inline std::uintptr_t LightColor;
        inline std::uintptr_t LightDirection;
        inline std::uintptr_t MoonPosition;
        inline std::uintptr_t OutdoorAmbient;
        inline std::uintptr_t Sky;
        inline std::uintptr_t Source;
        inline std::uintptr_t SunPosition;
    }

    namespace LocalScript
    {
        inline std::uintptr_t ByteCode;
        inline std::uintptr_t GUID;
        inline std::uintptr_t Hash;
    }

    namespace MaterialColors
    {
        inline std::uintptr_t Asphalt;
        inline std::uintptr_t Basalt;
        inline std::uintptr_t Brick;
        inline std::uintptr_t Cobblestone;
        inline std::uintptr_t Concrete;
        inline std::uintptr_t CrackedLava;
        inline std::uintptr_t Glacier;
        inline std::uintptr_t Grass;
        inline std::uintptr_t Ground;
        inline std::uintptr_t Ice;
        inline std::uintptr_t LeafyGrass;
        inline std::uintptr_t Limestone;
        inline std::uintptr_t Mud;
        inline std::uintptr_t Pavement;
        inline std::uintptr_t Rock;
        inline std::uintptr_t Salt;
        inline std::uintptr_t Sand;
        inline std::uintptr_t Sandstone;
        inline std::uintptr_t Slate;
        inline std::uintptr_t Snow;
        inline std::uintptr_t WoodPlanks;
    }

    namespace MeshPart
    {
        inline std::uintptr_t MeshId;
        inline std::uintptr_t Texture;
    }

    namespace Misc
    {
        inline std::uintptr_t Adornee;
        inline std::uintptr_t AnimationId;
        inline std::uintptr_t StringLength;
        inline std::uintptr_t Value;
    }

    namespace Model
    {
        inline std::uintptr_t PrimaryPart;
        inline std::uintptr_t Scale;
    }

    namespace ModuleScript
    {
        inline std::uintptr_t ByteCode;
        inline std::uintptr_t GUID;
        inline std::uintptr_t Hash;
        inline std::uintptr_t IsCoreScript;
    }

    namespace MouseService
    {
        inline std::uintptr_t InputObject;
        inline std::uintptr_t InputObject2;
        inline std::uintptr_t MousePosition;
        inline std::uintptr_t SensitivityPointer;
    }

    namespace ParticleEmitter
    {
        inline std::uintptr_t Acceleration;
        inline std::uintptr_t Brightness;
        inline std::uintptr_t Drag;
        inline std::uintptr_t Lifetime;
        inline std::uintptr_t LightEmission;
        inline std::uintptr_t LightInfluence;
        inline std::uintptr_t Rate;
        inline std::uintptr_t RotSpeed;
        inline std::uintptr_t Rotation;
        inline std::uintptr_t Speed;
        inline std::uintptr_t SpreadAngle;
        inline std::uintptr_t Texture;
        inline std::uintptr_t TimeScale;
        inline std::uintptr_t VelocityInheritance;
        inline std::uintptr_t ZOffset;
    }

    namespace Player
    {
        inline std::uintptr_t AccountAge;
        inline std::uintptr_t CameraMode;
        inline std::uintptr_t DisplayName;
        inline std::uintptr_t HealthDisplayDistance;
        inline std::uintptr_t LocalPlayer;
        inline std::uintptr_t LocaleId;
        inline std::uintptr_t MaxZoomDistance;
        inline std::uintptr_t MinZoomDistance;
        inline std::uintptr_t ModelInstance;
        inline std::uintptr_t Mouse;
        inline std::uintptr_t NameDisplayDistance;
        inline std::uintptr_t Team;
        inline std::uintptr_t TeamColor;
        inline std::uintptr_t UserId;
    }

    namespace PlayerMouse
    {
        inline std::uintptr_t Icon;
        inline std::uintptr_t Workspace;
    }

    namespace Primitive
    {
        inline std::uintptr_t AssemblyAngularVelocity;
        inline std::uintptr_t AssemblyLinearVelocity;
        inline std::uintptr_t Flags;
        inline std::uintptr_t Material;
        inline std::uintptr_t Owner;
        inline std::uintptr_t Position;
        inline std::uintptr_t Rotation;
        inline std::uintptr_t Size;
        inline std::uintptr_t Validate;
    }

    namespace PrimitiveFlags
    {
        inline std::uintptr_t Anchored;
        inline std::uintptr_t CanCollide;
        inline std::uintptr_t CanQuery;
        inline std::uintptr_t CanTouch;
    }

    namespace ProximityPrompt
    {
        inline std::uintptr_t ActionText;
        inline std::uintptr_t Enabled;
        inline std::uintptr_t GamepadKeyCode;
        inline std::uintptr_t HoldDuration;
        inline std::uintptr_t KeyCode;
        inline std::uintptr_t MaxActivationDistance;
        inline std::uintptr_t ObjectText;
        inline std::uintptr_t RequiresLineOfSight;
    }

    namespace RenderJob
    {
        inline std::uintptr_t FakeDataModel;
        inline std::uintptr_t RealDataModel;
        inline std::uintptr_t RenderView;
    }

    namespace RenderView
    {
        inline std::uintptr_t DeviceD3D11;
        inline std::uintptr_t LightingValid;
        inline std::uintptr_t SkyValid;
        inline std::uintptr_t VisualEngine;
    }

    namespace RunService
    {
        inline std::uintptr_t HeartbeatFPS;
        inline std::uintptr_t HeartbeatTask;
    }

    namespace Script
    {
        inline std::uintptr_t ByteCode;
        inline std::uintptr_t GUID;
        inline std::uintptr_t Hash;
    }

    namespace Seat
    {
        inline std::uintptr_t Occupant;
    }

    namespace Sky
    {
        inline std::uintptr_t MoonAngularSize;
        inline std::uintptr_t MoonTextureId;
        inline std::uintptr_t SkyboxBk;
        inline std::uintptr_t SkyboxDn;
        inline std::uintptr_t SkyboxFt;
        inline std::uintptr_t SkyboxLf;
        inline std::uintptr_t SkyboxOrientation;
        inline std::uintptr_t SkyboxRt;
        inline std::uintptr_t SkyboxUp;
        inline std::uintptr_t StarCount;
        inline std::uintptr_t SunAngularSize;
        inline std::uintptr_t SunTextureId;
    }

    namespace Sound
    {
        inline std::uintptr_t IsPlaying;
        inline std::uintptr_t Looped;
        inline std::uintptr_t PlaybackSpeed;
        inline std::uintptr_t RollOffMaxDistance;
        inline std::uintptr_t RollOffMinDistance;
        inline std::uintptr_t SoundGroup;
        inline std::uintptr_t SoundId;
        inline std::uintptr_t Volume;
    }

    namespace SpawnLocation
    {
        inline std::uintptr_t AllowTeamChangeOnTouch;
        inline std::uintptr_t Enabled;
        inline std::uintptr_t ForcefieldDuration;
        inline std::uintptr_t Neutral;
        inline std::uintptr_t TeamColor;
    }

    namespace SpecialMesh
    {
        inline std::uintptr_t MeshId;
        inline std::uintptr_t Scale;
    }

    namespace StatsItem
    {
        inline std::uintptr_t Value;
    }

    namespace SunRaysEffect
    {
        inline std::uintptr_t Enabled;
        inline std::uintptr_t Intensity;
        inline std::uintptr_t Spread;
    }

    namespace SurfaceAppearance
    {
        inline std::uintptr_t AlphaMode;
        inline std::uintptr_t Color;
        inline std::uintptr_t ColorMap;
        inline std::uintptr_t EmissiveMaskContent;
        inline std::uintptr_t EmissiveStrength;
        inline std::uintptr_t EmissiveTint;
        inline std::uintptr_t MetalnessMap;
        inline std::uintptr_t NormalMap;
        inline std::uintptr_t RoughnessMap;
    }

    namespace TaskScheduler
    {
        inline std::uintptr_t JobEnd;
        inline std::uintptr_t JobName;
        inline std::uintptr_t JobStart;
        inline std::uintptr_t MaxFPS;
        inline std::uintptr_t Pointer;
    }

    namespace Team
    {
        inline std::uintptr_t BrickColor;
    }

    namespace Terrain
    {
        inline std::uintptr_t GrassLength;
        inline std::uintptr_t MaterialColors;
        inline std::uintptr_t WaterColor;
        inline std::uintptr_t WaterReflectance;
        inline std::uintptr_t WaterTransparency;
        inline std::uintptr_t WaterWaveSize;
        inline std::uintptr_t WaterWaveSpeed;
    }

    namespace Textures
    {
        inline std::uintptr_t Decal_Texture;
        inline std::uintptr_t Texture_Texture;
    }

    namespace Tool
    {
        inline std::uintptr_t CanBeDropped;
        inline std::uintptr_t Enabled;
        inline std::uintptr_t Grip;
        inline std::uintptr_t ManualActivationOnly;
        inline std::uintptr_t RequiresHandle;
        inline std::uintptr_t TextureId;
        inline std::uintptr_t Tooltip;
    }

    namespace UnionOperation
    {
        inline std::uintptr_t AssetId;
    }

    namespace UserInputService
    {
        inline std::uintptr_t WindowInputState;
    }

    namespace VehicleSeat
    {
        inline std::uintptr_t MaxSpeed;
        inline std::uintptr_t SteerFloat;
        inline std::uintptr_t ThrottleFloat;
        inline std::uintptr_t Torque;
        inline std::uintptr_t TurnSpeed;
    }

    namespace VisualEngine
    {
        inline std::uintptr_t Dimensions;
        inline std::uintptr_t FakeDataModel;
        inline std::uintptr_t Pointer;
        inline std::uintptr_t RenderView;
        inline std::uintptr_t ViewMatrix;
    }

    namespace Weld
    {
        inline std::uintptr_t Part0;
        inline std::uintptr_t Part1;
    }

    namespace WeldConstraint
    {
        inline std::uintptr_t Part0;
        inline std::uintptr_t Part1;
    }

    namespace WindowInputState
    {
        inline std::uintptr_t CapsLock;
        inline std::uintptr_t CurrentTextBox;
    }

    namespace Workspace
    {
        inline std::uintptr_t CurrentCamera;
        inline std::uintptr_t DistributedGameTime;
        inline std::uintptr_t ReadOnlyGravity;
        inline std::uintptr_t World;
    }

    namespace World
    {
        inline std::uintptr_t AirProperties;
        inline std::uintptr_t FallenPartsDestroyHeight;
        inline std::uintptr_t Gravity;
        inline std::uintptr_t Primitives;
        inline std::uintptr_t worldStepsPerSec;
    }

    auto fetch() -> bool;
    auto last_error() -> const std::string&;
}
