// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma de Barcelona (UAB). This work is licensed under the terms of the MIT license. For a copy, see <https://opensource.org/licenses/MIT>.


#include "Carla/Sensor/RadioSensor.h"



// Copyright (c) 2019 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include <PxScene.h>

#include "Carla.h"
#include "Carla/Actor/ActorBlueprintFunctionLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Runtime/Core/Public/Async/ParallelFor.h"

#include <compiler/disable-ue4-macros.h>
#include "carla/geom/Math.h"
#include "carla/ros2/ROS2.h"
#include <compiler/enable-ue4-macros.h>

FActorDefinition ARadioSensor::GetSensorDefinition()
{
  return UActorBlueprintFunctionLibrary::MakeRadioDefinition();
}

ARadioSensor::ARadioSensor(const FObjectInitializer& ObjectInitializer)
  : Super(ObjectInitializer)
{
  PrimaryActorTick.bCanEverTick = true;

  RandomEngine = CreateDefaultSubobject<URandomEngine>(TEXT("RandomEngine"));

  TraceParams = FCollisionQueryParams(FName(TEXT("Laser_Trace")), true, this);
  TraceParams.bTraceComplex = true;
  TraceParams.bReturnPhysicalMaterial = false;

}

void ARadioSensor::Set(const FActorDescription &ActorDescription)
{
  Super::Set(ActorDescription);
  UActorBlueprintFunctionLibrary::SetRadioSensor(ActorDescription, this);
}

void ARadioSensor::SetHorizontalFOV(float NewHorizontalFOV)
{
  HorizontalFOV = NewHorizontalFOV;
}

void  ARadioSensor::SetVerticalFOV(float NewVerticalFOV)
{
  VerticalFOV = NewVerticalFOV;
}

void ARadioSensor::SetRange(float NewRange)
{
  Range = NewRange;
}

void ARadioSensor::SetPointsPerSecond(int NewPointsPerSecond)
{
  PointsPerSecond = NewPointsPerSecond;
  RadioData.SetResolution(PointsPerSecond);
}

void ARadioSensor::BeginPlay()
{
  Super::BeginPlay();

  PrevLocation = GetActorLocation();
}

void ARadioSensor::PostPhysTick(UWorld *World, ELevelTick TickType, float DeltaTime)
{
  TRACE_CPUPROFILER_EVENT_SCOPE(ARadioSensor::PostPhysTick);
  CalculateCurrentVelocity(DeltaTime);

  RadioData.Reset();
  SendLineTraces(DeltaTime);

  auto DataStream = GetDataStream(*this);

  // ROS2
  #if defined(WITH_ROS2)
  auto ROS2 = carla::ros2::ROS2::GetInstance();
  if (ROS2->IsEnabled())
  {
    TRACE_CPUPROFILER_EVENT_SCOPE_STR("ROS2 Send");
    auto StreamId = carla::streaming::detail::token_type(GetToken()).get_stream_id();
    AActor* ParentActor = GetAttachParentActor();
    if (ParentActor)
    {
      FTransform LocalTransformRelativeToParent = GetActorTransform().GetRelativeTransform(ParentActor->GetActorTransform());
      ROS2->ProcessDataFromRadio(DataStream.GetSensorType(), StreamId, LocalTransformRelativeToParent, RadioData, this);
    }
    else
    {
      ROS2->ProcessDataFromRadio(DataStream.GetSensorType(), StreamId, DataStream.GetSensorTransform(), RadioData, this);
    }
  }
  #endif

  {
    TRACE_CPUPROFILER_EVENT_SCOPE_STR("Send Stream");
    DataStream.SerializeAndSend(*this, RadioData, DataStream.PopBufferFromPool());
  }
}

void ARadioSensor::CalculateCurrentVelocity(const float DeltaTime)
{
  const FVector RadarLocation = GetActorLocation();
  CurrentVelocity = (RadarLocation - PrevLocation) / DeltaTime;
  PrevLocation = RadarLocation;
}

