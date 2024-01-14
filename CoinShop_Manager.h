#pragma once

#include “CoreMinimal.h”
#include “UI/Base/ManagerBase.h”
#include “CoinShop_Manager.generated.h”

class UButton;
class UImage;
class UListView;
class UTextBlock;

class Table;

class UListItmeBase;
class UCoinShop_Tab_ListItem_ItemData;
class UCoinShop_Item_ListItem_ItemData;

UClass()
Class ProjectAPI UCoinShop_Manager : public UManagerBase
{
GENERATED_BODY()

public:
	UCoinShop_Manager();
	virtual void InitWidget();

private:
#pragma region init
	void InitTab();
#pragma endregion init

#pragma region main process
	void ChangeTab(const int32 InTabIndex);
	void UpdateShopView();
	void AddCoinShopItem(const FCoinShopMain& InCoinShopItemData);
	void AddCategoryItem(const FCoinShopCategory& InCategoiryData);
	void RecvServerDataUpdateView();
	void ShowNotice(const int32 InTableID);
#pragma endregion main process

#pragma region setting ui
	void SetNeedCost();
	void SetTargetCostInfo(const int32 InItemTableID);
	void SetTargetAmountText(const int32 InHaveAmount);
#pragma endregion setting ui

#pragma region listview event
	UFUNCTION() void OnListItemObjectSet_Event_ListView_Tab(UListItemBase* InItemBase);
	UFUNCTION() void OnListItemObjectSetEvent_ListView_Shop(UListItemBase* InItemBase);
#pragma endregion listview event

#pragma region onclick event
	UFUNCTION() void OnClick_AllRefresh();					//	초기화 버튼
#pragma endregion onclick event 

#pragma region callback event
	void CallBack_Update();							//	+, - 버튼 인풋
	void CallBack_ClickTab(const int32& InTabIndex);			//	탭 변경 
	void CallBack_Buy(const int32 InTableID, const int32 InCount);		//	아이템 교환
#pragma endregion callback event

private:
#pragma region bind widget
	UTextBlock* Text_TargetName;		  	// ex) 길드 주화 사용
	UTextBlock* Text_TargetAmount;			// 현재 가지고 있는 수
	UTextBlock* Text_AllNeedCost;		  	// - 3,000 전체 얼만큼 빠질지.

	UListView* ListView_CategoryTab;
	UListView* ListView_CoinShop;

	UButton* Bt_AllRefresh;

	UImage* Icon_TargetCoin;
#pragma endregion bind widget

#pragma region listview item data
	UPROPERTY() TMap<int32, UCoinShop_Tab_ListItem_ItemData*> TabList;
	UPROPERTY() TArray<UCoinShop_Item_ListItem_ItemData*> ItemList;
	UPROPERTY() TArray<UCoinShop_Item_ListItem_ItemData*> PoolList;
#pragma endregion listview item data

	UPROPERTY() UTable* Table = nullptr;
	EServerSendType ServerSendType;

	int32 SelectTabIndex;
	int32 SelectItemIndex;
}
