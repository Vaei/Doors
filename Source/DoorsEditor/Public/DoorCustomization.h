// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

/**
 * 
 */
class DOORSEDITOR_API FDoorCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