void ARadioSensor::SendLineTraces(float DeltaTime)
{
  TRACE_CPUPROFILER_EVENT_SCOPE(ARadioSensor::SendLineTraces);
  constexpr float TO_METERS = 1e-2;
  const FTransform& ActorTransform = GetActorTransform();
  const FRotator& TransformRotator = ActorTransform.Rotator();
  const FVector& RadarLocation = GetActorLocation();
  const FVector& ForwardVector = GetActorForwardVector();
  const FVector TransformXAxis = ActorTransform.GetUnitAxis(EAxis::X);
  const FVector TransformYAxis = ActorTransform.GetUnitAxis(EAxis::Y);
  const FVector TransformZAxis = ActorTransform.GetUnitAxis(EAxis::Z);

  // Maximum radar radius in horizontal and vertical direction
  const float MaxRx = FMath::Tan(FMath::DegreesToRadians(HorizontalFOV * 0.5f)) * Range;
  const float MaxRy = FMath::Tan(FMath::DegreesToRadians(VerticalFOV * 0.5f)) * Range;
  const int NumPoints = (int)(PointsPerSecond * DeltaTime);

  // Generate the parameters of the rays in a deterministic way
  Rays.clear();
  Rays.resize(NumPoints);
  for (int i = 0; i < Rays.size(); i++) {
    Rays[i].Radius = RandomEngine->GetUniformFloat();
    Rays[i].Angle = RandomEngine->GetUniformFloatInRange(0.0f, carla::geom::Math::Pi2<float>());
    Rays[i].Hitted = false;
  }

  FCriticalSection Mutex;
  GetWorld()->GetPhysicsScene()->GetPxScene()->lockRead();
  {
    TRACE_CPUPROFILER_EVENT_SCOPE(ParallelFor);
    ParallelFor(NumPoints, [&](int32 idx) {
      TRACE_CPUPROFILER_EVENT_SCOPE(ParallelForTask);
      FHitResult OutHit(ForceInit);
      const float Radius = Rays[idx].Radius;
      const float Angle  = Rays[idx].Angle;

      float Sin, Cos;
      FMath::SinCos(&Sin, &Cos, Angle);

      const FVector EndLocation = RadarLocation + TransformRotator.RotateVector({
        Range,
        MaxRx * Radius * Cos,
        MaxRy * Radius * Sin
      });

      const bool Hitted = GetWorld()->ParallelLineTraceSingleByChannel(
        OutHit,
        RadarLocation,
        EndLocation,
        ECC_GameTraceChannel2,
        TraceParams,
        FCollisionResponseParams::DefaultResponseParam
      );

      const TWeakObjectPtr<AActor> HittedActor = OutHit.Actor;
      if (Hitted && HittedActor.Get()) {
        Rays[idx].Hitted = true;

        Rays[idx].RelativeVelocity = CalculateRelativeVelocity(OutHit, RadarLocation);

        Rays[idx].AzimuthAndElevation = FMath::GetAzimuthAndElevation (
          (EndLocation - RadarLocation).GetSafeNormal() * Range,
          TransformXAxis,
          TransformYAxis,
          TransformZAxis
        );

        Rays[idx].Distance = OutHit.Distance * TO_METERS;
        //Rays[idx].HittedActor = TCHAR_TO_UTF8(*HittedActor->GetActorGuid().ToString());
        //Rays[idx].HittedActor = HittedActor->GetActorLabel();
        Rays[idx].HittedActorId = HittedActor->GetUniqueID();
      }
    });
  }
  GetWorld()->GetPhysicsScene()->GetPxScene()->unlockRead();

  // Write the detections in the output structure
  for (auto& ray : Rays) {
    if (ray.Hitted) {
      RadioData.WriteDetection({        
        ray.HittedActorId
      });
    }
  }

}

float ARadioSensor::CalculateRelativeVelocity(const FHitResult& OutHit, const FVector& RadarLocation)
{
  constexpr float TO_METERS = 1e-2;

  const TWeakObjectPtr<AActor> HittedActor = OutHit.Actor;
  const FVector TargetVelocity = HittedActor->GetVelocity();
  const FVector TargetLocation = OutHit.ImpactPoint;
  const FVector Direction = (TargetLocation - RadarLocation).GetSafeNormal();
  const FVector DeltaVelocity = (TargetVelocity - CurrentVelocity);
  const float V = TO_METERS * FVector::DotProduct(DeltaVelocity, Direction);

  return V;
}
