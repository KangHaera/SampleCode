#include "CoinShop_Manager.h"

#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"

#include "Table/Table.h"
#include "Table/ItemMain.h"
#include "Table/CoinShopMain.h"
#include "Table/CoinShopCategory.h"

#include "FunctionLibrary_System.h"
#include "FunctionLibrary_Server.h"

#include "System/GameUISystem.h"

#include "User/User.h"
#include "User/Item/Item.h"
#include "User/Item/ItemInventory.h"

#include "Network/Server/Requester/ShopRequester.h"

#include "UI/Contents/CoinShop/ListItem/CoinShop_Tab_ListItem.h"
#include "UI/Contents/CoinShop/ListItem/CoinShop_Item_ListItem.h"


UCoinShop_Manager::UCoinShop_Manager(const FObjectInitializer &ObjectInitializer)
	: Super(ObjectInitializer), SelectTabIndex(0), SelectItemIndex(0), ServerSendType(EServerSendType::None)
{
}

void UCoinShop_Manager::InitWidget()
{
	Table = FunctionLibrary_System::GetTable(this);

	UE_BIND_WIDGET(UListView, ListView_CategoryTab);
	UE_BIND_WIDGET(UListView, ListView_CoinShop);

	UE_BIND_WIDGET(UImage, Icon_TargetCoin);

	UE_BIND_WIDGET(UTextBlock, Text_TargetName);
	UE_BIND_WIDGET(UTextBlock, Text_TargetAmount);
	UE_BIND_WIDGET(UTextBlock, Text_AllNeedCost);

	UE_BIND_WIDGET(UButton, Bt_AllRefresh);
	if (Bt_AllRefresh == nullptr || Bt_AllRefresh->IsValidLowLevel() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("Bt_AllRefresh is nullptr"));
		return;
	}
	Bt_AllRefresh->OnClicked.AddUniqueDynamic(this, &CoinShop_Manager::OnClick_AllRefresh);

	InitTab();
}

void UCoinShop_Manager::InitTab()
{
	if (Table == nullptr || Table->IsValidLowLevel() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("Table is nullptr"));
		return;
	}

	TArray<FCoinShopCategory> CategoryDataList;
	if (Table->GetAllCoinShopCategoryData(CategoryDataList) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FCoinShopCategory data is empty."));
		return;
	}

	if (ListView_CategoryTab == nullptr || ListView_CategoryTab->IsValidLowLevel() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ListView_CategoryTab is nullptr"));
		return;
	}

	for (int i = 0; i < CategoryDataList.Num(); ++i)
	{
		AddCategoryItem(CategoryDataList[i]);
	}

	ChangeTab(CategoryDataList[0].coin_shop_category);
	ListView_CategoryTab->RegenerateAllEntries();
}

void UCoinShop_Manager::ChangeTab(const int32 InTabIndex)
{
	if (SelectTabIndex == InTabIndex)
	{
		//  같은 탭 예외처리
		return;
	}

	//  이전 탭 off
	if (TabList.Contains(SelectTabIndex) == true)
	{
		TabList[SelectTabIndex]->IsActive = false;
	}

	//  탭 갱신
	SelectTabIndex = InTabIndex;

	if (TabList.Contains(SelectTabIndex) == true)
	{
		TabList[SelectTabIndex]->IsActive = true;
	}

	if (ListView_CategoryTab == nullptr || ListView_CategoryTab->IsValidLowLevel() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ListView_CategoryTab is nullptr"));
		return;
	}

	ListView_CategoryTab->RegenerateAllEntries();
	UpdateShopView();
}

