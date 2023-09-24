// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma de Barcelona (UAB). This work is licensed under the terms of the MIT license. For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "CoreMinimal.h"

#include "Carla/Sensor/Sensor.h"

#include "Carla/Actor/ActorDefinition.h"

#include <compiler/disable-ue4-macros.h>
#include <carla/sensor/data/RadioData.h>
#include <compiler/enable-ue4-macros.h>

#include "RadioSensor.generated.h"

/**
 * 
 */
UCLASS()
class CARLA_API ARadioSensor : public ASensor
{
	GENERATED_BODY()
	
	using FRadioData = carla::sensor::data::RadioData;

public:

  static FActorDefinition GetSensorDefinition();

  ARadioSensor(const FObjectInitializer &ObjectInitializer);

  void Set(const FActorDescription &Description) override;

  UFUNCTION(BlueprintCallable, Category = "Radio")
  void SetHorizontalFOV(float NewHorizontalFOV);

  UFUNCTION(BlueprintCallable, Category = "Radio")
  void SetVerticalFOV(float NewVerticalFOV);

  UFUNCTION(BlueprintCallable, Category = "Radio")
  void SetRange(float NewRange);

  UFUNCTION(BlueprintCallable, Category = "Radio")
  void SetPointsPerSecond(int NewPointsPerSecond);

protected:

  void BeginPlay() override;

  // virtual void PrePhysTick(float DeltaTime) override;
  virtual void PostPhysTick(UWorld *World, ELevelTick TickType, float DeltaTime) override;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Detection")
  float Range;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Detection")
  float HorizontalFOV;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Detection")
  float VerticalFOV;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Detection")
  int PointsPerSecond;

private:

  void CalculateCurrentVelocity(const float DeltaTime);

  void SendLineTraces(float DeltaTime);

  float CalculateRelativeVelocity(const FHitResult& OutHit, const FVector& RadarLocation);

  FRadioData RadioData;

  FCollisionQueryParams TraceParams;

  FVector CurrentVelocity;

  /// Used to compute the velocity of the radar
  FVector PrevLocation;

  struct RayData {
    float Radius;
    float Angle;
    bool Hitted;
    int HittedActor;
    float RelativeVelocity;
    FVector2D AzimuthAndElevation;
    float Distance;
  };

  std::vector<RayData> Rays;
};