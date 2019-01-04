#include "cbase.h"

#include "ClientTimesDisplay.h"
#include "inputsystem/iinputsystem.h"
#include <voice_status.h>

#include <vgui/ILocalize.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>

#include <vgui_controls/ImageList.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/SectionedListPanel.h>
#include "vgui_controls/TextImage.h"
#include "vgui_controls/ToggleButton.h"
#include "LeaderboardsContextMenu.h"

#include "filesystem.h"
#include <util/mom_util.h>
#include "vgui_avatarimage.h"
#include "UtlSortVector.h"
#include <time.h>
#include "IMessageboxPanel.h"
#include "run/mom_replay_factory.h"
#include "fmtstr.h"
#include "clientmode.h"
#include "lobby/SavelocRequestFrame.h"
#include "lobby/LobbyMembersPanel.h"
#include "mom_modulecomms.h"
#include "run/mom_replay_base.h"
#include "mom_api_requests.h"
#include "mom_run_poster.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define RANKSTRING "00000"               // A max of 99999 ranks (too generous)
#define DATESTRING "59 minutes ago" // Entire date string
#define TIMESTRING "00:00:00.000"        // Entire time string

#define UPDATE_INTERVAL 15.0f  // The amount of seconds minimum between online checks

#define ENABLE_ONLINE_LEADERBOARDS 1 // MOM_TODO: Removeme when working on the online section

const char* const g_pszTimesStatusStrings[] = {
    "", // STATUS_TIMES_LOADED
    "#MOM_API_WaitingForResponse", // STATUS_TIMES_LOADING
    "#MOM_API_NoTimesReturned", // STATUS_NO_TIMES_RETURNED
    "#MOM_API_ServerError", // STATUS_SERVER_ERROR
    "#MOM_API_NoPBSet", // STATUS_NO_PB_SET
    "#MOM_API_NoFriends", // STATUS_NO_FRIENDS
    "#MOM_API_UnauthFriendsList", // STATUS_UNAUTHORIZED_FRIENDS_LIST
};