void UCoinShop_Manager::UpdateShopView()
{
	//  이전 데이터 저장
	ClearItemListView();

	if (Table == nullptr || Table->IsValidLowLevel() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("Table is nullptr"));
		return;
	}

	//	테이블 데이터 가져오기
	TArray<FCoinShopMain> CoinShopItemList;
	if (Table->GetCoinShopMainDataListByCategoryID(SelectTabIndex, CoinShopItemList) == false)
	{
		UE_LOG(LogTemp, Warning
			, TEXT("Not Found FCoinShopMain Table Data.")
			, SelectTabIndex);
		return;
	}

	//  가지고 있는 코인 세팅
	SetTargetCostInfo(CoinShopItemList[0].cost_item_idx);

	//	가지고 있는 아이템 카운트 세팅
	UUser* User = UFunctionLibrary_System::GetUser(this);
	if (User == nullptr && User->IsValidLowLevel() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("User is nullptr"));
		return;
	}

	UItemInventory* Inven = User->GetInventoryByInvenType<UItemInventory>(EInvenType::Item);
	if (Inven == nullptr && Inven->IsValidLowLevel() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("Inven is nullptr"));
		return;
	}

	int32 TargetItemCount = Inven->GetInventoryItemCountByDataID(MainData.cost_item_idx);
	SetTargetAmountText(TargetItemCount);

	if (ListView_CoinShop == nullptr && ListView_CoinShop->IsValidLowLevel() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("mListView_CoinShop is nullptr"));
		return;
	}

	//	테이블 데이터만큼 위젯 생성
	for (int i = 0; i < CoinShopItemList.Num(); ++i)
	{
		AddCoinShopItem(CoinShopItemList[i]);
	}

	ListView_CoinShop->RegenerateAllEntries();

	SetAllNeedCount();
}

void UCoinShop_Manager::AddCoinShopItem(const FCoinShopMain& InCoinShopItemData)
{
	UCoinShop_Item_ListItem_ItemData* CreateData = nullptr;

	if (PoolItemList.Num() > 0)
	{
		CreateData = PoolItemList.Pop();
	}
	else
	{
		CreateData = NewObject<UCoinShop_Item_ListItem_ItemData>(this);
	}

	if (CreateData == nullptr && CreateData->IsValidLowLevel() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateData is nullptr"));
		continue;
	}

	FItemMain* CoinItemData = Table->GetItemMainDataByID(InCoinShopItemData.item_idx);
	if (CoinItemData == nullptr)
	{
		continue;
	}

	if (CoinItemData.icon_path.Get() == nullptr)
	{
		CoinItemData.icon_path.LoadSynchronous();
	}

	FItemMain* ChangeItemData = Table->GetItemMainDataByID(InCoinShopItemData.change_item_idx);
	if (ChangeItemData == nullptr)
	{
		continue;
	}

	if (ChangeItemData.icon_path.Get() == nullptr)
	{
		ChangeItemData.icon_path.LoadSynchronous();
	}

	CreateData.SetData(InCoinShopItemData.coin_shop_idx, CoinItemData.icon_path.Get(), InCoinShopItemData.cost_count
		, ChangeItemData.icon_path.Get(), InCoinShopItemData.change_count);
	CreateData->SetOnDataEvent(UListItemBase::FOnSetDataBase::CreateUObejct(this, &UCoinShop_Manager::OnListItemObjectSetEvent_ListView_CoinShop));

	ItemList.Emplace(CreateData);
	ListView_CoinShop->AddItem(CreateData);
}

void UCoinShop_Manager::AddCategoryItem(const FCoinShopCategory& InCategoiryData)
{
	UCoinShop_Tab_ListItem_ItemData* CreateData = NewObject<UCoinShop_Tab_ListItem_ItemData>(this);
	FItemMain* CoinItemData = Table->GetItemMainDataByID(InCategoiryData.item_idx);
	if (CoinItemData == nullptr)
	{
		continue;
	}

	if (CoinItemData.icon_path.Get() == nullptr)
	{
		CoinItemData.icon_path.LoadSynchronous();
	}

	CreateData->SetData(CoinItemData.icon_path.Get(), CoinItemData->name.ToString(), InCategoiryData.coin_shop_category);
	CreateData->SetActive(false);
	CreateData->SetOnDataEvent(UListItemBase::FOnSetDataBase::CreateUObejct(this, &UCoinShop_Manager::OnListItemObjectSet_Event_ListView_CategoryTab));

	TabList.Emplace(InCategoiryData.coin_shop_category, CreateData);
	ListView_CategoryTab->AddItem(CreateData);
}

