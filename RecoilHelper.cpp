#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "Math/UnrealMathUtility.h"
#include "Kismet/KismetMathLibrary.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"

UCLASS(Blueprintable)
class URecoilHelper :public UObject
{
	GENERATED_BODY()

  //Constructor
		URecoilHelper()
	{
	  //Just setting the Player controller reference so that you won't have to do it again before calling the functions.
		PCRef = UGameplayStatics::GetPlayerController(this, 0);
	}
public:
 // **REQUIRED** Set the recoil curve before calling the recoil functions

	UPROPERTY(BlueprintReadWrite)
		UCurveVector* RecoilCurve;

	UPROPERTY(BlueprintReadWrite)
		bool bRecoil;
	UPROPERTY(BlueprintReadWrite)
		bool Firing;
	UPROPERTY(BlueprintReadWrite)
		bool bRecoilRecovery;

//Timer Handles
	UPROPERTY(BlueprintReadwrite)
		FTimerHandle FireTimer;
	UPROPERTY(BlueprintReadwrite)
		FTimerHandle RecoveryTimer;

/*Optional variables to customize how fast the recoil resets and what is the max time 
	upto which the recovery can last */

	UPROPERTY(BlueprintReadWrite)
		float RecoveryTime = 1.0f;
	UPROPERTY(BlueprintReadWrite)
		float RecoverySpeed =10.0f;

//Rotators

	//Control rotation at the start of the recoil
	UPROPERTY()
		FRotator RecoilStartRot;
	//Control rotation change due to recoil
	UPROPERTY()
		FRotator RecoilDeltaRot;
	//Control rotation chnage due to player moving the mouse
	UPROPERTY()
		FRotator PlayerDeltaRot;
	//Temporary variable used in tick
	UPROPERTY(BlueprintReadWrite)
		FRotator Del;

//Player controller reference
	UPROPERTY(BlueprintReadWrite)
		APlayerController* PCRef;

//Call this function when the firing begins, the recoil starts here
	UFUNCTION(BlueprintCallable)
		void RecoilStart()
	{
		if (RecoilCurve)
		{

		//Setting all rotators to default values

			PlayerDeltaRot = FRotator(0.0f, 0.0f, 0.0f);
			RecoilDeltaRot = FRotator(0.0f, 0.0f, 0.0f);
			Del = FRotator(0.0f, 0.0f, 0.0f);
			RecoilStartRot = PCRef->GetControlRotation();

			Firing = true;

		//Timer for the recoil: I have set it to 10s but dependeding how long it takes to empty the gun mag, you can increase the time.
			
			GetWorld()->GetTimerManager().SetTimer(FireTimer, this, &URecoilHelper::RecoilTimerFunction, 10.0f, false);
			
			bRecoil = true;
			bRecoilRecovery = false;
		}
	}

  //Automatically called in RecoilStart(), no need to call this explicitly

		UFUNCTION()
		void RecoilTimerFunction()
	{
		bRecoil = false;
		GetWorld()->GetTimerManager().PauseTimer(FireTimer);
	}
  //Called when firing stops
	UFUNCTION(BlueprintCallable)
		void RecoilStop()
	{
		Firing = false;
	}
	UPROPERTY(BlueprintReadWrite)
		float FireRate;

  //This function is automatically called, no need to call this. It is inside the Tick function
	
	UFUNCTION()
		void RecoveryStart()
	{
		bRecoilRecovery = true;
		GetWorld()->GetTimerManager().SetTimer(RecoveryTimer, this, &URecoilHelper::RecoveryTimerFunction, RecoveryTime, false);

	}
	

  //This function too is automatically called from the recovery start function.

	UFUNCTION()
		void RecoveryTimerFunction()
	{
		bRecoilRecovery = false;
	}
  //Needs to be called on event tick to update the control rotation.
	UFUNCTION(BlueprintCallable)
		void RecoilTick(float DeltaTime)
	{
		float recoiltime;
		FVector RecoilVec;
		if (bRecoil)
		{

		  //Calculation of control rotation to update 

			recoiltime = GetWorld()->GetTimerManager().GetTimerElapsed(FireTimer);
			RecoilVec = RecoilCurve->GetVectorValue(recoiltime);
			Del.Roll = 0;
			Del.Pitch = (RecoilVec.Y);
			Del.Yaw = (RecoilVec.Z);
			PlayerDeltaRot = PCRef->GetControlRotation() - RecoilStartRot - RecoilDeltaRot;
			PCRef->SetControlRotation(RecoilStartRot + PlayerDeltaRot + Del);
			RecoilDeltaRot = Del;

		//Conditionally start resetting the recoil

			if (!Firing)
			{
				if (recoiltime > FireRate)
				{
					GetWorld()->GetTimerManager().ClearTimer(FireTimer);
					RecoilStop();
					bRecoil = false;
					RecoveryStart();

				}
			}
		}
		else if (bRecoilRecovery)
		{
		  //Recoil resetting
			FRotator tmprot = PCRef->GetControlRotation();
			PCRef->SetControlRotation(UKismetMathLibrary::RInterpTo(PCRef->GetControlRotation(), PCRef->GetControlRotation() - RecoilDeltaRot, DeltaTime, RecoverySpeed));
			RecoilDeltaRot = RecoilDeltaRot + (PCRef->GetControlRotation() - tmprot);

		}
	}
};