class CUtlSortVectorTimeValue
{
public:
    bool Less(CMomReplayBase *lhs, CMomReplayBase *rhs, void *) const
    {
        return lhs->GetRunTime() < rhs->GetRunTime();
    }
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CClientTimesDisplay::CClientTimesDisplay(IViewPort *pViewPort) : EditablePanel(nullptr, PANEL_TIMES),
    m_bUnauthorizedFriendlist(false)
{
    SetSize(10, 10); // Quiet the "parent not sized yet" spew, actual size in leaderboards.res

    m_iPlayerIndexSymbol = KeyValuesSystem()->GetSymbolForString("playerIndex");
    m_nCloseKey = BUTTON_CODE_INVALID;

    m_iSectionId = 0;

    m_bGetTop10Scores = true;
    m_bLoadedLocalPlayerAvatar = false;

    m_pViewPort = pViewPort;
    // initialize dialog
    SetProportional(true);
    SetKeyBoardInputEnabled(false);
    SetMouseInputEnabled(false);
    // Create a "popup" so we can get the mouse to detach
    surface()->CreatePopup(GetVPanel(), false, false, false, false, false);

    // set the scheme before any child control is created
    // SetScheme("ClientScheme");
    SetScheme(scheme()->LoadSchemeFromFile("resource/LeaderboardsScheme.res", "LeaderboardsScheme"));

    m_pLobbyMembersPanel = new LobbyMembersPanel(this);
    m_pSavelocReqFrame = new SavelocReqFrame();

    m_pLeaderboardReplayCMenu = new CLeaderboardsContextMenu(this);

    LoadControlSettings("resource/ui/leaderboards.res");

    m_pHeader = FindChildByName("Header", true);
    m_pMapName = FindControl<Label>("MapName", true);
    m_pMapAuthor = FindControl<Label>("MapAuthor", true);
    m_pMapDetails = FindControl<Label>("MapDetails", true);
    m_pPlayerStats = FindChildByName("PlayerStats", true);
    m_pPlayerAvatar = FindControl<ImagePanel>("PlayerAvatar", true);
    m_pPlayerName = FindControl<Label>("PlayerName", true);
    m_pPlayerMapRank = FindControl<Label>("PlayerMapRank", true);
    m_pPlayerPersonalBest = FindControl<Label>("PlayerPersonalBest", true);
    m_pPlayerGlobalRank = FindControl<Label>("PlayerGlobalRank", true);
    m_pPlayerExperience = FindControl<Label>("PlayerExperience", true);
    m_pOnlineTimesStatus = FindControl<Label>("OnlineTimesStatus", true);
    m_pLeaderboards = FindChildByName("Leaderboards", true);
    m_pOnlineLeaderboards = FindControl<SectionedListPanel>("OnlineLeaderboards", true);
    m_pAroundLeaderboards = FindControl<SectionedListPanel>("AroundLeaderboards", true);
    m_pLocalLeaderboards = FindControl<SectionedListPanel>("LocalLeaderboards", true);
    m_pFriendsLeaderboards = FindControl<SectionedListPanel>("FriendsLeaderboards", true);
    m_pGlobalLeaderboardsButton = FindControl<Button>("GlobalLeaderboardsButton", true);
    m_pGlobalTop10Button = FindControl<Button>("GlobalTop10Button", true);
    m_pGlobalAroundButton = FindControl<Button>("GlobalAroundButton", true);
    m_pFriendsLeaderboardsButton = FindControl<Button>("FriendsLeaderboardsButton", true);
    m_pLocalLeaderboardsButton = FindControl<Button>("LocalLeaderboardsButton", true);
    m_pRunFilterButton = FindControl<ToggleButton>("FilterButton", true);
    m_pFilterPanel = FindControl<EditablePanel>("FilterPanel", true);
    m_pFilterPanel->LoadControlSettings("resource/ui/leaderboards_filter.res");

    m_pCurrentLeaderboards = m_pLocalLeaderboards;

    if (!m_pHeader || !m_pMapName || !m_pPlayerStats || !m_pPlayerAvatar || !m_pPlayerName ||
        !m_pPlayerMapRank || !m_pPlayerGlobalRank || !m_pLeaderboards || !m_pOnlineLeaderboards || !m_pAroundLeaderboards ||
        !m_pLocalLeaderboards || !m_pFriendsLeaderboards || !m_pPlayerPersonalBest || !m_pOnlineTimesStatus ||
        !m_pPlayerExperience || !m_pGlobalLeaderboardsButton || !m_pFriendsLeaderboardsButton ||
        !m_pGlobalTop10Button || !m_pGlobalAroundButton)
    {
        AssertMsg(0, "Null pointer(s) on scoreboards");
    }

    // Pin to each other
    m_pLeaderboards->PinToSibling(m_pHeader, PIN_TOPLEFT, PIN_BOTTOMLEFT);
    m_pPlayerStats->PinToSibling(m_pLeaderboards, PIN_TOPLEFT, PIN_BOTTOMLEFT);
    m_pFilterPanel->PinToSibling(m_pLeaderboards, PIN_TOPLEFT, PIN_TOPRIGHT);

    // Override the parents of the controls (the current parent is this)
    m_pMapName->SetParent(m_pHeader);
    m_pMapAuthor->SetParent(m_pHeader);
    m_pMapDetails->SetParent(m_pHeader);
    m_pPlayerAvatar->SetParent(m_pPlayerStats);
    m_pPlayerName->SetParent(m_pPlayerStats);
    m_pPlayerMapRank->SetParent(m_pPlayerStats);
    m_pPlayerGlobalRank->SetParent(m_pPlayerStats);
    m_pPlayerPersonalBest->SetParent(m_pPlayerStats);
    m_pPlayerExperience->SetParent(m_pPlayerStats);
    m_pOnlineLeaderboards->SetParent(m_pLeaderboards);
    m_pAroundLeaderboards->SetParent(m_pLeaderboards);
    m_pOnlineTimesStatus->SetParent(m_pLeaderboards);
    m_pLocalLeaderboards->SetParent(m_pLeaderboards);
    m_pFriendsLeaderboards->SetParent(m_pLeaderboards);
    m_pGlobalLeaderboardsButton->SetParent(m_pLeaderboards);
    m_pGlobalTop10Button->SetParent(m_pLeaderboards);
    m_pGlobalAroundButton->SetParent(m_pLeaderboards);
    m_pFriendsLeaderboardsButton->SetParent(m_pLeaderboards);
    m_pLocalLeaderboardsButton->SetParent(m_pLeaderboards);
    m_pRunFilterButton->SetParent(m_pLeaderboards);

    m_pLobbyMembersPanel->SetParent(m_pPlayerStats);

    // Get rid of the scrollbars for the panels
    // MOM_TODO: Do we want the player to be able to explore the ranks?
    m_pOnlineLeaderboards->SetVerticalScrollbar(false);
    m_pAroundLeaderboards->SetVerticalScrollbar(false);
    m_pLocalLeaderboards->SetVerticalScrollbar(false);
    m_pFriendsLeaderboards->SetVerticalScrollbar(false);

    m_iDesiredHeight = GetTall();

    // update scoreboard instantly if on of these events occur
    ListenForGameEvent("replay_save");
    ListenForGameEvent("run_upload");

    m_fLastHeaderUpdate = 0.0f;
    m_flTimesLastUpdate[TIMES_TOP10] = m_flTimesLastUpdate[TIMES_AROUND] = m_flTimesLastUpdate[TIMES_FRIENDS] = 0.0f;

    m_bFirstHeaderUpdate = true;

    m_bMapInfoLoaded = false;

    flaggedRuns = RUNFLAG_NONE;
    SetDefLessFunc(m_umMapNames);
    SetDefLessFunc(m_mapAvatarsToImageList);

#if ENABLE_ONLINE_LEADERBOARDS
    // MOM_TODO: HACKHACK: this is changed to false because deleting a scheme image is a no-no.
    // While we do know that the image list will only hold scheme and avatar images, we cannot delete
    // either one. We do not have to delete the scheme images, as they are cached, and the avatar images
    // are also cached, but indefinitely. There's a memory leak with avatar images, since every image just
    // keeps creating Texture IDs and never destroying them, so if you download a lot of *different* avatars 
    // (play a lot of maps and look at the leaderboards for them), you could start to see the perf impact of it.
    // I'll leave it as a HACKHACK for now because this is ugly and that memory needs freed after a while, but may
    // be unnoticeable for most people... we'll see how big the memory leak impact really is.
    m_pImageList = new ImageList(false);
    SetupIcons();

    SetDefLessFunc(m_mapReplayDownloads);

#if ENABLE_STEAM_LEADERBOARDS
    m_hCurrentLeaderboard = 0;
#endif
#else
    m_pImageList = nullptr;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CClientTimesDisplay::~CClientTimesDisplay()
{
    m_pCurrentLeaderboards = nullptr;

    if (m_pImageList)
        delete m_pImageList;

    m_umMapNames.RemoveAll();
    m_mapAvatarsToImageList.RemoveAll();

    if (m_pLeaderboardReplayCMenu)
    {
        m_pLeaderboardReplayCMenu->DeletePanel();
        m_pLeaderboardReplayCMenu = nullptr;
    }
#if ENABLE_STEAM_LEADERBOARDS
    if (m_cLeaderboardFindResult.IsActive())
        m_cLeaderboardFindResult.Cancel();

    if (m_cLeaderboardFriendsScoresDownloaded.IsActive())
        m_cLeaderboardFriendsScoresDownloaded.Cancel();

    if (m_cLeaderboardGlobalScoresDownloaded.IsActive())
        m_cLeaderboardGlobalScoresDownloaded.Cancel();
#endif
}

//-----------------------------------------------------------------------------
// Call every frame
//-----------------------------------------------------------------------------
void CClientTimesDisplay::OnThink()
{
    BaseClass::OnThink();

    // NOTE: this is necessary because of the way input works.
    // If a key down message is sent to vgui, then it will get the key up message
    // Sometimes the scoreboard is activated by other vgui menus,
    // sometimes by console commands. In the case where it's activated by
    // other vgui menus, we lose the key up message because this panel
    // doesn't accept keyboard input. It *can't* accept keyboard input
    // because another feature of the dialog is that if it's triggered
    // from within the game, you should be able to still run around while
    // the scoreboard is up. That feature is impossible if this panel accepts input.
    // because if a vgui panel is up that accepts input, it prevents the engine from
    // receiving that input. So, I'm stuck with a polling solution.
    //
    // Close key is set to non-invalid when something other than a keybind
    // brings the scoreboard up, and it's set to invalid as soon as the
    // dialog becomes hidden.

    if (m_nCloseKey != BUTTON_CODE_INVALID)
    {
        if (!g_pInputSystem->IsButtonDown(m_nCloseKey))
        {
            m_nCloseKey = BUTTON_CODE_INVALID;
            gViewPortInterface->ShowPanel(PANEL_TIMES, false);
            GetClientVoiceMgr()->StopSquelchMode();
        }
    }
}

//-----------------------------------------------------------------------------
// Called by vgui panels that activate the client scoreboard
//-----------------------------------------------------------------------------
void CClientTimesDisplay::OnPollHideCode(int code) { m_nCloseKey = static_cast<ButtonCode_t>(code); }

//-----------------------------------------------------------------------------
// Purpose: clears everything in the scoreboard and all it's state
//-----------------------------------------------------------------------------
void CClientTimesDisplay::Reset() { Reset(false); }

void CClientTimesDisplay::Reset(bool pFullReset)
{
    if (m_pLocalLeaderboards)
    {
        m_pLocalLeaderboards->DeleteAllItems();
        m_pLocalLeaderboards->RemoveAllSections();
    }

    if (m_pOnlineLeaderboards)
    {
        m_pOnlineLeaderboards->DeleteAllItems();
        m_pOnlineLeaderboards->RemoveAllSections();
    }

    if (m_pAroundLeaderboards)
    {
        m_pAroundLeaderboards->DeleteAllItems();
        m_pAroundLeaderboards->RemoveAllSections();
    }

    if (m_pFriendsLeaderboards)
    {
        m_pFriendsLeaderboards->DeleteAllItems();
        m_pFriendsLeaderboards->RemoveAllSections();
    }

    m_iSectionId = 0;
    m_fNextUpdateTime = 0;
    // add all the sections
    InitScoreboardSections();

    Update(pFullReset);
}

//-----------------------------------------------------------------------------
// Purpose: adds all the team sections to the scoreboard
//-----------------------------------------------------------------------------
void CClientTimesDisplay::InitScoreboardSections()
{
    if (m_pLocalLeaderboards)
    {
        m_pLocalLeaderboards->AddSection(m_iSectionId, "", StaticLocalTimeSortFunc);
        m_pLocalLeaderboards->SetSectionAlwaysVisible(m_iSectionId);
        m_pLocalLeaderboards->AddColumnToSection(m_iSectionId, "time", "#MOM_Time", 0, GetScaledVal(m_aiColumnWidths[2]));
        m_pLocalLeaderboards->AddColumnToSection(m_iSectionId, "date", "#MOM_Achieved", 0, GetScaledVal(m_aiColumnWidths[0]));
        //m_pLocalLeaderboards->AddColumnToSection(m_iSectionId, "flags_input", "", SectionedListPanel::COLUMN_IMAGE, 16);
        //m_pLocalLeaderboards->AddColumnToSection(m_iSectionId, "flags_movement", "", SectionedListPanel::COLUMN_IMAGE, 16);
        //m_pLocalLeaderboards->AddColumnToSection(m_iSectionId, "flags_bonus", "", SectionedListPanel::COLUMN_IMAGE, 16);
    }

#if ENABLE_ONLINE_LEADERBOARDS
    SectionedListPanel *panels[] = {m_pOnlineLeaderboards, m_pAroundLeaderboards, m_pFriendsLeaderboards};
    for (SectionedListPanel *panel : panels)
    {
        if (!panel)
            continue;

        // We use online timer sort func as it's the same type of data
        panel->AddSection(m_iSectionId, "", StaticOnlineTimeSortFunc);
        panel->SetSectionAlwaysVisible(m_iSectionId);
        panel->SetImageList(m_pImageList, false);
        panel->AddColumnToSection(m_iSectionId, "rank", "#MOM_Rank", SectionedListPanel::COLUMN_CENTER,
                                                   GetScaledVal(m_aiColumnWidths[1]));
        panel->AddColumnToSection(m_iSectionId, "avatar", "",
                                                   SectionedListPanel::COLUMN_IMAGE,
                                                   DEFAULT_AVATAR_SIZE + 4);
        panel->AddColumnToSection(m_iSectionId, "icon_tm", "", SectionedListPanel::COLUMN_IMAGE, 16);
        panel->AddColumnToSection(m_iSectionId, "icon_vip", "", SectionedListPanel::COLUMN_IMAGE, 16);
        panel->AddColumnToSection(m_iSectionId, "icon_friend", "", SectionedListPanel::COLUMN_IMAGE, 16);
        panel->AddColumnToSection(m_iSectionId, "personaname", "#MOM_Name",
                                                   0, NAME_WIDTH);
        panel->AddColumnToSection(m_iSectionId, "time_f", "#MOM_Time",
                                                   0, GetScaledVal(m_aiColumnWidths[2]));
        panel->AddColumnToSection(m_iSectionId, "date", "#MOM_Achieved", 0, GetScaledVal(m_aiColumnWidths[0]));
        // Scroll only icon
        panel->AddColumnToSection(m_iSectionId, "flags_input", "", SectionedListPanel::COLUMN_IMAGE,
                                                   16);
        // HSW/SW/BW/WOnly Icons
        panel->AddColumnToSection(m_iSectionId, "flags_movement", "", SectionedListPanel::COLUMN_IMAGE,
                                                   16);
        // Bonus Icon
        panel->AddColumnToSection(m_iSectionId, "flags_bonus", "", SectionedListPanel::COLUMN_IMAGE,
                                                   16);
    }
#endif
}

//-----------------------------------------------------------------------------
// Purpose: sets up screen
//-----------------------------------------------------------------------------
void CClientTimesDisplay::ApplySchemeSettings(IScheme *pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);

    m_cFirstPlace = pScheme->GetColor("FirstPlace", Color(240, 210, 147, 50));
    m_cSecondPlace = pScheme->GetColor("SecondPlace", Color(175, 175, 175, 50));
    m_cThirdPlace = pScheme->GetColor("ThirdPlace", Color(205, 127, 50, 50));

    PostApplySchemeSettings(pScheme);
}

//-----------------------------------------------------------------------------
// Purpose: Does dialog-specific customization after applying scheme settings.
//-----------------------------------------------------------------------------
void CClientTimesDisplay::PostApplySchemeSettings(IScheme *pScheme)
{
    // resize the images to our resolution
    /*for (int i = 0; i < m_pImageList->GetImageCount(); i++)
    {
    int wide, tall;
    m_pImageList->GetImage(i)->GetSize(wide, tall);
    m_pImageList->GetImage(i)->SetSize(SCALE(wide), SCALE(tall));
    }*/

    const char *columnNames[] = {DATESTRING, RANKSTRING, TIMESTRING};

    HFont font = pScheme->GetFont("Default", true);
    for (int i = 0; i < 3; i++)
    {
        const char *currName = columnNames[i];
        const int len = Q_strlen(currName);
        int pixels = 0;
        for (int currentChar = 0; currentChar < len; currentChar++)
        {
            pixels += surface()->GetCharacterWidth(font, currName[currentChar]);
        }
        m_aiColumnWidths[i] = pixels;
    }

    if (m_pMapName)
        m_pMapName->SetVisible(true);

    // Make it the size of the screen and center
    int screenWide, screenHeight;
    surface()->GetScreenSize(screenWide, screenHeight);
    MoveToCenterOfScreen();
    SetSize(screenWide, screenHeight);

    // light up scoreboard a bit
    SetBgColor(Color(0, 0, 0, 0));
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CClientTimesDisplay::ShowPanel(bool bShow)
{
    // Catch the case where we call ShowPanel before ApplySchemeSettings, eg when
    // going from windowed <-> fullscreen
    if (!m_pImageList && bShow)
    {
        InvalidateLayout(true, true);
    }

    if (m_pLeaderboardReplayCMenu)
    {
        // Close the menu
        m_pLeaderboardReplayCMenu->OnKillFocus();
        m_pLeaderboardReplayCMenu->DeletePanel();
    }
    m_pLeaderboardReplayCMenu = nullptr;

    if (!bShow)
    {
        m_nCloseKey = BUTTON_CODE_INVALID;
    }

    if (BaseClass::IsVisible() == bShow)
        return;

    if (bShow)
    {
        Reset(true);
        SetVisible(true);
        // SetEnabled(true);
        MoveToFront();
    }
    else
    {
        SetVisible(false);
        SetMouseInputEnabled(false); // Turn mouse off
        SetKeyBoardInputEnabled(false);
    }
}

void CClientTimesDisplay::FireGameEvent(IGameEvent *event)
{
    if (!event)
        return;

    const char *type = event->GetName();

    if (FStrEq(type, "replay_save") && event->GetBool("save"))
    {
        // this updates the local times file, needing a reload of it
        m_bTimesNeedUpdate[TIMES_LOCAL] = true;
    }
#if ENABLE_ONLINE_LEADERBOARDS
    else if (FStrEq(type, "run_upload"))
    {
        m_bTimesNeedUpdate[TIMES_TOP10] = m_bTimesNeedUpdate[TIMES_AROUND] = m_bTimesNeedUpdate[TIMES_FRIENDS] = event->GetBool("run_posted");
    }
#endif

    // MOM_TODO: there's a crash here if you uncomment it,
    // if (IsVisible())
    //    Update(true);
}

bool CClientTimesDisplay::NeedsUpdate(void) { return (m_fNextUpdateTime < gpGlobals->curtime); }

//-----------------------------------------------------------------------------
// Purpose: Recalculate the internal scoreboard data
//-----------------------------------------------------------------------------
void CClientTimesDisplay::Update(void) { Update(false); }

void CClientTimesDisplay::Update(bool pFullUpdate)
{
    FillScoreBoard(pFullUpdate);

    MoveToCenterOfScreen();

    // update every X seconds
    // we don't need to update this too often. (Player is not finishing a run every second, so...)
#if ENABLE_ONLINE_LEADERBOARDS
    m_fNextUpdateTime = gpGlobals->curtime + UPDATE_INTERVAL;

    // This starts as true on the constructor.
    m_bFirstHeaderUpdate = false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CClientTimesDisplay::UpdatePlayerInfo(KeyValues *kv, bool fullUpdate)
{
    m_iSectionId = 0; // 0'th row is a header

    // add the player to the list
    KeyValues *playerData = new KeyValues("data");
    UpdatePlayerAvatarStandalone();

    player_info_t pi;
    engine->GetPlayerInfo(engine->GetLocalPlayer(), &pi);
    const char *oldName = playerData->GetString("name", pi.name);

    char newName[MAX_PLAYER_NAME_LENGTH];
    UTIL_MakeSafeName(oldName, newName, MAX_PLAYER_NAME_LENGTH);
    playerData->SetString("name", newName);
    // What this if is:
    // We want to do a full update if (we ask for it with fullUpdate boolean AND (the minimum time has passed OR it is
    // the first update)) OR the maximum time has passed
    float flLastUp = gpGlobals->curtime - m_fLastHeaderUpdate;
    if (fullUpdate && (flLastUp >= UPDATE_INTERVAL || m_bFirstHeaderUpdate))
    {
        char p_sCalculating[BUFSIZELOCL];
        char p_sWaitingResponse[BUFSIZELOCL];
        LOCALIZE_TOKEN(p_wcCalculating, "MOM_Calculating", p_sCalculating);
        LOCALIZE_TOKEN(p_wcWaitingResponse, "MOM_API_WaitingForResponse", p_sWaitingResponse);

        char p_sMapRank[BUFSIZELOCL];
        char p_sGlobalRank[BUFSIZELOCL];
        char p_sPersonalBest[BUFSIZELOCL];
        char p_sExperiencePoints[BUFSIZELOCL];

        char mrLocalized[BUFSIZELOCL];
        char grLocalized[BUFSIZELOCL];
        char pbLocalized[BUFSIZELOCL];
        char xpLocalized[BUFSIZELOCL];

        LOCALIZE_TOKEN(p_wcMapRank, "MOM_MapRank", p_sMapRank);
        LOCALIZE_TOKEN(p_wcGlobalRank, "MOM_GlobalRank", p_sGlobalRank);
        LOCALIZE_TOKEN(p_wcPersonalBest, "MOM_PersonalBestTime", p_sPersonalBest);
        LOCALIZE_TOKEN(p_wcExperiencePoints, "MOM_ExperiencePoints", p_sExperiencePoints);

        Q_snprintf(mrLocalized, BUFSIZELOCL, "%s: %s", p_sMapRank, p_sCalculating);
        Q_snprintf(grLocalized, BUFSIZELOCL, "%s: %s", p_sGlobalRank, p_sCalculating);
        Q_snprintf(pbLocalized, BUFSIZELOCL, "%s: %s", p_sPersonalBest, p_sWaitingResponse);
        Q_snprintf(xpLocalized, BUFSIZELOCL, "%s: %s", p_sExperiencePoints, p_sWaitingResponse);

        m_pPlayerMapRank->SetText(mrLocalized);
        m_pPlayerGlobalRank->SetText(grLocalized);
        m_pPlayerPersonalBest->SetText(pbLocalized);
        m_pPlayerExperience->SetText(xpLocalized);

#if ENABLE_HTTP_LEADERBOARDS
        // MOM_TODO: Get player map rank info for this map
        /*char requrl[MAX_PATH];
        // Mapname, tickrate, rank, radius
        Q_snprintf(requrl, MAX_PATH, "%s/getusermaprank/%s/%llu", MOM_APIDOMAIN, g_pGameRules->MapName(),
                   GetSteamIDForPlayerIndex(GetLocalPlayerIndex()).ConvertToUint64());
        g_pMomentumUtil->CreateAndSendHTTPReq(requrl, &cbGetPlayerDataForMapCallback, &CClientTimesDisplay::GetPlayerDataForMapCallback, this);
        m_fLastHeaderUpdate = gpGlobals->curtime;*/
#endif
    }

    kv->AddSubKey(playerData);
}

//-----------------------------------------------------------------------------
// Purpose: adds the top header of the scoreboars
//-----------------------------------------------------------------------------
void CClientTimesDisplay::AddHeader()
{
    if (m_pMapName)
    {
        m_pMapName->SetText(g_pGameRules->MapName());
        // Set the author label to be at the end of this label
        int wide, tall;
        m_pMapName->GetContentSize(wide, tall);
        m_pMapAuthor->SetPos(m_pMapName->GetXPos() + wide + GetScaledVal(4),
                             m_pMapName->GetYPos() + tall - GetScaledVal(surface()->GetFontTall(m_pMapAuthor->GetFont())));
    }

#if ENABLE_HTTP_LEADERBOARDS
    if (m_pMapDetails && !m_bMapInfoLoaded)
    {
        // MOM_TODO: actual API call for map info here
        /*char requrl[MAX_PATH];
        Q_snprintf(requrl, MAX_PATH, "%s/getmapinfo/%s", MOM_APIDOMAIN, g_pGameRules->MapName());
        g_pMomentumUtil->CreateAndSendHTTPReq(requrl, &cbGetMapInfoCallback, &CClientTimesDisplay::GetMapInfoCallback, this);*/
    }
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Used for sorting local times
//-----------------------------------------------------------------------------
bool CClientTimesDisplay::StaticLocalTimeSortFunc(SectionedListPanel *list, int itemID1, int itemID2)
{
    KeyValues *it1 = list->GetItemData(itemID1);
    KeyValues *it2 = list->GetItemData(itemID2);
    Assert(it1 && it2);

    float t1 = it1->GetFloat("time_f");
    float t2 = it2->GetFloat("time_f");
    // Ascending order
    if (t1 < t2)
        return true; // this time is faster, place it up higher
    if (t1 > t2)
        return false;

    // If the same, use IDs
    return itemID1 < itemID2;
}

//-----------------------------------------------------------------------------
// Purpose: Used for sorting players
//-----------------------------------------------------------------------------
bool CClientTimesDisplay::StaticOnlineTimeSortFunc(SectionedListPanel *list, int itemID1, int itemID2)
{
    // Uses rank insetad of time (Momentum page will handle players with same times)
    KeyValues *it1 = list->GetItemData(itemID1);
    KeyValues *it2 = list->GetItemData(itemID2);
    Assert(it1 && it2);
    int t1 = it1->GetFloat("rank");
    int t2 = it2->GetFloat("rank");
    // Ascending order
    if (t1 < t2)
        return true; // this time is faster, place it up higher
    if (t1 > t2)
        return false;
    // We will *almost* never need this, but just in case...

    float s1 = it1->GetFloat("time");
    float s2 = it2->GetFloat("time");
    // Ascending order
    if (s1 < s2)
        return true; // this time is faster, place it up higher
    if (s1 > s2)
        return false;
    return itemID1 < itemID2;
}

void CClientTimesDisplay::LoadLocalTimes(KeyValues *kv)
{
    if (m_bTimesNeedUpdate[TIMES_LOCAL])
    {
        // Clear the local times for a refresh
        m_vLocalTimes.PurgeAndDeleteElements();

        char path[MAX_PATH];
        Q_snprintf(path, MAX_PATH, "%s/%s-*%s", RECORDING_PATH, MapName(), EXT_RECORDING_FILE);
        V_FixSlashes(path);

        FileFindHandle_t found;
        const char *pFoundFile = filesystem->FindFirstEx(path, "MOD", &found);
        while (pFoundFile)
        {
            // NOTE: THIS NEEDS TO BE MANUALLY CLEANED UP!
            char pReplayPath[MAX_PATH];
            V_ComposeFileName(RECORDING_PATH, pFoundFile, pReplayPath, MAX_PATH);

            CMomReplayBase *pBase = g_ReplayFactory.LoadReplayFile(pReplayPath, false);
            Assert(pBase != nullptr);
            
            if (pBase)
                m_vLocalTimes.InsertNoSort(pBase);

            pFoundFile = filesystem->FindNext(found);
        }

        filesystem->FindClose(found);

        if (!m_vLocalTimes.IsEmpty())
        {
            m_vLocalTimes.RedoSort();
            m_bTimesNeedUpdate[TIMES_LOCAL] = false;
        }
    }

    // Convert
    if (!m_vLocalTimes.IsEmpty())
        ConvertLocalTimes(kv);
}

void CClientTimesDisplay::ConvertLocalTimes(KeyValues *kvInto)
{
    FOR_EACH_VEC(m_vLocalTimes, i)
    {
        CMomReplayBase *t = m_vLocalTimes[i];

        KeyValues *kvLocalTimeFormatted = new KeyValues("localtime");
        char filename[MAX_PATH];

        Q_snprintf(filename, MAX_PATH, "%s-%s%s", t->GetMapName(), t->GetRunHash(), EXT_RECORDING_FILE);
        kvLocalTimeFormatted->SetString("fileName", filename);

        kvLocalTimeFormatted->SetFloat("time_f", t->GetRunTime()); // Used for static compare
        kvLocalTimeFormatted->SetInt("date_t", t->GetRunDate());   // Used for finding

        char timeString[BUFSIZETIME];
        g_pMomentumUtil->FormatTime(t->GetRunTime(), timeString);
        kvLocalTimeFormatted->SetString("time", timeString); // Used for display
        
        char dateString[64];
        time_t date = t->GetRunDate();
        if (g_pMomentumUtil->GetTimeAgoString(&date, dateString, sizeof(dateString)))
        {
            kvLocalTimeFormatted->SetString("date", dateString);
        }
        else
            kvLocalTimeFormatted->SetInt("date", date);

        // MOM_TODO: Convert the run flags to pictures

        kvInto->AddSubKey(kvLocalTimeFormatted);
    }
}

void CClientTimesDisplay::ConvertOnlineTimes(KeyValues *kv, float seconds)
{
    char timeString[BUFSIZETIME];

    g_pMomentumUtil->FormatTime(seconds, timeString);
    kv->SetString("time_f", timeString);
}

#if ENABLE_STEAM_LEADERBOARDS
void CClientTimesDisplay::OnLeaderboardFindResult(LeaderboardFindResult_t* pParam, bool bIOFailure)
{
    if (pParam->m_bLeaderboardFound)
    {
        m_hCurrentLeaderboard = pParam->m_hSteamLeaderboard;
        m_bOnlineNeedUpdate = true;
    }
}

void CClientTimesDisplay::OnLeaderboardGlobalScoresDownloaded(LeaderboardScoresDownloaded_t* pResult, bool bIOFailure)
{
    if (bIOFailure)
    {
        m_bOnlineTimesLoaded = false;
        m_bOnlineNeedUpdate = true;
        Warning("OnLeaderboardTop10ScoresDownloaded:: bIOFailure\n");
        return;
    }

    // By now we're pretty sure everything will be ok, so we can do this
    m_vOnlineTimes.PurgeAndDeleteElements();

    for (int i = 0; i < pResult->m_cEntryCount; i++)
    {
        LeaderboardEntry_t entry;
        SteamUserStats()->GetDownloadedLeaderboardEntry(pResult->m_hSteamLeaderboardEntries, i, &entry, nullptr, 0);
        
        // MOM_TODO: Convert this entry to something (KeyValues) in the leaderboards
        KeyValues *kvEntry = new KeyValues("Entry");
        // Time is handled by the converter
        kvEntry->SetFloat("time", (float) entry.m_nScore / 1000.0f);

        // SteamID, Avatar, and Persona Name
        uint64 steamID = entry.m_steamIDUser.ConvertToUint64();
        kvEntry->SetUint64("steamid", steamID);
        ISteamFriends *steamFriends = SteamFriends();
        if (steamFriends && SteamUser())
        {
            UpdateLeaderboardPlayerAvatar(steamID, kvEntry);

            // persona name
            if (!steamFriends->RequestUserInformation(entry.m_steamIDUser, true))
            {
                kvEntry->SetString("personaname", steamFriends->GetFriendPersonaName(entry.m_steamIDUser));
            }
            else
            {
                kvEntry->SetString("personaname", "Unknown");
            }
        }

        // Persona name for the time they accomplished the run
        //kvEntry->SetString("personaname_onruntime", pRun->GetString("personaname_t"));

        // Rank
        kvEntry->SetInt("rank", entry.m_nGlobalRank);
        // MOM_TODO: Implement the other end of this (rank is not a number)

        // Tickrate
        //kvEntry->SetInt("rate", static_cast<int>(pRun->GetFloat("rate")));

        // Date
        //kvEntry->SetString("date", pRun->GetString("date"));

        // ID
        kvEntry->SetInt("id", entry.m_nGlobalRank);

        // UGC Handle
        kvEntry->SetUint64("UGC", entry.m_hUGC);

        // Is part of the momentum team?
        //kvEntry->SetBool("tm", pRun->GetBool("tm"));

        // Is vip?
        //kvEntry->SetBool("vip", pRun->GetBool("vip"));

        // Add this baby to the online times vector
        TimeOnline *ot = new TimeOnline(kvEntry);
        // Convert the time
        ConvertOnlineTimes(ot->m_kv, ot->time_sec);
        m_vOnlineTimes.AddToTail(ot);
    }

    m_bOnlineTimesLoaded = true;
    m_bOnlineNeedUpdate = false;
    m_flLastOnlineTimeUpdate = gpGlobals->curtime;
    Update();
}

void CClientTimesDisplay::OnLeaderboardFriendScoresDownloaded(LeaderboardScoresDownloaded_t* pResult, bool bIOFailure)
{
    if (bIOFailure)
    {
        m_bFriendsTimesLoaded = false;
        m_bFriendsNeedUpdate = true;
        Warning("OnLeaderboardFriendScoresDownloaded:: bIOFailure\n");
        return;
    }

    // By now we're pretty sure everything will be ok, so we can do this
    m_vFriendsTimes.PurgeAndDeleteElements();

    for (int i = 0; i < pResult->m_cEntryCount; i++)
    {
        LeaderboardEntry_t entry;
        SteamUserStats()->GetDownloadedLeaderboardEntry(pResult->m_hSteamLeaderboardEntries, i, &entry, nullptr, 0);

        // MOM_TODO: Convert this entry to something (KeyValues) in the leaderboards
        KeyValues *kvEntry = new KeyValues("Entry");
        // Time is handled by the converter
        kvEntry->SetFloat("time", (float) entry.m_nScore / 1000.0f);

        // SteamID, Avatar, and Persona Name
        uint64 steamID = entry.m_steamIDUser.ConvertToUint64();
        kvEntry->SetUint64("steamid", steamID);
        ISteamFriends *steamFriends = SteamFriends();
        if (steamFriends && SteamUser())
        {
            UpdateLeaderboardPlayerAvatar(steamID, kvEntry);

            // persona name
            if (!steamFriends->RequestUserInformation(entry.m_steamIDUser, true))
            {
                kvEntry->SetString("personaname", steamFriends->GetFriendPersonaName(entry.m_steamIDUser));
            }
            else
            {
                kvEntry->SetString("personaname", "Unknown");
            }
        }

        // Persona name for the time they accomplished the run
        //kvEntry->SetString("personaname_onruntime", pRun->GetString("personaname_t"));

        // Rank
        kvEntry->SetInt("rank", entry.m_nGlobalRank);
        // MOM_TODO: Implement the other end of this (rank is not a number)

        // Tickrate
        //kvEntry->SetInt("rate", static_cast<int>(pRun->GetFloat("rate")));

        // Date
        //kvEntry->SetString("date", pRun->GetString("date"));

        // ID
        kvEntry->SetInt("id", entry.m_nGlobalRank);

        // UGC Handle
        kvEntry->SetUint64("UGC", entry.m_hUGC);

        // Is part of the momentum team?
        //kvEntry->SetBool("tm", pRun->GetBool("tm"));

        // Is vip?
        //kvEntry->SetBool("vip", pRun->GetBool("vip"));

        // Add this baby to the online times vector
        TimeOnline *ot = new TimeOnline(kvEntry);
        // Convert the time
        ConvertOnlineTimes(ot->m_kv, ot->time_sec);
        m_vFriendsTimes.AddToTail(ot);
    }

    m_bFriendsTimesLoaded = true;
    m_bFriendsNeedUpdate = false;
    m_flLastFriendsTimeUpdate = gpGlobals->curtime;
    Update();
}
#endif

void CClientTimesDisplay::LoadOnlineTimes()
{
    if (!m_bTimesLoading[TIMES_TOP10] && m_bTimesNeedUpdate[TIMES_TOP10])
    {

#if ENABLE_STEAM_LEADERBOARDS
        if (m_hCurrentLeaderboard)
        {
            SteamAPICall_t global = SteamUserStats()->DownloadLeaderboardEntries(m_hCurrentLeaderboard, 
                        m_bGetTop10Scores ? k_ELeaderboardDataRequestGlobal : k_ELeaderboardDataRequestGlobalAroundUser, 
                        m_bGetTop10Scores ? 1 : -5, 
                        m_bGetTop10Scores ? 10 : 5);
            m_cLeaderboardGlobalScoresDownloaded.Set(global, this, &CClientTimesDisplay::OnLeaderboardGlobalScoresDownloaded);

            m_bOnlineTimesLoaded = true;
            m_bOnlineNeedUpdate = false;

            m_pLoadingOnlineTimes->SetVisible(m_pOnlineLeaderboards->IsVisible() || m_pFriendsLeaderboards->IsVisible());
        }
#endif

#if ENABLE_HTTP_LEADERBOARDS

        // MOM_TODO: Use a local map ID variable here when we get map info!
        if (g_pRunPoster->m_iMapID)
        {
            if (g_pAPIRequests->GetTop10MapTimes(g_pRunPoster->m_iMapID, UtlMakeDelegate(this, &CClientTimesDisplay::GetTop10TimesCallback)))
            {
                m_bTimesLoading[TIMES_TOP10] = true;
                m_bTimesNeedUpdate[TIMES_TOP10] = false;
                m_flTimesLastUpdate[TIMES_TOP10] = gpGlobals->curtime;
                m_eTimesStatus[TIMES_TOP10] = STATUS_TIMES_LOADING;
            }
        }
#endif
    }
}

void CClientTimesDisplay::LoadAroundTimes()
{
    if (!m_bTimesLoading[TIMES_AROUND] && m_bTimesNeedUpdate[TIMES_AROUND])
    {
#if ENABLE_HTTP_LEADERBOARDS

        // MOM_TODO: Use a local map ID variable here when we get map info!
        if (g_pRunPoster->m_iMapID)
        {
            if (g_pAPIRequests->GetAroundTimes(g_pRunPoster->m_iMapID, UtlMakeDelegate(this, &CClientTimesDisplay::GetAroundTimesCallback)))
            {
                m_bTimesNeedUpdate[TIMES_AROUND] = false;
                m_bTimesLoading[TIMES_AROUND] = true;
                m_flTimesLastUpdate[TIMES_AROUND] = gpGlobals->curtime;
                m_eTimesStatus[TIMES_AROUND] = STATUS_TIMES_LOADING;
            }
        }
#endif
    }
}

void CClientTimesDisplay::LoadFriendsTimes()
{
    if (!m_bTimesLoading[TIMES_FRIENDS] && m_bTimesNeedUpdate[TIMES_FRIENDS] && !m_bUnauthorizedFriendlist)
    {
#if ENABLE_HTTP_LEADERBOARDS

        // MOM_TODO: Use a local map ID variable here when we get map info!
        if (g_pRunPoster->m_iMapID)
        {
            if (g_pAPIRequests->GetFriendsTimes(g_pRunPoster->m_iMapID, UtlMakeDelegate(this, &CClientTimesDisplay::GetFriendsTimesCallback)))
            {
                m_bTimesNeedUpdate[TIMES_FRIENDS] = false;
                m_bTimesLoading[TIMES_FRIENDS] = true;
                m_flTimesLastUpdate[TIMES_FRIENDS] = gpGlobals->curtime;
                m_eTimesStatus[TIMES_FRIENDS] = STATUS_TIMES_LOADING;
            }
        }

#endif
#if ENABLE_STEAM_LEADERBOARDS
        if (m_hCurrentLeaderboard)
        {
            // MOM_TODO: 10, for now
            SteamAPICall_t friends = SteamUserStats()->DownloadLeaderboardEntries(m_hCurrentLeaderboard, k_ELeaderboardDataRequestFriends, 1, 10);
            m_cLeaderboardFriendsScoresDownloaded.Set(friends, this, &CClientTimesDisplay::OnLeaderboardFriendScoresDownloaded);

            m_pLoadingOnlineTimes->SetVisible(m_pOnlineLeaderboards->IsVisible() || m_pFriendsLeaderboards->IsVisible());

            m_bFriendsTimesLoaded = true;
            m_bFriendsNeedUpdate = false;
        }
#endif
    }
}

#if ENABLE_HTTP_LEADERBOARDS

void CClientTimesDisplay::GetTop10TimesCallback(KeyValues* pKv)
{
    ParseTimesCallback(pKv, TIMES_TOP10);
}

void CClientTimesDisplay::GetFriendsTimesCallback(KeyValues* pKv)
{
    ParseTimesCallback(pKv, TIMES_FRIENDS);
}

void CClientTimesDisplay::GetAroundTimesCallback(KeyValues* pKv)
{
    ParseTimesCallback(pKv, TIMES_AROUND);
}

void CClientTimesDisplay::ParseTimesCallback(KeyValues* pKv, TIME_TYPE type)
{
    m_bTimesLoading[type] = false;

    KeyValues *pData = pKv->FindKey("data");
    KeyValues *pErr = pKv->FindKey("error");
    if (pData)
    {
        KeyValues *pRuns = pData->FindKey("runs");

        if (pRuns && pData->GetInt("count") > 0)
        {
            CUtlVector<TimeOnline*> *vecs[] = {nullptr, &m_vOnlineTimes, &m_vFriendsTimes, &m_vAroundTimes};
            // By now we're pretty sure everything will be ok, so we can do this
            vecs[type]->PurgeAndDeleteElements();

            // Iterate through each loaded run
            FOR_EACH_SUBKEY(pRuns, pRun)
            {
                KeyValues *kvEntry = new KeyValues("Entry");

                // Around does UserMapRank -> Run instead of the other way around, so we do some funny business here
                KeyValues *pOuter = nullptr;
                if (type == TIMES_AROUND)
                {
                    pOuter = pRun;
                    pRun = pOuter->FindKey("run");
                    AssertMsg(pRun, "Around times didn't work!");
                }

                // Time is handled by the converter
                kvEntry->SetFloat("time", pRun->GetFloat("time"));

                // Tickrate
                kvEntry->SetFloat("rate", pRun->GetFloat("tickRate"));

                // Date
                char timeAgoStr[64];
                if (g_pMomentumUtil->GetTimeAgoString(pRun->GetString("dateAchieved"), timeAgoStr, sizeof(timeAgoStr)))
                    kvEntry->SetString("date", timeAgoStr);
                else
                    kvEntry->SetString("date", pRun->GetString("dateAchieved"));

                // ID
                kvEntry->SetUint64("id", pRun->GetUint64("id"));

                // File
                kvEntry->SetString("file", pRun->GetString("file"));

                // Hash
                kvEntry->SetString("hash", pRun->GetString("hash"));

                KeyValues *kvUserObj = pRun->FindKey("user");
                if (kvUserObj)
                {
                    uint64 steamID = Q_atoui64(kvUserObj->GetString("id"));
                    kvEntry->SetUint64("steamid", steamID);

                    int permissions = kvUserObj->GetInt("permissions");

                    // Is part of the momentum team?
                    // MOM_TODO: Make this the actual permission
                    kvEntry->SetBool("tm", permissions & (USER_ADMIN | USER_MODERATOR));

                    // Is vip?
                    // MOM_TODO: Make this the actual permission
                    kvEntry->SetBool("vip", pRun->GetBool("vip"));

                    // MOM_TODO: check if alias banned
                    kvEntry->SetString("personaname", kvUserObj->GetString("alias"));

                    if (SteamFriends() && SteamUser())
                    {
                        uint64 localSteamID = SteamUser()->GetSteamID().ConvertToUint64();
                        // These handle setting "avatar" for kvEntry
                        if (localSteamID == steamID)
                        {
                            kvEntry->SetInt("avatar", TryAddAvatar(localSteamID, &m_mapAvatarsToImageList, m_pImageList));
                        }
                        else
                        {
                            // MOM_TODO: check if avatar banned
                            UpdateLeaderboardPlayerAvatar(steamID, kvEntry);
                        }
                    }
                }

                // Rank
                if (type != TIMES_AROUND)
                {
                    KeyValues *kvRankObj = pRun->FindKey("rank");
                    if (kvRankObj)
                    {
                        kvEntry->SetInt("rank", kvRankObj->GetInt("rank"));
                    }
                }
                else
                {
                    kvEntry->SetInt("rank", pOuter->GetInt("rank"));
                    pRun = pOuter; // Make sure to reset to outer so we can continue the loop
                }

                // Add this baby to the online times vector
                TimeOnline *ot = new TimeOnline(kvEntry);
                // Convert the time
                ConvertOnlineTimes(ot->m_kv, ot->time_sec);
                vecs[type]->AddToTail(ot);
            }

            m_eTimesStatus[type] = STATUS_TIMES_LOADED;
        }
        else
        {
            m_eTimesStatus[type] = STATUS_NO_TIMES_RETURNED;
        }
    }
    else if (pErr)
    {
        int code = pKv->GetInt("code");

        // Handle general errors
        m_eTimesStatus[type] = STATUS_SERVER_ERROR;

        // Handle specific error cases
        if (type == TIMES_AROUND)
        {
            if (code == 403) // User has not done a run yet
            {
                m_eTimesStatus[type] = STATUS_NO_PB_SET;
            }
        }
        else if (type == TIMES_FRIENDS)
        {
            if (code == 409) // The profile is private, we cannot read their friends
            {
                m_eTimesStatus[type] = STATUS_UNAUTHORIZED_FRIENDS_LIST;
                m_bUnauthorizedFriendlist = true;
            }
            else if (code == 418) // Short and stout~
            {
                m_eTimesStatus[type] = STATUS_NO_FRIENDS;
            }
        }
    }

    if (pData || pErr)
        Update();
}

void CClientTimesDisplay::OnReplayDownloadStart(KeyValues* pKvHeaders)
{
    // MOM_TODO: Create a progress bar here
}

void CClientTimesDisplay::OnReplayDownloadProgress(KeyValues* pKvProgress)
{
    uint16 fileIndx = m_mapReplayDownloads.Find(pKvProgress->GetUint64("request"));
    if (fileIndx != m_mapReplayDownloads.InvalidIndex())
    {
        // DevLog("Progress: %0.2f!\n", pKvProgress->GetFloat("percent"));

        // MOM_TODO: update the progress bar here, but do not use the percent! Use the offset and size of the chunk!
        // Percent seems to be cached, i.e. sends a lot of "100%" if Steam downloaded the file and is sending the chunks from cache to us
    }
}

void CClientTimesDisplay::OnReplayDownloadEnd(KeyValues* pKvEnd)
{
    uint16 fileIndx = m_mapReplayDownloads.Find(pKvEnd->GetUint64("request"));
    if (fileIndx != m_mapReplayDownloads.InvalidIndex())
    {
        if (pKvEnd->GetBool("error"))
        {
            // MOM_TODO: Show some sort of error icon on the progress bar
            Warning("Could not download replay! Error code: %i\n", pKvEnd->GetInt("code"));
        }
        else
        {
            // MOM_TODO: show success on the progress bar here
            DevLog("Successfully downloaded the replay with ID: %i\n", m_mapReplayDownloads[fileIndx]);

            // Play it
            CFmtStr command("mom_replay_play %s/%s-%lld%s\n", RECORDING_ONLINE_PATH, MapName(), m_mapReplayDownloads[fileIndx], EXT_RECORDING_FILE);
            engine->ClientCmd(command.Get());
        }

        m_mapReplayDownloads.RemoveAt(fileIndx);
    }
}

void CClientTimesDisplay::GetPlayerDataForMapCallback(HTTPRequestCompleted_t *pCallback, bool bIOFailure)
{
    /*Warning("%s - Callback received.\n", __FUNCTION__);
    if (bIOFailure)
        return;

    uint32 size;
    SteamHTTP()->GetHTTPResponseBodySize(pCallback->m_hRequest, &size);

    if (size == 0)
    {
        Warning("%s - size is 0!\n", __FUNCTION__);
        return;
    }

    DevLog("Size of body: %u\n", size);
    uint8 *pData = new uint8[size];
    SteamHTTP()->GetHTTPResponseBodyData(pCallback->m_hRequest, pData, size);

    JsonValue val; // Outer object
    JsonAllocator alloc;
    char *pDataPtr = reinterpret_cast<char *>(pData);
    char *endPtr;
    int status = jsonParse(pDataPtr, &endPtr, &val, alloc);

    if (status == JSON_OK)
    {
        DevLog("JSON Parsed!\n");
        if (val.getTag() == JSON_OBJECT) // Outer should be a JSON Object
        {
            KeyValues *pResponse = CJsonToKeyValues::ConvertJsonToKeyValues(val.toNode());
            KeyValues::AutoDelete ad(pResponse);

            int mrank = -1;
            int mtotal = -1;

            int grank = -1;
            int gtotal = -1;
            int gexp = -1;

            float seconds = 0.0f;

            KeyValues *pRun = pResponse->FindKey("run");
            if (pRun)
            {
                mrank = static_cast<int>(pRun->GetFloat("rank"));
                seconds = pRun->GetFloat("time");
            }

            KeyValues *pMap = pResponse->FindKey("mapranking");
            if (pMap)
            {
                mtotal = static_cast<int>(pMap->GetFloat("total", -1.0f));
            }

            KeyValues *pExperience = pResponse->FindKey("globalranking");
            if (pExperience)
            {
                grank = static_cast<int>(pExperience->GetFloat("rank"));
                gtotal = static_cast<int>(pExperience->GetFloat("total"));
                gexp = static_cast<int>(pExperience->GetFloat("experience"));
            }

            if (mrank > -1 && mtotal > -1)
            {
                char p_sMapRank[BUFSIZELOCL];
                char p_sLocalized[BUFSIZELOCL];
                LOCALIZE_TOKEN(p_wcMapRank, "MOM_MapRank", p_sMapRank);
                Q_snprintf(p_sLocalized, BUFSIZELOCL, "%s: %i/%i", p_sMapRank, mrank, mtotal);
                m_pPlayerMapRank->SetText(p_sLocalized);
            }
            if (seconds > 0.0f)
            {
                char p_sPersonalBestTime[BUFSIZETIME];
                char p_sPersonalBest[BUFSIZELOCL];
                char p_sLocalized[BUFSIZELOCL];
                g_pMomentumUtil->FormatTime(seconds, p_sPersonalBestTime);
                LOCALIZE_TOKEN(p_wcPersonalBest, "MOM_PersonalBestTime", p_sPersonalBest);
                Q_snprintf(p_sLocalized, BUFSIZELOCL, "%s: %s", p_sPersonalBest, p_sPersonalBestTime);
                m_pPlayerPersonalBest->SetText(p_sLocalized);
            }

            if (grank > -1 && gtotal > -1)
            {
                char p_sGlobalRank[BUFSIZELOCL];
                char p_sLocalized[BUFSIZELOCL];
                LOCALIZE_TOKEN(p_wcGlobalRank, "MOM_GlobalRank", p_sGlobalRank);
                Q_snprintf(p_sLocalized, BUFSIZELOCL, "%s: %i/%i", p_sGlobalRank, grank, gtotal);
                m_pPlayerGlobalRank->SetText(p_sLocalized);

                char p_sExperience[BUFSIZELOCL];
                char p_sLocalized2[BUFSIZELOCL];
                LOCALIZE_TOKEN(p_wcExperience, "MOM_ExperiencePoints", p_sExperience);
                Q_snprintf(p_sLocalized2, BUFSIZELOCL, "%s: %i", p_sExperience, gexp);
                m_pPlayerExperience->SetText(p_sLocalized2);
            }
            m_fLastHeaderUpdate = gpGlobals->curtime;
        }
    }
    else
    {
        Warning("%s at %zd\n", jsonStrError(status), endPtr - pDataPtr);
    }
    // Last but not least, free resources
    delete[] pData;
    pData = nullptr;
    alloc.deallocate();
    SteamHTTP()->ReleaseHTTPRequest(pCallback->m_hRequest);*/
}

void CClientTimesDisplay::GetMapInfoCallback(HTTPRequestCompleted_t *pCallback, bool bIOFailure)
{
    /*Warning("%s - Callback received.\n", __FUNCTION__);
    if (bIOFailure)
    {
        UpdateMapInfoLabel(); // Default param is nullptr, so it hides it
        Warning("%s - bIOFailure is true!\n", __FUNCTION__);
        return;
    }

    if (pCallback->m_eStatusCode == k_EHTTPStatusCode409Conflict ||
        pCallback->m_eStatusCode == k_EHTTPStatusCode404NotFound)
    {
        char locl[BUFSIZELOCL];
        LOCALIZE_TOKEN(staged, "MOM_API_Unavailable", locl);
        UpdateMapInfoLabel(locl);
        Warning("%s - Map not found on server!\n", __FUNCTION__);
        return;
    }

    uint32 size;
    SteamHTTP()->GetHTTPResponseBodySize(pCallback->m_hRequest, &size);

    if (size == 0)
    {
        UpdateMapInfoLabel();
        Warning("%s - size is 0!\n", __FUNCTION__);
    }

    DevLog("Size of body: %u\n", size);
    uint8 *pData = new uint8[size];
    SteamHTTP()->GetHTTPResponseBodyData(pCallback->m_hRequest, pData, size);

    JsonValue val; // Outer object
    JsonAllocator alloc;
    char *pDataPtr = reinterpret_cast<char *>(pData);
    char *endPtr;
    int status = jsonParse(pDataPtr, &endPtr, &val, alloc);

    if (status == JSON_OK)
    {
        DevLog("JSON Parsed!\n");
        if (val.getTag() == JSON_OBJECT) // Outer should be a JSON Object
        {
            KeyValues *pResponse = CJsonToKeyValues::ConvertJsonToKeyValues(val.toNode());
            KeyValues::AutoDelete ad(pResponse);
            if (pResponse)
            {
                const char *author = pResponse->GetString("submitter", "Unknown");
                const int tier = pResponse->GetInt("difficulty", -1);
                const int bonus = pResponse->GetInt("bonus", -1);
                char layout[BUFSIZELOCL];
                if (pResponse->GetBool("linear", false))
                {
                    LOCALIZE_TOKEN(linear, "MOM_Linear", layout);
                }
                else
                {
                    Q_snprintf(layout, BUFSIZELOCL, "%i STAGES", pResponse->GetInt("zones", -1));
                }

                UpdateMapInfoLabel(author, tier, layout, bonus);
                m_bMapInfoLoaded = true; // Stop this info from being fetched again
            }
        }
    }
    else
    {
        Warning("%s at %zd\n", jsonStrError(status), endPtr - pDataPtr);
    }
    // Last but not least, free resources
    delete[] pData;
    pData = nullptr;
    alloc.deallocate();
    SteamHTTP()->ReleaseHTTPRequest(pCallback->m_hRequest);*/
}
#endif


//-----------------------------------------------------------------------------
// Purpose: Updates the leaderboard lists
//-----------------------------------------------------------------------------
bool CClientTimesDisplay::GetPlayerTimes(KeyValues *kv, bool fullUpdate)
{
    ConVarRef gm("mom_gamemode");
    if (!kv || gm.GetInt() == GAMEMODE_ALLOWED)
        return false;

    KeyValues *pLeaderboards = new KeyValues("leaderboards");

    // Fill local times:
    KeyValues *pLocal = new KeyValues("local");
    LoadLocalTimes(pLocal);
    pLeaderboards->AddSubKey(pLocal);

#if ENABLE_ONLINE_LEADERBOARDS
    // Skip over local
    for (int i = 1; i < TIMES_COUNT; i++)
    {
        // Only if we need to calculate it
        if (!m_bTimesNeedUpdate[i])
        {
            float lastUp = gpGlobals->curtime - m_flTimesLastUpdate[i];
            m_bTimesNeedUpdate[i] = fullUpdate && lastUp >= UPDATE_INTERVAL;
        }
    }

    // Fill online times only if needed
    LoadOnlineTimes();
    LoadAroundTimes();
    LoadFriendsTimes();
#else
    m_bTimesNeedUpdate[TIMES_TOP10] = m_bTimesNeedUpdate[TIMES_AROUND] = m_bTimesNeedUpdate[TIMES_FRIENDS] = false;
#endif

    kv->AddSubKey(pLeaderboards);
    return true;
}

void CClientTimesDisplay::UpdatePlayerAvatarStandalone()
{
    // Update their avatar
    if (SteamUser())
    {
        if (!m_bLoadedLocalPlayerAvatar)
        {
            CSteamID steamIDForPlayer = SteamUser()->GetSteamID();

            CAvatarImage *pImage = new CAvatarImage();
            // 64 is enough up to full HD resolutions.
            pImage->SetAvatarSteamID(steamIDForPlayer, k_EAvatarSize64x64);

            pImage->SetDrawFriend(false);
            pImage->SetAvatarSize(64, 64); // Deliberately non scaling, the ImagePanel does that for us

            // Get rid of the other image if it was there
            m_pPlayerAvatar->EvictImage();

            m_pPlayerAvatar->SetImage(pImage);
            m_bLoadedLocalPlayerAvatar = true;
        }
    }
    else
    {
        m_pPlayerAvatar->SetImage("default_steam");
    }
}

void CClientTimesDisplay::UpdateLeaderboardPlayerAvatar(uint64 steamid, KeyValues *kv)
{
    // Update their avatar
    if (SteamFriends())
    {
        kv->SetBool("is_friend", SteamFriends()->HasFriend(CSteamID(steamid), k_EFriendFlagImmediate));
        kv->SetInt("avatar", TryAddAvatar(steamid, &m_mapAvatarsToImageList, m_pImageList));
    }
}

//-----------------------------------------------------------------------------
// Purpose: reload the player list on the scoreboard
//-----------------------------------------------------------------------------
void CClientTimesDisplay::FillScoreBoard()
{
    // update player info
    FillScoreBoard(false);
}

void CClientTimesDisplay::SetPlaceColors(SectionedListPanel *panel, TIME_TYPE type) const
{
    int itemCount = panel->GetItemCount();
    if (type == TIMES_LOCAL || type == TIMES_TOP10)
    {
        panel->SetItemBgColor(panel->GetItemIDFromRow(0), m_cFirstPlace);
        if (itemCount > 1)
        {
            panel->SetItemBgColor(panel->GetItemIDFromRow(1), m_cSecondPlace);

            if (itemCount > 2)
                panel->SetItemBgColor(panel->GetItemIDFromRow(2), m_cThirdPlace);
        }
    } 
    else
    {
        Color colors[3] = {m_cFirstPlace, m_cSecondPlace, m_cThirdPlace};
        for (int row = 0; row < 3 && row < itemCount; row++)
        {
            int itemID = panel->GetItemIDFromRow(row);
            KeyValues *pItem = panel->GetItemData(itemID);
            int rank = pItem->GetInt("rank");
            if (rank < 4)
                panel->SetItemBgColor(itemID, colors[rank - 1]);
            if (rank == 3)
                break;
        }
    }
}

void CClientTimesDisplay::FillScoreBoard(bool pFullUpdate)
{
    KeyValuesAD kvPlayerData("playdata");
    UpdatePlayerInfo(kvPlayerData, pFullUpdate);
    if (pFullUpdate)
        AddHeader();

    // Player Stats panel:
    if (m_pPlayerStats && m_pPlayerName)
    {
        m_pPlayerStats->SetVisible(false); // Hidden so it is not seen being changed

        KeyValues *playdata = kvPlayerData->FindKey("data");
        if (playdata)
            m_pPlayerName->SetText(playdata->GetString("name", "Unknown"));

        m_pPlayerStats->SetVisible(true); // And seen again!
    }

    GetPlayerTimes(kvPlayerData, pFullUpdate);

    if (m_pLeaderboards && m_pCurrentLeaderboards && m_pOnlineLeaderboards && m_pAroundLeaderboards 
        && m_pLocalLeaderboards && m_pFriendsLeaderboards)
    {
        m_pLeaderboards->SetVisible(false);

        if (m_pCurrentLeaderboards == m_pLocalLeaderboards)
        {
            KeyValues *kvLeaderboards = kvPlayerData->FindKey("leaderboards");
            if (kvLeaderboards)
            {
                KeyValues *kvLocalTimes = kvLeaderboards->FindKey("local");
                if (kvLocalTimes && !kvLocalTimes->IsEmpty())
                {
                    FOR_EACH_SUBKEY(kvLocalTimes, kvLocalTime)
                    {
                        int itemID = FindItemIDForLocalTime(kvLocalTime);
                        if (itemID == -1)
                            m_pLocalLeaderboards->AddItem(m_iSectionId, kvLocalTime);
                        else
                            m_pLocalLeaderboards->ModifyItem(itemID, m_iSectionId, kvLocalTime);
                    }

                    SetPlaceColors(m_pLocalLeaderboards, TIMES_LOCAL);
                }
            }
        }
        // Online works slightly different, we use the vector content, not the ones from m_kvPlayerData
        else if (m_pCurrentLeaderboards == m_pOnlineLeaderboards)
        {
            OnlineTimesVectorToLeaderboards(TIMES_TOP10);
        }
        else if (m_pCurrentLeaderboards == m_pAroundLeaderboards)
        {
            OnlineTimesVectorToLeaderboards(TIMES_AROUND);
        }
        else if (m_pCurrentLeaderboards == m_pFriendsLeaderboards)
        {
            OnlineTimesVectorToLeaderboards(TIMES_FRIENDS);
        }

        m_pLeaderboards->SetVisible(true);
    }
}

void CClientTimesDisplay::OnlineTimesVectorToLeaderboards(TIME_TYPE type)
{
    CUtlVector<TimeOnline *> *pVector;
    SectionedListPanel *pList;
    switch (type)
    {
    case TIMES_FRIENDS:
        pVector = &m_vFriendsTimes;
        pList = m_pFriendsLeaderboards;
        break;
    case TIMES_TOP10:
        pVector = &m_vOnlineTimes;
        pList = m_pOnlineLeaderboards;
        break;
    case TIMES_AROUND:
        pVector = &m_vAroundTimes;
        pList = m_pAroundLeaderboards;
        break;
    default:
        return;
    }
    if (pVector && pVector->Count() > 0 && pList)
    {
        // To clear up any count discrepancies, just remove all items
        if (pList->GetItemCount() != pVector->Count())
            pList->DeleteAllItems();

        FOR_EACH_VEC(*pVector, entry)
        {
            TimeOnline *runEntry = pVector->Element(entry);

            int itemID = FindItemIDForOnlineTime(runEntry->id, type);

            runEntry->m_kv->SetInt("icon_tm", runEntry->momember ? m_IconsIndex[ICON_TEAMMEMBER] : -1);
            runEntry->m_kv->SetInt("icon_vip", runEntry->vip ? m_IconsIndex[ICON_VIP] : -1);
            runEntry->m_kv->SetInt("icon_friend", runEntry->is_friend ? m_IconsIndex[ICON_FRIEND] : -1);

            if (itemID == -1)
            {
                itemID = pList->AddItem(m_iSectionId, runEntry->m_kv);
            }
            else
            {
                pList->ModifyItem(itemID, m_iSectionId, runEntry->m_kv);
            }

            // MOM_TODO: highlight the local player's thing (some outline?), if it's in the list!
            //if (runEntry->steamid == SteamUser()->GetSteamID().ConvertToUint64())
            //    pList->SetBorderForItem(itemID, someBorder);
        }

        SetPlaceColors(pList, type);
    }
    if (m_pOnlineTimesStatus)
    {
        if (m_eTimesStatus[type] == STATUS_TIMES_LOADED)
            m_pOnlineTimesStatus->SetVisible(false);
        else
        {
            m_pOnlineTimesStatus->SetText(g_pszTimesStatusStrings[m_eTimesStatus[type]]);
            m_pOnlineTimesStatus->SetVisible(true);
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: searches for the player in the scoreboard
//-----------------------------------------------------------------------------
int CClientTimesDisplay::FindItemIDForLocalTime(KeyValues *kvRef)
{
    for (int i = 0; i <= m_pLocalLeaderboards->GetHighestItemID(); i++)
    {
        if (m_pLocalLeaderboards->IsItemIDValid(i))
        {
            KeyValues *kv = m_pLocalLeaderboards->GetItemData(i);
            if (kv && (kv->GetInt("date_t") == kvRef->GetInt("date_t")))
            {
                return i;
            }
        }
    }
    return -1;
}

int CClientTimesDisplay::FindItemIDForOnlineTime(uint64 runID, TIME_TYPE type)
{
    SectionedListPanel *pLeaderboard;
    switch (type)
    {
    case TIMES_FRIENDS:
        pLeaderboard = m_pFriendsLeaderboards;
        break;
    case TIMES_TOP10:
        pLeaderboard = m_pOnlineLeaderboards;
        break;
    case TIMES_AROUND:
        pLeaderboard = m_pAroundLeaderboards;
        break;
    default:
        return -1;
    }
    for (int i = 0; i <= pLeaderboard->GetHighestItemID(); i++)
    {
        if (pLeaderboard->IsItemIDValid(i))
        {
            KeyValues *kv = pLeaderboard->GetItemData(i);
            if (kv && (kv->GetUint64("id") == runID))
            {
                return i;
            }
        }
    }
    return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Center the dialog on the screen.  (vgui has this method on
//			Frame, but we're an EditablePanel, need to roll our own.)
//-----------------------------------------------------------------------------
void CClientTimesDisplay::MoveToCenterOfScreen()
{
    int wx, wy, ww, wt;
    surface()->GetWorkspaceBounds(wx, wy, ww, wt);
    SetPos((ww - GetWide()) / 2, (wt - GetTall()) / 2);
}

CLeaderboardsContextMenu *CClientTimesDisplay::GetLeaderboardContextMenu(Panel *pParent)
{
    // create a drop down for this object's states
    // This will stop being created after the second time you open the leaderboards?
    if (m_pLeaderboardReplayCMenu)
        delete m_pLeaderboardReplayCMenu;

    m_pLeaderboardReplayCMenu = new CLeaderboardsContextMenu(this);
    m_pLeaderboardReplayCMenu->SetAutoDelete(false);
    m_pLeaderboardReplayCMenu->SetParent(pParent ? pParent : this);
    m_pLeaderboardReplayCMenu->AddActionSignalTarget(pParent ? pParent : nullptr);
    m_pLeaderboardReplayCMenu->SetVisible(false);

    return m_pLeaderboardReplayCMenu;
}

void CClientTimesDisplay::SetupIcons()
{
    for (int index = 0; index < ICON_TOTAL; index++)
    {
        m_IconsIndex[index] = -1;
        IImage *image = nullptr;
        switch (index)
        {
        case ICON_VIP:
            image = scheme()->GetImage("leaderboards_icon_vip", false);
            break;
        case ICON_TEAMMEMBER:
            image = scheme()->GetImage("leaderboards_icon_mom", false);
            break;
        case ICON_FRIEND:
            image = scheme()->GetImage("leaderboards_icon_friends", false);
            break;
        default:
            break;
        }
        if (image)
        {
            image->SetSize(16, 16);
            m_IconsIndex[index] = m_pImageList->AddImage(image);
        }
    }
}

void CClientTimesDisplay::LevelInitPostEntity()
{
    m_bTimesLoading[TIMES_TOP10] = m_bTimesLoading[TIMES_AROUND] = m_bTimesLoading[TIMES_FRIENDS] = false;
    m_bTimesNeedUpdate[TIMES_LOCAL] = m_bTimesNeedUpdate[TIMES_TOP10] = m_bTimesNeedUpdate[TIMES_AROUND] = m_bTimesNeedUpdate[TIMES_FRIENDS] = true;
    m_bMapInfoLoaded = false;

    // Clear out the old index map and image list every map load
#if ENABLE_ONLINE_LEADERBOARDS
    if (m_pImageList)
        delete m_pImageList;

    m_pImageList = new ImageList(false);
    m_mapAvatarsToImageList.RemoveAll();

    SetupIcons();

#if ENABLE_STEAM_LEADERBOARDS
    // Get our current leaderboard
    if (SteamUserStats())
    {
        SteamAPICall_t find = SteamUserStats()->FindLeaderboard(g_pGameRules->MapName());
        m_cLeaderboardFindResult.Set(find, this, &CClientTimesDisplay::OnLeaderboardFindResult);
    }
#endif
#endif
}

void CClientTimesDisplay::OnContextWatchReplay(const char *runName)
{
    if (runName)
    {
        char command[MAX_PATH];
        Q_snprintf(command, MAX_PATH, "mom_replay_play %s", runName);
        engine->ServerCmd(command);
        ShowPanel(false);
    }
}

void CClientTimesDisplay::OnContextDeleteReplay(int itemID, const char* runName)
{
    if (runName)
    {
        char file[MAX_PATH];
        V_ComposeFileName(RECORDING_PATH, runName, file, MAX_PATH);

        KeyValues *pCommand = new KeyValues("ConfirmDeleteReplay", "file", file);
        pCommand->SetInt("itemID", itemID);
        messageboxpanel->CreateConfirmationBox(this, "#MOM_Leaderboards_DeleteReplay", 
            "#MOM_MB_DeleteRunConfirmation", pCommand,
             nullptr, "#MOM_Leaderboards_DeleteReplay");
    }
}

void CClientTimesDisplay::OnConfirmDeleteReplay(int itemID, const char *file)
{
    if (file)
    {
        g_pFullFileSystem->RemoveFile(file, "MOD");
        m_bTimesNeedUpdate[TIMES_LOCAL] = true;
        m_pLocalLeaderboards->RemoveItem(itemID);
    }
}

void CClientTimesDisplay::OnSpectateLobbyMember(uint64 target)
{
    char command[128];
    Q_snprintf(command, 128, "mom_spectate %llu", target);
    engine->ServerCmd(command);
    ShowPanel(false);
}

void CClientTimesDisplay::OnContextReqSavelocs(uint64 target)
{
    KeyValues *pReq = new KeyValues("req_savelocs");
    // Stage 1 is request the count, make the other player make a copy of their savelocs for us
    pReq->SetInt("stage", 1);
    pReq->SetUint64("target", target);
    g_pModuleComms->FireEvent(pReq);

    m_pSavelocReqFrame->Activate(target);
}


void CClientTimesDisplay::OnContextGoToMap(const char* map)
{
    // MOM_TODO: We're going to need to feed this into a map downloader first, if they don't have the map!
    char command[128];
    Q_snprintf(command, 128, "map %s\n", map);
    ShowPanel(false);
    engine->ClientCmd_Unrestricted(command);
}


inline bool CheckParent(Panel *pPanel, SectionedListPanel *pParentToCheck, int itemID)
{
    return pPanel->GetParent() == pParentToCheck && pParentToCheck->IsItemIDValid(itemID);
}

void CClientTimesDisplay::OnItemContextMenu(KeyValues *pData)
{
    int itemID = pData->GetInt("itemID", -1);
    Panel *pPanel = static_cast<Panel *>(pData->GetPtr("SubPanel", nullptr));
    if (pPanel && pPanel->GetParent())
    {
        if (CheckParent(pPanel, m_pLocalLeaderboards, itemID))
        {
            KeyValues *selectedRun = m_pLocalLeaderboards->GetItemData(itemID);

            const char *pFileName = selectedRun->GetString("fileName");

            CLeaderboardsContextMenu *pContextMenu = GetLeaderboardContextMenu(pPanel->GetParent());
            pContextMenu->AddMenuItem("StartMap", "#MOM_Leaderboards_WatchReplay", new KeyValues("ContextWatchReplay", "runName", pFileName), this);
            pContextMenu->AddSeparator();
            KeyValues *pMessage = new KeyValues("ContextDeleteReplay", "runName", pFileName);
            pMessage->SetInt("itemID", itemID);
            pContextMenu->AddMenuItem("DeleteRun", "#MOM_Leaderboards_DeleteReplay", pMessage, this);
            pContextMenu->ShowMenu();
        }
        else if (CheckParent(pPanel, m_pFriendsLeaderboards, itemID) || CheckParent(pPanel, m_pOnlineLeaderboards, itemID) || CheckParent(pPanel, m_pAroundLeaderboards, itemID))
        {
            SectionedListPanel *pLeaderboard = static_cast<SectionedListPanel *>(pPanel->GetParent());
            CLeaderboardsContextMenu *pContextMenu = GetLeaderboardContextMenu(pLeaderboard);
            KeyValues *pKVItemData = pLeaderboard->GetItemData(itemID);

            KeyValues *pKv = new KeyValues("ContextVisitProfile");
            pKv->SetUint64("profile", pKVItemData->GetUint64("steamid"));
            pContextMenu->AddMenuItem("VisitProfile", "#MOM_Leaderboards_SteamProfile", pKv, this);

            KeyValues *data;
#if ENABLE_HTTP_LEADERBOARDS
            data = pKVItemData->MakeCopy();
            data->SetName("ContextWatchOnlineReplay");
#endif
#if ENABLE_STEAM_LEADERBOARDS
            data = new KeyValues("ContextWatchOnlineReplay");
            data->SetUint64("UGC", pKVItemData->GetUint64("UGC"));
#endif
            pContextMenu->AddMenuItem("WatchOnlineReplay", "#MOM_Leaderboards_WatchReplay", data, this);

            pContextMenu->ShowMenu();
        }
        else if (CheckParent(pPanel, m_pLobbyMembersPanel, itemID))
        {
            CLeaderboardsContextMenu *pContextMenu = GetLeaderboardContextMenu(m_pLobbyMembersPanel);

            KeyValues *pKVData = m_pLobbyMembersPanel->GetItemData(itemID);

            uint64 steamID = pKVData->GetUint64("steamid");
            const char *pMap = pKVData->GetString("map");
            const char *pOurMap = g_pGameRules->MapName();

            KeyValues *pKv;

            if (pOurMap && FStrEq(pMap, pOurMap))
            {
                pKv = new KeyValues("ContextSpectate");
                pKv->SetUint64("target", steamID);
                pContextMenu->AddMenuItem("SpectateLobbyMember", "#MOM_Leaderboards_Spectate", pKv, this);
            }
            else
            {
                pKv = new KeyValues("ContextGoToMap");
                pKv->SetString("map", pMap);
                pContextMenu->AddMenuItem("GoToMap", "#MOM_Leaderboards_GoToMap", pKv, this);
            }

            pKv = new KeyValues("ContextReqSavelocs");
            pKv->SetUint64("target", steamID);
            pContextMenu->AddMenuItem("ReqSavelocs", "#MOM_Saveloc_Frame", pKv, this);

            // MOM_TODO: More options here, such as:
            // kicking the player if we're the lobby leader
            // hiding decals (maybe toggle paint, bullets separately?)
            // etc
            pContextMenu->AddSeparator();
            // Visit profile
            pKv = new KeyValues("ContextVisitProfile");
            pKv->SetUint64("profile", steamID);
            pContextMenu->AddMenuItem("VisitProfile", "#MOM_Leaderboards_SteamProfile", pKv, this);

            pContextMenu->ShowMenu();
        }
    }
}

void CClientTimesDisplay::OnContextVisitProfile(uint64 profile)
{
    if (profile != 0 && SteamFriends())
    {
        SteamFriends()->ActivateGameOverlayToUser("steamid", CSteamID(profile));
    }
}

void CClientTimesDisplay::OnContextWatchOnlineReplay(KeyValues *data)
{
    DevLog("Attempting to download UGC...\n");

#if ENABLE_HTTP_LEADERBOARDS
    uint64 replayID = data->GetUint64("id");
    const char *pFileURL = data->GetString("file", nullptr);
    const char *pMapName = MapName();
    const char *pReplayHash = data->GetString("hash");
    CFmtStr fileNameLocal("%s-%s%s", pMapName, pReplayHash, EXT_RECORDING_FILE);
    CFmtStr filePathLocal("%s/%s", RECORDING_PATH, fileNameLocal.Get());
    CFmtStr fileNameOnline("%s-%lld%s", pMapName, replayID, EXT_RECORDING_FILE);
    CFmtStr filePathOnline("%s/%s/%s", RECORDING_PATH, RECORDING_ONLINE_PATH, fileNameOnline.Get());
    DevLog("File URL: %s\n", pFileURL);
    DevLog("File name: %s\n", fileNameOnline.Get());
    DevLog("ID: %lld\n", replayID);

    // Check if we already have it
    if (g_pFullFileSystem->FileExists(filePathLocal.Get(), "MOD"))
    {
        DevLog("Already had the replay locally, no need to download!\n");
        CFmtStr comm("mom_replay_play %s\n", fileNameLocal.Get());
        engine->ClientCmd(comm.Get());
    }
    else if (g_pMomentumUtil->FileExists(filePathOnline.Get(), pReplayHash, "MOD"))
    {
        DevLog("Already downloaded the replay, no need to download again!\n");
        CFmtStr command("mom_replay_play %s/%s\n", RECORDING_ONLINE_PATH, fileNameOnline.Get());
        engine->ClientCmd(command.Get());
    }
    else
    {
        // Check if we're already downloading it
        bool bFound = false;
        unsigned short indx = m_mapReplayDownloads.FirstInorder();
        while (indx != m_mapReplayDownloads.InvalidIndex())
        {
            if (m_mapReplayDownloads[indx] == replayID)
            {
                bFound = true;
                break;
            }

            indx = m_mapReplayDownloads.NextInorder(indx);
        }
        if (bFound)
        {
            // Already downloading!
            Log("Already downloading replay %lld!\n", replayID);
        }
        else if (replayID)
        {
            // We either don't have it, or it's outdated, so let's get the latest one!
            auto handle = g_pAPIRequests->DownloadFile(
                pFileURL, UtlMakeDelegate(this, &CClientTimesDisplay::OnReplayDownloadStart),
                UtlMakeDelegate(this, &CClientTimesDisplay::OnReplayDownloadProgress),
                UtlMakeDelegate(this, &CClientTimesDisplay::OnReplayDownloadEnd),
                filePathOnline.Get(),
                "MOD");
            if (handle != INVALID_HTTPREQUEST_HANDLE)
            {
                m_mapReplayDownloads.Insert(handle, replayID);
            }
            else
            {
                Warning("Failed to try to download the replay %lld!\n", replayID);
            }
        }
    }
#endif

#if ENABLE_STEAM_LEADERBOARDS
    SteamAPICall_t download = SteamRemoteStorage()->UGCDownload(UGC, 0);
    m_cOnlineReplayDownloaded.Set(download, this, &CClientTimesDisplay::OnOnlineReplayDownloaded);
#endif
}

#if ENABLE_STEAM_LEADERBOARDS
void CClientTimesDisplay::OnOnlineReplayDownloaded(RemoteStorageDownloadUGCResult_t* pResult, bool bIOFailure)
{

    if (bIOFailure || pResult->m_eResult != k_EResultOK)
    {
        Warning("Couldn't download the online replay! %i\n", pResult->m_eResult);
        return;
    }

    CFmtStr path("%s/%s/%s", RECORDING_PATH, RECORDING_ONLINE_PATH, pResult->m_pchFileName);
    bool bReplayExists = false;

    if (filesystem->FileExists(path.Get(), "MOD"))
    {
        DevMsg("Replay already exists! Playing it...\n");
        bReplayExists = true; 
    }
    else
    {
        DevMsg("Replay doesn't exist, trying to download it...\n");
        byte *data = new byte[pResult->m_nSizeInBytes];
        if (SteamRemoteStorage()->UGCRead(pResult->m_hFile, data, pResult->m_nSizeInBytes, 0, k_EUGCRead_ContinueReadingUntilFinished))
        {
            CUtlBuffer buf;
            buf.AssumeMemory(data, pResult->m_nSizeInBytes, pResult->m_nSizeInBytes, CUtlBuffer::READ_ONLY);

            if (filesystem->WriteFile(path.Get(), "MOD", buf))
            {
                DevMsg("Downloaded online replay %s! Playing it...\n", pResult->m_pchFileName);
                bReplayExists = true;
            }
            else
            {
                DevWarning("Failed to write the file for the online replay!\n");
            }
            // CUtlBuffer clears memory here as it destructs from scope
        }
    }

    if (bReplayExists)
    {
        CFmtStr command("mom_replay_play %s/%s\n", RECORDING_ONLINE_PATH, pResult->m_pchFileName);
        engine->ClientCmd(command.Get());
    }
    else
    {
        DevWarning("Online replay does not exist and could not be downloaded!\n");
    }
}
#endif


void CClientTimesDisplay::OnPersonaStateChange(PersonaStateChange_t *pParam)
{
#if ENABLE_ONLINE_LEADERBOARDS
    // MOM_TODO: should we check only the FirstSet one here?
    if (pParam->m_nChangeFlags & k_EPersonaChangeNameFirstSet || pParam->m_nChangeFlags & k_EPersonaChangeName)
    {
        m_umMapNames.InsertOrReplace(
            pParam->m_ulSteamID, SteamFriends()->GetFriendPersonaName(CSteamID(pParam->m_ulSteamID)));

        // MOM_TODO: should this be called here?
        /*OnlineTimesVectorToLeaderboards(TIMES_TOP10);
        OnlineTimesVectorToLeaderboards(TIMES_AROUND);
        OnlineTimesVectorToLeaderboards(TIMES_FRIENDS);*/
    }
#endif
}

int CClientTimesDisplay::TryAddAvatar(const uint64 &steamid, CUtlMap<uint64, int> *pIDtoIndxMap, ImageList *pImageList)
{
    // Update their avatar
    if (pIDtoIndxMap && pImageList)
    {
        // See if we already have that avatar in our list
        const unsigned short mapIndex = pIDtoIndxMap->Find(steamid);
        int iImageIndex;
        if (!pIDtoIndxMap->IsValidIndex(mapIndex))
        {
            CAvatarImage *pImage = new CAvatarImage();
            // 64 is enough up to full HD resolutions.
            pImage->SetAvatarSteamID(CSteamID(steamid), k_EAvatarSize64x64);

            pImage->SetDrawFriend(false);
            pImage->SetAvatarSize(32, 32);
            iImageIndex = pImageList->AddImage(pImage);
            pIDtoIndxMap->Insert(steamid, iImageIndex);
        }
        else
        {
            iImageIndex = pIDtoIndxMap->Element(mapIndex);
        }
        return iImageIndex;
    }
    return -1;
}

void CClientTimesDisplay::OnCommand(const char *pCommand)
{
    BaseClass::OnCommand(pCommand);
    // MOM_TODO: Implement run tags
    // Leaderboards type
    bool isTop10 = FStrEq(pCommand, "GlobalTypeTop10");
    bool isAround = FStrEq(pCommand, "GlobalTypeAround");
    bool isLocal = FStrEq(pCommand, "ShowLocal");
    bool isFriends = FStrEq(pCommand, "ShowFriends");
    bool isGlobal = FStrEq(pCommand, "ShowGlobal");
    bool isFilter = FStrEq(pCommand, "ShowFilter");
    bool isReset = FStrEq(pCommand, "ResetFlags");
    bool isFlagScrollOnly = FStrEq(pCommand, "ToggleScrollOnly");
    bool isFlagWOnly = FStrEq(pCommand, "ToggleWOnly");
    bool isFlagHSW = FStrEq(pCommand, "ToggleHSW");
    bool isFlagSideways = FStrEq(pCommand, "ToggleSideways");
    bool isFlagBackwards = FStrEq(pCommand, "ToggleBackwards");
    bool isFlagBonus = FStrEq(pCommand, "ToggleBonus");
    if (isTop10 || isAround)
    {
        m_pFriendsLeaderboardsButton->SetEnabled(true);

        m_pGlobalTop10Button->SetEnabled(!isTop10);
        m_pGlobalAroundButton->SetEnabled(isTop10);

        // Show the right type of leaderboards
        m_pCurrentLeaderboards->SetVisible(false);
        m_pCurrentLeaderboards = isTop10 ? m_pOnlineLeaderboards : m_pAroundLeaderboards;
        m_pCurrentLeaderboards->SetVisible(true);

        Update();
    }
    else if (isLocal || isFriends || isGlobal)
    {
        // Show the right type of leaderboards
        m_pCurrentLeaderboards->SetVisible(false);
        m_pCurrentLeaderboards =
            isGlobal ? m_pOnlineLeaderboards : (isLocal ? m_pLocalLeaderboards : m_pFriendsLeaderboards);
        m_pCurrentLeaderboards->SetVisible(true);

        m_pGlobalLeaderboardsButton->SetEnabled(!isGlobal && !isFriends);
        m_pGlobalAroundButton->SetEnabled(true);
        m_pGlobalTop10Button->SetEnabled(!isGlobal);
        m_pFriendsLeaderboardsButton->SetEnabled(!isFriends);
        m_pLocalLeaderboardsButton->SetEnabled(!isLocal);

        m_pGlobalTop10Button->SetVisible(isGlobal || isFriends);
        m_pGlobalAroundButton->SetVisible(isGlobal || isFriends);
        m_pFriendsLeaderboardsButton->SetVisible(isGlobal || isFriends);

        if (isLocal)
            m_pOnlineTimesStatus->SetVisible(false);

        else if (isFriends)
        {
            m_pGlobalAroundButton->SetEnabled(true);
            m_pGlobalTop10Button->SetEnabled(true);
        }
        Update();
    }
    else if (isFilter)
    {
        m_pFilterPanel->SetVisible(m_pRunFilterButton->IsSelected());
    }
    else if (isReset)
    {
        flaggedRuns = RUNFLAG_NONE;
        for (int i = 0; i < m_pFilterPanel->GetChildCount(); i++)
        {
            ToggleButton *pChild = dynamic_cast<ToggleButton*>(m_pFilterPanel->GetChild(i));
            if (pChild)
            {
                pChild->ForceDepressed(false);
                pChild->SetSelected(false);
            }
        }
    }
    else if (isFlagScrollOnly)
    {
        flaggedRuns = static_cast<RUN_FLAG>(flaggedRuns ^ RUNFLAG_SCROLL);
    }
    else if (isFlagWOnly)
    {
        flaggedRuns = static_cast<RUN_FLAG>(flaggedRuns ^ RUNFLAG_W_ONLY);
    }
    else if (isFlagHSW)
    {
        flaggedRuns = static_cast<RUN_FLAG>(flaggedRuns ^ RUNFLAG_HSW);
    }
    else if (isFlagSideways)
    {
        flaggedRuns = static_cast<RUN_FLAG>(flaggedRuns ^ RUNFLAG_SW);
    }
    else if (isFlagBackwards)
    {
        flaggedRuns = static_cast<RUN_FLAG>(flaggedRuns ^ RUNFLAG_BW);
    }
    else if (isFlagBonus)
    {
        flaggedRuns = static_cast<RUN_FLAG>(flaggedRuns ^ RUNFLAG_BONUS);
    }
    else
    {
        DevLog("Caught an unhandled command: %s\n", pCommand);
    }
}

void CClientTimesDisplay::UpdateMapInfoLabel(const char *author, const int tier, const char *layout, const int bonus)
{
    if (m_pMapDetails && m_pMapAuthor)
    {
        char mapAuthor[MAX_PLAYER_NAME_LENGTH + 3];
        Q_snprintf(mapAuthor, MAX_PLAYER_NAME_LENGTH + 3, "By %s", author);
        m_pMapAuthor->SetText(mapAuthor);

        char mapDetails[BUFSIZ];
        Q_snprintf(mapDetails, BUFSIZ, "TIER %i - %s - %i BONUS", tier, layout, bonus);
        UpdateMapInfoLabel(mapDetails);
    }
}

void CClientTimesDisplay::UpdateMapInfoLabel(const char *text)
{
    if (m_pMapDetails)
    {
        m_pMapDetails->SetText(text);
        if (text == nullptr)
        {
            m_pMapDetails->SetVisible(false);
        }
    }
}