void UCoinShop_Manager::RecvServerDataUpdateView()
{
	if (ItemList.Num() <= 0)
	{
		UpdateShopView();
		return;
	}

	FCoinShopMain MainData;
	if (Table->GetCoinShopMainDataAt(ItemList[0]->GetTableID(), MainData) == false)
	{
		UE_LOG(LogTemp, Warning
			, TEXT("Not Found FCoinShopMain Table Data. ItemList[0]->GetTableID() index is [%d]")
			, ItemList[0]->GetTableID());
		return;
	}

	//	가지고 있는 아이템 카운트 세팅
	UUser* User = UFunctionLibrary_System::GetUser(this);
	if (User == nullptr && User->IsValidLowLevel() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("User is nullptr"));
		return;
	}

	UItemInventory* Inven = User->GetInventoryByInvenType<UItemInventory>(EInvenType::Item);
	if (Inven == nullptr && Inven->IsValidLowLevel() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("Inven is nullptr"));
		return;
	}

	int32 TargetItemCount = Inven->GetInventoryItemCountByDataID(MainData.cost_item_idx);
	SetTargetAmountText(TargetItemCount);

	//	아이템 카운트 재세팅
	int32 ChangeItemHaveCount = Inven->GetInventoryItemCountByDataID(MainData.item_idx);
	for (int i = 0; i < ItemList.Num(); ++i)
	{
		if (ItemList[i]->GetTableID() == SelectItemIndex)
		{
			ItemList[i]->SetCount(0);
			ItemList[i]->SetChangeCoinHaveCount(ChangeItemHaveCount);
			SelectItemIndex = 0;
			break;
		}
	}

	if (ListView_CoinShop == nullptr && ListView_CoinShop->IsValidLowLevel() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("mListView_CoinShop is nullptr"));
		return;
	}
	ListView_CoinShop->RegenerateAllEntries();

	SetNeedCost();
}

void UCoinShop_Manager::ShowNotice(const int32 InTableID)
{
	if (Owner == nullptr && Owner->IsValidLowLevel() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("Owner is nullptr"));
		return;
	}

	FNoticeArgs Args;
	Args.TableID = InTableID;
	Args.NoticeTableType = ENoticeTableType::SystemMsg;

	Ower->ShowNotice(ENoticeWidget::Notice_Larg, &Args);
}

void UCoinShop_Manager::SetNeedCost()
{
	if (Text_AllNeedCost == nullptr || Text_AllNeedCost->IsValidLowLevel() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("Text_AllNeedCost is nullptr"));
		return;
	}

	int32 NeedCost = 0;
	for (int i = 0; i < ItemList.Num(); ++i)
	{
		if (ItemList[i] == nullptr || ItemList[i]->IsValidLowLevel() == false)
		{
			continue;
		}

		NeedCost += ItemList[i]->GetNeedCostCount();
	}

	Text_AllNeedCost->SetText(NeedCost == 0 ? TEXT() : FText::FromString(TEXT("-") + FText::AsNumber(NeedCost).ToString()));
}

void UCoinShop_Manager::SetTargetCostInfo(const int32 InItemTableID)
{
	if (Table == nullptr || Table->IsValidLowLevel() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("Table is nullptr"));
		return;
	}

	FItemMain* CoinItemData = Table->GetItemMainDataByID(InItemTableID);
	if (CoinItemData == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("CoinItemData is nullptr"));
		return;
	}

	if (CoinItemData.icon_path.Get() == nullptr)
	{
		CoinItemData.icon_path.LoadSynchronous();
	}

	if (Icon_TargetCoin == nullptr || Icon_TargetCoin->IsValidLowLevel() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("Icon_TargetCoin is nullptr"));
		return;
	}

	Icon_TargetCoin->SetBrushResourceObject(CoinItemData.icon_path.Get());

	if (Text_TargetName == nullptr || Text_TargetName->IsValidLowLevel() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("Text_TargetName is nullptr"));
		return;
	}

	FText HaveText;
	Table->GetUIMSgStringDataAt(EUIMsg::UseAmount, HaveText);	//	사용
	HaveText = FText::FromString(TEXT(" ") + CoinItemData.name.ToString() + HaveText.ToString());
	Text_TargetName->SetText(HaveText);
}

void UCoinShop_Manager::SetTargetAmountText(const int32 InHaveAmount)
{
	if (Text_TargetAmount == nullptr || Text_TargetAmount->IsValidLowLevel() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("Text_TargetAmount  is nullptr"));
		return;
	}

	Text_TargetAmount->SetText(FText::AsNumber(InHaveAmount));
}

void UCoinShop_Manager::ClearItemListView()
{
	if (ListView_CoinShop == nullptr || ListView_CoinShop->IsValidLowLevel() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ListView_CoinShop is nullptr"));
		return;
	}

	for (int i = 0; i < ListView_CoinShop->GetNumItems(); ++i)
	{
		PoolItemList.Emplace(Cast<UCoinShop_Item_ListItem_ItemData>(ListView_CoinShop->GetItemAt(i)));
	}

	ItemList.Empty();
	ListView_CoinShop->ClearListItems();
}

void UCoinShop_Manager::OnListItemObjectSet_Event_ListView_CategoryTab(UListItemBase* InItemBase)
{
	if (InItemBase == nullptr || InItemBase->IsValidLowLevel() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("InItemBase is nullptr"));
		return;
	}

	UCoinShop_Tab_ListItem* CastItem = Cast<UCoinShop_Tab_ListItem>(InItemBase);
	if (CastItem == nullptr || CastItem->IsValidLowLevel() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("CastItem is nullptr"));
		return;
	}

	CastItem->SetOnTabClickEvent(UCoinShop_Tab_ListItem::FOnClickTab::CreateUObject(this, &CoinShop_Manager::CallBack_ClickTab));
}

void UCoinShop_Manager::OnListItemObjectSetEvent_ListView_CoinShop(UListItemBase* InItemBase)
{
	if (InItemBase == nullptr || InItemBase->IsValidLowLevel() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("InItemBase is nullptr"));
		return;
	}

	UCoinShop_Item_ListItem* CastItem = Cast<UCoinShop_Item_ListItem>(InItemBase);
	if (CastItem == nullptr || CastItem->IsValidLowLevel() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("CastItem is nullptr"));
		return;
	}

	CastItem->SetOnClickBuy(UCoinShop_Item_ListItem::FOnCLickBuyEvent::CreateUObject(this, &CoinShop_Manager::CallBack_Buy));
	CastItem->SetUpdateEvent(UCoinShop_Item_ListItem::FOnUpdateEvent::CreateUObject(this, &CoinShop_Manager::CallBack_Update));
}

void UCoinShop_Manager::CallBack_Update()
{
	SetNeedCost();
}

void UCoinShop_Manager::OnClick_AllRefresh()
{
	if (ListView_CoinShop == nullptr || ListView_CoinShop->IsValidLowLevel() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ListView_CoinShop is nullptr"));
		return;
	}

	for (int i = 0; i < ItemList.Num(); ++i)
	{
		if (ItemList[i] == nullptr || ItemList[i]->IsValidLowLevel() == false)
		{
			continue;
		}
		ItemList[i]->SetCount(0);
	}

	ListView_CoinShop->RegenerateAllEntries();
}

void UCoinShop_Manager::CallBack_ClickTab(const int32& InTabIndex)
{
	ChangeTab(InTabIndex);
}

void UCoinShop_Manager::CallBack_Buy(const int32 InTableID, const int32 InCount)
{
	if (ServerSendType == EServerSendType::Send)
	{
		return;
	}

	if (InTableID == INDEX_NONE)
	{
		ShowNotice(static_cast<int32>(InCount <= 0 ? ESysMsg::SysMsg_ShopReceiptInput : ESysMsg::SysMsg_ShopInsuffcient));
		return;
	}

	SelectItemIndex = InTableID;

	UShopRequester* Requester = UFunctionLibrary_Server::GetRequester<UShopRequester>();
	if (Requester == nullptr || Requester->IsValidLowLevel() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("Requester is nullptr"));
		return;
	}

	UUser* User = UFunctionLibrary_System::GetUser();
	if (User == nullptr || User->IsValidLowLevel() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("User is nullptr"));
		return;
	}

	Requester->Send_CoinShopItem(this, User->GetUserInfo(), User->GetRandKey(), InTableID, InCount);

	ServerSendType = EServerSendType::Send;
}

void UCoinShop_Manager::Recv_CoinShop_Item()
{
	ServerSendType = EServerSendType::Recv;
	RecvServerDataUpdateView();
	ShowNotice(static_cast<int32>(ESysMsg::SysMsg_TradeComplete));
}
